#include "AniStudio.hpp"
#include "OpenGLWrapper.hpp"
#include "AniStudioViews.hpp"
#include "StudioContext.hpp"
#include "FilePathSystem.hpp"
#include "FileDialogUtil.hpp"
#include "ImGuiSettingsUtil.hpp"
#include "ImGuiStateUtils.hpp"
#include "Events.hpp"
#include "AniStudioComponents.hpp"
#include "AniStudioSystems.hpp"
#include "MenuBar.hpp"
#include "DragDropUtils.hpp"
#include "AniStudioRegistration.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <cfloat>
#include <imgui.h>
#include "GuiStyleHelpers.hpp"
#include "MissingPathsPopup.hpp"
#include "GeneralSettingsTab.hpp"
#include "ImGuiStyleSettingsTab.hpp"
#include "ImGuiRenderSettingsTab.hpp"
#include "FontSettingsTab.hpp"
#include "TextEditorSettingsTab.hpp"
#include "StringWidgets.hpp"
#include "TextEditorUtil.hpp"
#include "TextEditorFontUtil.hpp"
#include "GeneralSettingsComponent.hpp"
#include "SettingsSystem.hpp"
#include "ProjectManagerView.hpp"
#include "Log.hpp"
#include "ErrorBus.hpp"          // <-- NEW
#include "ImageSystem.hpp"
#include "TextureSystem.hpp"

#ifdef _WIN32
#include <GLFW/glfw3native.h>
#endif

namespace ANI {

    namespace {

        // Collapses runs of identical (source, message) pairs into a single
        // entry with a count, so a broken model that emits 200 identical
        // errors doesn't bury the user in near-duplicate lines.
        std::vector<ErrorBus::Entry> DeduplicateErrors(
            std::vector<ErrorBus::Entry> errors) {
            std::vector<ErrorBus::Entry> out;
            out.reserve(errors.size());
            for (auto& e : errors) {
                if (!out.empty() &&
                    out.back().source == e.source &&
                    out.back().message == e.message) {
                    // Same error again. Bump the visible count instead of
                    // pushing another identical entry.
                    out.back().line = out.back().line;  // keep first location
                    if (out.back().file.empty() && !e.file.empty()) {
                        out.back().file = e.file;
                        out.back().line = e.line;
                    }
                    // Append a marker the user can see in the popup.
                    if (out.back().message.find("\n(+") == std::string::npos) {
                        out.back().message += "\n(+duplicate)";
                    }
                    else {
                        // Cheap repeated-marker increment.
                        auto pos = out.back().message.rfind("(+");
                        auto end = out.back().message.find(')', pos);
                        if (pos != std::string::npos && end != std::string::npos) {
                            int n = 1;
                            std::sscanf(out.back().message.c_str() + pos,
                                "(+%d", &n);
                            out.back().message.erase(pos, end - pos + 1);
                            out.back().message += "(+" + std::to_string(n + 1) + ")";
                        }
                    }
                    continue;
                }
                out.push_back(std::move(e));
            }
            return out;
        }

        // Builds a copy-paste friendly block for GitHub issues. Includes
        // source, file:line (when available), the message, and the session
        // log path so the user can attach the full trace.
        std::string BuildErrorReport(
            const std::vector<ErrorBus::Entry>& errors) {
            std::string out;
            out.reserve(1024);
            out += "AniStudio error report\n";
            out += "======================\n";
            for (const auto& e : errors) {
                out += "\n[";
                out += e.source;
                out += "] ";
                if (!e.file.empty()) {
                    out += e.file;
                    out += ":";
                    out += std::to_string(e.line);
                    out += " ";
                }
                out += "\n";
                out += e.message;
                if (out.empty() || out.back() != '\n') out += "\n";
            }
            out += "\nSession log: ";
            out += Log::SessionPath();
            out += "\n";
            return out;
        }

    } // anonymous namespace

    StudioCore::StudioCore()
        : initialized(false), running(false), windowHandle(nullptr), imguiContext(nullptr),
        m_isShuttingDown(false), m_showMissingPathsPopup(false) {
        ANI_LOG_INFO("StudioCore constructor");
    }

    StudioCore::~StudioCore() {
        if (initialized) {
            Shutdown();
        }
    }

    bool StudioCore::InitializeCoreOnly() {
        if (initialized) {
            ANI_LOG_ERROR("[StudioCore] Already initialized!");
            return false;
        }

        try {
            ANI_LOG_INFO("[StudioCore] =========================================");
            ANI_LOG_INFO("[StudioCore] InitializeCoreOnly (server / headless)...");

            if (!engineCore.Initialize()) {
                ANI_LOG_ERROR("[StudioCore] Failed to initialize EngineCore!");
                return false;
            }

            auto engineContext = engineCore.GetEngineContext();
            if (!engineContext) {
                ANI_LOG_ERROR("[StudioCore] Failed to get EngineContext!");
                return false;
            }

            studioContext = StudioContext::FromEngine(engineContext);
            if (!studioContext || !studioContext->isValid()) {
                ANI_LOG_ERROR("[StudioCore] Failed to create valid StudioContext!");
                return false;
            }
            studioContext->mode = StudioContext::Mode::Server;
            studioContext->viewManager->SetEntityManager(*studioContext->entityManager);

            auto& entityMgr = GetEntityManager();
            Registration::RegisterComponents(entityMgr);
            Registration::RegisterSystems(entityMgr,
                studioContext->viewManager.get(),
                windowHandle,
                studioContext.get());

            SetupProjectCallbacks();
            SetCoreCallbacks();
            SetCoreEvents();

            initialized = true;
            running = true;

            ANI_LOG_INFO("[StudioCore] Core-only initialization complete (no GUI).");
            return true;
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("[StudioCore] Core-only initialization failed: %s", e.what());
            Shutdown();
            return false;
        }
    }

