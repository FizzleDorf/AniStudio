#include "AniStudio.hpp"
#include "AniEngine.hpp"
#include "PluginManager.hpp"
#include <iostream>
#include <filesystem>

using namespace GUI;

namespace ANI {
	// Static members
	std::unique_ptr<ECS::EntityManager> StudioCore::s_entityManager = nullptr;
	std::unique_ptr<GUI::ViewManager> StudioCore::s_viewManager = nullptr;
	std::unique_ptr<Plugin::PluginManager> StudioCore::s_pluginManager = nullptr;
	bool StudioCore::s_initialized = false;
	bool StudioCore::s_running = false;
	void* StudioCore::s_windowHandle = nullptr;
	void* StudioCore::s_imguiContext = nullptr;

	void StudioCore::RegisterCoreViews() {
		auto& viewMgr = *s_viewManager;

		// Register Views - ONLY our responsibility
		viewMgr.RegisterViewType<DebugView>("DebugView");
		viewMgr.RegisterViewType<SettingsView>("SettingsView");
		viewMgr.RegisterViewType<DiffusionView>("DiffusionView");
		viewMgr.RegisterViewType<ImageView>("ImageView");
		viewMgr.RegisterViewType<NodeGraphView>("NodeGraphView");
		viewMgr.RegisterViewType<ConvertView>("ConvertView");
		viewMgr.RegisterViewType<ViewListManagerView>("ViewListManagerView");
		viewMgr.RegisterViewType<SequencerView>("SequencerView");
		viewMgr.RegisterViewType<PluginView>("PluginView");
		viewMgr.RegisterViewType<NodeView>("NodeView");
		viewMgr.RegisterViewType<UpscaleView>("UpscaleView");
		viewMgr.RegisterViewType<VideoView>("VideoView");
		viewMgr.RegisterViewType<VideoView>("VideoSequencerView");
		viewMgr.RegisterViewType<ZepView>("ZepView");
		viewMgr.RegisterViewType<HelpView>("HelpView");

		std::cout << "[StudioCore] Core views registered" << std::endl;
	}

	void StudioCore::CreateCoreViews() {
		try {
			auto& viewMgr = *s_viewManager;
			auto& entityMgr = *s_entityManager;

			// Create essential views only - plugins will create their own
			// Example:
			// auto settingsViewID = viewMgr.CreateView<SettingsView>(entityMgr);

			std::cout << "[StudioCore] Core views created" << std::endl;
		}
		catch (const std::exception& e) {
			std::cerr << "[StudioCore] Failed to create core views: " << e.what() << std::endl;
		}
	}

	bool StudioCore::Initialize() {
		if (s_initialized) {
			std::cerr << "[StudioCore] Already initialized!" << std::endl;
			return false;
		}

		try {
			std::cout << "[StudioCore] Initializing..." << std::endl;

			// Initialize file paths
			Utils::FilePaths::LoadFilePathDefaults();

			// Create managers
			s_entityManager = std::make_unique<ECS::EntityManager>();
			s_viewManager = std::make_unique<GUI::ViewManager>();

			// Invalidate ID 0 for consistency
			const ECS::EntityID temp = s_entityManager->AddNewEntity();
			s_entityManager->DestroyEntity(temp);
			auto tempView = s_viewManager->CreateView();
			s_viewManager->DestroyView(tempView);

			// ===== USE ENGINECORE FOR ALL ECS REGISTRATION =====
			// EngineCore is the master registry for components and systems
			EngineCore::RegisterCoreComponents(*s_entityManager);
			EngineCore::RegisterCoreSystems(*s_entityManager);

			// ===== STUDIOCORE ONLY HANDLES GUI =====
			// Register views (our responsibility)
			RegisterCoreViews();

			// Create plugin manager with BOTH EntityManager and ViewManager
			s_pluginManager = std::make_unique<Plugin::PluginManager>(*s_entityManager, *s_viewManager);

			// Initialize the plugin manager
			s_pluginManager->Init();

			// Create core views
			CreateCoreViews();

			s_initialized = true;
			s_running = true;

			std::cout << "[StudioCore] Initialized successfully with plugin system" << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[StudioCore] Initialization failed: " << e.what() << std::endl;
			Shutdown();
			return false;
		}
	}

