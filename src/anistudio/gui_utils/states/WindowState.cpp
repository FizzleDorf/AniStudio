#include "WindowState.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>

namespace Utils {

	// Static GLFW callback implementations
	void WindowState::GLFWWindowSizeCallback(GLFWwindow* window, int width, int height) {
		WindowState* state = static_cast<WindowState*>(glfwGetWindowUserPointer(window));
		if (state) {
			state->currentWidth = width;
			state->currentHeight = height;
			state->stateChanged = true;

			if (state->onResize) {
				state->onResize(width, height);
			}
		}
	}

	void WindowState::GLFWWindowPosCallback(GLFWwindow* window, int xpos, int ypos) {
		WindowState* state = static_cast<WindowState*>(glfwGetWindowUserPointer(window));
		if (state) {
			state->currentPosX = xpos;
			state->currentPosY = ypos;
			state->stateChanged = true;

			if (state->onPosition) {
				state->onPosition(xpos, ypos);
			}
		}
	}

	void WindowState::GLFWWindowMaximizeCallback(GLFWwindow* window, int maximized) {
		WindowState* state = static_cast<WindowState*>(glfwGetWindowUserPointer(window));
		if (state) {
			state->isMaximized = (maximized == GLFW_TRUE);
			state->stateChanged = true;

			if (state->onMaximize) {
				state->onMaximize();
			}
		}
	}

	void WindowState::GLFWWindowIconifyCallback(GLFWwindow* window, int iconified) {
		WindowState* state = static_cast<WindowState*>(glfwGetWindowUserPointer(window));
		if (state) {
			state->stateChanged = true;

			if (iconified == GLFW_TRUE && state->onMinimize) {
				state->onMinimize();
			}
		}
	}

	void WindowState::GLFWWindowFocusCallback(GLFWwindow* window, int focused) {
		WindowState* state = static_cast<WindowState*>(glfwGetWindowUserPointer(window));
		if (state && state->onFocus) {
			state->onFocus();
		}
	}

	void WindowState::GLFWWindowCloseCallback(GLFWwindow* window) {
		WindowState* state = static_cast<WindowState*>(glfwGetWindowUserPointer(window));
		if (state && state->onClose) {
			state->onClose();
		}
	}

	// WindowState implementation
	WindowState::WindowState()
		: window(nullptr)
		, currentWidth(DEFAULT_WINDOW_WIDTH)
		, currentHeight(DEFAULT_WINDOW_HEIGHT)
		, currentPosX(100)
		, currentPosY(100)
		, isMaximized(false)
		, isFullscreen(false)
		, vsyncEnabled(true)
		, windowTitle("AniStudio")
		, globalDataPath("")
		, preFullscreenWidth(DEFAULT_WINDOW_WIDTH)
		, preFullscreenHeight(DEFAULT_WINDOW_HEIGHT)
		, preFullscreenPosX(100)
		, preFullscreenPosY(100)
		, preFullscreenMaximized(false)
		, stateChanged(false)
		, lastUpdateTime(0.0) {
	}

	WindowState::~WindowState() {
		DestroyWindow();
	}

	bool WindowState::CreateGLFWWindow(const WindowConfig& config) {
		if (window) {
			std::cerr << "[WindowState] Window already exists" << std::endl;
			return false;
		}

		// Apply configuration
		currentWidth = std::max(config.width, MIN_WINDOW_WIDTH);
		currentHeight = std::max(config.height, MIN_WINDOW_HEIGHT);
		windowTitle = config.title;
		vsyncEnabled = config.vsync;

		// Set window hints
		SetWindowHints();

		// Create window
		window = glfwCreateWindow(currentWidth, currentHeight, windowTitle.c_str(), nullptr, nullptr);
		if (!window) {
			std::cerr << "[WindowState] Failed to create GLFW window" << std::endl;
			return false;
		}

		// Set user pointer for callbacks
		glfwSetWindowUserPointer(window, this);

		// Register callbacks
		glfwSetWindowSizeCallback(window, GLFWWindowSizeCallback);
		glfwSetWindowPosCallback(window, GLFWWindowPosCallback);
		glfwSetWindowMaximizeCallback(window, GLFWWindowMaximizeCallback);
		glfwSetWindowIconifyCallback(window, GLFWWindowIconifyCallback);
		glfwSetWindowFocusCallback(window, GLFWWindowFocusCallback);
		glfwSetWindowCloseCallback(window, GLFWWindowCloseCallback);

		// Set position
		if (config.posX >= 0 && config.posY >= 0) {
			SetPosition(config.posX, config.posY);
		}
		else {
			CenterWindow();
		}

		// Handle maximized state
		if (config.maximized) {
			Maximize();
		}

		// Handle fullscreen
		if (config.fullscreen) {
			SetFullscreen(true);
		}

		// Make context current and set VSync
		glfwMakeContextCurrent(window);
		SetVSync(vsyncEnabled);

		// Update initial state
		UpdateCurrentState();

		std::cout << "[WindowState] Window created successfully ("
			<< currentWidth << "x" << currentHeight << ")" << std::endl;

		return true;
	}

