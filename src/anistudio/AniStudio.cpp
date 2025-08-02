/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license information, please contact legal@kframe.ai.
 */

#include "AniStudio.hpp"
#include "StudioPluginManager.hpp"
#include "AllViews.h"
#include "FilePaths.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>

namespace ANI {

	StudioCore::StudioCore()
		: initialized(false), running(false), windowHandle(nullptr), imguiContext(nullptr),
		m_projectManager(viewManager, engineCore.GetEntityManager()) {
		std::cout << "[StudioCore] Constructor called" << std::endl;

		// Create studio plugin manager with BOTH ECS and GUI support
		studioPluginManager = std::make_unique<Plugin::StudioPluginManager>(
			engineCore.GetEntityManager(),
			viewManager
			);

		// Create standalone project view (only ProjectManagerView - contains the popups now)
		m_projectManagerView = std::make_unique<GUI::ProjectManagerView>(m_projectManager);
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
		viewManager.RegisterViewWithFactory("WorkspaceView", "Views",
			[this](ECS::EntityManager& mgr) -> std::unique_ptr<GUI::BaseView> {
			return std::make_unique<GUI::WorkspaceView>(mgr, viewManager);
		},
			[]() -> GUI::ViewMetadata { return GUI::WorkspaceView::GetMetadata(); }
		);

		// Register PluginView with StudioPluginManager
		viewManager.RegisterViewWithFactory("PluginView", "Development",
			[this](ECS::EntityManager& mgr) -> std::unique_ptr<GUI::BaseView> {
			return std::make_unique<GUI::PluginView>(mgr, *studioPluginManager);
		},
			[]() -> GUI::ViewMetadata { return GUI::PluginView::GetMetadata(); }
		);

		std::cout << "[StudioCore] Core view types registered successfully!" << std::endl;
	}

	void StudioCore::SetupProjectCallbacks() {
		// Set up project manager callbacks for workspace management
		m_projectManager.SetProjectLoadedCallback([this](const std::string& projectPath) {
			OnProjectLoaded(projectPath);
		});

		m_projectManager.SetProjectCreatedCallback([this](const std::string& projectPath) {
			OnProjectCreated(projectPath);
		});

		m_projectManager.SetProjectClosedCallback([this]() {
			OnProjectClosed();
		});
	}

	void StudioCore::RenderActiveWorkspaceViews() {
		if (!m_projectManager.IsProjectOpen()) return;

		auto& viewState = m_projectManager.GetViewState();
		const GUI::WorkspaceState* activeWorkspace = viewState.GetActiveWorkspace();

		if (!activeWorkspace) return;

		// Get the views that should be open in this workspace
		auto openViewTypes = activeWorkspace->GetOpenViews();

		// For each open view type, create and render the view if it doesn't exist
		for (const std::string& viewTypeName : openViewTypes) {
			try {
				// Check if this view type is registered
				auto registeredViews = viewManager.GetRegisteredViews();
				if (registeredViews.find(viewTypeName) == registeredViews.end()) {
					std::cerr << "[StudioCore] View type not registered: " << viewTypeName << std::endl;
					continue;
				}

				// Create a unique WorkspaceID for this view in this workspace
				// Use a hash of workspace ID + view type name for consistency
				GUI::WorkspaceID viewInstanceID = std::hash<std::string>{}(std::to_string(activeWorkspace->workspaceID) + "_" + viewTypeName);

				// Check if view instance already exists in ViewManager
				if (!m_activeViewInstances.count(viewInstanceID)) {
					// Create the view instance using ViewManager
					GUI::WorkspaceID createdID = viewManager.CreateViewByName(viewTypeName, engineCore.GetEntityManager());
					if (createdID != 0) {
						m_activeViewInstances[viewInstanceID] = createdID;
						std::cout << "[StudioCore] Created view instance: " << viewTypeName << " with ID: " << createdID << std::endl;
					}
					else {
						std::cerr << "[StudioCore] Failed to create view: " << viewTypeName << std::endl;
						continue;
					}
				}

			}
			catch (const std::exception& e) {
				std::cerr << "[StudioCore] Error rendering view " << viewTypeName << ": " << e.what() << std::endl;
			}
		}

		// Clean up views that are no longer open
		auto it = m_activeViewInstances.begin();
		while (it != m_activeViewInstances.end()) {
			GUI::WorkspaceID viewInstanceID = it->first;
			GUI::WorkspaceID actualViewID = it->second;

			// Check if this view should still be open
			bool shouldBeOpen = false;
			for (const std::string& viewTypeName : openViewTypes) {
				GUI::WorkspaceID expectedID = std::hash<std::string>{}(std::to_string(activeWorkspace->workspaceID) + "_" + viewTypeName);
				if (viewInstanceID == expectedID) {
					shouldBeOpen = true;
					break;
				}
			}

			if (!shouldBeOpen) {
				// Remove the view
				viewManager.DestroyView(actualViewID);
				it = m_activeViewInstances.erase(it);
			}
			else {
				++it;
			}
		}

		// Now render all the active view instances through ViewManager
		viewManager.RenderGenericWorkspaces();
	}

