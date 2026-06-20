#include "AniStudio.hpp"
#include "StudioContext.hpp"
#include "AllViews.h"
#include "FilePaths.hpp"
#include "FilePathService.hpp"
#include "ImGuiSettingsUtil.hpp"
#include "ImGuiStateUtils.hpp"
#include "Events.hpp"
#include "SettingsView.hpp"
#include "MenuBar.hpp"
#include "ProjectManagerView.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <imgui.h>
#include "GuiStyleHelpers.hpp"

namespace ANI {

    StudioCore::StudioCore()
        : initialized(false), running(false), windowHandle(nullptr), imguiContext(nullptr),
        m_isShuttingDown(false) {
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

            // Inject ImGui context if available
            if (imguiContext) {
                m_settingsView->SetImGuiContext(static_cast<ImGuiContext*>(imguiContext));
                std::cout << "[StudioCore] Injected ImGui context into SettingsView" << std::endl;
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

        // If SettingsView already exists, inject context
        if (m_settingsView) {
            m_settingsView->SetImGuiContext(static_cast<ImGuiContext*>(context));
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
        viewManager.RegisterView<GUI::ZepView>("ZepView");

        if (studioContext->studioPluginManager) {
            viewManager.RegisterViewWithFactory("PluginView", "Tools",
                [this](ECS::EntityManager& mgr) -> std::unique_ptr<GUI::BaseView> {
                    return std::make_unique<GUI::PluginView>(mgr, *studioContext->studioPluginManager);
                },
                []() -> GUI::ViewMetadata {
                    return GUI::BaseView::GetMetadataFor<GUI::PluginView>();
                }
            );
        }

        viewManager.RegisterViewWithFactory("WorkspaceView", "Views",
            [this](ECS::EntityManager& mgr) -> std::unique_ptr<GUI::BaseView> {
                return std::make_unique<GUI::WorkspaceView>(mgr, *studioContext->viewManager);
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

        // Create StudioPluginManager and store in context
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

        std::string pluginDirectory = Utils::FilePathService::GetPath("Plugins");
        if (pluginDirectory.empty()) {
            pluginDirectory = "./plugins";
            std::cerr << "[StudioCore] WARNING: Plugin directory not found in FilePathService, using default: " << pluginDirectory << std::endl;
        }

        if (!std::filesystem::exists(pluginDirectory)) {
            std::filesystem::create_directories(pluginDirectory);
            std::cout << "[StudioCore] Created plugin directory: " << pluginDirectory << std::endl;
        }

        std::string dataPath = Utils::FilePathService::GetDataPath();
        if (!dataPath.empty()) {
            std::cout << "[StudioCore] Setting global data path: " << dataPath << std::endl;
            studioContext->studioPluginManager->SetGlobalDataPath(dataPath);
        }
        else {
            std::cerr << "[StudioCore] ERROR: Data path not available from FilePathService!" << std::endl;
        }

        studioContext->studioPluginManager->scanPluginDirectory(pluginDirectory);
        studioContext->studioPluginManager->enableHotReload(false);

        std::cout << "[StudioCore] Loading global plugin state..." << std::endl;
        studioContext->studioPluginManager->LoadGlobalPluginState();

        std::cout << "[StudioCore] Studio plugin system initialized with selective loading" << std::endl;
    }

    void StudioCore::SetupProjectCallbacks() {
        if (!studioContext || !studioContext->projectManager) {
            std::cerr << "[StudioCore] Context or ProjectManager not initialized!" << std::endl;
            return;
        }

        auto& projectManager = *studioContext->projectManager;

        projectManager.SetProjectLoadedCallback([this](const std::string& projectPath) {
            OnProjectLoaded(projectPath);
            });

        projectManager.SetProjectCreatedCallback([this](const std::string& projectPath) {
            OnProjectCreated(projectPath);
            });

        projectManager.SetProjectClosedCallback([this]() {
            if (!m_isShuttingDown) {
                OnProjectClosed();
            }
            });

        projectManager.SetViewStateLoadedCallback([this](GUI::WorkspaceID activeWorkspaceID) {
            std::cout << "[StudioCore] Syncing ViewManager with loaded active workspace: " << activeWorkspaceID << std::endl;
            studioContext->viewManager->SetActiveWorkspace(activeWorkspaceID);
            });
    }

    void StudioCore::InitializeWindowState() {
        if (!studioContext || !studioContext->filePaths) {
            std::cerr << "[StudioCore] Context or FilePaths not initialized!" << std::endl;
            return;
        }

        m_windowState.SetGlobalDataPath(studioContext->filePaths->GetDataPath());

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

            if (studioContext && studioContext->projectManager) {
                studioContext->projectManager->SetWindowHandle(window);
            }

            if (studioContext) {
                studioContext->windowHandle = window;
            }

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

        glfwSetWindowSize(glfwWindow, m_windowState.GetWidth(), m_windowState.GetHeight());
        glfwSetWindowPos(glfwWindow, m_windowState.GetPosX(), m_windowState.GetPosY());

        if (m_windowState.IsMaximized()) {
            glfwMaximizeWindow(glfwWindow);
        }
        else {
            glfwRestoreWindow(glfwWindow);
        }

        glfwSwapInterval(1);

        std::cout << "[StudioCore] Applied WindowState to GLFW window: "
            << m_windowState.GetWidth() << "x" << m_windowState.GetHeight()
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

            // Create ProjectManagerView with StudioCore reference
            m_projectManagerView = std::make_unique<GUI::ProjectManagerView>(*studioContext->projectManager, this);

            studioContext->entityManager->RegisterSystem<TextureSystem>();

            if (!Utils::FilePathService::IsInitialized()) {
                Utils::FilePathService::SetInstance(studioContext->filePaths);
            }

            std::string defaultProjectPath = Utils::FilePathService::GetPath("DefaultProject");
            if (defaultProjectPath.empty()) {
                std::string exeDir = ".";
                if (!exeDir.empty()) {
                    std::filesystem::path basePath = std::filesystem::path(exeDir).parent_path();
                    defaultProjectPath = (basePath / "projects").string();
                    Utils::FilePathService::SetPath("DefaultProject", defaultProjectPath);
                    Utils::FilePathService::SaveFilepathDefaults();
                    std::cout << "[StudioCore] Set DefaultProject to: " << defaultProjectPath << std::endl;
                }
            }

            if (!defaultProjectPath.empty() && !std::filesystem::exists(defaultProjectPath)) {
                std::filesystem::create_directories(defaultProjectPath);
            }

            studioContext->projectManager->SetDefaultProjectPath(defaultProjectPath);

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

        if (!Utils::FilePathService::IsInitialized()) {
            std::cout << "[StudioCore] Initializing FilePathService in CreateWithContext..." << std::endl;
            Utils::FilePathService::Init();
        }

        if (!studioCore->engineCore.Initialize()) {
            std::cerr << "[StudioCore] Failed to initialize EngineCore with existing context!" << std::endl;
            return nullptr;
        }

        studioCore->studioContext->viewManager->SetEntityManager(*studioCore->studioContext->entityManager);

        // Create ProjectManagerView with StudioCore reference
        studioCore->m_projectManagerView = std::make_unique<GUI::ProjectManagerView>(*studioCore->studioContext->projectManager, studioCore.get());

        std::string defaultProjectPath = Utils::FilePathService::GetPath("DefaultProject");
        if (!defaultProjectPath.empty() && !std::filesystem::exists(defaultProjectPath)) {
            std::filesystem::create_directories(defaultProjectPath);
        }

        studioCore->studioContext->entityManager->RegisterSystem<TextureSystem>();

        studioCore->SetupProjectCallbacks();
        studioCore->SetCoreCallbacks();
        studioCore->SetCoreEvents();

        studioCore->initialized = true;
        studioCore->running = true;

        std::cout << "[StudioCore] Created with existing context successfully" << std::endl;
        return studioCore;
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

        if (!Utils::FilePathService::IsInitialized()) {
            std::cerr << "[StudioCore] ERROR: FilePathService not initialized!" << std::endl;
            return;
        }

        std::string defaultProjectPath = Utils::FilePathService::GetPath("DefaultProject");
        if (defaultProjectPath.empty()) {
            std::cerr << "[StudioCore] CRITICAL: DefaultProject path is still empty!" << std::endl;
            std::string exeDir = ".";
            if (!exeDir.empty()) {
                std::filesystem::path basePath = std::filesystem::path(exeDir).parent_path();
                defaultProjectPath = (basePath / "projects").string();
                Utils::FilePathService::SetPath("DefaultProject", defaultProjectPath);
                Utils::FilePathService::SaveFilepathDefaults();
                std::cout << "[StudioCore] EMERGENCY RECOVERY: Set DefaultProject to: " << defaultProjectPath << std::endl;
            }
        }

        if (!defaultProjectPath.empty() && !std::filesystem::exists(defaultProjectPath)) {
            std::filesystem::create_directories(defaultProjectPath);
            std::cout << "[StudioCore] Created default project directory: " << defaultProjectPath << std::endl;
        }

        std::cout << "[StudioCore] Setting proper INI file path from FilePathService..." << std::endl;
        ImGuiIO& io = ImGui::GetIO();

        std::string imguiIniPath = Utils::FilePathService::GetPath("ImguiState");
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
            std::cerr << "[StudioCore] WARNING: Could not get ImguiState path from FilePathService!" << std::endl;
        }

        std::cout << "[StudioCore] Loading ImGui style..." << std::endl;
        std::string dataPath = Utils::FilePathService::GetDataPath();
        std::string stylePath = dataPath + "/settings/imgui_style.json";
        if (std::filesystem::exists(stylePath)) {
            LoadStyleFromFile(ImGui::GetStyle(), stylePath);
            std::cout << "[StudioCore] Loaded custom style from: " << stylePath << std::endl;
        }
        else {
            SetCustomDarkTheme();
            std::cout << "[StudioCore] Using custom dark theme as default" << std::endl;
        }

        if (!io.Fonts || io.Fonts->Fonts.Size == 0) {
            std::cerr << "[StudioCore] ERROR: ImGui fonts not loaded!" << std::endl;
            return;
        }

        std::cout << "[StudioCore] ImGui is ready" << std::endl;

        std::cout << "[StudioCore] Loading ImGui IO settings..." << std::endl;
        try {
            std::string settingsPath = dataPath + "/settings/imgui_render_settings.json";
            if (std::filesystem::exists(settingsPath)) {
                Utils::ImGuiSettingsUtil::LoadFromFile(settingsPath, io);
                std::cout << "[StudioCore] ImGui IO settings loaded from file" << std::endl;
            }
            else {
                std::cout << "[StudioCore] No saved ImGui IO settings, using defaults with docking" << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[StudioCore] Warning: Failed to load ImGui IO settings: " << e.what() << std::endl;
        }

        InitializeStudioPlugins();
        std::cout << "[StudioCore] Plugins initialized" << std::endl;

        RegisterCoreViews();
        std::cout << "[StudioCore] Core views registered" << std::endl;

        m_projectManagerView->Init();
        std::cout << "[StudioCore] ProjectManagerView initialized" << std::endl;

        // SettingsView is NOT created here - lazy created when first opened

        m_menuBar = std::make_unique<GUI::MenuBar>(*studioContext->projectManager, *studioContext->viewManager, *this);
        std::cout << "[StudioCore] MenuBar created" << std::endl;

        if (studioContext && studioContext->projectManager) {
            m_showProjectManagerView = studioContext->projectManager->ShouldShowStartup();
            std::cout << "[StudioCore] Should show startup view: " << (m_showProjectManagerView ? "YES" : "NO") << std::endl;
        }
        else {
            m_showProjectManagerView = true;
            std::cout << "[StudioCore] No ProjectManager, showing startup view" << std::endl;
        }

        completedInitialization = true;
        std::cout << "[StudioCore] Complete initialization finished!" << std::endl;
    }

    void StudioCore::OnProjectLoaded(const std::string& projectPath) {
        std::cout << "[StudioCore] Project loaded: " << projectPath << std::endl;

        if (studioContext && studioContext->studioPluginManager) {
            std::cout << "[StudioCore] Setting plugin manager project context: " << projectPath << std::endl;
            studioContext->studioPluginManager->SetProjectContext(projectPath);
        }

        m_showProjectManagerView = false;
        Utils::ImGuiStateUtils::OnProjectLoaded(projectPath);
    }

    void StudioCore::OnProjectCreated(const std::string& projectPath) {
        std::cout << "[StudioCore] Project created: " << projectPath << std::endl;

        if (studioContext && studioContext->studioPluginManager) {
            std::cout << "[StudioCore] Setting plugin manager project context for new project: " << projectPath << std::endl;
            studioContext->studioPluginManager->SetProjectContext(projectPath);
        }

        m_showProjectManagerView = false;
        Utils::ImGuiStateUtils::OnProjectCreated(projectPath);
    }

    void StudioCore::OnProjectClosed() {
        std::cout << "[StudioCore] OnProjectClosed() called" << std::endl;

        if (studioContext && studioContext->studioPluginManager) {
            std::cout << "[StudioCore] Saving project plugin state and reverting to global..." << std::endl;
            studioContext->studioPluginManager->SaveProjectPluginState();
            studioContext->studioPluginManager->UseGlobalPluginState();
        }

        m_showProjectManagerView = true;

        SyncWindowStateFromGLFW();
        std::string defaultPath = GetDefaultWindowStatePath();
        std::filesystem::create_directories(std::filesystem::path(defaultPath).parent_path());
        m_windowState.SaveToFile(defaultPath);
        std::cout << "[StudioCore] Saved current window state as default" << std::endl;

        Utils::ImGuiStateUtils::OnProjectClosed();
    }

    std::string StudioCore::GetDefaultWindowStatePath() const {
        std::string dataPath = Utils::FilePathService::GetDataPath();
        if (dataPath.empty()) {
            std::cerr << "[StudioCore] Data path not available from FilePathService!" << std::endl;
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
            if (studioContext && studioContext->projectManager && studioContext->projectManager->IsProjectOpen()) {
                std::cout << "[StudioCore] Saving open project BEFORE shutdown: "
                    << studioContext->projectManager->GetCurrentProjectName() << std::endl;

                if (studioContext->studioPluginManager) {
                    studioContext->studioPluginManager->SaveProjectPluginState();
                }

                studioContext->projectManager->SaveProject();
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

            std::cout << "[StudioCore] Shutting down FilePathService..." << std::endl;
            Utils::FilePathService::Shutdown();

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
            auto textureSystem = GetEntityManager().GetSystem<ECS::TextureSystem>();
            if (textureSystem) {
                textureSystem->CreatePendingTextures();
            }

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

            bool IsProjectOpen = studioContext->projectManager->IsProjectOpen();

            if (!IsProjectOpen && m_showProjectManagerView && m_projectManagerView) {
                m_projectManagerView->Render();
            }

            if (IsProjectOpen) {
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

            // Render settings popup if it exists and is visible
            if (m_settingsView && m_settingsView->IsVisible()) {
                m_settingsView->Render();
            }
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

        if (textureSystem && imageSystem) {
            imageSystem->RegisterImageAddedCallback([this, textureSystem](EntityID entityID) {
                auto& entityMgr = GetEntityManager();
                if (entityMgr.HasComponent<ImageComponent>(entityID)) {
                    auto& imageComp = entityMgr.GetComponent<ImageComponent>(entityID);

                    std::cout << "[StudioCore] CALLBACK: Image added for entity " << entityID << std::endl;

                    textureSystem->QueueTextureCreation(
                        entityID,
                        imageComp.imageData,
                        imageComp.width,
                        imageComp.height,
                        imageComp.channels
                    );

                    std::cout << "[StudioCore] Queued texture creation for entity " << entityID << std::endl;

                    ANI::Events::Ref().QueueEventWithData("ImageLoaded", entityID);
                    std::cout << "[StudioCore] Fired ImageLoaded event for entity " << entityID << std::endl;
                }
                });

            imageSystem->RegisterImageRemovedCallback([this, textureSystem](EntityID entityID) {
                std::cout << "[StudioCore] CALLBACK: Image removed for entity " << entityID << std::endl;
                textureSystem->RemoveTexture(entityID);

                ANI::Events::Ref().QueueEventWithData("ImageRemoved", entityID);
                std::cout << "[StudioCore] Fired ImageRemoved event for entity " << entityID << std::endl;
                });

            std::cout << "[StudioCore] Core system callbacks set up successfully" << std::endl;
        }
        else {
            std::cerr << "[StudioCore] ERROR: Could not find TextureSystem or ImageSystem" << std::endl;
        }
    }

    void StudioCore::SetCoreEvents() {
        std::cout << "[StudioCore] Registering core system events..." << std::endl;

        auto& entityMgr = GetEntityManager();
        auto imageSystem = entityMgr.GetSystem<ImageSystem>();

        if (!imageSystem) {
            std::cerr << "[StudioCore] ERROR: ImageSystem not found for event registration" << std::endl;
            return;
        }

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

        std::cout << "[StudioCore] Core system events registered successfully" << std::endl;
    }

    void StudioCore::SetActiveWorkspace(GUI::WorkspaceID workspaceID) {
        if (studioContext && studioContext->viewManager) {
            studioContext->viewManager->SetActiveWorkspace(workspaceID);

            if (studioContext->projectManager && studioContext->projectManager->IsProjectOpen()) {
                studioContext->projectManager->SetLastActiveWorkspace(workspaceID);
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