    bool StudioCore::InitializeGUI() {
        if (!initialized || !studioContext) {
            ANI_LOG_ERROR("[StudioCore] InitializeGUI called before InitializeCoreOnly!");
            return false;
        }
        if (studioContext->isServer()) {
            ANI_LOG_INFO("[StudioCore] InitializeGUI skipped in Server mode.");
            return true;
        }
        if (!imguiContext) {
            ANI_LOG_ERROR("[StudioCore] InitializeGUI requires a valid ImGui context!");
            return false;
        }

        try {
            ANI_LOG_INFO("[StudioCore] InitializeGUI...");

            ImGui::SetCurrentContext(static_cast<ImGuiContext*>(imguiContext));

            auto& entityMgr = GetEntityManager();
            auto fileSys = entityMgr.GetSystem<ECS::FilePathSystem>();

            std::string defaultProjectPath =
                fileSys ? fileSys->GetPath("DefaultProject") : std::string{};

            if (defaultProjectPath.empty()) {
                std::filesystem::path basePath = std::filesystem::path(".").parent_path();
                defaultProjectPath = (basePath / "projects").string();
                if (fileSys) {
                    fileSys->SetPath("DefaultProject", defaultProjectPath);
                }
                ANI_LOG_INFO("[StudioCore] Set DefaultProject to: %s",
                    defaultProjectPath.c_str());
            }
            if (!defaultProjectPath.empty() && !std::filesystem::exists(defaultProjectPath)) {
                std::filesystem::create_directories(defaultProjectPath);
            }

            std::string dataPath = fileSys ? fileSys->GetPath("DataPath") : std::string{};
            if (dataPath.empty()) {
                dataPath = "./data";
                if (fileSys) fileSys->SetPath("DataPath", dataPath);
            }
            if (!std::filesystem::exists(dataPath)) {
                std::filesystem::create_directories(dataPath);
            }

            EnsureCorePaths();
            if (fileSys) {
                fileSys->LoadFromFile(dataPath + "/paths.json");
                EnsureCorePaths();
                Utils::CheckMissingPaths(fileSys.get());
            }

            ConfigureImGuiIniPath();

            ImGuiIO& io = ImGui::GetIO();
            if (!io.Fonts || io.Fonts->Fonts.Size == 0) {
                ANI_LOG_ERROR("[StudioCore] ImGui fonts not loaded!");
                return false;
            }

            InitializeStudioPlugins();

            if (auto ps = entityMgr.GetSystem<ECS::ProjectSystem>()) {
                if (studioContext && studioContext->studioPluginManager) {
                    ps->SetPluginManager(studioContext->studioPluginManager.get());
                }
            }

            Registration::RegisterViews(entityMgr,
                *studioContext->viewManager,
                studioContext->studioPluginManager.get());

            RegisterSettingsTabs();

            auto projectSystem = entityMgr.GetSystem<ECS::ProjectSystem>();
            m_projectManagerView =
                std::make_unique<GUI::ProjectManagerView>(*projectSystem, this);
            m_projectManagerView->Init();

            if (studioContext->viewManager) {
                m_menuBar = std::make_unique<GUI::MenuBar>(
                    *projectSystem,
                    *studioContext->viewManager,
                    *this);
            }

            m_showProjectManagerView =
                projectSystem ? projectSystem->ShouldShowStartup() : true;

            ANI_LOG_INFO("[StudioCore] GUI initialization complete. Show startup view: %s",
                m_showProjectManagerView ? "YES" : "NO");
            return true;
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("[StudioCore] InitializeGUI failed: %s", e.what());
            return false;
        }
    }

    bool StudioCore::Initialize() {
        if (!InitializeCoreOnly()) return false;

        studioContext->mode = StudioContext::Mode::Local;

        if (!InitializeGUI()) {
            Shutdown();
            return false;
        }
        return true;
    }

    std::unique_ptr<StudioCore> StudioCore::CreateWithContext(
        std::shared_ptr<StudioContext> existingContext) {
        if (!existingContext || !existingContext->isValid()) {
            ANI_LOG_ERROR("[StudioCore] Invalid context provided to CreateWithContext!");
            return nullptr;
        }

        auto studioCore = std::make_unique<StudioCore>();
        studioCore->studioContext = existingContext;

        if (!studioCore->engineCore.Initialize()) {
            ANI_LOG_ERROR("[StudioCore] Failed to initialize EngineCore with existing context!");
            return nullptr;
        }

        studioCore->studioContext->viewManager->SetEntityManager(
            *studioCore->studioContext->entityManager);

        auto& entityMgr = studioCore->GetEntityManager();
        Registration::RegisterComponents(entityMgr);
        Registration::RegisterSystems(entityMgr,
            studioCore->studioContext->viewManager.get(),
            studioCore->windowHandle,
            studioCore->studioContext.get());

        auto projectSystem = entityMgr.GetSystem<ECS::ProjectSystem>();
        studioCore->m_projectManagerView =
            std::make_unique<GUI::ProjectManagerView>(*projectSystem, studioCore.get());

        auto fileSys = entityMgr.GetSystem<ECS::FilePathSystem>();
        std::string defaultProjectPath =
            fileSys ? fileSys->GetPath("DefaultProject") : std::string{};
        if (!defaultProjectPath.empty() &&
            !std::filesystem::exists(defaultProjectPath)) {
            std::filesystem::create_directories(defaultProjectPath);
        }

        studioCore->SetupProjectCallbacks();

        if (projectSystem && studioCore->studioContext->studioPluginManager) {
            projectSystem->SetPluginManager(
                studioCore->studioContext->studioPluginManager.get());
        }

        studioCore->SetCoreCallbacks();
        studioCore->SetCoreEvents();

        studioCore->initialized = true;
        studioCore->running = true;

        ANI_LOG_INFO("[StudioCore] Created with existing context successfully");
        return studioCore;
    }