	bool WindowState::CreateGLFWWindow() {
		WindowConfig defaultConfig;
		return CreateGLFWWindow(defaultConfig);
	}

	void WindowState::DestroyWindow() {
		if (window) {
			glfwDestroyWindow(window);
			window = nullptr;
			std::cout << "[WindowState] Window destroyed" << std::endl;
		}
	}

	bool WindowState::IsMinimized() const {
		return window ? glfwGetWindowAttrib(window, GLFW_ICONIFIED) == GLFW_TRUE : false;
	}

	bool WindowState::IsVisible() const {
		return window ? glfwGetWindowAttrib(window, GLFW_VISIBLE) == GLFW_TRUE : false;
	}

	bool WindowState::HasFocus() const {
		return window ? glfwGetWindowAttrib(window, GLFW_FOCUSED) == GLFW_TRUE : false;
	}

	void WindowState::SetSize(int width, int height) {
		if (!window || !IsValidSize(width, height)) return;

		glfwSetWindowSize(window, width, height);
		currentWidth = width;
		currentHeight = height;
	}

	void WindowState::SetPosition(int x, int y) {
		if (!window) return;

		ClampToMonitorBounds(x, y, currentWidth, currentHeight);
		glfwSetWindowPos(window, x, y);
		currentPosX = x;
		currentPosY = y;
	}

	void WindowState::SetTitle(const std::string& title) {
		if (!window) return;

		windowTitle = title;
		glfwSetWindowTitle(window, title.c_str());
	}

	void WindowState::Maximize() {
		if (!window || isFullscreen) return;

		glfwMaximizeWindow(window);
		isMaximized = true;
	}

	void WindowState::Restore() {
		if (!window) return;

		if (isFullscreen) {
			SetFullscreen(false);
		}
		else if (isMaximized) {
			glfwRestoreWindow(window);
			isMaximized = false;
		}
	}

	void WindowState::Minimize() {
		if (!window) return;

		glfwIconifyWindow(window);
	}

	void WindowState::ToggleFullscreen() {
		SetFullscreen(!isFullscreen);
	}

	void WindowState::SetFullscreen(bool fullscreen) {
		if (!window || isFullscreen == fullscreen) return;

		if (fullscreen) {
			// Save current state before going fullscreen
			SavePreFullscreenState();

			// Get primary monitor
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);

			glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
			isFullscreen = true;
		}
		else {
			// Restore windowed mode
			glfwSetWindowMonitor(window, nullptr,
				preFullscreenPosX, preFullscreenPosY,
				preFullscreenWidth, preFullscreenHeight,
				GLFW_DONT_CARE);

			if (preFullscreenMaximized) {
				glfwMaximizeWindow(window);
			}

			isFullscreen = false;
			isMaximized = preFullscreenMaximized;
		}