	void StudioCore::InitializeWindowState() {
		// Set the global data path for WindowState utility
		m_windowState.SetGlobalDataPath(Utils::FilePaths::dataPath);

		// Load default window state from build/data/window_state.json
		std::string defaultPath = GetDefaultWindowStatePath();
		if (std::filesystem::exists(defaultPath)) {
			std::cout << "[StudioCore] Loading default window state from: " << defaultPath << std::endl;
			m_windowState.LoadFromFile(defaultPath);

			// Apply the loaded state to the existing GLFW window
			ApplyWindowStateToGLFW();
		}
		else {
			std::cout << "[StudioCore] No default window state found, using current configuration" << std::endl;
			// Sync WindowState with current GLFW window state
			SyncWindowStateFromGLFW();
		}

		std::cout << "[StudioCore] Window state initialized" << std::endl;
	}

	void StudioCore::SetWindowHandle(void* window) {
		windowHandle = window;

		if (window) {
			std::cout << "[StudioCore] Window handle set for WindowState utility" << std::endl;

			// Pass the window handle to ProjectManager so it can apply window states
			m_projectManager.SetWindowHandle(window);

			// Now that we have the window handle, we can sync or apply states
			if (initialized) {
				InitializeWindowState();
			}
		}
	}

	void StudioCore::SyncWindowStateFromGLFW() {
		if (!windowHandle) return;

		GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(windowHandle);

		// Update WindowState to match current GLFW window
		int width, height, x, y;
		glfwGetWindowSize(glfwWindow, &width, &height);
		glfwGetWindowPos(glfwWindow, &x, &y);

		// Load current state into a temporary JSON to update WindowState
		nlohmann::json currentState;
		currentState["width"] = width;
		currentState["height"] = height;
		currentState["posX"] = x;
		currentState["posY"] = y;
		currentState["maximized"] = (glfwGetWindowAttrib(glfwWindow, GLFW_MAXIMIZED) == GLFW_TRUE);
		currentState["fullscreen"] = (glfwGetWindowMonitor(glfwWindow) != nullptr);
		currentState["vsync"] = true; // Default to true
		currentState["title"] = "AniStudio";

		m_windowState.Deserialize(currentState);
		std::cout << "[StudioCore] Synced WindowState with current GLFW window" << std::endl;
	}

