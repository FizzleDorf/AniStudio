#include "Core.hpp"
#include "Events.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <filesystem>

namespace ANI {
	Core& appCore = Core::Ref();

	void WindowCloseCallback(GLFWwindow* window) {
		appCore.Quit();
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

		// Setup window context
		studioCore.SetWindowHandle(window);
		studioCore.SetImGuiContext(ImGui::GetCurrentContext());
		std::cout << "[Core] Window handle and ImGui context set" << std::endl;

		// managers for Events
		std::cout << "[Core] Setting up Events system..." << std::endl;
		ANI::Events::Ref().SetManagers(&studioCore.GetViewManager(), &studioCore.GetProjectManager());
		std::cout << "[Core] Events system configured" << std::endl;

		// Complete StudioCore initialization (plugins, views, etc.)
		std::cout << "[Core] Completing StudioCore initialization..." << std::endl;
		studioCore.CompleteInitialization();
		std::cout << "[Core] StudioCore fully initialized" << std::endl;

		std::cout << "[Core] Initialization complete!" << std::endl;
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

		// Initialize ImGui
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

		// Set INI file path
		std::cout << "[Core] Setting INI file path..." << std::endl;
		std::string iniFilePath = std::filesystem::absolute(Utils::FilePaths::ImguiStatePath).string();
		std::filesystem::path iniDir = std::filesystem::path(iniFilePath).parent_path();
		if (!std::filesystem::exists(iniDir)) {
			std::filesystem::create_directories(iniDir);
		}

		static std::string persistentIniPath = iniFilePath;
		io.IniFilename = persistentIniPath.c_str();
		std::cout << "[Core] INI file path set to: " << io.IniFilename << std::endl;

		// Initialize ImGui backends
		std::cout << "[Core] Initializing GLFW backend..." << std::endl;
		bool glfwOk = ImGui_ImplGlfw_InitForOpenGL(window, true);
		std::cout << "[Core] GLFW backend result: " << (glfwOk ? "SUCCESS" : "FAILED") << std::endl;
		if (!glfwOk) return false;

		std::cout << "[Core] Initializing OpenGL3 backend..." << std::endl;
		bool gl3Ok = ImGui_ImplOpenGL3_Init("#version 330");
		std::cout << "[Core] OpenGL3 backend result: " << (gl3Ok ? "SUCCESS" : "FAILED") << std::endl;
		if (!gl3Ok) return false;

		// Add default font
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

		// Update studio core
		try {
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

			// Clear and render
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			// Render studio content
			studioCore.Render();
		}
		catch (const std::exception& e) {
			std::cerr << "[Core] Render error: " << e.what() << std::endl;
		}

		glfwSwapBuffers(window);
	}

} // namespace ANI