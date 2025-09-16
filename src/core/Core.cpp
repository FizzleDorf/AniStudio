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

		// CRITICAL FIX: Set INI filename BEFORE initializing ImGui backends
		ImGuiIO& io = ImGui::GetIO();

		// Get the absolute path to the imgui.ini file
		std::string iniFilePath = std::filesystem::absolute(Utils::FilePaths::ImguiStatePath).string();

		// Create the directory if it doesn't exist
		std::filesystem::path iniDir = std::filesystem::path(iniFilePath).parent_path();
		if (!std::filesystem::exists(iniDir)) {
			std::filesystem::create_directories(iniDir);
			std::cout << "[Core] Created ImGui INI directory: " << iniDir << std::endl;
		}

		// Convert to C-style string that ImGui can use
		static std::string persistentIniPath = iniFilePath; // Static to ensure it persists
		io.IniFilename = persistentIniPath.c_str();

		std::cout << "[Core] ImGui INI file set to: " << io.IniFilename << std::endl;

		// Check if INI file exists, if not copy from defaults
		if (!std::filesystem::exists(iniFilePath)) {
			std::cout << "[Core] ImGui INI file not found, checking for default..." << std::endl;

			// Try to copy from defaults directory
			std::string defaultIniPath = Utils::FilePaths::dataPath + "/defaults/imgui.ini";
			if (std::filesystem::exists(defaultIniPath)) {
				try {
					std::filesystem::copy_file(defaultIniPath, iniFilePath);
					std::cout << "[Core] Copied default ImGui INI from: " << defaultIniPath << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[Core] Warning: Could not copy default ImGui INI: " << e.what() << std::endl;
				}
			}
			else {
				std::cout << "[Core] No default ImGui INI found at: " << defaultIniPath << std::endl;
				std::cout << "[Core] ImGui will create a new INI file on first run" << std::endl;
			}
		}

		// Now initialize ImGui backends
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