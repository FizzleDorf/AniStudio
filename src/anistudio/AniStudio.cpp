#include "AniStudio.hpp"
#include "AllViews.h"
#include "FilePaths.hpp"
#include "guiSystems.h"
#include "DiffusionCallbackUtils.hpp"
#include "ImGuiSettingsUtil.hpp"
#include "Events.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <imgui.h>

namespace ANI {

	StudioCore::StudioCore()
		: initialized(false), running(false), windowHandle(nullptr), imguiContext(nullptr),
		m_projectManager(viewManager, engineCore.GetEntityManager()), m_isShuttingDown(false) {
		std::cout << "[StudioCore] Constructor called" << std::endl;

		m_projectManagerView = std::make_unique<GUI::ProjectManagerView>(m_projectManager);
		GUI::DiffusionCallbackUtils::InitializeCallbacks();
	}

	StudioCore::~StudioCore() {
		if (initialized) {
			Shutdown();
		}
	}

	void StudioCore::RegisterCoreViews() {
		std::cout << "[StudioCore] Registering core view types..." << std::endl;
		
		engineCore.GetEntityManager().RegisterSystem<TextureSystem>();

		viewManager.RegisterView<GUI::DebugView>("DebugView");
		viewManager.RegisterView<GUI::SettingsView>("SettingsView");
		viewManager.RegisterView<GUI::DiffusionView>("DiffusionView");
		viewManager.RegisterView<GUI::VideoDiffusionView>("VideoDiffusionView");
		viewManager.RegisterView<GUI::ImageView>("ImageView");
		viewManager.RegisterView<GUI::ConvertView>("ConvertView");
		// viewManager.RegisterView<GUI::SequencerView>("SequencerView");
		// viewManager.RegisterView<GUI::NodeView>("NodeView");
		viewManager.RegisterView<GUI::UpscaleView>("UpscaleView");
		viewManager.RegisterView<GUI::VideoView>("VideoView");
		viewManager.RegisterView<GUI::HelpView>("HelpView");
		viewManager.RegisterView<GUI::ZepView>("ZepView");

		if (studioPluginManager) {
			viewManager.RegisterViewWithFactory("PluginView", "Tools",
				[this](ECS::EntityManager& mgr) -> std::unique_ptr<GUI::BaseView> {
				return std::make_unique<GUI::PluginView>(mgr, *studioPluginManager);
			},
				[]() -> GUI::ViewMetadata {
				return GUI::BaseView::GetMetadataFor<GUI::PluginView>();
			}
			);
		}

		viewManager.RegisterViewWithFactory("WorkspaceView", "Views",
			[this](ECS::EntityManager& mgr) -> std::unique_ptr<GUI::BaseView> {
			return std::make_unique<GUI::WorkspaceView>(mgr, viewManager);
		},
			[]() -> GUI::ViewMetadata { return GUI::WorkspaceView::GetMetadata(); }
		);

		std::cout << "[StudioCore] Core view types registered successfully!" << std::endl;
	}

	void StudioCore::InitializeStudioPlugins() {
		std::cout << "[StudioCore] Initializing studio plugin system..." << std::endl;

		imguiContext = ImGui::GetCurrentContext();
		std::cout << "[StudioCore] Captured ImGui context for plugins: " << imguiContext << std::endl;

		if (!imguiContext) {
			std::cerr << "[StudioCore] ERROR: ImGui context is null!" << std::endl;
			return;
		}

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

		studioPluginManager = std::make_unique<Plugins::StudioPluginManager>(
			engineCore.GetEntityManager(),
			viewManager,
			static_cast<ImGuiContext*>(imguiContext)
			);

		std::string pluginDirectory = "../plugins";

		if (!std::filesystem::exists(pluginDirectory)) {
			std::filesystem::create_directories(pluginDirectory);
			std::cout << "[StudioCore] Created plugin directory: " << pluginDirectory << std::endl;
		}

		// CRITICAL: Set up state management FIRST
		std::cout << "[StudioCore] Setting global data path: " << Utils::FilePaths::dataPath << std::endl;
		studioPluginManager->SetGlobalDataPath(Utils::FilePaths::dataPath);

		// Scan plugins directory (discover what's available)
		studioPluginManager->scanPluginDirectory(pluginDirectory);

		// Hot reload is disabled by default - will be enabled when PluginView opens
		studioPluginManager->enableHotReload(false);

		// CRITICAL: Load global plugin state AFTER setting the data path
		std::cout << "[StudioCore] Loading global plugin state..." << std::endl;
		studioPluginManager->LoadGlobalPluginState();

		std::cout << "[StudioCore] Studio plugin system initialized with selective loading" << std::endl;
	}