    void StudioCore::ConfigureImGuiIniPath() {
        auto fileSys = studioContext->entityManager->GetSystem<ECS::FilePathSystem>();
        if (!fileSys) return;

        std::string imguiIniPath = fileSys->GetPath("ImguiState");
        if (imguiIniPath.empty()) {
            ANI_LOG_WARN("[StudioCore] ImguiState path missing!");
            return;
        }

        std::filesystem::path iniDir = std::filesystem::path(imguiIniPath).parent_path();
        if (!iniDir.empty() && !std::filesystem::exists(iniDir)) {
            std::filesystem::create_directories(iniDir);
        }

        static std::string persistentIniPath = imguiIniPath;
        ImGui::GetIO().IniFilename = persistentIniPath.c_str();
        ANI_LOG_INFO("[StudioCore] ImGui INI path: %s", ImGui::GetIO().IniFilename);
    }

    void StudioCore::RegisterSettingsTabs() {
        auto settingsSystem =
            studioContext->entityManager->GetSystem<ECS::SettingsSystem>();
        if (!settingsSystem) return;

        EntityID settingsEntity = settingsSystem->GetSettingsEntity();
        auto& entityMgr = *studioContext->entityManager;
        if (!entityMgr.IsEntityValid(settingsEntity)) {
            ANI_LOG_ERROR("[StudioCore] Settings entity not valid; skipping tabs.");
            return;
        }

        auto& generalComp =
            entityMgr.GetComponent<ECS::GeneralSettingsComponent>(settingsEntity);
        auto& styleComp =
            entityMgr.GetComponent<ECS::ImGuiStyleSettingsComponent>(settingsEntity);
        auto& renderComp =
            entityMgr.GetComponent<ECS::ImGuiRenderSettingsComponent>(settingsEntity);
        auto& fontComp =
            entityMgr.GetComponent<ECS::FontSettingsComponent>(settingsEntity);
        auto& textEditorComp =
            entityMgr.GetComponent<ECS::TextEditorSettingsComponent>(settingsEntity);

        settingsSystem->RegisterTab(
            std::make_unique<ECS::GeneralSettingsTab>(generalComp));
        settingsSystem->RegisterTab(
            std::make_unique<ECS::ImGuiStyleSettingsTab>(styleComp));
        settingsSystem->RegisterTab(
            std::make_unique<ECS::ImGuiRenderSettingsTab>(renderComp));
        settingsSystem->RegisterTab(
            std::make_unique<ECS::FontSettingsTab>(fontComp));
        settingsSystem->RegisterTab(
            std::make_unique<ECS::TextEditorSettingsTab>(textEditorComp, fontComp));

        UISchema::StringWidgets::SetSettingsComponent(&textEditorComp);
        TextEditorUtil::SetSettingsComponent(&textEditorComp);
        TextEditorUtil::SetFontComponent(&fontComp);

        settingsSystem->SetImGuiContext(static_cast<ImGuiContext*>(imguiContext));

        for (auto& tab : settingsSystem->GetTabs()) {
            tab->LoadSettings();
            tab->CreateBackup();
        }

        settingsSystem->LoadAllSettings();

        ANI_LOG_INFO("[StudioCore] Core settings tabs registered.");
    }

    void StudioCore::EnsureCorePaths() {
        auto fileSys = studioContext->entityManager->GetSystem<ECS::FilePathSystem>();
        if (!fileSys) return;

        const std::vector<std::string> coreKeys = {
            "DataPath", "DefaultProject", "Plugins", "ImguiState",
            "Assets", "Docs", "Scripts", "Templates", "Shaders"
        };

        for (const auto& key : coreKeys) {
            if (!fileSys->HasPath(key)) {
                fileSys->SetPath(key, "");
            }
        }

        Utils::SetDefaultPath("DataPath", "./data");
        Utils::SetDefaultPath("DefaultProject", "./projects");
        Utils::SetDefaultPath("Plugins", "./plugins");
        Utils::SetDefaultPath("ImguiState", "./data/imgui.ini");
        Utils::SetDefaultPath("Assets", "./assets");
        Utils::SetDefaultPath("Docs", "./docs");
        Utils::SetDefaultPath("Scripts", "./scripts");
        Utils::SetDefaultPath("Templates", "./data/templates");
        Utils::SetDefaultPath("Shaders", "./shaders");
    }

    void StudioCore::SetNetworkClientMode(bool on) {
        if (m_projectManagerView) {
            m_projectManagerView->SetNetworkMode(on);
        }
    }

    GUI::ProjectManagerView& StudioCore::GetProjectManagerView() {
        return *m_projectManagerView;
    }

    GUI::SettingsView& StudioCore::GetSettingsView() {
        if (!m_settingsView) {
            ANI_LOG_INFO("[StudioCore] Lazy creating SettingsView...");
            m_settingsView = std::make_unique<GUI::SettingsView>();

            if (imguiContext) {
                m_settingsView->SetImGuiContext(
                    static_cast<ImGuiContext*>(imguiContext));
            }
            if (studioContext && studioContext->entityManager) {
                m_settingsView->SetEntityManager(*studioContext->entityManager);
            }
        }
        return *m_settingsView;
    }

