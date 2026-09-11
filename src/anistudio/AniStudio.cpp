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
#include "StudioRegistration.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>
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

#ifdef _WIN32
#include <GLFW/glfw3native.h>
#endif

namespace ANI {

    StudioCore::StudioCore()
        : initialized(false), running(false), windowHandle(nullptr), imguiContext(nullptr),
        m_isShuttingDown(false), m_showMissingPathsPopup(false) {
        std::cout << "[StudioCore] Constructor called" << std::endl;
        ANI_LOG_INFO("StudioCore constructor (smoke test)");
    }

    StudioCore::~StudioCore() {
        if (initialized) {
            Shutdown();
        }
    }

    bool StudioCore::InitializeCoreOnly() {
        if (initialized) {
            std::cerr << "[StudioCore] Already initialized!" << std::endl;
            return false;
        }

        try {
            std::cout << "[StudioCore] =========================================" << std::endl;
            std::cout << "[StudioCore] InitializeCoreOnly (server / headless)..." << std::endl;

            if (!engineCore.Initialize()) {
                std::cerr << "[StudioCore] Failed to initialize EngineCore!" << std::endl;
                return false;
            }

            auto engineContext = engineCore.GetEngineContext();
            if (!engineContext) {
                std::cerr << "[StudioCore] Failed to get EngineContext!" << std::endl;
                return false;
            }

            studioContext = StudioContext::FromEngine(engineContext);
            if (!studioContext || !studioContext->isValid()) {
                std::cerr << "[StudioCore] Failed to create valid StudioContext!" << std::endl;
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

            std::cout << "[StudioCore] Core-only initialization complete (no GUI)." << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[StudioCore] Core-only initialization failed: " << e.what() << std::endl;
            Shutdown();
            return false;
        }
    }

    bool StudioCore::InitializeGUI() {
        if (!initialized || !studioContext) {
            std::cerr << "[StudioCore] InitializeGUI called before InitializeCoreOnly!" << std::endl;
            return false;
        }
        if (studioContext->isServer()) {
            std::cerr << "[StudioCore] InitializeGUI skipped in Server mode." << std::endl;
            return true;
        }
        if (!imguiContext) {
            std::cerr << "[StudioCore] InitializeGUI requires a valid ImGui context!" << std::endl;
            return false;
        }

        try {
            std::cout << "[StudioCore] InitializeGUI..." << std::endl;

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
                std::cout << "[StudioCore] Set DefaultProject to: "
                    << defaultProjectPath << std::endl;
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
                std::cerr << "[StudioCore] ERROR: ImGui fonts not loaded!" << std::endl;
                return false;
            }

            InitializeStudioPlugins();

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

            std::cout << "[StudioCore] GUI initialization complete. "
                << "Show startup view: "
                << (m_showProjectManagerView ? "YES" : "NO") << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[StudioCore] InitializeGUI failed: " << e.what() << std::endl;
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
            std::cerr << "[StudioCore] Invalid context provided to CreateWithContext!"
                << std::endl;
            return nullptr;
        }

        auto studioCore = std::make_unique<StudioCore>();
        studioCore->studioContext = existingContext;

        if (!studioCore->engineCore.Initialize()) {
            std::cerr << "[StudioCore] Failed to initialize EngineCore "
                << "with existing context!" << std::endl;
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
        studioCore->SetCoreCallbacks();
        studioCore->SetCoreEvents();

        studioCore->initialized = true;
        studioCore->running = true;

        std::cout << "[StudioCore] Created with existing context successfully" << std::endl;
        return studioCore;
    }

    void StudioCore::ConfigureImGuiIniPath() {
        auto fileSys = studioContext->entityManager->GetSystem<ECS::FilePathSystem>();
        if (!fileSys) return;

        std::string imguiIniPath = fileSys->GetPath("ImguiState");
        if (imguiIniPath.empty()) {
            std::cerr << "[StudioCore] WARNING: ImguiState path missing!" << std::endl;
            return;
        }

        std::filesystem::path iniDir = std::filesystem::path(imguiIniPath).parent_path();
        if (!iniDir.empty() && !std::filesystem::exists(iniDir)) {
            std::filesystem::create_directories(iniDir);
        }

        static std::string persistentIniPath = imguiIniPath;
        ImGui::GetIO().IniFilename = persistentIniPath.c_str();
        std::cout << "[StudioCore] ImGui INI path: "
            << ImGui::GetIO().IniFilename << std::endl;
    }

    void StudioCore::RegisterSettingsTabs() {
        auto settingsSystem =
            studioContext->entityManager->GetSystem<ECS::SettingsSystem>();
        if (!settingsSystem) return;

        EntityID settingsEntity = settingsSystem->GetSettingsEntity();
        auto& entityMgr = *studioContext->entityManager;
        if (!entityMgr.IsEntityValid(settingsEntity)) {
            std::cerr << "[StudioCore] Settings entity not valid; skipping tabs."
                << std::endl;
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

        std::cout << "[StudioCore] Core settings tabs registered." << std::endl;
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
            std::cout << "[StudioCore] Lazy creating SettingsView..." << std::endl;
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
        std::cout << "[StudioCore] ImGui context set to: " << imguiContext << std::endl;

        if (studioContext) {
            studioContext->imguiContext = context;
        }
        if (m_settingsView) {
            m_settingsView->SetImGuiContext(static_cast<ImGuiContext*>(context));
        }
    }

    void StudioCore::InitializeStudioPlugins() {
        if (!studioContext) {
            std::cerr << "[StudioCore] StudioContext not initialized!" << std::endl;
            return;
        }
        if (!imguiContext) {
            std::cerr << "[StudioCore] ERROR: ImGui context is null!" << std::endl;
            return;
        }

        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(imguiContext));
        ImGuiIO& io = ImGui::GetIO();
        if (!io.Fonts || io.Fonts->Fonts.Size == 0) {
            std::cerr << "[StudioCore] ERROR: ImGui not fully initialized!" << std::endl;
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
            std::cerr << "[StudioCore] WARNING: Using default plugin dir: "
                << pluginDirectory << std::endl;
        }
        if (!std::filesystem::exists(pluginDirectory)) {
            std::filesystem::create_directories(pluginDirectory);
        }

        studioContext->studioPluginManager->scanPluginDirectory(pluginDirectory);
        studioContext->studioPluginManager->enableHotReload(true);

        std::cout << "[StudioCore] Plugin system initialized (hot reload on)."
            << std::endl;
    }

    void StudioCore::SetupProjectCallbacks() {
        auto projectSystem =
            GetEntityManager().GetSystem<ECS::ProjectSystem>();
        if (!projectSystem) {
            std::cerr << "[StudioCore] ProjectSystem not initialized!" << std::endl;
            return;
        }

        if (studioContext && studioContext->studioPluginManager) {
            projectSystem->SetPluginManager(
                studioContext->studioPluginManager.get());
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
            std::cerr << "[StudioCore] FilePathSystem not available!" << std::endl;
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
        std::cout << "[StudioCore] Project loaded: " << projectPath << std::endl;

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

        if (studioContext && studioContext->studioPluginManager) {
            studioContext->studioPluginManager->SetProjectContext(projectPath);
        }

        m_showProjectManagerView = false;
        Utils::ImGuiStateUtils::OnProjectLoaded(projectPath);
        Events::Ref().QueueEventWithData("ProjectOpened", projectPath);
    }

    void StudioCore::OnProjectCreated(const std::string& projectPath) {
        std::cout << "[StudioCore] Project created: " << projectPath << std::endl;

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

        if (studioContext && studioContext->studioPluginManager) {
            studioContext->studioPluginManager->SetProjectContext(projectPath);
        }

        m_showProjectManagerView = false;
        Utils::ImGuiStateUtils::OnProjectCreated(projectPath);
        Events::Ref().QueueEventWithData("ProjectCreated", projectPath);
    }

    void StudioCore::OnProjectClosed() {
        std::cout << "[StudioCore] OnProjectClosed() called" << std::endl;

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

        std::cout << "[StudioCore] Starting shutdown sequence..." << std::endl;
        running = false;
        m_isShuttingDown = true;

        try {
            if (studioContext && !studioContext->isServer()) {
                auto projectSystem =
                    GetEntityManager().GetSystem<ECS::ProjectSystem>();
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
            std::cerr << "[StudioCore] Exception during shutdown: "
                << e.what() << std::endl;
        }

        initialized = false;
        std::cout << "[StudioCore] Shutdown complete." << std::endl;
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
            std::cerr << "[StudioCore] Update error: " << e.what() << std::endl;
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
        }
        catch (const std::exception& e) {
            std::cerr << "[StudioCore] Render error: " << e.what() << std::endl;
        }
    }

    void StudioCore::SetCoreCallbacks() {
        std::cout << "[StudioCore] Setting up core system callbacks..." << std::endl;

        auto& entityMgr = GetEntityManager();
        auto textureSystem = entityMgr.GetSystem<TextureSystem>();
        auto imageSystem = entityMgr.GetSystem<ImageSystem>();
        auto videoSystem = entityMgr.GetSystem<ECS::VideoSystem>();

        if (!textureSystem || !imageSystem) {
            std::cerr << "[StudioCore] ERROR: Required systems missing." << std::endl;
            return;
        }

        imageSystem->RegisterImageAddedCallback(
            [this, textureSystem](EntityID entityID) {
                auto& mgr = GetEntityManager();
                if (mgr.HasComponent<ImageComponent>(entityID)) {
                    auto& img = mgr.GetComponent<ImageComponent>(entityID);
                    textureSystem->QueueTextureCreation(entityID, img.imageData,
                        img.width, img.height, img.channels);
                    ANI::Events::Ref().QueueEventWithData("ImageLoaded", entityID);
                }
            });

        imageSystem->RegisterImageRemovedCallback(
            [this, textureSystem](EntityID entityID) {
                textureSystem->RemoveTexture(entityID);
                ANI::Events::Ref().QueueEventWithData("ImageRemoved", entityID);
            });

        if (videoSystem) {
            videoSystem->SetVideoTextureCallback(
                [textureSystem](ECS::EntityID entityID, unsigned char* data,
                    int width, int height, int channels,
                    GLuint* targetTexture) {
                        textureSystem->QueueVideoTextureCreation(
                            entityID, data, width, height, channels, targetTexture);
                });
        }
    }

    void StudioCore::SetCoreEvents() {
        std::cout << "[StudioCore] Registering core system events..." << std::endl;

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
                        std::cerr << "[StudioCore] LoadImageRequest error: "
                            << e.what() << std::endl;
                    }
                });

