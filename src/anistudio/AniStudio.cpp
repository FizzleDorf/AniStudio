// AniStudio.cpp - UPDATED to use ViewManager factory system
#include "AniStudio.hpp"
#include "GUI.h"
#include "AllViews.h"
#include "PluginManager.hpp"
#include <iostream>
#include <filesystem>

using namespace GUI;

namespace ANI {
	// Static members initialization
	bool StudioCore::s_initialized = false;
	bool StudioCore::s_running = false;
	void* StudioCore::s_windowHandle = nullptr;
	void* StudioCore::s_imguiContext = nullptr;

	// CRITICAL FIX: Direct instances like the working Engine.cpp
	static ECS::EntityManager g_entityManager;
	static GUI::ViewManager g_viewManager;
	static Plugin::PluginManager g_pluginManager(g_entityManager, g_viewManager);

	ECS::EntityManager& StudioCore::GetEntityManagerImpl() {
		return g_entityManager;
	}

	GUI::ViewManager& StudioCore::GetViewManagerImpl() {
		return g_viewManager;
	}

	Plugin::PluginManager& StudioCore::GetPluginManagerImpl() {
		return g_pluginManager;
	}

	void StudioCore::RegisterCoreViews() {
		std::cout << "[StudioCore] Registering core view factories..." << std::endl;

		// Register standard views with factory functions (standard constructor: EntityManager&)
		g_viewManager.RegisterViewFactory<DebugView>("DebugView");
		g_viewManager.RegisterViewFactory<SettingsView>("SettingsView");
		g_viewManager.RegisterViewFactory<DiffusionView>("DiffusionView");
		g_viewManager.RegisterViewFactory<ImageView>("ImageView");
		g_viewManager.RegisterViewFactory<NodeGraphView>("NodeGraphView");
		g_viewManager.RegisterViewFactory<ConvertView>("ConvertView");
		g_viewManager.RegisterViewFactory<SequencerView>("SequencerView");
		g_viewManager.RegisterViewFactory<NodeView>("NodeView");
		g_viewManager.RegisterViewFactory<UpscaleView>("UpscaleView");
		g_viewManager.RegisterViewFactory<VideoView>("VideoView");
		g_viewManager.RegisterViewFactory<ZepView>("ZepView");
		g_viewManager.RegisterViewFactory<HelpView>("HelpView");

		// Register special views with custom factory functions (different constructors)

		// PluginView requires PluginManager
		g_viewManager.RegisterCustomViewFactory("PluginView",
			[](GUI::ViewListID viewID, ECS::EntityManager& mgr) {
			auto& pluginMgr = StudioCore::GetPluginManager();
			g_viewManager.AddView<PluginView>(viewID, PluginView(mgr, pluginMgr));
			g_viewManager.GetView<PluginView>(viewID).Init();
		}
		);

		// ViewListManagerView requires both EntityManager and ViewManager
		g_viewManager.RegisterCustomViewFactory("ViewListManagerView",
			[](GUI::ViewListID viewID, ECS::EntityManager& mgr) {
			g_viewManager.AddView<ViewListManagerView>(viewID, ViewListManagerView(mgr, g_viewManager));
			g_viewManager.GetView<ViewListManagerView>(viewID).Init();
		}
		);

		std::cout << "[StudioCore] Core view factories registered successfully!" << std::endl;
		std::cout << "[StudioCore] Total registered factories: " << g_viewManager.GetRegisteredFactories().size() << std::endl;
	}

	// CRITICAL FIX: Remove CreateCoreViews entirely - MenuBar will handle all view creation
	void StudioCore::CreateCoreViews() {
		// DO NOTHING - Let MenuBar handle all view creation on demand using factories
		std::cout << "[StudioCore] Skipping core view creation - MenuBar will use factory system" << std::endl;
	}

