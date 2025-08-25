// Core.cpp - FIXED - No LoadDefaultPlugins call
#include "Core.hpp"
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
		studioCore.Shutdown();
		CleanupWindow();
	}

	void Core::Quit() {
		std::cout << "[Core] Quit called - setting run to false" << std::endl;
		run = false;
		studioCore.SetRunning(false);
		// Destructor will handle the actual shutdown when Core goes out of scope
	}

	void Core::Init() {
		std::cout << "[Core] Initializing..." << std::endl;

		if (!InitializeWindow()) {
			throw std::runtime_error("Failed to initialize window");
		}

		// Initialize the studio core
		if (!studioCore.Initialize()) {
			throw std::runtime_error("Failed to initialize StudioCore");
		}

		// Setup window context
		studioCore.SetWindowHandle(window);
		studioCore.SetImGuiContext(ImGui::GetCurrentContext());

		std::cout << "[Core] Initialization complete!" << std::endl;
	}

	bool Core::InitializeWindow() {
		if (!glfwInit()) {
			std::cerr << "[Core] Failed to initialize GLFW" << std::endl;
			return false;
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

		window = glfwCreateWindow(videoWidth, videoHeight, "AniStudio", nullptr, nullptr);
		if (!window) {
			std::cerr << "[Core] Failed to create GLFW window" << std::endl;
			glfwTerminate();
			return false;
		}

		glfwMakeContextCurrent(window);
		glfwSetWindowCloseCallback(window, WindowCloseCallback);
		glfwSwapInterval(1); // VSync

		if (glewInit() != GLEW_OK) {
			std::cerr << "[Core] Failed to initialize GLEW" << std::endl;
			return false;
		}

		glViewport(0, 0, videoWidth, videoHeight);

		// Initialize ImGui
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		std::string iniFilePath = std::filesystem::absolute(Utils::FilePaths::ImguiStatePath).string();

		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 330");

		return true;
	}

	void Core::CleanupWindow() {
		if (window) {
			std::cout << "[Core] Cleaning up ImGui and GLFW..." << std::endl;
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
			glfwDestroyWindow(window);
			window = nullptr;
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
			// Clear and render
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			// Render studio content
			studioCore.Render();
		}
		catch (const std::exception& e) {
			std::cerr << "[Core] Render error: " << e.what() << std::endl;
		}

		glfwSwapBuffers(window);
	}

} // namespace ANI