    void StudioCore::SetImGuiContext(void* context) {
        imguiContext = context;
        ANI_LOG_INFO("[StudioCore] ImGui context set to: %p", imguiContext);

        if (studioContext) {
            studioContext->imguiContext = context;
        }
        if (m_settingsView) {
            m_settingsView->SetImGuiContext(static_cast<ImGuiContext*>(context));
        }
    }

    void StudioCore::InitializeStudioPlugins() {
        if (!studioContext) {
            ANI_LOG_ERROR("[StudioCore] StudioContext not initialized!");
            return;
        }
        if (!imguiContext) {
            ANI_LOG_ERROR("[StudioCore] ImGui context is null!");
            return;
        }

        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(imguiContext));
        ImGuiIO& io = ImGui::GetIO();
        if (!io.Fonts || io.Fonts->Fonts.Size == 0) {
            ANI_LOG_ERROR("[StudioCore] ImGui not fully initialized!");
            return;
        }

        studioContext->studioPluginManager =
            std::make_shared<Plugins::StudioPluginManager>(
                *studioContext->entityManager,
                *studioContext->viewManager,
                static_cast<ImGuiContext*>(imguiContext));

        if (studioContext->studioPluginManager) {
            auto engineContext =
                std::static_pointer_cast<ANI::EngineContext>(studioContext);
            studioContext->studioPluginManager->SetEngineContext(engineContext);
            studioContext->studioPluginManager->SetStudioContext(studioContext);
        }

        auto fileSys =
            studioContext->entityManager->GetSystem<ECS::FilePathSystem>();
        std::string pluginDirectory =
            fileSys ? fileSys->GetPath("Plugins") : std::string{};
        if (pluginDirectory.empty()) {
            pluginDirectory = "./plugins";
            ANI_LOG_WARN("[StudioCore] Using default plugin dir: %s", pluginDirectory.c_str());
        }
        if (!std::filesystem::exists(pluginDirectory)) {
            std::filesystem::create_directories(pluginDirectory);
        }

        studioContext->studioPluginManager->scanPluginDirectory(pluginDirectory);
        studioContext->studioPluginManager->enableHotReload(true);

