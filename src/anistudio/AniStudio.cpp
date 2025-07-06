// AniStudio.cpp - INSTANCE-BASED, NO MORE STATIC BULLSHIT
#include "AniStudio.hpp"
#include "AllViews.h"
#include <iostream>

namespace ANI {

	StudioCore::StudioCore()
		: initialized(false), running(false), windowHandle(nullptr), imguiContext(nullptr) {
		std::cout << "[StudioCore] Constructor called" << std::endl;
	}

	StudioCore::~StudioCore() {
		if (initialized) {
			Shutdown();
		}
	}

	void StudioCore::RegisterCoreViews() {
		std::cout << "[StudioCore] Registering core view types..." << std::endl;

		viewManager.RegisterView<GUI::DebugView>("DebugView");
		viewManager.RegisterView<GUI::SettingsView>("SettingsView");
		viewManager.RegisterView<GUI::DiffusionView>("DiffusionView");
		viewManager.RegisterView<GUI::ImageView>("ImageView");
		viewManager.RegisterView<GUI::NodeGraphView>("NodeGraphView");
		viewManager.RegisterView<GUI::ConvertView>("ConvertView");
		viewManager.RegisterView<GUI::ViewListManagerView>("ViewListManagerView");
		viewManager.RegisterView<GUI::SequencerView>("SequencerView");
		viewManager.RegisterView<GUI::PluginView>("PluginView");
		viewManager.RegisterView<GUI::NodeView>("NodeView");
		viewManager.RegisterView<GUI::UpscaleView>("UpscaleView");
		viewManager.RegisterView<GUI::VideoView>("VideoView");

		std::cout << "[StudioCore] Core view types registered successfully!" << std::endl;
	}

	bool StudioCore::Initialize() {
		if (initialized) {
			std::cerr << "[StudioCore] Already initialized!" << std::endl;
			return false;
		}

		try {
			std::cout << "[StudioCore] Initializing..." << std::endl;

			// Initialize Engine FIRST - this registers systems on THE EntityManager
			if (!engineCore.Initialize()) {
				std::cerr << "[StudioCore] Failed to initialize EngineCore!" << std::endl;
				return false;
			}

			// Invalidate ViewList ID 0 for consistency
			auto tempView = viewManager.CreateView();
			viewManager.DestroyView(tempView);

			// Register view types
			RegisterCoreViews();

			initialized = true;
			running = true;

			std::cout << "[StudioCore] Initialized successfully!" << std::endl;

			// DEBUG: Verify systems are there
			ECS::EntityManager& entityMgr = engineCore.GetEntityManager();
			auto imageSystem = entityMgr.GetSystem<ECS::ImageSystem>();
			std::cout << "[StudioCore] DEBUG: EntityManager address: " << &entityMgr << std::endl;
			std::cout << "[StudioCore] DEBUG: ImageSystem found: " << (imageSystem ? "YES" : "NO") << std::endl;

			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[StudioCore] Initialization failed: " << e.what() << std::endl;
			Shutdown();
			return false;
		}
	}

	void StudioCore::Shutdown() {
		if (!initialized) return;

		std::cout << "[StudioCore] Shutting down..." << std::endl;

		running = false;
		engineCore.Shutdown();  // This handles EntityManager shutdown

		windowHandle = nullptr;
		imguiContext = nullptr;
		initialized = false;

		std::cout << "[StudioCore] Shutdown complete" << std::endl;
	}

	void StudioCore::Update(float deltaTime) {
		if (!initialized) return;

		engineCore.Update(deltaTime);      // Update ECS Systems + Plugins
		viewManager.Update(deltaTime);     // Update Views
	}

	void StudioCore::Render() {
		if (!initialized) return;

		try {
			// Setup ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

			GUI::ShowMenuBar(*this);

			viewManager.Render();

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

} // namespace ANI