	void StudioCore::Shutdown() {
		if (!s_initialized) return;

		std::cout << "[StudioCore] Shutting down..." << std::endl;

		s_running = false;

		// Shutdown plugin manager first
		if (s_pluginManager) {
			s_pluginManager.reset();
		}

		// Reset managers
		if (s_viewManager) {
			s_viewManager->Reset();
			s_viewManager.reset();
		}

		if (s_entityManager) {
			s_entityManager->Reset();
			s_entityManager.reset();
		}

		s_windowHandle = nullptr;
		s_imguiContext = nullptr;
		s_initialized = false;

		std::cout << "[StudioCore] Shutdown complete" << std::endl;
	}

	void StudioCore::Update(float deltaTime) {
		if (!s_initialized) return;

		// Update entity manager (systems)
		if (s_entityManager) {
			s_entityManager->Update(deltaTime);
		}

		// Update view manager
		if (s_viewManager) {
			s_viewManager->Update(deltaTime);
		}

		// Update plugin manager
		if (s_pluginManager) {
			s_pluginManager->Update(deltaTime);
		}
	}

	void StudioCore::Render() {
		if (!s_initialized || !s_viewManager) return;

		try {
			// Setup ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			// Create dockspace
			ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

			// Show the existing menu bar from MenuBar.hpp
			GUI::ShowMenuBar(static_cast<GLFWwindow*>(s_windowHandle));

			// Render all views (including plugin views)
			s_viewManager->Render();

			// Render ImGui
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			// Handle multi-viewport rendering
			if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
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

	// Manager access
	ECS::EntityManager& StudioCore::GetEntityManager() {
		if (!s_initialized || !s_entityManager) {
			throw std::runtime_error("[StudioCore] EntityManager accessed before initialization!");
		}
		return *s_entityManager;
	}

	GUI::ViewManager& StudioCore::GetViewManager() {
		if (!s_initialized || !s_viewManager) {
			throw std::runtime_error("[StudioCore] ViewManager accessed before initialization!");
		}
		return *s_viewManager;
	}

	Plugin::PluginManager& StudioCore::GetPluginManager() {
		if (!s_initialized || !s_pluginManager) {
			throw std::runtime_error("[StudioCore] PluginManager accessed before initialization!");
		}
		return *s_pluginManager;
	}

	// State management
	bool StudioCore::IsRunning() {
		return s_running;
	}

	void StudioCore::SetRunning(bool running) {
		s_running = running;
	}

	// Window management
	void StudioCore::SetWindowHandle(void* window) {
		s_windowHandle = window;
	}

	void StudioCore::SetImGuiContext(void* context) {
		s_imguiContext = context;
	}

	// Plugin management
	bool StudioCore::LoadPlugin(const std::string& path) {
		if (!s_pluginManager) return false;
		return s_pluginManager->LoadPlugin(path);
	}

	void StudioCore::UnloadPlugin(const std::string& name) {
		if (!s_pluginManager) return;
		s_pluginManager->UnloadPlugin(name);
	}

	void StudioCore::LoadDefaultPlugins() {
		if (!s_pluginManager) return;

		std::string pluginsDir = Utils::FilePaths::pluginPath;

		if (!std::filesystem::exists(pluginsDir)) {
			std::cout << "[StudioCore] Creating plugins directory: " << pluginsDir << std::endl;
			std::filesystem::create_directories(pluginsDir);
			return;
		}

		std::cout << "[StudioCore] Loading plugins from: " << pluginsDir << std::endl;

		for (const auto& entry : std::filesystem::directory_iterator(pluginsDir)) {
			if (entry.is_regular_file()) {
				std::string extension = entry.path().extension().string();

#ifdef _WIN32
				if (extension == ".dll") {
#else
				if (extension == ".so") {
#endif
					std::string pluginPath = entry.path().string();
					std::cout << "[StudioCore] Loading plugin: " << pluginPath << std::endl;
					s_pluginManager->LoadPlugin(pluginPath);
				}
				}
			}
		}

	} // namespace ANI