        ANI_LOG_INFO("[StudioCore] Plugin system initialized (hot reload on).");
    }

    void StudioCore::SetupProjectCallbacks() {
        auto projectSystem =
            GetEntityManager().GetSystem<ECS::ProjectSystem>();
        if (!projectSystem) {
            ANI_LOG_ERROR("[StudioCore] ProjectSystem not initialized!");
            return;
        }

        if (studioContext && studioContext->viewManager) {
            projectSystem->SetViewManager(studioContext->viewManager.get());
        }

        projectSystem->SetProjectLoadedCallback(
            [this](const std::string& projectPath) { OnProjectLoaded(projectPath); });
        projectSystem->SetProjectCreatedCallback(
            [this](const std::string& projectPath) { OnProjectCreated(projectPath); });
        projectSystem->SetProjectClosedCallback([this]() {
            if (!m_isShuttingDown) OnProjectClosed();
            });
        projectSystem->SetViewStateLoadedCallback(
            [this](GUI::WorkspaceID activeWorkspaceID) {
                if (studioContext && studioContext->viewManager) {
                    studioContext->viewManager->SetActiveWorkspace(activeWorkspaceID);
                }
            });
    }

    void StudioCore::InitializeWindowState() {
        auto fileSys =
            studioContext->entityManager->GetSystem<ECS::FilePathSystem>();
        if (!fileSys) {
            ANI_LOG_ERROR("[StudioCore] FilePathSystem not available!");
            return;
        }

        m_windowState.SetGlobalDataPath(fileSys->GetPath("DataPath"));

        std::string defaultPath = GetDefaultWindowStatePath();
        if (!defaultPath.empty() && std::filesystem::exists(defaultPath)) {
            m_windowState.LoadFromFile(defaultPath);
            ApplyWindowStateToGLFW();
        }
        else {
            SyncWindowStateFromGLFW();
        }
    }

    void StudioCore::SetWindowHandle(void* window) {
        windowHandle = window;

        if (!window) return;

#ifdef _WIN32
        GLFWwindow* glfwWin = static_cast<GLFWwindow*>(window);
        HWND hwnd = glfwGetWin32Window(glfwWin);
        FileDialog::SetGlobalWindowHandle(glfwWin);
        GUI::ViewManager::SetWindowHandle(hwnd);
        GUI::DragDrop::SetWindowHandle(hwnd);
#else
        GUI::ViewManager::SetWindowHandle(window);
        GUI::DragDrop::SetWindowHandle(window);
#endif

        auto projectSystem =
            GetEntityManager().GetSystem<ECS::ProjectSystem>();
        if (projectSystem) projectSystem->SetWindowHandle(window);

        if (studioContext) studioContext->windowHandle = window;

        GUI::DragDrop::InitializeFileDrop(static_cast<GLFWwindow*>(window));

        if (initialized) InitializeWindowState();
    }

    void StudioCore::SyncWindowStateFromGLFW() {
        if (!windowHandle) return;
        GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(windowHandle);

        int width, height, x, y;
        glfwGetWindowSize(glfwWindow, &width, &height);
        glfwGetWindowPos(glfwWindow, &x, &y);

        nlohmann::json currentState;
        currentState["width"] = width;
        currentState["height"] = height;
        currentState["posX"] = x;
        currentState["posY"] = y;
        currentState["maximized"] =
            (glfwGetWindowAttrib(glfwWindow, GLFW_MAXIMIZED) == GLFW_TRUE);
        currentState["fullscreen"] =
            (glfwGetWindowMonitor(glfwWindow) != nullptr);
        currentState["vsync"] = true;
        currentState["title"] = "AniStudio";

        m_windowState.Deserialize(currentState);
    }

    void StudioCore::ApplyWindowStateToGLFW() {
        if (!windowHandle) return;
        GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(windowHandle);

        int width = std::max(m_windowState.GetWidth(), Utils::MIN_WINDOW_WIDTH);
        int height = std::max(m_windowState.GetHeight(), Utils::MIN_WINDOW_HEIGHT);

        glfwSetWindowSize(glfwWindow, width, height);
        glfwSetWindowPos(glfwWindow, m_windowState.GetPosX(), m_windowState.GetPosY());

        if (m_windowState.IsMaximized()) glfwMaximizeWindow(glfwWindow);
        else glfwRestoreWindow(glfwWindow);

        glfwSetWindowAttrib(glfwWindow, GLFW_DECORATED, GLFW_TRUE);
        glfwShowWindow(glfwWindow);
        glfwSwapInterval(1);
    }

    void StudioCore::OnProjectLoaded(const std::string& projectPath) {
        ANI_LOG_INFO("[StudioCore] Project loaded: %s", projectPath.c_str());

        if (auto fileSys =
            studioContext->entityManager->GetSystem<ECS::FilePathSystem>()) {
            fileSys->SetPath("CurrentProject", projectPath);
            fileSys->SetPath("ProjectData", projectPath + "/data");
            fileSys->SetPath("Assets", projectPath + "/assets");
            fileSys->SetPath("Output", projectPath + "/output");
            std::filesystem::create_directories(projectPath + "/data");
            std::filesystem::create_directories(projectPath + "/assets");
            std::filesystem::create_directories(projectPath + "/output");
        }

        m_showProjectManagerView = false;
        Utils::ImGuiStateUtils::OnProjectLoaded(projectPath);
        Events::Ref().QueueEventWithData("ProjectOpened", projectPath);
    }

    void StudioCore::OnProjectCreated(const std::string& projectPath) {
        ANI_LOG_INFO("[StudioCore] Project created: %s", projectPath.c_str());

        if (auto fileSys =
            studioContext->entityManager->GetSystem<ECS::FilePathSystem>()) {
            fileSys->SetPath("CurrentProject", projectPath);
            fileSys->SetPath("ProjectData", projectPath + "/data");
            fileSys->SetPath("Assets", projectPath + "/assets");
            fileSys->SetPath("Output", projectPath + "/output");
            std::filesystem::create_directories(projectPath + "/data");
            std::filesystem::create_directories(projectPath + "/assets");
            std::filesystem::create_directories(projectPath + "/output");
        }

        m_showProjectManagerView = false;
        Utils::ImGuiStateUtils::OnProjectCreated(projectPath);
        Events::Ref().QueueEventWithData("ProjectCreated", projectPath);
    }

    void StudioCore::OnProjectClosed() {
        ANI_LOG_INFO("[StudioCore] OnProjectClosed() called");

        if (auto fileSys =
            studioContext->entityManager->GetSystem<ECS::FilePathSystem>()) {
            fileSys->SetPath("CurrentProject", "");
            fileSys->SetPath("ProjectData", "");
            fileSys->SetPath("Assets", "");
            fileSys->SetPath("Output", "");
        }

        if (studioContext && studioContext->studioPluginManager) {
            studioContext->studioPluginManager->SaveProjectPluginState();
        }

        m_showProjectManagerView = true;

        if (windowHandle) {
            SyncWindowStateFromGLFW();
            std::string defaultPath = GetDefaultWindowStatePath();
            if (!defaultPath.empty()) {
                std::filesystem::create_directories(
                    std::filesystem::path(defaultPath).parent_path());
                m_windowState.SaveToFile(defaultPath);
            }
        }

        Utils::ImGuiStateUtils::OnProjectClosed();
        Events::Ref().QueueEvent("ProjectClosed");
    }

    std::string StudioCore::GetDefaultWindowStatePath() const {
        auto fileSys = studioContext
            ? studioContext->entityManager->GetSystem<ECS::FilePathSystem>()
            : nullptr;
        std::string dataPath = fileSys ? fileSys->GetPath("DataPath") : "";
        if (dataPath.empty()) return "";
        return dataPath + "/window_state.json";
    }

    void StudioCore::Shutdown() {
        if (!initialized) return;

        ANI_LOG_INFO("[StudioCore] Starting shutdown sequence...");
        running = false;
        m_isShuttingDown = true;

        try {
            auto projectSystem =
                GetEntityManager().GetSystem<ECS::ProjectSystem>();

            if (studioContext && !studioContext->isServer()) {
                if (projectSystem && projectSystem->IsProjectOpen()) {
                    if (studioContext->studioPluginManager) {
                        studioContext->studioPluginManager->SaveProjectPluginState();
                    }
                    projectSystem->SaveProject();
                    Events::Ref().QueueEvent("ProjectSaved");
                }
                else if (windowHandle) {
                    SyncWindowStateFromGLFW();
                    std::string defaultPath = GetDefaultWindowStatePath();
                    if (!defaultPath.empty()) {
                        std::filesystem::create_directories(
                            std::filesystem::path(defaultPath).parent_path());
                        m_windowState.SaveToFile(defaultPath);
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            if (projectSystem) {
                projectSystem->SetSuppressViewStateSave(true);
            }

            if (auto imgSys = GetEntityManager().GetSystem<ImageSystem>()) {
                imgSys->UnregisterCallbacksForOwner(this);
            }

            m_menuBar.reset();
            m_projectManagerView.reset();
            m_settingsView.reset();

            if (studioContext && studioContext->studioPluginManager) {
                studioContext->studioPluginManager.reset();
            }

            if (studioContext && studioContext->viewManager) {
                studioContext->viewManager->FullReset();
            }

            engineCore.Shutdown();
            studioContext.reset();
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("[StudioCore] Exception during shutdown: %s", e.what());
        }

        initialized = false;
        ANI_LOG_INFO("[StudioCore] Shutdown complete.");
    }

    void StudioCore::Update(float deltaTime) {
        if (!running || !initialized || !studioContext) return;

        ANI::Events::Ref().Poll();
        try {
            engineCore.Update(deltaTime);

            if (studioContext->studioPluginManager) {
                studioContext->studioPluginManager->updatePlugins(deltaTime);
            }
            if (m_menuBar) m_menuBar->Update(deltaTime);
            if (m_showProjectManagerView && m_projectManagerView) {
                m_projectManagerView->Update(deltaTime);
            }
            if (studioContext->viewManager) {
                studioContext->viewManager->Update(deltaTime);
            }
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("[StudioCore] Update error: %s", e.what());
        }
    }

    void StudioCore::Render() {
        if (!running || !initialized || !studioContext) return;
        if (studioContext->isServer()) return;

        try {
            auto projectSystem =
                GetEntityManager().GetSystem<ECS::ProjectSystem>();
            const bool isProjectOpen =
                projectSystem ? projectSystem->IsProjectOpen() : false;

            const bool forceDockspace =
                (studioContext->mode == StudioContext::Mode::Client &&
                    studioContext->networkConnected);

            if (!isProjectOpen && !forceDockspace &&
                m_showProjectManagerView && m_projectManagerView) {
                m_projectManagerView->Render();
            }

            if (isProjectOpen || forceDockspace) {
                ImGuiViewport* viewport = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(viewport->WorkPos);
                ImGui::SetNextWindowSize(viewport->WorkSize);
                ImGui::SetNextWindowViewport(viewport->ID);

                ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar |
                    ImGuiWindowFlags_NoDocking |
                    ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoBringToFrontOnFocus |
                    ImGuiWindowFlags_NoNavFocus |
                    ImGuiWindowFlags_NoBackground;

                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

                bool open = true;
                if (ImGui::Begin("MainDockSpaceWindow", &open, flags)) {
                    ImGui::PopStyleVar(3);

                    if (m_menuBar) m_menuBar->Render();

                    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
                    ImGui::DockSpace(dockspace_id, ImVec2(0, 0),
                        ImGuiDockNodeFlags_None);

                    studioContext->viewManager->Render();
                    ImGui::End();
                }
                else {
                    ImGui::PopStyleVar(3);
                    ImGui::End();
                }
            }

            if (m_settingsView && m_settingsView->IsVisible()) {
                m_settingsView->Render();
            }

            Utils::RenderMissingPathsPopup();

            {
                for (auto& e : ErrorBus::Drain()) {
                    m_pendingErrors.push_back(std::move(e));
                }

                if (!m_pendingErrors.empty()) {
                    m_showErrorPopup = true;
                }

                if (m_showErrorPopup) {
                    m_pendingErrors = DeduplicateErrors(std::move(m_pendingErrors));

                    ImGui::SetNextWindowSize(ImVec2(640, 440), ImGuiCond_Appearing);
                    ImGui::OpenPopup("AniStudio Errors");

                    if (ImGui::BeginPopupModal("AniStudio Errors", nullptr,
                        ImGuiWindowFlags_NoSavedSettings)) {
                        ImGui::Text("%zu error%s occurred.",
                            m_pendingErrors.size(),
                            m_pendingErrors.size() == 1 ? "" : "s");
                        ImGui::Separator();

                        std::string report = BuildErrorReport(m_pendingErrors);

                        static std::vector<char> copyBuf;
                        copyBuf.assign(report.begin(), report.end());
                        copyBuf.push_back('\0');

                        ImGui::PushTextWrapPos(0.0f);
                        ImGui::InputTextMultiline(
                            "##error_text",
                            copyBuf.data(),
                            copyBuf.size(),
                            ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 14),
                            ImGuiInputTextFlags_ReadOnly);
                        ImGui::PopTextWrapPos();

                        if (ImGui::Button("Copy all")) {
                            ImGui::SetClipboardText(report.c_str());
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Copy session log path")) {
                            ImGui::SetClipboardText(Log::SessionPath());
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Dismiss")) {
                            m_pendingErrors.clear();
                            m_showErrorPopup = false;
                            ImGui::CloseCurrentPopup();
                        }

                        ImGui::EndPopup();
                    }
                }
            }
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("[StudioCore] Render error: %s", e.what());
        }
    }

    void StudioCore::SetCoreCallbacks() {
        ANI_LOG_INFO("[StudioCore] Setting up core system callbacks...");

        auto& entityMgr = GetEntityManager();
        auto imageSystem = entityMgr.GetSystem<ImageSystem>();
        auto videoSystem = entityMgr.GetSystem<ECS::VideoSystem>();

        if (!imageSystem) {
            ANI_LOG_ERROR("[StudioCore] ImageSystem missing.");
            return;
        }

        imageSystem->RegisterImageAddedCallback(this,
            [this](EntityID entityID) {
                ANI::Events::Ref().QueueEventWithData("ImageLoaded", entityID);
            });

        imageSystem->RegisterImageRemovedCallback(this,
            [this](EntityID entityID) {
                ANI::Events::Ref().QueueEventWithData("ImageRemoved", entityID);
            });
    }

    void StudioCore::SetCoreEvents() {
        ANI_LOG_INFO("[StudioCore] Registering core system events...");

        auto& entityMgr = GetEntityManager();
        auto imageSystem = entityMgr.GetSystem<ImageSystem>();
        auto videoSystem = entityMgr.GetSystem<ECS::VideoSystem>();
        auto projectSystem = entityMgr.GetSystem<ECS::ProjectSystem>();
        auto pluginManager =
            studioContext ? studioContext->studioPluginManager : nullptr;

        if (imageSystem) {
            Events::Ref().RegisterEventWithData("LoadImageRequest",
                [this, imageSystem](const std::any& data) {
                    try {
                        auto ev = std::any_cast<
                            std::unordered_map<std::string, std::any>>(data);
                        std::string filePath =
                            std::any_cast<std::string>(ev.at("filePath"));
                        auto& mgr = GetEntityManager();
                        ECS::EntityID entity = mgr.AddNewEntity();
                        mgr.AddComponent<ImageComponent>(entity);
                        imageSystem->SetImage(entity, filePath);
                    }
                    catch (const std::exception& e) {
                        ANI_LOG_ERROR("[StudioCore] LoadImageRequest error: %s", e.what());
                    }
                });

            Events::Ref().RegisterEventWithData("RemoveImageRequest",
                [imageSystem](const std::any& data) {
                    try {
                        imageSystem->RemoveImage(
                            std::any_cast<ECS::EntityID>(data));
                    }
                    catch (const std::exception& e) {
                        ANI_LOG_ERROR("[StudioCore] RemoveImageRequest error: %s", e.what());
                    }
                });

            Events::Ref().RegisterEventWithData("ImageLoaded",
                [](const std::any& data) {
                    try {
                        (void)std::any_cast<ECS::EntityID>(data);
                    }
                    catch (...) {}
                });

            Events::Ref().RegisterEventWithData("ImageRemoved",
                [](const std::any& data) {
                    try {
                        (void)std::any_cast<ECS::EntityID>(data);
                    }
                    catch (...) {}
                });
        }

        if (videoSystem) {
            Events::Ref().RegisterEventWithData("LoadVideoRequest",
                [this, videoSystem](const std::any& data) {
                    try {
                        auto ev = std::any_cast<
                            std::unordered_map<std::string, std::any>>(data);
                        std::string filePath =
                            std::any_cast<std::string>(ev.at("filePath"));
                        auto& mgr = GetEntityManager();
                        ECS::EntityID entity = mgr.AddNewEntity();
                        mgr.AddComponent<ECS::VideoComponent>(entity);
                        videoSystem->SetVideo(entity, filePath);
                    }
                    catch (const std::exception& e) {
                        ANI_LOG_ERROR("[StudioCore] LoadVideoRequest error: %s", e.what());
                    }
                });

            Events::Ref().RegisterEventWithData("RemoveVideoRequest",
                [videoSystem](const std::any& data) {
                    try {
                        videoSystem->RemoveVideo(
                            std::any_cast<ECS::EntityID>(data));
                    }
                    catch (const std::exception& e) {
                        ANI_LOG_ERROR("[StudioCore] RemoveVideoRequest error: %s", e.what());
                    }
                });

            Events::Ref().RegisterEventWithData("VideoLoaded",
                [](const std::any&) {});
            Events::Ref().RegisterEventWithData("VideoRemoved",
                [](const std::any&) {});
        }

        if (projectSystem) {
            Events::Ref().RegisterEventWithData("ProjectOpened",
                [](const std::any&) {});
            Events::Ref().RegisterEventWithData("ProjectClosed",
                [](const std::any&) {});
            Events::Ref().RegisterEventWithData("ProjectCreated",
                [](const std::any&) {});
            Events::Ref().RegisterEventWithData("ProjectSaved",
                [](const std::any&) {});

            Events::Ref().RegisterEvent("SaveProject", [projectSystem]() {
                if (projectSystem->IsProjectOpen()) {
                    projectSystem->SaveProject();
                    Events::Ref().QueueEvent("ProjectSaved");
                }
                });

            Events::Ref().RegisterEvent("CloseProject",
                [this, projectSystem]() {
                    if (projectSystem->IsProjectOpen()) {
                        projectSystem->CloseProject();
                        OnProjectClosed();
                    }
                });
        }

        Events::Ref().RegisterEventWithData("SetActiveWorkspace",
            [this](const std::any& data) {
                try {
                    GUI::WorkspaceID id =
                        std::any_cast<GUI::WorkspaceID>(data);
                    if (studioContext && studioContext->viewManager) {
                        studioContext->viewManager->SetActiveWorkspace(id);
                        auto ps = GetEntityManager()
                            .GetSystem<ECS::ProjectSystem>();
                        if (ps) ps->SetLastActiveWorkspace(id);
                    }
                }
                catch (const std::exception& e) {
                    ANI_LOG_ERROR("[StudioCore] SetActiveWorkspace error: %s", e.what());
                }
            });

        Events::Ref().RegisterEventWithData("CreateWorkspace",
            [this](const std::any& data) {
                try {
                    auto ev = std::any_cast<
                        std::unordered_map<std::string, std::string>>(data);
                    std::string name = ev.at("workspaceName");
                    if (studioContext && studioContext->viewManager) {
                        auto vm = studioContext->viewManager.get();
                        GUI::WorkspaceID newID = vm->CreateView();
                        vm->SetWorkspaceName(newID, name);
                        vm->SetActiveWorkspace(newID);
                    }
                }
                catch (const std::exception& e) {
                    ANI_LOG_ERROR("[StudioCore] CreateWorkspace error: %s", e.what());
                }
            });

        Events::Ref().RegisterEventWithData("DeleteWorkspace",
            [this](const std::any& data) {
                try {
                    GUI::WorkspaceID id =
                        std::any_cast<GUI::WorkspaceID>(data);
                    if (!studioContext || !studioContext->viewManager) return;
                    auto vm = studioContext->viewManager.get();
                    auto all = vm->GetAllWorkspaces();
                    if (all.size() <= 1) return;
                    for (auto ws : all) {
                        if (ws != id) { vm->SetActiveWorkspace(ws); break; }
                    }
                    vm->DestroyView(id);
                }
                catch (const std::exception& e) {
                    ANI_LOG_ERROR("[StudioCore] DeleteWorkspace error: %s", e.what());
                }
            });

        Events::Ref().RegisterEventWithData("AddView",
            [this](const std::any& data) {
                try {
                    auto ev = std::any_cast<
                        std::unordered_map<std::string, std::any>>(data);
                    GUI::WorkspaceID ws =
                        std::any_cast<GUI::WorkspaceID>(ev.at("workspaceID"));
                    std::string viewType =
                        std::any_cast<std::string>(ev.at("viewTypeName"));
                    if (studioContext && studioContext->viewManager) {
                        auto vm = studioContext->viewManager.get();
                        vm->AddViewByType(ws, vm->GetViewType(viewType));
                    }
                }
                catch (const std::exception& e) {
                    ANI_LOG_ERROR("[StudioCore] AddView error: %s", e.what());
                }
            });

        Events::Ref().RegisterEventWithData("RemoveView",
            [this](const std::any& data) {
                try {
                    auto ev = std::any_cast<
                        std::unordered_map<std::string, std::any>>(data);
                    GUI::WorkspaceID ws =
                        std::any_cast<GUI::WorkspaceID>(ev.at("workspaceID"));
                    std::string viewType =
                        std::any_cast<std::string>(ev.at("viewTypeName"));
                    if (studioContext && studioContext->viewManager) {
                        auto vm = studioContext->viewManager.get();
                        vm->RemoveViewByType(ws, vm->GetViewType(viewType));
                    }
                }
                catch (const std::exception& e) {
                    ANI_LOG_ERROR("[StudioCore] RemoveView error: %s", e.what());
                }
            });

        Events::Ref().RegisterEvent("CreateEntity", [this]() {
            GetEntityManager().AddNewEntity();
            });

        Events::Ref().RegisterEventWithData("DestroyEntity",
            [this](const std::any& data) {
                try {
                    GetEntityManager().DestroyEntity(
                        std::any_cast<ECS::EntityID>(data));
                }
                catch (const std::exception& e) {
                    ANI_LOG_ERROR("[StudioCore] DestroyEntity error: %s", e.what());
                }
            });

        Events::Ref().RegisterEventWithData("CloneEntity",
            [this](const std::any& data) {
                try {
                    GetEntityManager().CloneEntity(
                        std::any_cast<ECS::EntityID>(data));
                }
                catch (const std::exception& e) {
                    ANI_LOG_ERROR("[StudioCore] CloneEntity error: %s", e.what());
                }
            });

        Events::Ref().RegisterEventWithData("AddComponent",
            [this](const std::any& data) {
                try {
                    auto ev = std::any_cast<
                        std::unordered_map<std::string, std::any>>(data);
                    ECS::EntityID entityID =
                        std::any_cast<ECS::EntityID>(ev.at("entityID"));
                    ECS::ComponentTypeID ctid =
                        std::any_cast<ECS::ComponentTypeID>(
                            ev.at("componentTypeID"));
                    auto& mgr = GetEntityManager();
                    if (mgr.IsPluginComponent(ctid)) {
                        mgr.AddPluginComponent(entityID, ctid);
                    }
                }
                catch (const std::exception& e) {
                    ANI_LOG_ERROR("[StudioCore] AddComponent error: %s", e.what());
                }
            });

        Events::Ref().RegisterEventWithData("RemoveComponent",
            [this](const std::any& data) {
                try {
                    auto ev = std::any_cast<
                        std::unordered_map<std::string, std::any>>(data);
                    ECS::EntityID entityID =
                        std::any_cast<ECS::EntityID>(ev.at("entityID"));
                    ECS::ComponentTypeID ctid =
                        std::any_cast<ECS::ComponentTypeID>(
                            ev.at("componentTypeID"));
                    GetEntityManager().RemoveComponentById(entityID, ctid);
                }
                catch (const std::exception& e) {
                    ANI_LOG_ERROR("[StudioCore] RemoveComponent error: %s", e.what());
                }
            });

        if (pluginManager) {
            Events::Ref().RegisterEventWithData("PluginLoaded",
                [](const std::any&) {});
            Events::Ref().RegisterEventWithData("PluginUnloaded",
                [](const std::any&) {});
        }

        Events::Ref().RegisterEventWithData("SettingsChanged",
            [](const std::any&) {});

        Events::Ref().RegisterEvent("OpenSettings", [this]() {
            GetSettingsView().Show();
            });
    }

    void StudioCore::SetActiveWorkspace(GUI::WorkspaceID workspaceID) {
        if (studioContext && studioContext->viewManager) {
            studioContext->viewManager->SetActiveWorkspace(workspaceID);
            auto projectSystem =
                GetEntityManager().GetSystem<ECS::ProjectSystem>();
            if (projectSystem && projectSystem->IsProjectOpen()) {
                projectSystem->SetLastActiveWorkspace(workspaceID);
            }
        }
    }

    GUI::WorkspaceID StudioCore::GetActiveWorkspace() const {
        if (studioContext && studioContext->viewManager) {
            return studioContext->viewManager->GetActiveWorkspace();
        }
        return 0;
    }

} // namespace ANI