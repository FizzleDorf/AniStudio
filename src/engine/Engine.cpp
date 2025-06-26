#include "Engine.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <filesystem>

namespace ANI {
	Engine &Core = Engine::Ref();

	void WindowCloseCallback(GLFWwindow *window) {
		Core.Quit();
	}

	Engine::Engine() : run(true), window(nullptr),
		videoWidth(SCREEN_WIDTH), videoHeight(SCREEN_HEIGHT),
		fpsSum(0.0), frameCount(0), timeElapsed(0.0) {
	}

	Engine::~Engine() {
		// Cleanup in reverse order
		StudioCore::Shutdown(); // API handles all the complex cleanup
		CleanupWindow();
	}

	void Engine::Quit() {
		run = false;
		StudioCore::SetRunning(false);
	}

	void Engine::Init() {
		std::cout << "[Engine] Initializing..." << std::endl;

		if (!InitializeWindow()) {
			throw std::runtime_error("Failed to initialize window");
		}

		// Initialize the studio core (handles ALL ECS, GUI, and Plugin logic)
		if (!StudioCore::Initialize()) {
			throw std::runtime_error("Failed to initialize StudioCore");
		}

		// Setup window context for plugins
		StudioCore::SetWindowHandle(window);
		StudioCore::SetImGuiContext(ImGui::GetCurrentContext());

		// Load default plugins
		StudioCore::LoadDefaultPlugins();

		std::cout << "[Engine] Initialization complete!" << std::endl;
	}

	bool Engine::InitializeWindow() {
		if (!glfwInit()) {
			std::cerr << "[Engine] Failed to initialize GLFW" << std::endl;
			return false;
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

		window = glfwCreateWindow(videoWidth, videoHeight, "AniStudio", nullptr, nullptr);
		if (!window) {
			std::cerr << "[Engine] Failed to create GLFW window" << std::endl;
			glfwTerminate();
			return false;
		}

		glfwMakeContextCurrent(window);
		glfwSetWindowCloseCallback(window, WindowCloseCallback);
		glfwSwapInterval(1); // VSync

		if (glewInit() != GLEW_OK) {
			std::cerr << "[Engine] Failed to initialize GLEW" << std::endl;
			return false;
		}

		glViewport(0, 0, videoWidth, videoHeight);

		// Initialize ImGui
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		std::string iniFilePath = std::filesystem::absolute(Utils::FilePaths::ImguiStatePath).string();

		ImGuiIO &io = ImGui::GetIO();
		io.IniFilename = iniFilePath.c_str();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

		ImGui::StyleColorsDark();
		ImGuiStyle &style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		// Load style from file if it exists
		// LoadStyleFromFile(style, "../data/defaults/style.json");

		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 330");

		return true;
	}

	void Engine::CleanupWindow() {
		if (window) {
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
			glfwDestroyWindow(window);
		}
		glfwTerminate();
	}

	void Engine::Update(const float deltaT) {
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

		// Update studio core (handles ECS + GUI + Plugins)
		StudioCore::Update(deltaT);
	}

	void Engine::Draw() {
		try {
			// Clear and render - StudioCore handles all ImGui setup and rendering
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			// Render studio content (all views including plugin views and menu bar)
			StudioCore::Render();
		}
		catch (const std::exception& e) {
			std::cerr << "[Engine] Render error: " << e.what() << std::endl;
		}

		glfwSwapBuffers(window);
	}

	// Dependency accessors - delegate to StudioCore APIs
	ECS::EntityManager& Engine::GetEntityManager() {
		return StudioCore::GetEntityManager();
	}

	GUI::ViewManager& Engine::GetViewManager() {
		return StudioCore::GetViewManager();
	}

	Plugin::PluginManager& Engine::GetPluginManager() {
		return StudioCore::GetPluginManager();
	}

} // namespace ANI