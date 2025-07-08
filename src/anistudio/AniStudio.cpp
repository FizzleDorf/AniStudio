// AniStudio.cpp - Fixed constructor issues
#include "AniStudio.hpp"
#include "AllViews.h"
#include <iostream>

namespace ANI {

	StudioCore::StudioCore()
		: initialized(false), running(false), windowHandle(nullptr), imguiContext(nullptr),
		m_projectManager(viewManager, engineCore.GetEntityManager()), m_menuBarID(0) {
		std::cout << "[StudioCore] Constructor called" << std::endl;
	}

	StudioCore::~StudioCore() {
		if (initialized) {
			Shutdown();
		}
	}

	void StudioCore::RegisterCoreViews() {
		std::cout << "[StudioCore] Registering core view types..." << std::endl;

		// Register standard views (only need EntityManager)
		viewManager.RegisterView<GUI::DebugView>("DebugView");
		viewManager.RegisterView<GUI::SettingsView>("SettingsView");
		viewManager.RegisterView<GUI::DiffusionView>("DiffusionView");
		viewManager.RegisterView<GUI::ImageView>("ImageView");
		viewManager.RegisterView<GUI::NodeGraphView>("NodeGraphView");
		viewManager.RegisterView<GUI::ConvertView>("ConvertView");
		viewManager.RegisterView<GUI::SequencerView>("SequencerView");
		viewManager.RegisterView<GUI::NodeView>("NodeView");
		viewManager.RegisterView<GUI::UpscaleView>("UpscaleView");
		viewManager.RegisterView<GUI::VideoView>("VideoView");
		viewManager.RegisterView<GUI::HelpView>("HelpView");
		viewManager.RegisterView<GUI::ZepView>("ZepView");


		// Register views that need special constructors with custom factories
		viewManager.RegisterViewWithFactory("ViewListManagerView", "Views",
			[this](ECS::EntityManager& mgr) -> std::unique_ptr<GUI::BaseView> {
			return std::make_unique<GUI::ViewListManagerView>(mgr, viewManager);
		},
			[]() -> GUI::ViewMetadata { return GUI::ViewListManagerView::GetMetadata(); }
		);

		viewManager.RegisterViewWithFactory("PluginView", "Plugins",
			[this](ECS::EntityManager& mgr) -> std::unique_ptr<GUI::BaseView> {
			return std::make_unique<GUI::PluginView>(mgr, engineCore.GetPluginManager());
		},
			[]() -> GUI::ViewMetadata { return GUI::PluginView::GetMetadata(); }
		);

		viewManager.RegisterViewWithFactory("ProjectManagerView", "Core",
			[this](ECS::EntityManager& mgr) -> std::unique_ptr<GUI::BaseView> {
			return std::make_unique<GUI::ProjectManagerView>(mgr, m_projectManager);
		},
			[]() -> GUI::ViewMetadata { return GUI::ProjectManagerView::GetMetadata(); }
		);

		viewManager.RegisterViewWithFactory("NewProjectView", "Core",
			[this](ECS::EntityManager& mgr) -> std::unique_ptr<GUI::BaseView> {
			return std::make_unique<GUI::NewProjectView>(mgr, m_projectManager);
		},
			[]() -> GUI::ViewMetadata { return GUI::NewProjectView::GetMetadata(); }
		);

		viewManager.RegisterViewWithFactory("LoadProjectView", "Core",
			[this](ECS::EntityManager& mgr) -> std::unique_ptr<GUI::BaseView> {
			return std::make_unique<GUI::LoadProjectView>(mgr, m_projectManager);
		},
			[]() -> GUI::ViewMetadata { return GUI::LoadProjectView::GetMetadata(); }
		);

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

			// Register view types with factories
			RegisterCoreViews();

			// Create MenuBar manually since it needs a special constructor
			m_menuBarID = viewManager.CreateView();
			viewManager.AddView<GUI::MenuBar>(m_menuBarID, GUI::MenuBar(m_projectManager, viewManager, engineCore.GetEntityManager()));

			// Show startup view if no project should be loaded
			if (m_projectManager.ShouldShowStartup()) {
				m_projectManager.GetViewState().SetViewOpen("ProjectManagerView", true);
				std::cout << "[StudioCore] Showing startup view - no project to auto-load" << std::endl;
			}

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