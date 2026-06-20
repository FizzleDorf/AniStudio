#include "Core.hpp"
#include "Events.hpp"
#include "guiSystems.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <filesystem>

namespace ANI {

	void WindowCloseCallback(GLFWwindow* window) {
		Core::Ref().Quit();
	}

	Core::Core() : m_isRunning(true), m_window(nullptr),
		m_videoWidth(SCREEN_WIDTH), m_videoHeight(SCREEN_HEIGHT),
		m_fpsSum(0.0), m_frameCount(0), m_timeElapsed(0.0) {
		std::cout << "[Core] Constructor called" << '\n';
	}

	Core::~Core() {
		std::cout << "[Core] Destructor - calling StudioCore shutdown..." << '\n';
		try {
			m_studioCore.Shutdown();
		}
		catch (const std::exception& e) {
			std::cerr << "[Core] Exception during StudioCore shutdown: " << e.what() << '\n';
		}
		CleanupWindow();
	}

	void Core::Quit() {
		std::cout << "[Core] Quit called - setting run to false" << '\n';
		m_isRunning = false;
		m_studioCore.SetRunning(false);
	}

	void Core::Init() {
		std::cout << "[Core] Initializing..." << '\n';

		// Initialize window and ImGui (FilePaths will be initialized inside EngineContext)
		if (!InitializeWindow()) {
			throw std::runtime_error("Failed to initialize window");
		}
		std::cout << "[Core] Window and ImGui fully initialized" << '\n';

		// Initialize the studio core (basic initialization)
		if (!m_studioCore.Initialize()) {
			throw std::runtime_error("Failed to initialize StudioCore");
		}
		std::cout << "[Core] StudioCore basic initialization complete" << '\n';

		// Setup window context and ImGui context
		m_studioCore.SetWindowHandle(m_window);
		m_studioCore.SetImGuiContext(GetImGuiContext());
		std::cout << "[Core] Window handle and ImGui context set" << '\n';

		// Complete StudioCore initialization (plugins, views, MenuBar, etc.)
		std::cout << "[Core] Completing StudioCore initialization..." << '\n';
		m_studioCore.CompleteInitialization();
		std::cout << "[Core] StudioCore fully initialized" << '\n';

		// Register ALL event handlers
		std::cout << "[Core] Registering event handlers..." << '\n';
		RegisterEventHandlers();
		std::cout << "[Core] Event handlers registered" << '\n';

		std::cout << "[Core] Initialization complete!" << '\n';
	}