	bool StudioCore::Initialize() {
		if (s_initialized) {
			std::cerr << "[StudioCore] Already initialized!" << std::endl;
			return false;
		}

		try {
			std::cout << "[StudioCore] Initializing..." << std::endl;

			// Initialize file paths - EXACTLY like working Engine.cpp
			Utils::FilePaths::LoadFilePathDefaults();
			Utils::FilePaths::Init();

			// CRITICAL FIX: Initialize ImGui config in StudioCore
			if (s_imguiContext) {
				std::cout << "[StudioCore] Setting up ImGui configuration..." << std::endl;
				ImGui::SetCurrentContext(static_cast<ImGuiContext*>(s_imguiContext));

				// CRITICAL: Set up ImGui config EXACTLY like working version
				std::string iniFilePath = std::filesystem::absolute(Utils::FilePaths::ImguiStatePath).string();
				std::cout << "[StudioCore] ImGui INI file path: " << iniFilePath << std::endl;

				// CRITICAL: Load existing INI settings if they exist
				if (std::filesystem::exists(iniFilePath)) {
					std::cout << "[StudioCore] Loading existing ImGui settings from: " << iniFilePath << std::endl;
					ImGui::LoadIniSettingsFromDisk(iniFilePath.c_str());
				}
				else {
					std::cout << "[StudioCore] No existing ImGui settings found at: " << iniFilePath << std::endl;
				}

				ImGuiIO &io = ImGui::GetIO();
				io.IniFilename = iniFilePath.c_str();  // Set the INI file path

				// CRITICAL: Load custom style if it exists (like working version)
				std::string stylePath = "../data/defaults/style.json";
				if (std::filesystem::exists(stylePath)) {
					std::cout << "[StudioCore] Loading custom style from: " << stylePath << std::endl;
					ImGuiStyle &style = ImGui::GetStyle();
					LoadStyleFromFile(style, stylePath);
				}
				else {
					std::cout << "[StudioCore] No custom style file found at: " << stylePath << std::endl;
				}

				std::cout << "[StudioCore] ImGui configuration complete" << std::endl;
			}

			// Reset managers (no need to construct, they're already static)
			g_entityManager.Reset();
			g_viewManager.Reset();

			// Invalidate ID 0 for consistency - EXACTLY like working Engine.cpp
			const ECS::EntityID temp = g_entityManager.AddNewEntity();
			g_entityManager.DestroyEntity(temp);
			auto tempView = g_viewManager.CreateView();
			g_viewManager.DestroyView(tempView);

			// Register view FACTORIES (not just types) - EXACTLY like working Engine.cpp
			RegisterCoreViews();

			// Register Component Names - EXACTLY like working Engine.cpp
			g_entityManager.RegisterComponentName<ModelComponent>("Model");
			g_entityManager.RegisterComponentName<ClipLComponent>("ClipL");
			g_entityManager.RegisterComponentName<ClipGComponent>("ClipG");
			g_entityManager.RegisterComponentName<T5XXLComponent>("T5XXL");
			g_entityManager.RegisterComponentName<DiffusionModelComponent>("DiffusionModel");
			g_entityManager.RegisterComponentName<LatentComponent>("Latent");
			g_entityManager.RegisterComponentName<LoraComponent>("Lora");
			g_entityManager.RegisterComponentName<PromptComponent>("Prompt");
			g_entityManager.RegisterComponentName<SamplerComponent>("Sampler");
			g_entityManager.RegisterComponentName<GuidanceComponent>("Guidance");
			g_entityManager.RegisterComponentName<EsrganComponent>("Esrgan");
			g_entityManager.RegisterComponentName<ClipSkipComponent>("ClipSkip");
			g_entityManager.RegisterComponentName<VaeComponent>("Vae");
			g_entityManager.RegisterComponentName<TaesdComponent>("Taesd");
			g_entityManager.RegisterComponentName<ImageComponent>("Image");
			g_entityManager.RegisterComponentName<InputImageComponent>("InputImage");
			g_entityManager.RegisterComponentName<OutputImageComponent>("OutputImage");
			g_entityManager.RegisterComponentName<EmbeddingComponent>("Embedding");
			g_entityManager.RegisterComponentName<ControlnetComponent>("Controlnet");
			g_entityManager.RegisterComponentName<LayerSkipComponent>("LayerSkip");
			g_entityManager.RegisterComponentName<VideoComponent>("Video");
			g_entityManager.RegisterComponentName<InputVideoComponent>("InputVideo");
			g_entityManager.RegisterComponentName<OutputVideoComponent>("OutputVideo");
			g_entityManager.RegisterComponentName<PythonComponent>("Python");
			g_entityManager.RegisterComponentName<ChromaComponent>("Chroma");
			g_entityManager.RegisterComponentName<StackedIdEmbedComponent>("StackedIdEmbed");

			// Register core systems - EXACTLY like working Engine.cpp
			g_entityManager.RegisterSystem<SDCPPSystem>();
			g_entityManager.RegisterSystem<ImageSystem>();
			g_entityManager.RegisterSystem<VideoSystem>();

			// Initialize the plugin manager
			g_pluginManager.Init();

			// CRITICAL FIX: DO NOT CREATE ANY VIEWS HERE
			// CreateCoreViews(); // REMOVED - MenuBar will handle this using factories

			s_initialized = true;
			s_running = true;

			std::cout << "[StudioCore] Initialized successfully with factory system!" << std::endl;
			std::cout << "[StudioCore] Available view factories: " << g_viewManager.GetRegisteredFactories().size() << std::endl;

			// Debug output
			for (const std::string& factoryName : g_viewManager.GetRegisteredFactories()) {
				std::cout << "[StudioCore] - Factory: " << factoryName << std::endl;
			}

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

		// CRITICAL: Save ImGui settings before shutdown (like working version)
		if (s_imguiContext) {
			try {
				ImGui::SetCurrentContext(static_cast<ImGuiContext*>(s_imguiContext));
				std::string iniFilePath = std::filesystem::absolute(Utils::FilePaths::ImguiStatePath).string();
				std::cout << "[StudioCore] Saving ImGui settings to: " << iniFilePath << std::endl;
				ImGui::SaveIniSettingsToDisk(iniFilePath.c_str());
			}
			catch (const std::exception& e) {
				std::cerr << "[StudioCore] Error saving ImGui settings: " << e.what() << std::endl;
			}
		}

		// Reset managers (no need to delete, they're static)
		g_viewManager.Reset();
		g_entityManager.Reset();

		s_windowHandle = nullptr;
		s_imguiContext = nullptr;
		s_initialized = false;

		std::cout << "[StudioCore] Shutdown complete" << std::endl;
	}

	void StudioCore::Update(float deltaTime) {
		if (!s_initialized) return;

		// Update managers - EXACTLY like working Engine.cpp
		g_entityManager.Update(deltaTime);
		g_viewManager.Update(deltaTime);
		g_pluginManager.Update(deltaTime);
	}

	void StudioCore::Render() {
		if (!s_initialized) return;

		try {
			// Setup ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			// Create dockspace - EXACTLY like working Engine.cpp
			ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

			// Show the menu bar and render views - EXACTLY like working Engine.cpp
			// MenuBar will use factory system to create views on demand
			GUI::ShowMenuBar(static_cast<GLFWwindow*>(s_windowHandle), g_viewManager, g_entityManager);

			// Render all views created by MenuBar using factories
			g_viewManager.Render();

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
		if (!s_initialized) {
			throw std::runtime_error("[StudioCore] EntityManager accessed before initialization!");
		}
		return g_entityManager;
	}

	GUI::ViewManager& StudioCore::GetViewManager() {
		if (!s_initialized) {
			throw std::runtime_error("[StudioCore] ViewManager accessed before initialization!");
		}
		return g_viewManager;
	}

	Plugin::PluginManager& StudioCore::GetPluginManager() {
		if (!s_initialized) {
			throw std::runtime_error("[StudioCore] PluginManager accessed before initialization!");
		}
		return g_pluginManager;
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
		return g_pluginManager.LoadPlugin(path);
	}

	void StudioCore::UnloadPlugin(const std::string& name) {
		g_pluginManager.UnloadPlugin(name);
	}

	void StudioCore::LoadDefaultPlugins() {
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
					g_pluginManager.LoadPlugin(pluginPath);
				}
				}
			}
		}

	} // namespace ANI