            Events::Ref().RegisterEventWithData("RemoveImageRequest",
                [imageSystem](const std::any& data) {
                    try {
                        imageSystem->RemoveImage(
                            std::any_cast<ECS::EntityID>(data));
                    }
                    catch (const std::exception& e) {
                        std::cerr << "[StudioCore] RemoveImageRequest error: "
                            << e.what() << std::endl;
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
                        std::cerr << "[StudioCore] LoadVideoRequest error: "
                            << e.what() << std::endl;
                    }
                });

            Events::Ref().RegisterEventWithData("RemoveVideoRequest",
                [videoSystem](const std::any& data) {
                    try {
                        videoSystem->RemoveVideo(
                            std::any_cast<ECS::EntityID>(data));
                    }
                    catch (const std::exception& e) {
                        std::cerr << "[StudioCore] RemoveVideoRequest error: "
                            << e.what() << std::endl;
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
                    std::cerr << "[StudioCore] SetActiveWorkspace error: "
                        << e.what() << std::endl;
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
                    std::cerr << "[StudioCore] CreateWorkspace error: "
                        << e.what() << std::endl;
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
                    std::cerr << "[StudioCore] DeleteWorkspace error: "
                        << e.what() << std::endl;
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
                    std::cerr << "[StudioCore] AddView error: "
                        << e.what() << std::endl;
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
                    std::cerr << "[StudioCore] RemoveView error: "
                        << e.what() << std::endl;
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
                    std::cerr << "[StudioCore] DestroyEntity error: "
                        << e.what() << std::endl;
                }
            });

        Events::Ref().RegisterEventWithData("CloneEntity",
            [this](const std::any& data) {
                try {
                    GetEntityManager().CloneEntity(
                        std::any_cast<ECS::EntityID>(data));
                }
                catch (const std::exception& e) {
                    std::cerr << "[StudioCore] CloneEntity error: "
                        << e.what() << std::endl;
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
                    std::cerr << "[StudioCore] AddComponent error: "
                        << e.what() << std::endl;
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
                    std::cerr << "[StudioCore] RemoveComponent error: "
                        << e.what() << std::endl;
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