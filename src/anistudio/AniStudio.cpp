#include "AniStudio.hpp"
#include "OpenGLWrapper.hpp"
#include "AllViews.h"
#include "StudioContext.hpp"
#include "FilePathSystem.hpp"
#include "FileDialogUtil.hpp"
#include "ImGuiSettingsUtil.hpp"
#include "ImGuiStateUtils.hpp"
#include "Events.hpp"
#include "guiComponents.h"
#include "guiSystems.h"
#include "SettingsView.hpp"
#include "MenuBar.hpp"
#include "ProjectManagerView.hpp"
#include "DragDropUtils.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <imgui.h>
#include "GuiStyleHelpers.hpp"
#include "FileDialogUtil.hpp"
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

#ifdef _WIN32
#include <GLFW/glfw3native.h>
#endif

namespace ANI {

    StudioCore::StudioCore()
        : initialized(false), running(false), windowHandle(nullptr), imguiContext(nullptr),
        m_isShuttingDown(false), m_showMissingPathsPopup(false) {
        std::cout << "[StudioCore] Constructor called" << std::endl;
    }

    StudioCore::~StudioCore() {
        if (initialized) {
            Shutdown();
        }
    }

    GUI::SettingsView& StudioCore::GetSettingsView() {
        if (!m_settingsView) {
            std::cout << "[StudioCore] Lazy creating SettingsView..." << std::endl;
            m_settingsView = std::make_unique<GUI::SettingsView>();

            if (imguiContext) {
                m_settingsView->SetImGuiContext(static_cast<ImGuiContext*>(imguiContext));
                std::cout << "[StudioCore] Injected ImGui context into SettingsView" << std::endl;
            }

            if (studioContext && studioContext->entityManager) {
                m_settingsView->SetEntityManager(*studioContext->entityManager);
                std::cout << "[StudioCore] Injected EntityManager into SettingsView" << std::endl;
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

        if (studioContext && studioContext->entityManager) {
            auto settingsSystem = studioContext->entityManager->GetSystem<ECS::SettingsSystem>();
            if (settingsSystem) {
                settingsSystem->SetImGuiContext(static_cast<ImGuiContext*>(context));
                settingsSystem->LoadAllSettings();
                std::cout << "[StudioCore] SettingsSystem updated with new ImGui context." << std::endl;
            }
        }
    }

    void StudioCore::RegisterCoreViews() {
        if (!studioContext || !studioContext->viewManager) {
            std::cerr << "[StudioCore] Context or ViewManager not initialized!" << std::endl;
            return;
        }

        std::cout << "[StudioCore] Registering core view types..." << std::endl;

        auto& viewManager = *studioContext->viewManager;

        viewManager.RegisterView<GUI::DebugView>("DebugView");
        viewManager.RegisterView<GUI::ImageView>("ImageView");
        viewManager.RegisterView<GUI::VideoView>("VideoView");
        viewManager.RegisterView<GUI::HelpView>("HelpView");
        viewManager.RegisterView<GUI::TextEditorView>("TextEditor");
        viewManager.RegisterView<GUI::MediaHistoryView>("MediaHistoryView");
        viewManager.RegisterView<GUI::AssetsView>("AssetsView");
        viewManager.RegisterView<GUI::MetadataView>("MetadataView");

        if (studioContext->studioPluginManager) {
            viewManager.RegisterViewWithFactory("PluginView", "Tools",
                [this](ECS::EntityManager& mgr, GUI::ViewManager& vm) -> std::unique_ptr<GUI::BaseView> {
                    return std::make_unique<GUI::PluginView>(mgr, vm, *studioContext->studioPluginManager);
                },
                []() -> GUI::ViewMetadata {
                    return GUI::BaseView::GetMetadataFor<GUI::PluginView>();
                }
            );
        }

        viewManager.RegisterViewWithFactory("WorkspaceView", "Views",
            [this](ECS::EntityManager& mgr, GUI::ViewManager& vm) -> std::unique_ptr<GUI::BaseView> {
                return std::make_unique<GUI::WorkspaceView>(mgr, vm);
            },
            []() -> GUI::ViewMetadata { return GUI::WorkspaceView::GetMetadata(); }
        );

        std::cout << "[StudioCore] Core view types registered successfully!" << std::endl;
    }

    void StudioCore::InitializeStudioPlugins() {
        if (!studioContext) {
            std::cerr << "[StudioCore] StudioContext not initialized!" << std::endl;
            return;
        }

        std::cout << "[StudioCore] Initializing studio plugin system..." << std::endl;

        if (!imguiContext) {
            std::cerr << "[StudioCore] ERROR: ImGui context is null! Cannot initialize plugins." << std::endl;
            return;
        }

        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(imguiContext));
        ImGuiContext* currentContext = ImGui::GetCurrentContext();
        std::cout << "[StudioCore] Using main ImGui context for plugins: " << currentContext << std::endl;

        ImGuiIO& io = ImGui::GetIO();
        if (!io.Fonts) {
            std::cerr << "[StudioCore] ERROR: ImGui fonts not initialized!" << std::endl;
            return;
        }

        if (io.Fonts->Fonts.Size == 0) {
            std::cerr << "[StudioCore] ERROR: No fonts loaded in ImGui!" << std::endl;
            return;
        }

        std::cout << "[StudioCore] ImGui context verified - fonts loaded: " << io.Fonts->Fonts.Size << std::endl;

        studioContext->studioPluginManager = std::make_shared<Plugins::StudioPluginManager>(
            *studioContext->entityManager,
            *studioContext->viewManager,
            static_cast<ImGuiContext*>(imguiContext)
        );

        if (studioContext->studioPluginManager) {
            auto engineContext = std::static_pointer_cast<ANI::EngineContext>(studioContext);
            studioContext->studioPluginManager->SetEngineContext(engineContext);
            studioContext->studioPluginManager->SetStudioContext(studioContext);
            std::cout << "[StudioCore] StudioPluginManager context initialized with StudioContext" << std::endl;
        }

        auto fileSys = studioContext->entityManager->GetSystem<ECS::FilePathSystem>();
        std::string pluginDirectory;
        if (fileSys) {
            pluginDirectory = fileSys->GetPath("Plugins");
        }
        if (pluginDirectory.empty()) {
            pluginDirectory = "./plugins";
            std::cerr << "[StudioCore] WARNING: Plugin directory not found, using default: " << pluginDirectory << std::endl;
        }

        if (!std::filesystem::exists(pluginDirectory)) {
            std::filesystem::create_directories(pluginDirectory);
            std::cout << "[StudioCore] Created plugin directory: " << pluginDirectory << std::endl;
        }

        studioContext->studioPluginManager->scanPluginDirectory(pluginDirectory);
        studioContext->studioPluginManager->enableHotReload(true);

        std::cout << "[StudioCore] Studio plugin system initialized with hot reload enabled" << std::endl;
    }

    void StudioCore::SetupProjectCallbacks() {
        auto projectSystem = GetEntityManager().GetSystem<ProjectSystem>();
        if (!projectSystem) {
            std::cerr << "[StudioCore] ProjectSystem not initialized!" << std::endl;
            return;
        }

        if (studioContext && studioContext->studioPluginManager) {
            projectSystem->SetPluginManager(studioContext->studioPluginManager.get());
        }

        projectSystem->SetProjectLoadedCallback([this](const std::string& projectPath) {
            OnProjectLoaded(projectPath);
            });

        projectSystem->SetProjectCreatedCallback([this](const std::string& projectPath) {
            OnProjectCreated(projectPath);
            });

        projectSystem->SetProjectClosedCallback([this]() {
            if (!m_isShuttingDown) {
                OnProjectClosed();
            }
            });

        projectSystem->SetViewStateLoadedCallback([this](GUI::WorkspaceID activeWorkspaceID) {
            std::cout << "[StudioCore] Syncing ViewManager with loaded active workspace: " << activeWorkspaceID << std::endl;
            if (studioContext && studioContext->viewManager) {
                studioContext->viewManager->SetActiveWorkspace(activeWorkspaceID);
            }
            });
    }

    void StudioCore::InitializeWindowState() {
        auto fileSys = studioContext->entityManager->GetSystem<ECS::FilePathSystem>();
        if (!fileSys) {
            std::cerr << "[StudioCore] FilePathSystem not available!" << std::endl;
            return;
        }

        std::string dataPath = fileSys->GetPath("DataPath");
        m_windowState.SetGlobalDataPath(dataPath);

        std::string defaultPath = GetDefaultWindowStatePath();
        if (std::filesystem::exists(defaultPath)) {
            std::cout << "[StudioCore] Loading default window state from: " << defaultPath << std::endl;
            m_windowState.LoadFromFile(defaultPath);
            ApplyWindowStateToGLFW();
        }
        else {
            std::cout << "[StudioCore] No default window state found, using current configuration" << std::endl;
            SyncWindowStateFromGLFW();
        }

        std::cout << "[StudioCore] Window state initialized" << std::endl;
    }

    void StudioCore::SetWindowHandle(void* window) {
        windowHandle = window;

        if (window) {
            std::cout << "[StudioCore] Window handle set for WindowState utility" << std::endl;
            std::cout << "[StudioCore] Received GLFWwindow* pointer: " << window << std::endl;

#ifdef _WIN32
            GLFWwindow* glfwWin = static_cast<GLFWwindow*>(window);
            HWND hwnd = glfwGetWin32Window(glfwWin);
            std::cout << "[StudioCore] glfwGetWin32Window returned HWND: " << hwnd << std::endl;

            FileDialog::SetGlobalWindowHandle(static_cast<GLFWwindow*>(window));
            std::cout << "[StudioCore] Set parent window handle for file dialogs: " << hwnd << std::endl;
            GUI::ViewManager::SetWindowHandle(hwnd);
            GUI::DragDrop::SetWindowHandle(hwnd);
#else
            GUI::ViewManager::SetWindowHandle(window);
            GUI::DragDrop::SetWindowHandle(window);
#endif

            auto projectSystem = GetEntityManager().GetSystem<ProjectSystem>();
            if (projectSystem) {
                projectSystem->SetWindowHandle(window);
            }

            if (studioContext) {
                studioContext->windowHandle = window;
            }

            GUI::DragDrop::InitializeFileDrop(static_cast<GLFWwindow*>(window));

            if (initialized) {
                InitializeWindowState();
            }
        }
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
        currentState["maximized"] = (glfwGetWindowAttrib(glfwWindow, GLFW_MAXIMIZED) == GLFW_TRUE);
        currentState["fullscreen"] = (glfwGetWindowMonitor(glfwWindow) != nullptr);
        currentState["vsync"] = true;
        currentState["title"] = "AniStudio";

        m_windowState.Deserialize(currentState);
        std::cout << "[StudioCore] Synced WindowState with current GLFW window" << std::endl;
    }

    void StudioCore::ApplyWindowStateToGLFW() {
        if (!windowHandle) return;

        GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(windowHandle);

        int width = std::max(m_windowState.GetWidth(), Utils::MIN_WINDOW_WIDTH);
        int height = std::max(m_windowState.GetHeight(), Utils::MIN_WINDOW_HEIGHT);

        glfwSetWindowSize(glfwWindow, width, height);
        glfwSetWindowPos(glfwWindow, m_windowState.GetPosX(), m_windowState.GetPosY());

        if (m_windowState.IsMaximized()) {
            glfwMaximizeWindow(glfwWindow);
        }
        else {
            glfwRestoreWindow(glfwWindow);
        }

        glfwSetWindowAttrib(glfwWindow, GLFW_DECORATED, GLFW_TRUE);
        glfwShowWindow(glfwWindow);

        glfwSwapInterval(1);

        std::cout << "[StudioCore] Applied WindowState to GLFW window: "
            << width << "x" << height
            << " at (" << m_windowState.GetPosX() << "," << m_windowState.GetPosY() << ")" << std::endl;
    }

    bool StudioCore::Initialize() {
        if (initialized) {
            std::cerr << "[StudioCore] Already initialized!" << std::endl;
            return false;
        }

        try {
            std::cout << "[StudioCore] =========================================" << std::endl;
            std::cout << "[StudioCore] Initializing StudioCore..." << std::endl;

            std::cout << "[StudioCore] Initializing EngineCore..." << std::endl;
            if (!engineCore.Initialize()) {
                std::cerr << "[StudioCore] Failed to initialize EngineCore!" << std::endl;
                return false;
            }

            auto engineContext = engineCore.GetEngineContext();
            if (!engineContext) {
                std::cerr << "[StudioCore] Failed to get EngineContext from EngineCore!" << std::endl;
                return false;
            }

            studioContext = StudioContext::FromEngine(engineContext);
            if (!studioContext || !studioContext->isValid()) {
                std::cerr << "[StudioCore] Failed to create valid StudioContext!" << std::endl;
                return false;
            }

            studioContext->viewManager->SetEntityManager(*studioContext->entityManager);

            auto& entityMgr = GetEntityManager();

            entityMgr.RegisterComponent<ECS::ImGuiStyleSettingsComponent>("ImGuiStyleSettings");
            entityMgr.RegisterComponent<ECS::ImGuiRenderSettingsComponent>("ImGuiRenderSettings");
            entityMgr.RegisterComponent<ECS::FontSettingsComponent>("FontSettings");
            entityMgr.RegisterComponent<ECS::TextEditorSettingsComponent>("TextEditorSettings");

            entityMgr.RegisterSystem<TextureSystem>();
            entityMgr.RegisterSystem<ECS::SettingsSystem>();
            entityMgr.RegisterSystem<ProjectSystem>();

            auto projectSystem = entityMgr.GetSystem<ProjectSystem>();
            if (projectSystem) {
                projectSystem->SetWindowHandle(windowHandle);
                if (studioContext && studioContext->viewManager) {
                    projectSystem->SetViewManager(studioContext->viewManager.get());
                }

                auto fileSys = entityMgr.GetSystem<ECS::FilePathSystem>();
                if (fileSys) {
                    std::string defaultPath = fileSys->GetPath("DefaultProject");
                    if (!defaultPath.empty()) {
                        projectSystem->SetDefaultProjectPath(defaultPath);
                    }
                }
            }

            m_projectManagerView = std::make_unique<GUI::ProjectManagerView>(*projectSystem, this);

            auto fileSys = entityMgr.GetSystem<ECS::FilePathSystem>();
            std::string defaultProjectPath;
            if (fileSys) {
                defaultProjectPath = fileSys->GetPath("DefaultProject");
            }

            if (defaultProjectPath.empty()) {
                std::string exeDir = ".";
                if (!exeDir.empty()) {
                    std::filesystem::path basePath = std::filesystem::path(exeDir).parent_path();
                    defaultProjectPath = (basePath / "projects").string();
                    if (fileSys) {
                        fileSys->SetPath("DefaultProject", defaultProjectPath);
                    }
                    std::cout << "[StudioCore] Set DefaultProject to: " << defaultProjectPath << std::endl;
                }
            }

            if (!defaultProjectPath.empty() && !std::filesystem::exists(defaultProjectPath)) {
                std::filesystem::create_directories(defaultProjectPath);
            }

            SetupProjectCallbacks();
            SetCoreCallbacks();
            SetCoreEvents();

            initialized = true;
            running = true;

            std::cout << "[StudioCore] StudioCore initialized successfully!" << std::endl;
            std::cout << "[StudioCore] =========================================" << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[StudioCore] Initialization failed: " << e.what() << std::endl;
            Shutdown();
            return false;
        }
    }

    std::unique_ptr<StudioCore> StudioCore::CreateWithContext(std::shared_ptr<StudioContext> existingContext) {
        if (!existingContext || !existingContext->isValid()) {
            std::cerr << "[StudioCore] Invalid context provided to CreateWithContext!" << std::endl;
            return nullptr;
        }

        auto studioCore = std::make_unique<StudioCore>();
        studioCore->studioContext = existingContext;

        if (!studioCore->engineCore.Initialize()) {
            std::cerr << "[StudioCore] Failed to initialize EngineCore with existing context!" << std::endl;
            return nullptr;
        }

        studioCore->studioContext->viewManager->SetEntityManager(*studioCore->studioContext->entityManager);

        auto& entityMgr = studioCore->GetEntityManager();
        entityMgr.RegisterSystem<TextureSystem>();

        auto projectSystem = entityMgr.GetSystem<ProjectSystem>();
        studioCore->m_projectManagerView = std::make_unique<GUI::ProjectManagerView>(*projectSystem, studioCore.get());

        auto fileSys = studioCore->studioContext->entityManager->GetSystem<ECS::FilePathSystem>();
        std::string defaultProjectPath;
        if (fileSys) {
            defaultProjectPath = fileSys->GetPath("DefaultProject");
        }
        if (!defaultProjectPath.empty() && !std::filesystem::exists(defaultProjectPath)) {
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

    void StudioCore::CompleteInitialization() {
        static bool completedInitialization = false;
        if (completedInitialization) return;

        std::cout << "[StudioCore] Completing full initialization..." << std::endl;

        if (!imguiContext) {
            std::cerr << "[StudioCore] ERROR: ImGui context is null! Cannot complete initialization." << std::endl;
            return;
        }

        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(imguiContext));
        ImGuiContext* currentContext = ImGui::GetCurrentContext();
        std::cout << "[StudioCore] Using main ImGui context: " << currentContext << std::endl;

        auto fileSys = studioContext->entityManager->GetSystem<ECS::FilePathSystem>();
        if (!fileSys) {
            std::cerr << "[StudioCore] ERROR: FilePathSystem not available!" << std::endl;
            return;
        }

        std::string defaultProjectPath = fileSys->GetPath("DefaultProject");
        if (defaultProjectPath.empty()) {
            std::cerr << "[StudioCore] CRITICAL: DefaultProject path is still empty!" << std::endl;
            std::string exeDir = ".";
            if (!exeDir.empty()) {
                std::filesystem::path basePath = std::filesystem::path(exeDir).parent_path();
                defaultProjectPath = (basePath / "projects").string();
                fileSys->SetPath("DefaultProject", defaultProjectPath);
                std::cout << "[StudioCore] EMERGENCY RECOVERY: Set DefaultProject to: " << defaultProjectPath << std::endl;
            }
        }

        if (!defaultProjectPath.empty() && !std::filesystem::exists(defaultProjectPath)) {
            std::filesystem::create_directories(defaultProjectPath);
            std::cout << "[StudioCore] Created default project directory: " << defaultProjectPath << std::endl;
        }

        std::cout << "[StudioCore] Setting proper INI file path from FilePathSystem..." << std::endl;
        ImGuiIO& io = ImGui::GetIO();

        std::string imguiIniPath = fileSys->GetPath("ImguiState");
        if (!imguiIniPath.empty()) {
            std::filesystem::path iniDir = std::filesystem::path(imguiIniPath).parent_path();
            if (!iniDir.empty() && !std::filesystem::exists(iniDir)) {
                std::filesystem::create_directories(iniDir);
                std::cout << "[StudioCore] Created directory for ImGui INI file: " << iniDir.string() << std::endl;
            }

            static std::string persistentIniPath = imguiIniPath;
            io.IniFilename = persistentIniPath.c_str();
            std::cout << "[StudioCore] ImGui INI file path updated to: " << io.IniFilename << std::endl;
        }
        else {
            std::cerr << "[StudioCore] WARNING: Could not get ImguiState path from FilePathSystem!" << std::endl;
        }

        if (!io.Fonts || io.Fonts->Fonts.Size == 0) {
            std::cerr << "[StudioCore] ERROR: ImGui fonts not loaded!" << std::endl;
            return;
        }

        std::cout << "[StudioCore] ImGui is ready" << std::endl;

        std::string dataPath = fileSys->GetPath("DataPath");
        if (dataPath.empty()) {
            dataPath = "./data";
            fileSys->SetPath("DataPath", dataPath);
        }
        if (!std::filesystem::exists(dataPath)) {
            std::filesystem::create_directories(dataPath);
        }

        std::string pathsFile = dataPath + "/paths.json";
        fileSys->LoadFromFile(pathsFile);

        EnsureCorePaths();

        Utils::CheckMissingPaths(fileSys.get());

        InitializeStudioPlugins();

        Utils::CheckMissingPaths(fileSys.get());

        RegisterCoreViews();
        std::cout << "[StudioCore] Core views registered" << std::endl;

        auto settingsSystem = studioContext->entityManager->GetSystem<ECS::SettingsSystem>();
        if (settingsSystem) {
            EntityID settingsEntity = settingsSystem->GetSettingsEntity();
            auto& entityMgr = *studioContext->entityManager;
            if (entityMgr.IsEntityValid(settingsEntity)) {
                auto& generalComp = entityMgr.GetComponent<ECS::GeneralSettingsComponent>(settingsEntity);
                auto& styleComp = entityMgr.GetComponent<ECS::ImGuiStyleSettingsComponent>(settingsEntity);
                auto& renderComp = entityMgr.GetComponent<ECS::ImGuiRenderSettingsComponent>(settingsEntity);
                auto& fontComp = entityMgr.GetComponent<ECS::FontSettingsComponent>(settingsEntity);
                auto& textEditorComp = entityMgr.GetComponent<ECS::TextEditorSettingsComponent>(settingsEntity);

                auto generalTab = std::make_unique<ECS::GeneralSettingsTab>(generalComp);
                settingsSystem->RegisterTab(std::move(generalTab));

                auto styleTab = std::make_unique<ECS::ImGuiStyleSettingsTab>(styleComp);
                settingsSystem->RegisterTab(std::move(styleTab));

                auto renderTab = std::make_unique<ECS::ImGuiRenderSettingsTab>(renderComp);
                settingsSystem->RegisterTab(std::move(renderTab));

                auto fontTab = std::make_unique<ECS::FontSettingsTab>(fontComp);
                settingsSystem->RegisterTab(std::move(fontTab));

                auto textEditorTab = std::make_unique<ECS::TextEditorSettingsTab>(textEditorComp, fontComp);
                settingsSystem->RegisterTab(std::move(textEditorTab));

                UISchema::StringWidgets::SetSettingsComponent(&textEditorComp);
                TextEditorUtil::SetSettingsComponent(&textEditorComp);
                TextEditorUtil::SetFontComponent(&fontComp);

                std::cout << "[StudioCore] Registered all core settings tabs." << std::endl;

                for (auto& tab : settingsSystem->GetTabs()) {
                    tab->LoadSettings();
                    tab->CreateBackup();
                }
            }
            else {
                std::cerr << "[StudioCore] Settings entity not valid yet, cannot register tabs." << std::endl;
            }
        }

        m_projectManagerView->Init();
        std::cout << "[StudioCore] ProjectManagerView initialized" << std::endl;

        auto projectSystem = GetEntityManager().GetSystem<ProjectSystem>();
        m_menuBar = std::make_unique<GUI::MenuBar>(*projectSystem, *studioContext->viewManager, *this);
        std::cout << "[StudioCore] MenuBar created" << std::endl;

        if (projectSystem) {
            m_showProjectManagerView = projectSystem->ShouldShowStartup();
            std::cout << "[StudioCore] Should show startup view: " << (m_showProjectManagerView ? "YES" : "NO") << std::endl;
        }
        else {
            m_showProjectManagerView = true;
            std::cout << "[StudioCore] No ProjectSystem, showing startup view" << std::endl;
        }

        completedInitialization = true;
        std::cout << "[StudioCore] Complete initialization finished!" << std::endl;
    }

    void StudioCore::OnProjectLoaded(const std::string& projectPath) {
        std::cout << "[StudioCore] Project loaded: " << projectPath << std::endl;

        auto fileSys = studioContext->entityManager->GetSystem<ECS::FilePathSystem>();
        if (fileSys) {
            fileSys->SetPath("CurrentProject", projectPath);
            fileSys->SetPath("ProjectData", projectPath + "/data");
            fileSys->SetPath("Assets", projectPath + "/assets");
            fileSys->SetPath("Output", projectPath + "/output");
            std::filesystem::create_directories(projectPath + "/data");
            std::filesystem::create_directories(projectPath + "/assets");
            std::filesystem::create_directories(projectPath + "/output");
        }

        if (studioContext && studioContext->studioPluginManager) {
            std::cout << "[StudioCore] Setting plugin manager project context: " << projectPath << std::endl;

            bool useNewest = false;
            auto settingsSystem = studioContext->entityManager->GetSystem<ECS::SettingsSystem>();
            if (settingsSystem) {
                EntityID settingsEntity = settingsSystem->GetSettingsEntity();
                if (settingsEntity && studioContext->entityManager->IsEntityValid(settingsEntity)) {
                    if (studioContext->entityManager->HasComponent<ECS::GeneralSettingsComponent>(settingsEntity)) {
                        auto& generalComp = studioContext->entityManager->GetComponent<ECS::GeneralSettingsComponent>(settingsEntity);
                        useNewest = generalComp.useNewestPluginVersions;
                        std::cout << "[StudioCore] useNewestPluginVersions = " << useNewest << std::endl;
                    }
                }
            }

            studioContext->studioPluginManager->SetProjectContext(projectPath);
        }

        m_showProjectManagerView = false;
        Utils::ImGuiStateUtils::OnProjectLoaded(projectPath);

        Events::Ref().QueueEventWithData("ProjectOpened", projectPath);
    }

    void StudioCore::OnProjectCreated(const std::string& projectPath) {
        std::cout << "[StudioCore] Project created: " << projectPath << std::endl;

        auto fileSys = studioContext->entityManager->GetSystem<ECS::FilePathSystem>();
        if (fileSys) {
            fileSys->SetPath("CurrentProject", projectPath);
            fileSys->SetPath("ProjectData", projectPath + "/data");
            fileSys->SetPath("Assets", projectPath + "/assets");
            fileSys->SetPath("Output", projectPath + "/output");
            std::filesystem::create_directories(projectPath + "/data");
            std::filesystem::create_directories(projectPath + "/assets");
            std::filesystem::create_directories(projectPath + "/output");
        }

        if (studioContext && studioContext->studioPluginManager) {
            std::cout << "[StudioCore] Setting plugin manager project context for new project: " << projectPath << std::endl;
            studioContext->studioPluginManager->SetProjectContext(projectPath);
        }

        m_showProjectManagerView = false;
        Utils::ImGuiStateUtils::OnProjectCreated(projectPath);

        Events::Ref().QueueEventWithData("ProjectCreated", projectPath);
    }

    void StudioCore::OnProjectClosed() {
        std::cout << "[StudioCore] OnProjectClosed() called" << std::endl;

        auto fileSys = studioContext->entityManager->GetSystem<ECS::FilePathSystem>();
        if (fileSys) {
            fileSys->SetPath("CurrentProject", "");
            fileSys->SetPath("ProjectData", "");
            fileSys->SetPath("Assets", "");
            fileSys->SetPath("Output", "");
        }

        if (studioContext && studioContext->studioPluginManager) {
            std::cout << "[StudioCore] Saving project plugin state..." << std::endl;
            studioContext->studioPluginManager->SaveProjectPluginState();
        }

        m_showProjectManagerView = true;

        SyncWindowStateFromGLFW();
        std::string defaultPath = GetDefaultWindowStatePath();
        std::filesystem::create_directories(std::filesystem::path(defaultPath).parent_path());
        m_windowState.SaveToFile(defaultPath);
        std::cout << "[StudioCore] Saved current window state as default" << std::endl;

        Utils::ImGuiStateUtils::OnProjectClosed();

        Events::Ref().QueueEvent("ProjectClosed");
    }

    std::string StudioCore::GetDefaultWindowStatePath() const {
        auto fileSys = studioContext ? studioContext->entityManager->GetSystem<ECS::FilePathSystem>() : nullptr;
        std::string dataPath;
        if (fileSys) {
            dataPath = fileSys->GetPath("DataPath");
        }
        if (dataPath.empty()) {
            std::cerr << "[StudioCore] Data path not available from FilePathSystem!" << std::endl;
            return "";
        }
        return dataPath + "/window_state.json";
    }

    void StudioCore::Shutdown() {
        if (!initialized) return;

        std::cout << "[StudioCore] Starting shutdown sequence..." << std::endl;
        running = false;
        m_isShuttingDown = true;

        try {
            auto projectSystem = GetEntityManager().GetSystem<ProjectSystem>();
            if (projectSystem && projectSystem->IsProjectOpen()) {
                std::cout << "[StudioCore] Saving open project BEFORE shutdown: "
                    << projectSystem->GetCurrentProjectName() << std::endl;

                if (studioContext && studioContext->studioPluginManager) {
                    studioContext->studioPluginManager->SaveProjectPluginState();
                }

                projectSystem->SaveProject();
                Events::Ref().QueueEvent("ProjectSaved");
            }
            else {
                SyncWindowStateFromGLFW();
                std::string defaultPath = GetDefaultWindowStatePath();
                if (!defaultPath.empty()) {
                    std::filesystem::create_directories(std::filesystem::path(defaultPath).parent_path());
                    m_windowState.SaveToFile(defaultPath);
                    std::cout << "[StudioCore] Saved default window state during shutdown" << std::endl;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::cout << "[StudioCore] Critical saves completed" << std::endl;

            m_menuBar.reset();
            m_projectManagerView.reset();
            m_settingsView.reset();

            if (studioContext && studioContext->studioPluginManager) {
                std::cout << "[StudioCore] Shutting down studio plugin manager..." << std::endl;
                studioContext->studioPluginManager.reset();
            }

            std::cout << "[StudioCore] Shutting down view manager..." << std::endl;
            if (studioContext && studioContext->viewManager) {
                studioContext->viewManager->FullReset();
            }

            std::cout << "[StudioCore] Shutting down engine core..." << std::endl;
            engineCore.Shutdown();

            studioContext.reset();

            std::cout << "[StudioCore] All components shut down successfully" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "[StudioCore] Exception during shutdown: " << e.what() << std::endl;
        }

        initialized = false;
        std::cout << "[StudioCore] Shutdown sequence completed" << std::endl;
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

            if (m_showProjectManagerView) {
                m_projectManagerView->Update(deltaTime);
            }

            studioContext->viewManager->Update(deltaTime);
        }
        catch (const std::exception& e) {
            std::cerr << "[StudioCore] Update error: " << e.what() << std::endl;
        }
    }

    void StudioCore::Render() {
        if (!running || !initialized || !studioContext) return;

        try {
            CompleteInitialization();

            auto projectSystem = GetEntityManager().GetSystem<ProjectSystem>();
            bool isProjectOpen = projectSystem ? projectSystem->IsProjectOpen() : false;

            if (!isProjectOpen && m_showProjectManagerView && m_projectManagerView) {
                m_projectManagerView->Render();
            }

            if (isProjectOpen) {
                ImGuiViewport* viewport = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(viewport->WorkPos);
                ImGui::SetNextWindowSize(viewport->WorkSize);
                ImGui::SetNextWindowViewport(viewport->ID);

                ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
                window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
                window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
                window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
                window_flags |= ImGuiWindowFlags_NoBackground;

                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

                bool open = true;
                if (ImGui::Begin("MainDockSpaceWindow", &open, window_flags)) {
                    ImGui::PopStyleVar(3);

                    if (m_menuBar) {
                        m_menuBar->Render();
                    }

                    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
                    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

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
        auto videoSystem = entityMgr.GetSystem<VideoSystem>();

        if (textureSystem && imageSystem) {
            imageSystem->RegisterImageAddedCallback([this, textureSystem](EntityID entityID) {
                auto& entityMgr = GetEntityManager();
                if (entityMgr.HasComponent<ImageComponent>(entityID)) {
                    auto& imageComp = entityMgr.GetComponent<ImageComponent>(entityID);
                    textureSystem->QueueTextureCreation(
                        entityID,
                        imageComp.imageData,
                        imageComp.width,
                        imageComp.height,
                        imageComp.channels
                    );
                    ANI::Events::Ref().QueueEventWithData("ImageLoaded", entityID);
                }
                });

            imageSystem->RegisterImageRemovedCallback([this, textureSystem](EntityID entityID) {
                textureSystem->RemoveTexture(entityID);
                ANI::Events::Ref().QueueEventWithData("ImageRemoved", entityID);
                });

            if (videoSystem) {
                videoSystem->SetVideoTextureCallback(
                    [textureSystem](EntityID entityID, unsigned char* data, int width, int height, int channels, GLuint* targetTexture) {
                        textureSystem->QueueVideoTextureCreation(entityID, data, width, height, channels, targetTexture);
                    }
                );
                std::cout << "[StudioCore] Video texture callback connected to TextureSystem" << std::endl;
            }

            std::cout << "[StudioCore] Core system callbacks set up successfully" << std::endl;
        }
        else {
            std::cerr << "[StudioCore] ERROR: Could not find required systems" << std::endl;
        }
    }

    void StudioCore::SetCoreEvents() {
        std::cout << "[StudioCore] Registering core system events..." << std::endl;

        auto& entityMgr = GetEntityManager();
        auto imageSystem = entityMgr.GetSystem<ImageSystem>();
        auto videoSystem = entityMgr.GetSystem<VideoSystem>();
        auto projectSystem = entityMgr.GetSystem<ProjectSystem>();
        auto pluginManager = studioContext ? studioContext->studioPluginManager : nullptr;

        if (imageSystem) {
            Events::Ref().RegisterEventWithData("LoadImageRequest", [this, imageSystem](const std::any& data) {
                try {
                    auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
                    std::string filePath = std::any_cast<std::string>(eventData.at("filePath"));
                    std::cout << "[StudioCore] LoadImageRequest: " << filePath << std::endl;
                    auto& entityMgr = GetEntityManager();
                    ECS::EntityID entity = entityMgr.AddNewEntity();
                    entityMgr.AddComponent<ImageComponent>(entity);
                    imageSystem->SetImage(entity, filePath);
                    std::cout << "[StudioCore] Created entity " << entity << " for image" << std::endl;
                }
                catch (const std::exception& e) {
                    std::cerr << "[StudioCore] LoadImageRequest error: " << e.what() << std::endl;
                }
                });

            Events::Ref().RegisterEventWithData("RemoveImageRequest", [imageSystem](const std::any& data) {
                try {
                    ECS::EntityID entityID = std::any_cast<ECS::EntityID>(data);
                    std::cout << "[StudioCore] RemoveImageRequest: entity " << entityID << std::endl;
                    imageSystem->RemoveImage(entityID);
                }
                catch (const std::exception& e) {
                    std::cerr << "[StudioCore] RemoveImageRequest error: " << e.what() << std::endl;
                }
                });

            Events::Ref().RegisterEventWithData("ImageLoaded", [](const std::any& data) {
                try {
                    ECS::EntityID entityID = std::any_cast<ECS::EntityID>(data);
                    std::cout << "[StudioCore] ImageLoaded event: entity " << entityID << std::endl;
                }
                catch (const std::exception& e) {
                    std::cerr << "[StudioCore] ImageLoaded event error: " << e.what() << std::endl;
                }
                });

            Events::Ref().RegisterEventWithData("ImageRemoved", [](const std::any& data) {
                try {
                    ECS::EntityID entityID = std::any_cast<ECS::EntityID>(data);
                    std::cout << "[StudioCore] ImageRemoved event: entity " << entityID << std::endl;
                }
                catch (const std::exception& e) {
                    std::cerr << "[StudioCore] ImageRemoved event error: " << e.what() << std::endl;
                }
                });
        }

        if (videoSystem) {
            Events::Ref().RegisterEventWithData("LoadVideoRequest", [this, videoSystem](const std::any& data) {
                try {
                    auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
                    std::string filePath = std::any_cast<std::string>(eventData.at("filePath"));
                    std::cout << "[StudioCore] LoadVideoRequest: " << filePath << std::endl;
                    auto& entityMgr = GetEntityManager();
                    ECS::EntityID entity = entityMgr.AddNewEntity();
                    entityMgr.AddComponent<VideoComponent>(entity);
                    videoSystem->SetVideo(entity, filePath);
                    std::cout << "[StudioCore] Created entity " << entity << " for video" << std::endl;
                }
                catch (const std::exception& e) {
                    std::cerr << "[StudioCore] LoadVideoRequest error: " << e.what() << std::endl;
                }
                });

            Events::Ref().RegisterEventWithData("RemoveVideoRequest", [videoSystem](const std::any& data) {
                try {
                    ECS::EntityID entityID = std::any_cast<ECS::EntityID>(data);
                    std::cout << "[StudioCore] RemoveVideoRequest: entity " << entityID << std::endl;
                    videoSystem->RemoveVideo(entityID);
                }
                catch (const std::exception& e) {
                    std::cerr << "[StudioCore] RemoveVideoRequest error: " << e.what() << std::endl;
                }
                });

            Events::Ref().RegisterEventWithData("VideoLoaded", [](const std::any& data) {
                try {
                    ECS::EntityID entityID = std::any_cast<ECS::EntityID>(data);
                    std::cout << "[StudioCore] VideoLoaded event: entity " << entityID << std::endl;
                }
                catch (const std::exception& e) {
                    std::cerr << "[StudioCore] VideoLoaded event error: " << e.what() << std::endl;
                }
                });

            Events::Ref().RegisterEventWithData("VideoRemoved", [](const std::any& data) {
                try {
                    ECS::EntityID entityID = std::any_cast<ECS::EntityID>(data);
                    std::cout << "[StudioCore] VideoRemoved event: entity " << entityID << std::endl;
                }
                catch (const std::exception& e) {
                    std::cerr << "[StudioCore] VideoRemoved event error: " << e.what() << std::endl;
                }
                });
        }

        if (projectSystem) {
            Events::Ref().RegisterEventWithData("ProjectOpened", [projectSystem](const std::any& data) {
                try {
                    std::string path = std::any_cast<std::string>(data);
                    std::cout << "[StudioCore] ProjectOpened event: " << path << std::endl;
                }
                catch (const std::exception& e) {
                    std::cerr << "[StudioCore] ProjectOpened event error: " << e.what() << std::endl;
                }
                });

            Events::Ref().RegisterEventWithData("ProjectClosed", [projectSystem](const std::any& data) {
                try {
                    std::cout << "[StudioCore] ProjectClosed event" << std::endl;
                }
                catch (const std::exception& e) {
                    std::cerr << "[StudioCore] ProjectClosed event error: " << e.what() << std::endl;
                }
                });

            Events::Ref().RegisterEventWithData("ProjectCreated", [projectSystem](const std::any& data) {
                try {
                    std::string path = std::any_cast<std::string>(data);
                    std::cout << "[StudioCore] ProjectCreated event: " << path << std::endl;
                }
                catch (const std::exception& e) {
                    std::cerr << "[StudioCore] ProjectCreated event error: " << e.what() << std::endl;
                }
                });

            Events::Ref().RegisterEventWithData("ProjectSaved", [projectSystem](const std::any& data) {
                try {
                    std::cout << "[StudioCore] ProjectSaved event" << std::endl;
                }
                catch (const std::exception& e) {
                    std::cerr << "[StudioCore] ProjectSaved event error: " << e.what() << std::endl;
                }
                });

            Events::Ref().RegisterEvent("SaveProject", [projectSystem]() {
                if (projectSystem->IsProjectOpen()) {
                    projectSystem->SaveProject();
                    Events::Ref().QueueEvent("ProjectSaved");
                    std::cout << "[StudioCore] SaveProject event handled" << std::endl;
                }
                });

            Events::Ref().RegisterEvent("CloseProject", [this, projectSystem]() {
                if (projectSystem->IsProjectOpen()) {
                    projectSystem->CloseProject();
                    OnProjectClosed();
                    std::cout << "[StudioCore] CloseProject event handled" << std::endl;
                }
                });
        }

        Events::Ref().RegisterEventWithData("SetActiveWorkspace", [this](const std::any& data) {
            try {
                GUI::WorkspaceID id = std::any_cast<GUI::WorkspaceID>(data);
                if (studioContext && studioContext->viewManager) {
                    studioContext->viewManager->SetActiveWorkspace(id);
                    auto projectSystem = GetEntityManager().GetSystem<ProjectSystem>();
                    if (projectSystem) {
                        projectSystem->SetLastActiveWorkspace(id);
                    }
                    std::cout << "[StudioCore] SetActiveWorkspace event handled: " << id << std::endl;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "[StudioCore] SetActiveWorkspace error: " << e.what() << std::endl;
            }
            });

        Events::Ref().RegisterEventWithData("CreateWorkspace", [this](const std::any& data) {
            try {
                auto eventData = std::any_cast<std::unordered_map<std::string, std::string>>(data);
                std::string name = eventData.at("workspaceName");
                if (studioContext && studioContext->viewManager) {
                    GUI::WorkspaceID newID = studioContext->viewManager->CreateView();
                    studioContext->viewManager->SetWorkspaceName(newID, name);
                    studioContext->viewManager->SetActiveWorkspace(newID);
                    std::cout << "[StudioCore] CreateWorkspace event handled: " << name << " ID: " << newID << std::endl;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "[StudioCore] CreateWorkspace error: " << e.what() << std::endl;
            }
            });

        Events::Ref().RegisterEventWithData("DeleteWorkspace", [this](const std::any& data) {
            try {
                GUI::WorkspaceID id = std::any_cast<GUI::WorkspaceID>(data);
                if (studioContext && studioContext->viewManager) {
                    auto allWorkspaces = studioContext->viewManager->GetAllWorkspaces();
                    if (allWorkspaces.size() <= 1) {
                        std::cout << "[StudioCore] Cannot delete the last workspace" << std::endl;
                        return;
                    }
                    for (auto ws : allWorkspaces) {
                        if (ws != id) {
                            studioContext->viewManager->SetActiveWorkspace(ws);
                            break;
                        }
                    }
                    studioContext->viewManager->DestroyView(id);
                    std::cout << "[StudioCore] DeleteWorkspace event handled: " << id << std::endl;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "[StudioCore] DeleteWorkspace error: " << e.what() << std::endl;
            }
            });

        Events::Ref().RegisterEventWithData("AddView", [this](const std::any& data) {
            try {
                auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
                GUI::WorkspaceID workspaceID = std::any_cast<GUI::WorkspaceID>(eventData.at("workspaceID"));
                std::string viewTypeName = std::any_cast<std::string>(eventData.at("viewTypeName"));
                if (studioContext && studioContext->viewManager) {
                    GUI::ViewTypeID typeID = studioContext->viewManager->GetViewType(viewTypeName);
                    studioContext->viewManager->AddViewByType(workspaceID, typeID);
                    std::cout << "[StudioCore] AddView event handled: " << viewTypeName << " to workspace " << workspaceID << std::endl;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "[StudioCore] AddView error: " << e.what() << std::endl;
            }
            });

        Events::Ref().RegisterEventWithData("RemoveView", [this](const std::any& data) {
            try {
                auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
                GUI::WorkspaceID workspaceID = std::any_cast<GUI::WorkspaceID>(eventData.at("workspaceID"));
                std::string viewTypeName = std::any_cast<std::string>(eventData.at("viewTypeName"));
                if (studioContext && studioContext->viewManager) {
                    GUI::ViewTypeID typeID = studioContext->viewManager->GetViewType(viewTypeName);
                    studioContext->viewManager->RemoveViewByType(workspaceID, typeID);
                    std::cout << "[StudioCore] RemoveView event handled: " << viewTypeName << " from workspace " << workspaceID << std::endl;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "[StudioCore] RemoveView error: " << e.what() << std::endl;
            }
            });

        Events::Ref().RegisterEvent("CreateEntity", [this]() {
            ECS::EntityID newEntity = GetEntityManager().AddNewEntity();
            std::cout << "[StudioCore] CreateEntity event: entity " << newEntity << std::endl;
            });

        Events::Ref().RegisterEventWithData("DestroyEntity", [this](const std::any& data) {
            try {
                ECS::EntityID entityID = std::any_cast<ECS::EntityID>(data);
                GetEntityManager().DestroyEntity(entityID);
                std::cout << "[StudioCore] DestroyEntity event: entity " << entityID << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "[StudioCore] DestroyEntity error: " << e.what() << std::endl;
            }
            });

        Events::Ref().RegisterEventWithData("CloneEntity", [this](const std::any& data) {
            try {
                ECS::EntityID entityID = std::any_cast<ECS::EntityID>(data);
                ECS::EntityID newEntity = GetEntityManager().CloneEntity(entityID);
                std::cout << "[StudioCore] CloneEntity event: cloned " << entityID << " to " << newEntity << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "[StudioCore] CloneEntity error: " << e.what() << std::endl;
            }
            });

        Events::Ref().RegisterEventWithData("AddComponent", [this](const std::any& data) {
            try {
                auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
                ECS::EntityID entityID = std::any_cast<ECS::EntityID>(eventData.at("entityID"));
                ECS::ComponentTypeID componentTypeID = std::any_cast<ECS::ComponentTypeID>(eventData.at("componentTypeID"));
                auto& mgr = GetEntityManager();
                if (mgr.IsPluginComponent(componentTypeID)) {
                    mgr.AddPluginComponent(entityID, componentTypeID);
                }
                std::cout << "[StudioCore] AddComponent event: component " << componentTypeID << " to entity " << entityID << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "[StudioCore] AddComponent error: " << e.what() << std::endl;
            }
            });

        Events::Ref().RegisterEventWithData("RemoveComponent", [this](const std::any& data) {
            try {
                auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
                ECS::EntityID entityID = std::any_cast<ECS::EntityID>(eventData.at("entityID"));
                ECS::ComponentTypeID componentTypeID = std::any_cast<ECS::ComponentTypeID>(eventData.at("componentTypeID"));
                GetEntityManager().RemoveComponentById(entityID, componentTypeID);
                std::cout << "[StudioCore] RemoveComponent event: component " << componentTypeID << " from entity " << entityID << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "[StudioCore] RemoveComponent error: " << e.what() << std::endl;
            }
            });

        if (pluginManager) {
            Events::Ref().RegisterEventWithData("PluginLoaded", [pluginManager](const std::any& data) {
                try {
                    std::string name = std::any_cast<std::string>(data);
                    std::cout << "[StudioCore] PluginLoaded event: " << name << std::endl;
                }
                catch (const std::exception& e) {
                    std::cerr << "[StudioCore] PluginLoaded event error: " << e.what() << std::endl;
                }
                });

            Events::Ref().RegisterEventWithData("PluginUnloaded", [pluginManager](const std::any& data) {
                try {
                    std::string name = std::any_cast<std::string>(data);
                    std::cout << "[StudioCore] PluginUnloaded event: " << name << std::endl;
                }
                catch (const std::exception& e) {
                    std::cerr << "[StudioCore] PluginUnloaded event error: " << e.what() << std::endl;
                }
                });
        }

        Events::Ref().RegisterEventWithData("SettingsChanged", [](const std::any& data) {
            try {
                std::cout << "[StudioCore] SettingsChanged event" << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "[StudioCore] SettingsChanged event error: " << e.what() << std::endl;
            }
            });

        std::cout << "[StudioCore] Core system events registered successfully" << std::endl;
    }

    void StudioCore::SetActiveWorkspace(GUI::WorkspaceID workspaceID) {
        if (studioContext && studioContext->viewManager) {
            studioContext->viewManager->SetActiveWorkspace(workspaceID);

            auto projectSystem = GetEntityManager().GetSystem<ProjectSystem>();
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

}