#include "AniStudio.hpp"
#include "AllViews.h"
#include "FilePaths.hpp"
#include "ImGuiSettingsUtil.hpp"
#include "ImGuiStateUtils.hpp"
#include "Events.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <imgui.h>
#include "GuiStyleHelpers.hpp"

namespace ANI {

	StudioCore::StudioCore()
		: initialized(false), running(false), windowHandle(nullptr), imguiContext(nullptr),
		m_projectManager(viewManager, engineCore.GetEntityManager()), m_isShuttingDown(false) {
		std::cout << "[StudioCore] Constructor called" << std::endl;

		m_projectManagerView = std::make_unique<GUI::ProjectManagerView>(m_projectManager);
		engineCore.GetEntityManager().RegisterSystem<TextureSystem>();
	}

	StudioCore::~StudioCore() {
		if (initialized) {
			Shutdown();
		}
	}

	void StudioCore::SetImGuiContext(void* context) {
		imguiContext = context;
		std::cout << "[StudioCore] ImGui context set to: " << imguiContext << std::endl;
	}

	void StudioCore::RegisterCoreViews() {
		std::cout << "[StudioCore] Registering core view types..." << std::endl;

		viewManager.RegisterView<GUI::DebugView>("DebugView");
		viewManager.RegisterView<GUI::SettingsView>("SettingsView");
		viewManager.RegisterView<GUI::ImageView>("ImageView");
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

		std::cout << "[StudioCore] Setting global data path: " << Utils::FilePaths::dataPath << std::endl;
		studioPluginManager->SetGlobalDataPath(Utils::FilePaths::dataPath);

		studioPluginManager->scanPluginDirectory(pluginDirectory);
		studioPluginManager->enableHotReload(false);

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

			SetCoreCallbacks();
			SetCoreEvents();

			initialized = true;
			running = true;

			std::cout << "[StudioCore] Core initialized successfully!" << std::endl;
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

		std::cout << "[StudioCore] Completing full initialization..." << std::endl;

		if (!imguiContext) {
			std::cerr << "[StudioCore] ERROR: ImGui context is null! Cannot complete initialization." << std::endl;
			return;
		}

		ImGui::SetCurrentContext(static_cast<ImGuiContext*>(imguiContext));
		ImGuiContext* currentContext = ImGui::GetCurrentContext();
		std::cout << "[StudioCore] Using main ImGui context: " << currentContext << std::endl;

		std::cout << "[StudioCore] Loading ImGui style..." << std::endl;
		std::string stylePath = Utils::FilePaths::dataPath + "/settings/imgui_style.json";
		if (std::filesystem::exists(stylePath)) {
			LoadStyleFromFile(ImGui::GetStyle(), stylePath);
			std::cout << "[StudioCore] Loaded custom style from: " << stylePath << std::endl;
		}
		else {
			SetCustomDarkTheme();
			std::cout << "[StudioCore] Using custom dark theme as default" << std::endl;
		}

		ImGuiIO& io = ImGui::GetIO();
		std::cout << "[StudioCore] ImGui fonts pointer: " << io.Fonts << std::endl;
		if (io.Fonts) {
			std::cout << "[StudioCore] ImGui fonts count: " << io.Fonts->Fonts.Size << std::endl;
		}

		if (!io.Fonts || io.Fonts->Fonts.Size == 0) {
			std::cerr << "[StudioCore] ERROR: ImGui fonts not loaded!" << std::endl;
			return;
		}

		std::cout << "[StudioCore] ImGui is ready" << std::endl;

		std::cout << "[StudioCore] Loading ImGui IO settings..." << std::endl;
		try {
			std::string settingsPath = Utils::FilePaths::dataPath + "/settings/imgui_render_settings.json";
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

		m_menuBar = std::make_unique<GUI::MenuBar>(m_projectManager, viewManager);
		std::cout << "[StudioCore] MenuBar created" << std::endl;

		m_showProjectManagerView = true;
		std::cout << "[StudioCore] Will show startup view on launch" << std::endl;

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

		if (studioPluginManager) {
			std::cout << "[StudioCore] Saving project plugin state and reverting to global..." << std::endl;
			studioPluginManager->SaveProjectPluginState();
			studioPluginManager->UseGlobalPluginState();
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

				if (studioPluginManager) {
					studioPluginManager->SaveProjectPluginState();
				}

				m_projectManager.SaveProject();
			}
			else {
				SyncWindowStateFromGLFW();
				std::string defaultPath = GetDefaultWindowStatePath();
				std::filesystem::create_directories(std::filesystem::path(defaultPath).parent_path());
				m_windowState.SaveToFile(defaultPath);
				std::cout << "[StudioCore] Saved default window state during shutdown" << std::endl;
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
		ANI::Events::Ref().Poll();
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
			CompleteInitialization();

			bool IsProjectOpen = m_projectManager.IsProjectOpen();

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

					viewManager.Render();
					ImGui::End();
				}
				else {
					ImGui::PopStyleVar(3);
					ImGui::End();
				}
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[StudioCore] Render error: " << e.what() << std::endl;
		}
	}

