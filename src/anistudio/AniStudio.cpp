#include "AniStudio.hpp"
#include "GUI.h"
#include "AllViews.h"
#include <iostream>
#include <filesystem>

using namespace GUI;

namespace ANI {

	// Static members for StudioCore state
	bool StudioCore::s_initialized = false;
	bool StudioCore::s_running = false;
	void* StudioCore::s_windowHandle = nullptr;
	void* StudioCore::s_imguiContext = nullptr;

	static GUI::ViewManager g_viewManager;

	GUI::ViewManager& StudioCore::GetViewManagerImpl() {
		return g_viewManager;
	}

	void StudioCore::RegisterCoreViews() {
		std::cout << "[StudioCore] Registering core view types..." << std::endl;

		g_viewManager.RegisterView<DebugView>("DebugView");
		g_viewManager.RegisterView<SettingsView>("SettingsView");
		g_viewManager.RegisterView<DiffusionView>("DiffusionView");
		g_viewManager.RegisterView<ImageView>("ImageView");
		g_viewManager.RegisterView<NodeGraphView>("NodeGraphView");
		g_viewManager.RegisterView<ConvertView>("ConvertView");
		g_viewManager.RegisterView<ViewListManagerView>("ViewListManagerView");
		g_viewManager.RegisterView<SequencerView>("SequencerView");
		g_viewManager.RegisterView<PluginView>("PluginView");
		g_viewManager.RegisterView<NodeView>("NodeView");
		g_viewManager.RegisterView<UpscaleView>("UpscaleView");
		g_viewManager.RegisterView<VideoView>("VideoView");
		g_viewManager.RegisterView<VideoSequencerView>("VideoSequencerView");

		std::cout << "[StudioCore] Core view types registered successfully!" << std::endl;
	}

	bool StudioCore::Initialize() {
		if (s_initialized) {
			std::cerr << "[StudioCore] Already initialized!" << std::endl;
			return false;
		}

		try {
			std::cout << "[StudioCore] Initializing..." << std::endl;

			// Initialize Engine FIRST - this sets up ECS, ThreadPools, etc.
			if (!EngineCore::Initialize()) {
				std::cerr << "[StudioCore] Failed to initialize EngineCore!" << std::endl;
				return false;
			}

			// Reset our ViewManager (not Engine's managers!)
			g_viewManager.Reset();

			// Invalidate ViewList ID 0 for consistency
			auto tempView = g_viewManager.CreateView();
			g_viewManager.DestroyView(tempView);

			// Register view types
			RegisterCoreViews();

			s_initialized = true;
			s_running = true;

			std::cout << "[StudioCore] Initialized successfully!" << std::endl;
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

		// Reset ONLY our ViewManager
		g_viewManager.Reset();

		// Shutdown Engine (this handles EntityManager, ThreadPools, PluginManager)
		EngineCore::Shutdown();

		s_windowHandle = nullptr;
		s_imguiContext = nullptr;
		s_initialized = false;

		std::cout << "[StudioCore] Shutdown complete" << std::endl;
	}

	void StudioCore::Update(float deltaTime) {
		if (!s_initialized) return;

		// Update Engine first (ECS Systems + Plugins + ThreadPools)
		EngineCore::Update(deltaTime);

		// Then update our ViewManager
		g_viewManager.Update(deltaTime);
	}

	void StudioCore::Render() {
		if (!s_initialized) return;

		try {
			// Setup ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

			// Render Views using Engine's EntityManager
			GUI::ShowMenuBar(static_cast<GLFWwindow*>(s_windowHandle), g_viewManager, EngineCore::GetEntityManager());
			g_viewManager.Render();

			// Render ImGui
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[StudioCore] Render error: " << e.what() << std::endl;
		}
	}

	ECS::EntityManager& StudioCore::GetEntityManager() {
		// ALWAYS use Engine's EntityManager
		return EngineCore::GetEntityManager();
	}

	GUI::ViewManager& StudioCore::GetViewManager() {
		if (!s_initialized) {
			throw std::runtime_error("[StudioCore] ViewManager accessed before initialization!");
		}
		return g_viewManager;
	}

	Plugin::PluginManager& StudioCore::GetPluginManager() {
		// ALWAYS use Engine's PluginManager
		return EngineCore::GetPluginManager();
	}

	bool StudioCore::IsRunning() {
		return s_running && EngineCore::IsRunning();
	}

	void StudioCore::SetRunning(bool running) {
		s_running = running;
		EngineCore::SetRunning(running);
	}

	void StudioCore::SetWindowHandle(void* window) {
		s_windowHandle = window;
	}

	void StudioCore::SetImGuiContext(void* context) {
		s_imguiContext = context;
	}

	bool StudioCore::LoadPlugin(const std::string& path) {
		return EngineCore::LoadPlugin(path);
	}

	void StudioCore::UnloadPlugin(const std::string& name) {
		std::cerr << "[StudioCore] UnloadPlugin not implemented in EngineCore yet" << std::endl;
		// TODO: Add unload support to EngineCore if needed
	}

	void StudioCore::LoadDefaultPlugins() {
		EngineCore::LoadDefaultPlugins();
	}

	void LoadStyleFromFile(ImGuiStyle& style, const std::string& path) {
		// Simple stub - just use default dark style for now
		ImGui::StyleColorsDark();
		std::cout << "[LoadStyleFromFile] Using default dark style (custom loading not implemented)" << std::endl;
	}

} // namespace ANI