	void Core::RegisterEventHandlers() {

		Events::Ref().RegisterEvent("Quit", [this]() {
			std::cout << "[Core] Quit event triggered" << '\n';
			this->Quit();
		});

		Events::Ref().RegisterEventWithData("CreateWorkspace",
			[this](const std::any& data) {
			try {
				auto eventData = std::any_cast<std::unordered_map<std::string, std::string>>(data);
				std::string workspaceName = eventData.at("workspaceName");

				std::cout << "[Core] CreateWorkspace event: " << workspaceName << '\n';
				GUI::WorkspaceID id = m_studioCore.GetViewManager().CreateView();
				m_studioCore.GetViewManager().SetWorkspaceName(id, workspaceName);
				m_studioCore.SetActiveWorkspace(id);
			}
			catch (const std::exception& e) {
				std::cerr << "[Core] Error in CreateWorkspace: " << e.what() << '\n';
			}
		});

		Events::Ref().RegisterEventWithData("DeleteWorkspace",
			[this](const std::any& data) {
			try {
				auto workspaceID = std::any_cast<GUI::WorkspaceID>(data);
				std::cout << "[Core] DeleteWorkspace event: " << workspaceID << '\n';

				auto allWorkspaces = m_studioCore.GetViewManager().GetAllWorkspaces();
				if (allWorkspaces.size() <= 1) {
					std::cout << "[Core] Cannot delete last workspace" << '\n';
					return;
				}

				for (auto id : allWorkspaces) {
					if (id != workspaceID) {
						m_studioCore.SetActiveWorkspace(id);
						break;
					}
				}

				m_studioCore.GetViewManager().DestroyView(workspaceID);
			}
			catch (const std::exception& e) {
				std::cerr << "[Core] Error in DeleteWorkspace: " << e.what() << '\n';
			}
		});

		Events::Ref().RegisterEventWithData("SetActiveWorkspace",
			[this](const std::any& data) {
			try {
				auto workspaceID = std::any_cast<GUI::WorkspaceID>(data);
				std::cout << "[Core] SetActiveWorkspace event: " << workspaceID << '\n';
				m_studioCore.SetActiveWorkspace(workspaceID);
			}
			catch (const std::exception& e) {
				std::cerr << "[Core] Error in SetActiveWorkspace: " << e.what() << '\n';
			}
		});

		Events::Ref().RegisterEventWithData("AddView",
			[this](const std::any& data) {
			try {
				auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
				auto workspaceID = std::any_cast<GUI::WorkspaceID>(eventData.at("workspaceID"));
				auto viewTypeName = std::any_cast<std::string>(eventData.at("viewTypeName"));

				std::cout << "[Core] AddView event: " << viewTypeName
					<< " to workspace: " << workspaceID << '\n';

				GUI::ViewTypeID viewType = m_studioCore.GetViewManager().GetViewType(viewTypeName);
				m_studioCore.GetViewManager().AddViewByType(workspaceID, viewType);
			}
			catch (const std::exception& e) {
				std::cerr << "[Core] Error in AddView: " << e.what() << '\n';
			}
		});

		Events::Ref().RegisterEventWithData("RemoveView",
			[this](const std::any& data) {
			try {
				auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
				auto workspaceID = std::any_cast<GUI::WorkspaceID>(eventData.at("workspaceID"));
				auto viewTypeName = std::any_cast<std::string>(eventData.at("viewTypeName"));

				std::cout << "[Core] RemoveView event: " << viewTypeName
					<< " from workspace: " << workspaceID << '\n';

				GUI::ViewTypeID viewType = m_studioCore.GetViewManager().GetViewType(viewTypeName);
				m_studioCore.GetViewManager().RemoveViewByType(workspaceID, viewType);
			}
			catch (const std::exception& e) {
				std::cerr << "[Core] Error in RemoveView: " << e.what() << '\n';
			}
		});

		Events::Ref().RegisterEvent("CreateEntity",
			[this]() {
			std::cout << "[Core] CreateEntity event" << '\n';
			ECS::EntityID newEntity = m_studioCore.GetEntityManager().AddNewEntity();
			std::cout << "[Core] Created entity: " << newEntity << '\n';
		});

		Events::Ref().RegisterEventWithData("DestroyEntity",
			[this](const std::any& data) {
			try {
				auto entityID = std::any_cast<ECS::EntityID>(data);
				std::cout << "[Core] DestroyEntity event: " << entityID << '\n';
				m_studioCore.GetEntityManager().DestroyEntity(entityID);
			}
			catch (const std::exception& e) {
				std::cerr << "[Core] Error in DestroyEntity: " << e.what() << '\n';
			}
		});

		Events::Ref().RegisterEventWithData("CloneEntity",
			[this](const std::any& data) {
			try {
				auto entityID = std::any_cast<ECS::EntityID>(data);
				std::cout << "[Core] CloneEntity event: " << entityID << '\n';
				ECS::EntityID newEntity = m_studioCore.GetEntityManager().CloneEntity(entityID);
				std::cout << "[Core] Cloned entity " << entityID << " to " << newEntity << '\n';
			}
			catch (const std::exception& e) {
				std::cerr << "[Core] Error in CloneEntity: " << e.what() << '\n';
			}
		});

		Events::Ref().RegisterEventWithData("AddComponent",
			[this](const std::any& data) {
			try {
				auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
				auto entityID = std::any_cast<ECS::EntityID>(eventData.at("entityID"));
				auto componentTypeID = std::any_cast<ECS::ComponentTypeID>(eventData.at("componentTypeID"));

				std::cout << "[Core] AddComponent event: component " << componentTypeID
					<< " to entity " << entityID << '\n';

				auto& mgr = m_studioCore.GetEntityManager();
				if (mgr.IsPluginComponent(componentTypeID)) {
					mgr.AddPluginComponent(entityID, componentTypeID);
				}
			}
			catch (const std::exception& e) {
				std::cerr << "[Core] Error in AddComponent: " << e.what() << '\n';
			}
		});

		Events::Ref().RegisterEventWithData("RemoveComponent",
			[this](const std::any& data) {
			try {
				auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
				auto entityID = std::any_cast<ECS::EntityID>(eventData.at("entityID"));
				auto componentTypeID = std::any_cast<ECS::ComponentTypeID>(eventData.at("componentTypeID"));

				std::cout << "[Core] RemoveComponent event: component " << componentTypeID
					<< " from entity " << entityID << '\n';

				m_studioCore.GetEntityManager().RemoveComponentById(entityID, componentTypeID);
			}
			catch (const std::exception& e) {
				std::cerr << "[Core] Error in RemoveComponent: " << e.what() << '\n';
			}
		});

		Events::Ref().RegisterEvent("SaveProject",
			[this]() {
			std::cout << "[Core] SaveProject event" << '\n';

			if (m_studioCore.GetProjectManager().IsProjectOpen()) {
				if (!m_studioCore.GetProjectManager().SaveProject()) {
					std::cerr << "[Core] Failed to save project: "
						<< m_studioCore.GetProjectManager().GetLastError() << '\n';
				}
				else {
					std::cout << "[Core] Project saved successfully" << '\n';
				}
			}
			else {
				std::cout << "[Core] No project open to save" << '\n';
			}
		});

		Events::Ref().RegisterEvent("CloseProject",
			[this]() {
			std::cout << "[Core] CloseProject event" << '\n';
			m_studioCore.GetProjectManager().CloseProject();
		});

		std::cout << "[Core] All event handlers registered successfully" << '\n';
	}

