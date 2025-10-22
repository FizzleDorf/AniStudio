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

	Core::Core() : run(true), window(nullptr),
		videoWidth(SCREEN_WIDTH), videoHeight(SCREEN_HEIGHT),
		fpsSum(0.0), frameCount(0), timeElapsed(0.0) {
		std::cout << "[Core] Constructor called" << std::endl;
	}

	Core::~Core() {
		std::cout << "[Core] Destructor - calling StudioCore shutdown..." << std::endl;
		try {
			studioCore.Shutdown();
		}
		catch (const std::exception& e) {
			std::cerr << "[Core] Exception during StudioCore shutdown: " << e.what() << std::endl;
		}
		CleanupWindow();
	}

	void Core::Quit() {
		std::cout << "[Core] Quit called - setting run to false" << std::endl;
		run = false;
		studioCore.SetRunning(false);
	}

	void Core::Init() {
		std::cout << "[Core] Initializing FilePaths..." << std::endl;
		Utils::FilePaths::Init();
		std::cout << "[Core] FilePaths initialized" << std::endl;

		// Initialize window and ImGui
		if (!InitializeWindow()) {
			throw std::runtime_error("Failed to initialize window");
		}
		std::cout << "[Core] Window and ImGui fully initialized" << std::endl;

		// Initialize the studio core (basic initialization)
		if (!studioCore.Initialize()) {
			throw std::runtime_error("Failed to initialize StudioCore");
		}
		std::cout << "[Core] StudioCore basic initialization complete" << std::endl;

		// Setup window context and ImGui context
		studioCore.SetWindowHandle(window);
		studioCore.SetImGuiContext(GetImGuiContext());
		std::cout << "[Core] Window handle and ImGui context set" << std::endl;

		// Complete StudioCore initialization (plugins, views, MenuBar, etc.)
		std::cout << "[Core] Completing StudioCore initialization..." << std::endl;
		studioCore.CompleteInitialization();
		std::cout << "[Core] StudioCore fully initialized" << std::endl;

		// Register ALL event handlers
		std::cout << "[Core] Registering event handlers..." << std::endl;
		RegisterEventHandlers();
		std::cout << "[Core] Event handlers registered" << std::endl;

		std::cout << "[Core] Initialization complete!" << std::endl;
	}

	void Core::RegisterEventHandlers() {

		Events::Ref().RegisterEvent("Quit", [this]() {
			std::cout << "[Core] Quit event triggered" << std::endl;
			this->Quit();
		});

		Events::Ref().RegisterEventWithData("CreateWorkspace",
			[this](const std::any& data) {
			try {
				auto eventData = std::any_cast<std::unordered_map<std::string, std::string>>(data);
				std::string workspaceName = eventData.at("workspaceName");

				std::cout << "[Core] CreateWorkspace event: " << workspaceName << std::endl;
				GUI::WorkspaceID id = studioCore.GetViewManager().CreateView();
				studioCore.GetViewManager().SetWorkspaceName(id, workspaceName);
				studioCore.SetActiveWorkspace(id);
			}
			catch (const std::exception& e) {
				std::cerr << "[Core] Error in CreateWorkspace: " << e.what() << std::endl;
			}
		});

		Events::Ref().RegisterEventWithData("DeleteWorkspace",
			[this](const std::any& data) {
			try {
				auto workspaceID = std::any_cast<GUI::WorkspaceID>(data);
				std::cout << "[Core] DeleteWorkspace event: " << workspaceID << std::endl;

				auto allWorkspaces = studioCore.GetViewManager().GetAllWorkspaces();
				if (allWorkspaces.size() <= 1) {
					std::cout << "[Core] Cannot delete last workspace" << std::endl;
					return;
				}

				for (auto id : allWorkspaces) {
					if (id != workspaceID) {
						studioCore.SetActiveWorkspace(id);
						break;
					}
				}

				studioCore.GetViewManager().DestroyView(workspaceID);
			}
			catch (const std::exception& e) {
				std::cerr << "[Core] Error in DeleteWorkspace: " << e.what() << std::endl;
			}
		});

		Events::Ref().RegisterEventWithData("SetActiveWorkspace",
			[this](const std::any& data) {
			try {
				auto workspaceID = std::any_cast<GUI::WorkspaceID>(data);
				std::cout << "[Core] SetActiveWorkspace event: " << workspaceID << std::endl;
				studioCore.SetActiveWorkspace(workspaceID);
			}
			catch (const std::exception& e) {
				std::cerr << "[Core] Error in SetActiveWorkspace: " << e.what() << std::endl;
			}
		});

		Events::Ref().RegisterEventWithData("AddView",
			[this](const std::any& data) {
			try {
				auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
				auto workspaceID = std::any_cast<GUI::WorkspaceID>(eventData.at("workspaceID"));
				auto viewTypeName = std::any_cast<std::string>(eventData.at("viewTypeName"));

				std::cout << "[Core] AddView event: " << viewTypeName
					<< " to workspace: " << workspaceID << std::endl;

				GUI::ViewTypeID viewType = studioCore.GetViewManager().GetViewType(viewTypeName);
				studioCore.GetViewManager().AddViewByType(workspaceID, viewType);
			}
			catch (const std::exception& e) {
				std::cerr << "[Core] Error in AddView: " << e.what() << std::endl;
			}
		});

		Events::Ref().RegisterEventWithData("RemoveView",
			[this](const std::any& data) {
			try {
				auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
				auto workspaceID = std::any_cast<GUI::WorkspaceID>(eventData.at("workspaceID"));
				auto viewTypeName = std::any_cast<std::string>(eventData.at("viewTypeName"));

				std::cout << "[Core] RemoveView event: " << viewTypeName
					<< " from workspace: " << workspaceID << std::endl;

				GUI::ViewTypeID viewType = studioCore.GetViewManager().GetViewType(viewTypeName);
				studioCore.GetViewManager().RemoveViewByType(workspaceID, viewType);
			}
			catch (const std::exception& e) {
				std::cerr << "[Core] Error in RemoveView: " << e.what() << std::endl;
			}
		});

		Events::Ref().RegisterEvent("CreateEntity",
			[this]() {
			std::cout << "[Core] CreateEntity event" << std::endl;
			ECS::EntityID newEntity = studioCore.GetEntityManager().AddNewEntity();
			std::cout << "[Core] Created entity: " << newEntity << std::endl;
		});

		Events::Ref().RegisterEventWithData("DestroyEntity",
			[this](const std::any& data) {
			try {
				auto entityID = std::any_cast<ECS::EntityID>(data);
				std::cout << "[Core] DestroyEntity event: " << entityID << std::endl;
				studioCore.GetEntityManager().DestroyEntity(entityID);
			}
			catch (const std::exception& e) {
				std::cerr << "[Core] Error in DestroyEntity: " << e.what() << std::endl;
			}
		});

		Events::Ref().RegisterEventWithData("CloneEntity",
			[this](const std::any& data) {
			try {
				auto entityID = std::any_cast<ECS::EntityID>(data);
				std::cout << "[Core] CloneEntity event: " << entityID << std::endl;
				ECS::EntityID newEntity = studioCore.GetEntityManager().CloneEntity(entityID);
				std::cout << "[Core] Cloned entity " << entityID << " to " << newEntity << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "[Core] Error in CloneEntity: " << e.what() << std::endl;
			}
		});

		Events::Ref().RegisterEventWithData("AddComponent",
			[this](const std::any& data) {
			try {
				auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
				auto entityID = std::any_cast<ECS::EntityID>(eventData.at("entityID"));
				auto componentTypeID = std::any_cast<ECS::ComponentTypeID>(eventData.at("componentTypeID"));

				std::cout << "[Core] AddComponent event: component " << componentTypeID
					<< " to entity " << entityID << std::endl;

				auto& mgr = studioCore.GetEntityManager();
				if (mgr.IsPluginComponent(componentTypeID)) {
					mgr.AddPluginComponent(entityID, componentTypeID);
				}
			}
			catch (const std::exception& e) {
				std::cerr << "[Core] Error in AddComponent: " << e.what() << std::endl;
			}
		});

		Events::Ref().RegisterEventWithData("RemoveComponent",
			[this](const std::any& data) {
			try {
				auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
				auto entityID = std::any_cast<ECS::EntityID>(eventData.at("entityID"));
				auto componentTypeID = std::any_cast<ECS::ComponentTypeID>(eventData.at("componentTypeID"));

				std::cout << "[Core] RemoveComponent event: component " << componentTypeID
					<< " from entity " << entityID << std::endl;

				studioCore.GetEntityManager().RemoveComponentById(entityID, componentTypeID);
			}
			catch (const std::exception& e) {
				std::cerr << "[Core] Error in RemoveComponent: " << e.what() << std::endl;
			}
		});

		Events::Ref().RegisterEvent("SaveProject",
			[this]() {
			std::cout << "[Core] SaveProject event" << std::endl;

			if (studioCore.GetProjectManager().IsProjectOpen()) {
				if (!studioCore.GetProjectManager().SaveProject()) {
					std::cerr << "[Core] Failed to save project: "
						<< studioCore.GetProjectManager().GetLastError() << std::endl;
				}
				else {
					std::cout << "[Core] Project saved successfully" << std::endl;
				}
			}
			else {
				std::cout << "[Core] No project open to save" << std::endl;
			}
		});

		Events::Ref().RegisterEvent("CloseProject",
			[this]() {
			std::cout << "[Core] CloseProject event" << std::endl;
			studioCore.GetProjectManager().CloseProject();
		});

		std::cout << "[Core] All event handlers registered successfully" << std::endl;
	}

	bool Core::InitializeWindow() {
		std::cout << "[Core] Initializing GLFW..." << std::endl;
		if (!glfwInit()) {
			std::cerr << "[Core] Failed to initialize GLFW" << std::endl;
			return false;
		}
		std::cout << "[Core] GLFW initialized successfully" << std::endl;

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

		std::cout << "[Core] Creating window..." << std::endl;
		window = glfwCreateWindow(videoWidth, videoHeight, "AniStudio", nullptr, nullptr);
		if (!window) {
			std::cerr << "[Core] Failed to create GLFW window" << std::endl;
			glfwTerminate();
			return false;
		}
		std::cout << "[Core] Window created successfully" << std::endl;

		glfwMakeContextCurrent(window);
		glfwSetWindowCloseCallback(window, WindowCloseCallback);
		glfwSwapInterval(1);
		std::cout << "[Core] Window context set" << std::endl;

		std::cout << "[Core] Initializing GLEW..." << std::endl;
		GLenum err = glewInit();
		if (err != GLEW_OK) {
			std::cerr << "[Core] Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl;
			return false;
		}
		std::cout << "[Core] GLEW initialized successfully" << std::endl;

		glViewport(0, 0, videoWidth, videoHeight);
		std::cout << "[Core] Viewport set" << std::endl;

		std::cout << "[Core] Calling IMGUI_CHECKVERSION()..." << std::endl;
		IMGUI_CHECKVERSION();
		std::cout << "[Core] Version check passed" << std::endl;

		std::cout << "[Core] Calling ImGui::CreateContext()..." << std::endl;
		ImGuiContext* ctx = ImGui::CreateContext();
		std::cout << "[Core] ImGui context created: " << ctx << std::endl;

		if (!ctx) {
			std::cerr << "[Core] ERROR: ImGui context is NULL!" << std::endl;
			return false;
		}

		ImGuiIO& io = ImGui::GetIO();
		std::cout << "[Core] Got ImGuiIO reference" << std::endl;

		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		std::cout << "[Core] Enabled docking by default" << std::endl;

		std::cout << "[Core] Setting INI file path..." << std::endl;
		std::string iniFilePath = std::filesystem::absolute(Utils::FilePaths::ImguiStatePath).string();
		std::filesystem::path iniDir = std::filesystem::path(iniFilePath).parent_path();
		if (!std::filesystem::exists(iniDir)) {
			std::filesystem::create_directories(iniDir);
		}

		static std::string persistentIniPath = iniFilePath;
		io.IniFilename = persistentIniPath.c_str();
		std::cout << "[Core] INI file path set to: " << io.IniFilename << std::endl;

		std::cout << "[Core] Initializing GLFW backend..." << std::endl;
		bool glfwOk = ImGui_ImplGlfw_InitForOpenGL(window, true);
		std::cout << "[Core] GLFW backend result: " << (glfwOk ? "SUCCESS" : "FAILED") << std::endl;
		if (!glfwOk) return false;

		std::cout << "[Core] Initializing OpenGL3 backend..." << std::endl;
		bool gl3Ok = ImGui_ImplOpenGL3_Init("#version 330");
		std::cout << "[Core] OpenGL3 backend result: " << (gl3Ok ? "SUCCESS" : "FAILED") << std::endl;
		if (!gl3Ok) return false;

		std::cout << "[Core] Adding default font..." << std::endl;
		if (io.Fonts->Fonts.Size == 0) {
			io.Fonts->AddFontDefault();
			std::cout << "[Core] Default font added" << std::endl;
		}
		std::cout << "[Core] Font count: " << io.Fonts->Fonts.Size << std::endl;

		std::cout << "[Core] Window initialization COMPLETE" << std::endl;
		return true;
	}

	void Core::CleanupWindow() {
		if (window) {
			std::cout << "[Core] Cleaning up ImGui and GLFW..." << std::endl;
			try {
				ImGui_ImplOpenGL3_Shutdown();
				ImGui_ImplGlfw_Shutdown();
				ImGui::DestroyContext();
				glfwDestroyWindow(window);
				window = nullptr;
			}
			catch (const std::exception& e) {
				std::cerr << "[Core] Exception during window cleanup: " << e.what() << std::endl;
			}
		}
		glfwTerminate();
		std::cout << "[Core] Window cleanup complete" << std::endl;
	}

	void Core::Update(const float deltaT) {
		if (!run) return;

		// FPS tracking
		timeElapsed += deltaT;
		frameCount++;
		if (timeElapsed >= 1.0) {
			double fps = frameCount / timeElapsed;
			std::ostringstream titleStream;
			titleStream << "AniStudio - FPS: " << static_cast<int>(fps);
			glfwSetWindowTitle(window, titleStream.str().c_str());
			frameCount = 0;
			timeElapsed = 0.0;
		}

		// Ensure OpenGL context is current before updating studio core
		glfwMakeContextCurrent(window);

		// Verify context is valid
		if (!ANI::OpenGLContextHelper::VerifyContext()) {
			std::cerr << "[Core] ERROR: OpenGL context lost before update!" << std::endl;
			return;
		}

		// Update studio core - ALL OpenGL operations happen in this call chain
		try {
			auto textureSystem = studioCore.GetEntityManager().GetSystem<ECS::TextureSystem>();
			if (textureSystem) {
				textureSystem->ProcessGLOperations();
			}
			studioCore.Update(deltaT);
		}
		catch (const std::exception& e) {
			std::cerr << "[Core] Update error: " << e.what() << std::endl;
		}
	}

	void Core::Draw() {
		if (!run) return;

		try {
			glfwPollEvents();

			// Ensure context is current before any OpenGL operations
			glfwMakeContextCurrent(window);

			if (!ANI::OpenGLContextHelper::VerifyContext()) {
				std::cerr << "[Core] ERROR: OpenGL context lost before render!" << std::endl;
				return;
			}

			// Clear and render
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			// Start ImGui frame - ONLY ONCE per frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			// Render studio content - uses the main ImGui context
			studioCore.Render();

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
			std::cerr << "[Core] Render error: " << e.what() << std::endl;
		}

		glfwSwapBuffers(window);
	}

} // namespace ANI