		SetVSync(vsyncEnabled);
	}

	void WindowState::SetVSync(bool enable) {
		if (!window) return;

		vsyncEnabled = enable;
		glfwSwapInterval(enable ? 1 : 0);
	}

	void WindowState::CenterWindow() {
		if (!window) return;

		int monitorX, monitorY, monitorWidth, monitorHeight;
		GetMonitorWorkArea(monitorX, monitorY, monitorWidth, monitorHeight);

		int x = monitorX + (monitorWidth - currentWidth) / 2;
		int y = monitorY + (monitorHeight - currentHeight) / 2;

		SetPosition(x, y);
	}

	void WindowState::GetMonitorInfo(int& monitorWidth, int& monitorHeight) const {
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		monitorWidth = mode->width;
		monitorHeight = mode->height;
	}

	void WindowState::GetMonitorWorkArea(int& x, int& y, int& width, int& height) const {
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		glfwGetMonitorWorkarea(monitor, &x, &y, &width, &height);
	}

	void WindowState::SetResizeCallback(ResizeCallback callback) {
		onResize = callback;
	}

	void WindowState::SetPositionCallback(PositionCallback callback) {
		onPosition = callback;
	}

	void WindowState::SetMaximizeCallback(StateChangeCallback callback) {
		onMaximize = callback;
	}

	void WindowState::SetMinimizeCallback(StateChangeCallback callback) {
		onMinimize = callback;
	}

	void WindowState::SetFocusCallback(StateChangeCallback callback) {
		onFocus = callback;
	}

	void WindowState::SetCloseCallback(StateChangeCallback callback) {
		onClose = callback;
	}

	nlohmann::json WindowState::Serialize() const {
		nlohmann::json j;
		j["width"] = currentWidth;
		j["height"] = currentHeight;
		j["posX"] = currentPosX;
		j["posY"] = currentPosY;
		j["maximized"] = isMaximized;
		j["fullscreen"] = isFullscreen;
		j["vsync"] = vsyncEnabled;
		j["title"] = windowTitle;
		return j;
	}

	void WindowState::Deserialize(const nlohmann::json& j) {
		if (j.contains("width")) currentWidth = std::max(j["width"].get<int>(), MIN_WINDOW_WIDTH);
		if (j.contains("height")) currentHeight = std::max(j["height"].get<int>(), MIN_WINDOW_HEIGHT);
		if (j.contains("posX")) currentPosX = j["posX"];
		if (j.contains("posY")) currentPosY = j["posY"];
		if (j.contains("maximized")) isMaximized = j["maximized"];
		if (j.contains("fullscreen")) isFullscreen = j["fullscreen"];
		if (j.contains("vsync")) vsyncEnabled = j["vsync"];
		if (j.contains("title")) windowTitle = j["title"];
	}

	bool WindowState::SaveToFile(const std::string& filepath) const {
		try {
			nlohmann::json j = Serialize();
			std::ofstream file(filepath);
			if (!file.is_open()) return false;

			file << j.dump(4);
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[WindowState] Failed to save: " << e.what() << std::endl;
			return false;
		}
	}

	bool WindowState::LoadFromFile(const std::string& filepath) {
		try {
			std::ifstream file(filepath);
			if (!file.is_open()) return false;

			nlohmann::json j;
			file >> j;
			Deserialize(j);
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[WindowState] Failed to load: " << e.what() << std::endl;
			return false;
		}
	}

	void WindowState::SetGlobalDataPath(const std::string& globalDataPath) {
		this->globalDataPath = globalDataPath;
		std::cout << "[WindowState] Global data path set to: " << globalDataPath << std::endl;
	}

	void WindowState::ConfigureFromSavedState() {
		std::cout << "[WindowState] Configuring from saved global state..." << std::endl;
		DebugPrintPaths();

		// Load global state first
		std::string globalPath = GetGlobalStatePath();
		if (std::filesystem::exists(globalPath)) {
			std::cout << "[WindowState] Loading global state from: " << globalPath << std::endl;
			LoadFromFile(globalPath);
		}
		else {
			std::cout << "[WindowState] No global state file found, using defaults" << std::endl;
		}

		std::cout << "[WindowState] Configuration complete" << std::endl;
		DebugPrintCurrentState();
	}

	void WindowState::ConfigureForGlobalState() {
		std::cout << "[WindowState] Configuring for global state..." << std::endl;

		// Debug before loading
		std::cout << "[WindowState] Current state before reverting to global:" << std::endl;
		DebugPrintCurrentState();

		// Update current state from window before saving
		UpdateCurrentState();

		// Save current state to global
		std::string globalPath = GetGlobalStatePath();
		EnsureDataDirectoryExists(std::filesystem::path(globalPath).parent_path().string());
		SaveToFile(globalPath);

		// Load and apply global state
		std::cout << "[WindowState] Loading global state from: " << globalPath << std::endl;
		if (std::filesystem::exists(globalPath)) {
			LoadFromFile(globalPath);
		}
		else {
			std::cout << "[WindowState] No global state file found" << std::endl;
		}

		// Debug after loading
		std::cout << "[WindowState] State after reverting to global:" << std::endl;
		DebugPrintCurrentState();

		ApplyCurrentStateToWindow();
		std::cout << "[WindowState] Applied global state to window" << std::endl;
	}

	void WindowState::SaveCurrentState() {
		std::cout << "[WindowState] Saving current state..." << std::endl;

		// Update current state from window
		UpdateCurrentState();

		// Debug what we're saving
		DebugPrintCurrentState();

		// Save to global
		std::string globalPath = GetGlobalStatePath();
		EnsureDataDirectoryExists(std::filesystem::path(globalPath).parent_path().string());
		std::cout << "[WindowState] Saving to global: " << globalPath << std::endl;
		SaveToFile(globalPath);

		std::cout << "[WindowState] Save complete" << std::endl;
	}

	void WindowState::DebugPrintCurrentState() const {
		std::cout << "[WindowState] Current State:" << std::endl;
		std::cout << "  Size: " << currentWidth << "x" << currentHeight << std::endl;
		std::cout << "  Position: " << currentPosX << ", " << currentPosY << std::endl;
		std::cout << "  Maximized: " << (isMaximized ? "true" : "false") << std::endl;
		std::cout << "  Fullscreen: " << (isFullscreen ? "true" : "false") << std::endl;
		std::cout << "  VSync: " << (vsyncEnabled ? "true" : "false") << std::endl;
	}

	void WindowState::DebugPrintPaths() const {
		std::cout << "[WindowState] Paths:" << std::endl;
		std::cout << "  Global data path: " << globalDataPath << std::endl;
		std::cout << "  Global state file: " << GetGlobalStatePath() << std::endl;
	}

	void WindowState::Update() {
		if (!window) return;

		double currentTime = glfwGetTime();
		if (currentTime - lastUpdateTime > 0.1) { // Update every 100ms
			UpdateCurrentState();
			lastUpdateTime = currentTime;
		}
	}

	void WindowState::SetWindowHints() {
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
		glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
	}

	bool WindowState::InitializeGLFW() {
		if (!glfwInit()) {
			std::cerr << "[WindowState] Failed to initialize GLFW" << std::endl;
			return false;
		}
		return true;
	}

	void WindowState::TerminateGLFW() {
		glfwTerminate();
	}

	void WindowState::UpdateCurrentState() {
		if (!window) return;

		// Update size
		glfwGetWindowSize(window, &currentWidth, &currentHeight);

		// Update position
		glfwGetWindowPos(window, &currentPosX, &currentPosY);

		// Update maximized state
		isMaximized = glfwGetWindowAttrib(window, GLFW_MAXIMIZED) == GLFW_TRUE;
	}

	void WindowState::SavePreFullscreenState() {
		preFullscreenWidth = currentWidth;
		preFullscreenHeight = currentHeight;
		preFullscreenPosX = currentPosX;
		preFullscreenPosY = currentPosY;
		preFullscreenMaximized = isMaximized;
	}

	void WindowState::RestorePreFullscreenState() {
		currentWidth = preFullscreenWidth;
		currentHeight = preFullscreenHeight;
		currentPosX = preFullscreenPosX;
		currentPosY = preFullscreenPosY;
		isMaximized = preFullscreenMaximized;
	}

	bool WindowState::IsValidSize(int width, int height) const {
		return width >= MIN_WINDOW_WIDTH && height >= MIN_WINDOW_HEIGHT;
	}

	void WindowState::ClampToMonitorBounds(int& x, int& y, int width, int height) const {
		int monitorX, monitorY, monitorWidth, monitorHeight;
		GetMonitorWorkArea(monitorX, monitorY, monitorWidth, monitorHeight);

		// Ensure window is not completely off-screen
		x = std::max(monitorX - width + 100, std::min(x, monitorX + monitorWidth - 100));
		y = std::max(monitorY, std::min(y, monitorY + monitorHeight - 100));
	}

	std::string WindowState::GetGlobalStatePath() const {
		return globalDataPath + "/window_state.json";
	}

	void WindowState::EnsureDataDirectoryExists(const std::string& path) const {
		try {
			if (!std::filesystem::exists(path)) {
				std::filesystem::create_directories(path);
				std::cout << "[WindowState] Created directory: " << path << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[WindowState] Failed to create directory " << path << ": " << e.what() << std::endl;
		}
	}

	void WindowState::ApplyCurrentStateToWindow() {
		if (!window) return;

		std::cout << "[WindowState] Applying state to window..." << std::endl;

		// Apply size
		if (IsValidSize(currentWidth, currentHeight)) {
			std::cout << "[WindowState] Setting size to: " << currentWidth << "x" << currentHeight << std::endl;
			SetSize(currentWidth, currentHeight);
		}

		// Apply position
		std::cout << "[WindowState] Setting position to: " << currentPosX << ", " << currentPosY << std::endl;
		SetPosition(currentPosX, currentPosY);

		// Apply window state
		if (isFullscreen) {
			std::cout << "[WindowState] Setting fullscreen" << std::endl;
			SetFullscreen(true);
		}
		else if (isMaximized) {
			std::cout << "[WindowState] Maximizing window" << std::endl;
			Maximize();
		}
		else {
			std::cout << "[WindowState] Restoring window" << std::endl;
			Restore();
		}

		// Apply VSync
		std::cout << "[WindowState] Setting VSync to: " << (vsyncEnabled ? "enabled" : "disabled") << std::endl;
		SetVSync(vsyncEnabled);

		std::cout << "[WindowState] State application complete" << std::endl;
	}

	// Singleton instance
	WindowState& GetGlobalWindowState() {
		static WindowState instance;
		return instance;
	}

} // namespace Utils