	bool Core::InitializeWindow() {
		std::cout << "[Core] Initializing GLFW..." << '\n';
		if (!glfwInit()) {
			std::cerr << "[Core] Failed to initialize GLFW" << '\n';
			return false;
		}
		std::cout << "[Core] GLFW initialized successfully" << '\n';

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

		std::cout << "[Core] Creating window..." << '\n';
		m_window = glfwCreateWindow(m_videoWidth, m_videoHeight, "AniStudio", nullptr, nullptr);
		if (!m_window) {
			std::cerr << "[Core] Failed to create GLFW window" << '\n';
			glfwTerminate();
			return false;
		}
		std::cout << "[Core] Window created successfully" << '\n';

		glfwMakeContextCurrent(m_window);
		glfwSetWindowCloseCallback(m_window, WindowCloseCallback);
		glfwSwapInterval(1);
		std::cout << "[Core] Window context set" << '\n';

		std::cout << "[Core] Initializing GLEW..." << '\n';
		GLenum err = glewInit();
		if (err != GLEW_OK) {
			std::cerr << "[Core] Failed to initialize GLEW: " << glewGetErrorString(err) << '\n';
			return false;
		}
		std::cout << "[Core] GLEW initialized successfully" << '\n';

		glViewport(0, 0, m_videoWidth, m_videoHeight);
		std::cout << "[Core] Viewport set" << '\n';

		std::cout << "[Core] Calling IMGUI_CHECKVERSION()..." << '\n';
		IMGUI_CHECKVERSION();
		std::cout << "[Core] Version check passed" << '\n';

		std::cout << "[Core] Calling ImGui::CreateContext()..." << '\n';
		ImGuiContext* ctx = ImGui::CreateContext();
		std::cout << "[Core] ImGui context created: " << ctx << '\n';

		if (!ctx) {
			std::cerr << "[Core] ERROR: ImGui context is NULL!" << '\n';
			return false;
		}

		ImGuiIO& io = ImGui::GetIO();
		std::cout << "[Core] Got ImGuiIO reference" << '\n';

		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		std::cout << "[Core] Enabled docking by default" << '\n';

		// CRITICAL FIX: Don't try to use FilePaths here. Use a temporary INI path
		// The proper path will be set in AniStudio.cpp after FilePaths is initialized
		std::cout << "[Core] Setting temporary INI file path..." << '\n';
		std::string iniFilePath = "imgui.ini";

		// Create directory if it doesn't exist
		try {
			std::filesystem::path iniDir = std::filesystem::path(iniFilePath).parent_path();
			if (!iniDir.empty() && !std::filesystem::exists(iniDir)) {
				std::filesystem::create_directories(iniDir);
				std::cout << "[Core] Created directory for temporary INI file" << '\n';
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[Core] Warning: Could not create INI directory: " << e.what() << '\n';
			// Continue anyway - ImGui will just not save settings
		}

		// Use a static string to ensure the pointer stays valid
		static std::string tempIniPath = iniFilePath;
		io.IniFilename = tempIniPath.c_str();
		std::cout << "[Core] Temporary INI file path set to: " << io.IniFilename << '\n';

		std::cout << "[Core] Initializing GLFW backend..." << '\n';
		bool glfwOk = ImGui_ImplGlfw_InitForOpenGL(m_window, true);
		std::cout << "[Core] GLFW backend result: " << (glfwOk ? "SUCCESS" : "FAILED") << '\n';
		if (!glfwOk) return false;

		std::cout << "[Core] Initializing OpenGL3 backend..." << '\n';
		bool gl3Ok = ImGui_ImplOpenGL3_Init("#version 330");
		std::cout << "[Core] OpenGL3 backend result: " << (gl3Ok ? "SUCCESS" : "FAILED") << '\n';
		if (!gl3Ok) return false;

		std::cout << "[Core] Adding default font..." << '\n';
		if (io.Fonts->Fonts.Size == 0) {
			io.Fonts->AddFontDefault();
			std::cout << "[Core] Default font added" << '\n';
		}
		std::cout << "[Core] Font count: " << io.Fonts->Fonts.Size << '\n';

		std::cout << "[Core] Window initialization COMPLETE" << '\n';
		return true;
	}

	void Core::CleanupWindow() {
		if (m_window) {
			std::cout << "[Core] Cleaning up ImGui and GLFW..." << '\n';
			try {
				ImGui_ImplOpenGL3_Shutdown();
				ImGui_ImplGlfw_Shutdown();
				ImGui::DestroyContext();
				glfwDestroyWindow(m_window);
				m_window = nullptr;
			}
			catch (const std::exception& e) {
				std::cerr << "[Core] Exception during window cleanup: " << e.what() << '\n';
			}
		}
		glfwTerminate();
		std::cout << "[Core] Window cleanup complete" << '\n';
	}

	void Core::Update(const float deltaT) {
		if (!m_isRunning) return;

		// FPS tracking
		m_timeElapsed += deltaT;
		m_frameCount++;
		if (m_timeElapsed >= 1.0) {
			double fps = m_frameCount / m_timeElapsed;
			std::ostringstream titleStream;
			titleStream << "AniStudio - FPS: " << static_cast<int>(fps);
			glfwSetWindowTitle(m_window, titleStream.str().c_str());
			m_frameCount = 0;
			m_timeElapsed = 0.0;
		}

		// Ensure OpenGL context is current before updating studio core
		glfwMakeContextCurrent(m_window);

		// Verify context is valid
		if (!ANI::OpenGLContextHelper::VerifyContext()) {
			std::cerr << "[Core] ERROR: OpenGL context lost before update!" << '\n';
			return;
		}

		// Update studio core - ALL OpenGL operations happen in this call chain
		try {
			m_studioCore.Update(deltaT);
		}
		catch (const std::exception& e) {
			std::cerr << "[Core] Update error: " << e.what() << '\n';
		}
	}

	void Core::Draw() {
		if (!m_isRunning) return;

		try {
			glfwPollEvents();

			// Ensure context is current before any OpenGL operations
			glfwMakeContextCurrent(m_window);

			if (!ANI::OpenGLContextHelper::VerifyContext()) {
				std::cerr << "[Core] ERROR: OpenGL context lost before render!" << '\n';
				return;
			}

#ifdef _WIN32
			// Create textures before ImGui frame to avoid context issues
			{
				auto& entityMgr = m_studioCore.GetEntityManager();
				auto textureSystem = entityMgr.GetSystem<ECS::TextureSystem>();
				if (textureSystem && textureSystem->HasPendingTextures()) {
					std::cout << "[Core] Windows: Creating pending textures before ImGui frame" << '\n';
					textureSystem->CreatePendingTextures();
				}
			}
#endif

			// Clear and render
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			// Start ImGui frame - ONLY ONCE per frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			// Render studio content
			m_studioCore.Render();

			// End ImGui frame and render - ONLY ONCE per frame
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			// Handle multi-viewport if enabled
			ImGuiIO& io = ImGui::GetIO();
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
				GLFWwindow* backup_current_context = glfwGetCurrentContext();
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				glfwMakeContextCurrent(backup_current_context);
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[Core] Render error: " << e.what() << '\n';
		}

		glfwSwapBuffers(m_window);
	}

} // namespace ANI