	void StudioCore::SetupProjectCallbacks() {
		m_projectManager.SetProjectLoadedCallback([this](const std::string& projectPath) {
			OnProjectLoaded(projectPath);
		});

		m_projectManager.SetProjectCreatedCallback([this](const std::string& projectPath) {
			OnProjectCreated(projectPath);
		});

		m_projectManager.SetProjectClosedCallback([this]() {
			if (!m_isShuttingDown) {
				OnProjectClosed();
			}
		});

		m_projectManager.SetViewStateLoadedCallback([this](GUI::WorkspaceID activeWorkspaceID) {
			std::cout << "[StudioCore] Syncing ViewManager with loaded active workspace: " << activeWorkspaceID << std::endl;
			viewManager.SetActiveWorkspace(activeWorkspaceID);
		});
	}

	void StudioCore::InitializeWindowState() {
		m_windowState.SetGlobalDataPath(Utils::FilePaths::dataPath);

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
			m_projectManager.SetWindowHandle(window);

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
			std::cout << "[StudioCore] Initializing..." << std::endl;

			if (!engineCore.Initialize()) {
				std::cerr << "[StudioCore] Failed to initialize EngineCore!" << std::endl;
				return false;
			}

			viewManager.SetEntityManager(engineCore.GetEntityManager());

			SetupProjectCallbacks();

			std::cout << "[StudioCore] Basic initialization complete, waiting for full ImGui setup..." << std::endl;

			initialized = true;
			running = true;

			std::cout << "[StudioCore] Core initialized successfully - plugins will be loaded after first render!" << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[StudioCore] Initialization failed: " << e.what() << std::endl;
			Shutdown();
			return false;
		}
	}

	void StudioCore::CompleteInitialization() {
		static bool completedInitialization = false;
		if (completedInitialization) return;

		std::cout << "[StudioCore] Completing initialization after ImGui is ready..." << std::endl;

		ImGuiContext* currentContext = ImGui::GetCurrentContext();
		if (!currentContext) {
			std::cerr << "[StudioCore] ERROR: ImGui context still null!" << std::endl;
			return;
		}

		ImGuiIO& io = ImGui::GetIO();
		if (!io.Fonts || io.Fonts->Fonts.Size == 0) {
			std::cerr << "[StudioCore] ERROR: ImGui fonts still not loaded!" << std::endl;
			return;
		}

		std::cout << "[StudioCore] ImGui is now fully ready - proceeding with plugin initialization" << std::endl;

		try {
			Utils::ImGuiSettingsUtil::LoadImGuiSettingsForApp(Utils::FilePaths::dataPath);
			std::cout << "[StudioCore] ImGui settings loaded successfully" << std::endl;
		}
		catch (const std::exception& e) {
			std::cerr << "[StudioCore] Warning: Failed to load ImGui settings: " << e.what() << std::endl;
		}

		viewManager.SetImGuiContext(currentContext);
		std::cout << "[StudioCore] Set ImGui context on ViewManager: " << currentContext << std::endl;

		InitializeStudioPlugins();

		RegisterCoreViews();

		std::cout << "[StudioCore] Setting managers for Events system..." << std::endl;
		ANI::Events::Ref().SetManagers(&viewManager, &m_projectManager);
		std::cout << "[StudioCore] Events managers set successfully!" << std::endl;

		m_projectManagerView->Init();
		m_menuBar = std::make_unique<GUI::MenuBar>(m_projectManager, viewManager);

		if (m_projectManager.ShouldShowStartup()) {
			m_showProjectManagerView = true;
			std::cout << "[StudioCore] Will show startup view - no project to auto-load" << std::endl;
		}

		completedInitialization = true;
		std::cout << "[StudioCore] Complete initialization finished!" << std::endl;
	}

	void StudioCore::OnProjectLoaded(const std::string& projectPath) {
		std::cout << "[StudioCore] Project loaded: " << projectPath << std::endl;

		if (studioPluginManager) {
			std::cout << "[StudioCore] Setting plugin manager project context: " << projectPath << std::endl;
			studioPluginManager->SetProjectContext(projectPath);
		}

		m_showProjectManagerView = false;
		Utils::ImGuiStateUtils::OnProjectLoaded(projectPath);
	}

	void StudioCore::OnProjectCreated(const std::string& projectPath) {
		std::cout << "[StudioCore] Project created: " << projectPath << std::endl;

		if (studioPluginManager) {
			std::cout << "[StudioCore] Setting plugin manager project context for new project: " << projectPath << std::endl;
			studioPluginManager->SetProjectContext(projectPath);
		}

		m_showProjectManagerView = false;
		Utils::ImGuiStateUtils::OnProjectCreated(projectPath);
	}

	void StudioCore::OnProjectClosed() {
		std::cout << "[StudioCore] OnProjectClosed() called" << std::endl;
		std::cout << "[StudioCore] m_isShuttingDown: " << m_isShuttingDown << std::endl;
		std::cout << "[StudioCore] Current m_showProjectManagerView: " << m_showProjectManagerView << std::endl;

		if (studioPluginManager) {
			std::cout << "[StudioCore] Saving project plugin state and reverting to global..." << std::endl;
			studioPluginManager->SaveProjectPluginState();
			studioPluginManager->UseGlobalPluginState();
		}

		// Show project manager view again
		m_showProjectManagerView = true;

		std::cout << "[StudioCore] Set m_showProjectManagerView to: " << m_showProjectManagerView << std::endl;

		SyncWindowStateFromGLFW();
		std::string defaultPath = GetDefaultWindowStatePath();
		std::filesystem::create_directories(std::filesystem::path(defaultPath).parent_path());
		m_windowState.SaveToFile(defaultPath);
		std::cout << "[StudioCore] Saved current window state as default" << std::endl;

		Utils::ImGuiStateUtils::OnProjectClosed();
	}

	std::string StudioCore::GetDefaultWindowStatePath() const {
		return Utils::FilePaths::dataPath + "/window_state.json";
	}

	void StudioCore::Shutdown() {
		if (!initialized) return;

		std::cout << "[StudioCore] Starting shutdown sequence..." << std::endl;
		running = false;
		m_isShuttingDown = true;

		try {
			if (m_projectManager.IsProjectOpen()) {
				std::cout << "[StudioCore] Saving open project BEFORE shutdown: " << m_projectManager.GetCurrentProjectName() << std::endl;

				// Save project-specific plugin state
				if (studioPluginManager) {
					studioPluginManager->SaveProjectPluginState();
				}

				GUI::WorkspaceID currentActive = viewManager.GetActiveWorkspace();
				m_projectManager.SetLastActiveWorkspace(currentActive);
				std::cout << "[StudioCore] Synced active workspace " << currentActive << " to project before saving" << std::endl;

				try {
					if (!m_projectManager.SaveProject()) {
						std::cerr << "[StudioCore] ERROR: Failed to save project: " << m_projectManager.GetLastError() << std::endl;
					}
					else {
						std::cout << "[StudioCore] Project saved successfully with all workspaces" << std::endl;
					}
				}
				catch (const std::exception& e) {
					std::cerr << "[StudioCore] Exception saving project: " << e.what() << std::endl;
				}
			}
			else {
				// No project open, save global plugin state
				if (studioPluginManager) {
					studioPluginManager->SaveGlobalPluginState();
				}
			}

			std::cout << "[StudioCore] Saving application state..." << std::endl;

			SyncWindowStateFromGLFW();
			std::string defaultPath = GetDefaultWindowStatePath();
			std::filesystem::create_directories(std::filesystem::path(defaultPath).parent_path());
			m_windowState.SaveToFile(defaultPath);
			std::cout << "[StudioCore] Saved window state as default" << std::endl;

			try {
				ImGuiIO& io = ImGui::GetIO();
				std::string settingsPath = Utils::FilePaths::dataPath + "/settings/imgui_render_settings.json";
				Utils::ImGuiSettingsUtil::SaveToFile(settingsPath, io);
				std::cout << "[StudioCore] ImGui settings saved successfully" << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "[StudioCore] Warning: Failed to save ImGui settings: " << e.what() << std::endl;
			}

			std::cout << "[StudioCore] ImGui will auto-save layout on shutdown" << std::endl;

			std::cout << "[StudioCore] Saving file paths and settings..." << std::endl;
			try {
				Utils::FilePaths::SaveFilepathDefaults();
				std::cout << "[StudioCore] File paths saved" << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "[StudioCore] Warning: Failed to save file paths: " << e.what() << std::endl;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(100));

			std::cout << "[StudioCore] Critical saves completed" << std::endl;

			m_menuBar.reset();
			m_projectManagerView.reset();

			if (studioPluginManager) {
				std::cout << "[StudioCore] Shutting down studio plugin manager..." << std::endl;
				studioPluginManager.reset();
			}

			std::cout << "[StudioCore] Shutting down view manager..." << std::endl;
			viewManager.FullReset();

			std::cout << "[StudioCore] Shutting down engine core..." << std::endl;
			engineCore.Shutdown();

			std::cout << "[StudioCore] All components shut down successfully" << std::endl;

		}
		catch (const std::exception& e) {
			std::cerr << "[StudioCore] Exception during shutdown: " << e.what() << std::endl;
		}

		initialized = false;
		std::cout << "[StudioCore] Shutdown sequence completed" << std::endl;
	}

	void StudioCore::Update(float deltaTime) {
		if (!running || !initialized) return;

		try {
			engineCore.Update(deltaTime);

			if (studioPluginManager) {
				studioPluginManager->updatePlugins(deltaTime);
			}

			if (m_menuBar) m_menuBar->Update(deltaTime);

			if (m_showProjectManagerView) {
				m_projectManagerView->Update(deltaTime);
			}

			viewManager.Update(deltaTime);

		}
		catch (const std::exception& e) {
			std::cerr << "[StudioCore] Update error: " << e.what() << std::endl;
		}
	}

	void StudioCore::Render() {
		if (!running || !initialized) return;

		try {
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			CompleteInitialization();

			// Track project state changes to avoid interfering with project close
			static bool wasProjectOpen = false;
			static bool startupShown = false;
			bool isProjectCurrentlyOpen = m_projectManager.IsProjectOpen();

			// Show startup view if no project is open and we haven't shown it yet
			if (!isProjectCurrentlyOpen && !startupShown) {
				if (!m_showProjectManagerView) {
					m_showProjectManagerView = true;
					startupShown = true;
					std::cout << "[StudioCore] Showing startup view - no project to auto-load" << std::endl;
				}
			}

			// Only hide project manager view when a project actually opens (not every frame)
			if (isProjectCurrentlyOpen && !wasProjectOpen) {
				// Project just opened
				startupShown = false;
				m_showProjectManagerView = false;
				std::cout << "[StudioCore] Project opened - hiding startup view" << std::endl;
			}

			// Reset startup flag when project closes (so it can be shown again)
			if (!isProjectCurrentlyOpen && wasProjectOpen) {
				// Project just closed - OnProjectClosed() should have set m_showProjectManagerView = true
				startupShown = false;
				std::cout << "[StudioCore] Project closed - startup view should be visible: " << m_showProjectManagerView << std::endl;
			}

			wasProjectOpen = isProjectCurrentlyOpen;

			if (m_showProjectManagerView) {
				m_projectManagerView->Render();
			}

			if (m_projectManager.IsProjectOpen()) {
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

					ImGuiID docksspace_id = ImGui::GetID("MainDockSpace");
					ImGui::DockSpace(docksspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

					viewManager.Render();
				}
				else {
					ImGui::PopStyleVar(3);
				}
				ImGui::End();
			}

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			ImGuiIO& io = ImGui::GetIO();
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
				GLFWwindow* backup_current_context = glfwGetCurrentContext();
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				glfwMakeContextCurrent(backup_current_context);
			}

		}
		catch (const std::exception& e) {
			std::cerr << "[StudioCore] Render error: " << e.what() << std::endl;
		}
	}

	void StudioCore::SetActiveWorkspace(GUI::WorkspaceID workspaceID) {
		viewManager.SetActiveWorkspace(workspaceID);
		if (m_projectManager.IsProjectOpen()) {
			m_projectManager.SetLastActiveWorkspace(workspaceID);
		}
	}

	GUI::WorkspaceID StudioCore::GetActiveWorkspace() const {
		return viewManager.GetActiveWorkspace();
	}

} // namespace ANI