	void StudioCore::ApplyWindowStateToGLFW() {
		if (!windowHandle) return;

		GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(windowHandle);

		// Apply WindowState to GLFW window
		glfwSetWindowSize(glfwWindow, m_windowState.GetWidth(), m_windowState.GetHeight());
		glfwSetWindowPos(glfwWindow, m_windowState.GetPosX(), m_windowState.GetPosY());

		if (m_windowState.IsMaximized()) {
			glfwMaximizeWindow(glfwWindow);
		}
		else {
			glfwRestoreWindow(glfwWindow);
		}

		// Apply VSync
		glfwSwapInterval(1); // For now, always use VSync

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

			// Setup project callbacks
			SetupProjectCallbacks();

			// Initialize studio plugin manager (NO AUTO-LOADING)
			std::cout << "[StudioCore] Initializing studio plugin manager..." << std::endl;
			std::string pluginsDir = Utils::FilePaths::pluginPath;
			if (!pluginsDir.empty()) {
				studioPluginManager->StartHotReload(pluginsDir);
				std::cout << "[StudioCore] Started studio plugin hot reload for: " << pluginsDir << std::endl;
			}

			// Initialize standalone view
			m_projectManagerView->Init();

			// Create MenuBar (standalone, not managed by ViewManager) - pass ProjectManagerView reference
			m_menuBar = std::make_unique<GUI::MenuBar>(m_projectManager, viewManager);

			// Show startup view if no project should be loaded
			if (m_projectManager.ShouldShowStartup()) {
				m_projectManager.GetViewState().SetViewOpen("ProjectManagerView", true);
				std::cout << "[StudioCore] Showing startup view - no project to auto-load" << std::endl;
			}

			initialized = true;
			running = true;

			std::cout << "[StudioCore] Initialized successfully with studio plugin system!" << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[StudioCore] Initialization failed: " << e.what() << std::endl;
			Shutdown();
			return false;
		}
	}

	void StudioCore::OnProjectLoaded(const std::string& projectPath) {
		std::cout << "[StudioCore] Project loaded: " << projectPath << std::endl;

		// The ProjectManager/ViewState handles workspace loading from project data

		// Use utility function for ImGui state management
		Utils::ImGuiStateUtils::OnProjectLoaded(projectPath);
	}

	void StudioCore::OnProjectCreated(const std::string& projectPath) {
		std::cout << "[StudioCore] Project created: " << projectPath << std::endl;

		// The NewProjectView triggers workspace creation based on selected template

		// Use utility function for ImGui state management
		Utils::ImGuiStateUtils::OnProjectCreated(projectPath);
	}

	void StudioCore::OnProjectClosed() {
		std::cout << "[StudioCore] Project closing..." << std::endl;

		// Clear all active view instances when project closes
		for (const auto&[viewInstanceID, actualViewID] : m_activeViewInstances) {
			viewManager.DestroyView(actualViewID);
		}
		m_activeViewInstances.clear();

		// ViewState/ProjectManager handles workspace cleanup

		// Sync current GLFW window state to WindowState, then save as default
		SyncWindowStateFromGLFW();
		std::string defaultPath = GetDefaultWindowStatePath();
		std::filesystem::create_directories(std::filesystem::path(defaultPath).parent_path());
		m_windowState.SaveToFile(defaultPath);
		std::cout << "[StudioCore] Saved current window state as default" << std::endl;

		// Use utility function for ImGui state management
		Utils::ImGuiStateUtils::OnProjectClosed();
	}

	std::string StudioCore::GetDefaultWindowStatePath() const {
		return Utils::FilePaths::dataPath + "/window_state.json";
	}

	void StudioCore::Shutdown() {
		if (!initialized) return;

		std::cout << "[StudioCore] Starting shutdown sequence..." << std::endl;

		// Set flag to prevent any further operations
		running = false;

		try {
			// CRITICAL: Save everything BEFORE starting shutdown
			std::cout << "[StudioCore] Saving application state before shutdown..." << std::endl;

			// Save current window state as default
			SyncWindowStateFromGLFW();
			std::string defaultPath = GetDefaultWindowStatePath();
			std::filesystem::create_directories(std::filesystem::path(defaultPath).parent_path());
			m_windowState.SaveToFile(defaultPath);
			std::cout << "[StudioCore] Saved window state as default" << std::endl;

			// Save current project if one is open
			if (m_projectManager.IsProjectOpen()) {
				std::cout << "[StudioCore] Saving open project: " << m_projectManager.GetCurrentProjectName() << std::endl;
				try {
					if (!m_projectManager.SaveProject()) {
						std::cerr << "[StudioCore] Warning: Failed to save project: " << m_projectManager.GetLastError() << std::endl;
					}
					else {
						std::cout << "[StudioCore] Project saved successfully" << std::endl;
					}

					// Save project-specific ImGui layout
					Utils::ImGuiStateUtils::SaveProjectImGuiLayout(m_projectManager.GetCurrentProjectPath());
				}
				catch (const std::exception& e) {
					std::cerr << "[StudioCore] Exception saving project: " << e.what() << std::endl;
				}
			}

			// ImGui will automatically save its layout when it shuts down
			std::cout << "[StudioCore] ImGui will auto-save layout on shutdown" << std::endl;

			// Save application paths and settings ONCE here
			std::cout << "[StudioCore] Saving file paths and settings..." << std::endl;
			try {
				Utils::FilePaths::SaveFilepathDefaults();
				std::cout << "[StudioCore] File paths saved" << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "[StudioCore] Warning: Failed to save file paths: " << e.what() << std::endl;
			}

			// Give a moment for file operations to complete
			std::this_thread::sleep_for(std::chrono::milliseconds(50));

			std::cout << "[StudioCore] Critical saves completed" << std::endl;

			// Clear all active view instances
			for (const auto&[viewInstanceID, actualViewID] : m_activeViewInstances) {
				viewManager.DestroyView(actualViewID);
			}
			m_activeViewInstances.clear();

			// Now shutdown components in reverse order
			std::cout << "[StudioCore] Shutting down plugin managers..." << std::endl;
			if (studioPluginManager) {
				studioPluginManager->StopHotReload();
				studioPluginManager->UnloadAllPlugins();
				studioPluginManager.reset();
				std::cout << "[StudioCore] Studio plugin manager shutdown complete" << std::endl;
			}

			// Clean up standalone views
			m_menuBar.reset();
			m_projectManagerView.reset();

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
			// Update core engine
			engineCore.Update(deltaTime);

			// Update studio plugin manager
			if (studioPluginManager) {
				studioPluginManager->Update(deltaTime);
			}

			// Update standalone views
			if (m_menuBar) m_menuBar->Update(deltaTime);

			// Update project view only if it should be visible
			auto& viewState = m_projectManager.GetViewState();
			if (viewState.IsViewOpen("ProjectManagerView")) {
				m_projectManagerView->Update(deltaTime);
			}

			// Update ViewManager's generic workspaces (this updates all view instances)
			viewManager.UpdateGenericWorkspaces(deltaTime);

		}
		catch (const std::exception& e) {
			std::cerr << "[StudioCore] Update error: " << e.what() << std::endl;
		}
	}

	void StudioCore::Render() {
		if (!running || !initialized) return;

		try {
			// Start new ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			auto& viewState = m_projectManager.GetViewState();

			// FIXED: Only set ProjectManagerView open ONCE, not every frame
			static bool startupShown = false;
			if (!m_projectManager.IsProjectOpen() && !startupShown) {
				if (!viewState.IsViewOpen("ProjectManagerView")) {
					viewState.SetViewOpen("ProjectManagerView", true);
					startupShown = true;
					std::cout << "[StudioCore] Showing startup view - no project to auto-load" << std::endl;
				}
			}

			// Reset flag when project is open
			if (m_projectManager.IsProjectOpen()) {
				startupShown = false;
			}

			// Render project management view (contains the popups now)
			if (viewState.IsViewOpen("ProjectManagerView")) {
				m_projectManagerView->Render();
			}

			// Only render main interface if project is open
			if (m_projectManager.IsProjectOpen()) {
				// Create main window that fills the viewport with menubar
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

					// Render MenuBar inside the main window
					if (m_menuBar) {
						m_menuBar->Render();
					}

					// Create dock space for views
					ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
					ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

					// Render the views that should be open in the active workspace
					RenderActiveWorkspaceViews();
				}
				else {
					ImGui::PopStyleVar(3);
				}
				ImGui::End();
			}

			// Render ImGui
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			// Handle multi-viewport
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

	// Plugin management functions
	bool StudioCore::LoadPlugin(const std::string& path) {
		if (studioPluginManager) {
			return studioPluginManager->LoadPlugin(path);
		}
		return false;
	}

	bool StudioCore::UnloadPlugin(const std::string& pluginName) {
		if (studioPluginManager) {
			return studioPluginManager->UnloadPlugin(pluginName);
		}
		return false;
	}

	std::vector<std::string> StudioCore::GetLoadedPlugins() const {
		if (studioPluginManager) {
			return studioPluginManager->GetLoadedPluginNames();
		}
		return {};
	}

} // namespace ANI