	void StudioCore::SetCoreCallbacks() {
		std::cout << "[StudioCore] Setting up core system callbacks..." << std::endl;

		auto& entityMgr = engineCore.GetEntityManager();
		auto textureSystem = entityMgr.GetSystem<TextureSystem>();
		auto imageSystem = entityMgr.GetSystem<ImageSystem>();

		if (textureSystem && imageSystem) {
			// System callback: When ImageSystem finishes loading, queue texture creation
			imageSystem->RegisterImageAddedCallback([this, textureSystem](EntityID entityID) {
				auto& entityMgr = engineCore.GetEntityManager();
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

					// FIRE EVENT FOR IMAGEVIEW
					ANI::Events::Ref().QueueEventWithData("ImageLoaded", entityID);
					std::cout << "[StudioCore] Fired ImageLoaded event for entity " << entityID << std::endl;
				}
			});

			// System callback: When an image is removed, cleanup its texture
			imageSystem->RegisterImageRemovedCallback([this, textureSystem](EntityID entityID) {
				std::cout << "[StudioCore] CALLBACK: Image removed for entity " << entityID << std::endl;
				textureSystem->RemoveTexture(entityID);

				// FIRE EVENT FOR IMAGEVIEW
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

		auto& entityMgr = engineCore.GetEntityManager();
		auto imageSystem = entityMgr.GetSystem<ImageSystem>();

		if (!imageSystem) {
			std::cerr << "[StudioCore] ERROR: ImageSystem not found for event registration" << std::endl;
			return;
		}

		// Event handler: LoadImageRequest from ImageView
		// Capture 'this' so we can access engineCore member
		Events::Ref().RegisterEventWithData("LoadImageRequest", [this, imageSystem](const std::any& data) {
			try {
				auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
				std::string filePath = std::any_cast<std::string>(eventData.at("filePath"));

				std::cout << "[StudioCore] LoadImageRequest: " << filePath << std::endl;

				auto& entityMgr = engineCore.GetEntityManager();
				ECS::EntityID entity = entityMgr.AddNewEntity();
				entityMgr.AddComponent<ImageComponent>(entity);
				imageSystem->SetImage(entity, filePath);

				std::cout << "[StudioCore] Created entity " << entity << " for image" << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "[StudioCore] LoadImageRequest error: " << e.what() << std::endl;
			}
		});

		// Event handler: RemoveImageRequest from ImageView
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
		viewManager.SetActiveWorkspace(workspaceID);
		if (m_projectManager.IsProjectOpen()) {
			m_projectManager.SetLastActiveWorkspace(workspaceID);
		}
	}

	GUI::WorkspaceID StudioCore::GetActiveWorkspace() const {
		return viewManager.GetActiveWorkspace();
	}

} // namespace ANI