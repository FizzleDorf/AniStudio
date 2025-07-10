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
 */

#pragma once

#include "OpenGLWrapper.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <functional>
#include <memory>
#include <filesystem>

namespace Utils {

	// Window size constants
	const int MIN_WINDOW_WIDTH = 800;
	const int MIN_WINDOW_HEIGHT = 600;
	const int DEFAULT_WINDOW_WIDTH = 1200;
	const int DEFAULT_WINDOW_HEIGHT = 720;

	/**
	 * Tracks and manages GLFW window state including position, size,
	 * maximized state, and fullscreen mode. Provides callbacks for state changes.
	 */
	class WindowState {
	public:
		struct WindowConfig {
			int width = DEFAULT_WINDOW_WIDTH;
			int height = DEFAULT_WINDOW_HEIGHT;
			int posX = -1;  // -1 means center
			int posY = -1;  // -1 means center
			bool maximized = false;
			bool fullscreen = false;
			bool vsync = true;
			std::string title = "AniStudio";
		};

		// Callback types for window events
		using ResizeCallback = std::function<void(int width, int height)>;
		using PositionCallback = std::function<void(int x, int y)>;
		using StateChangeCallback = std::function<void()>;

		WindowState();
		~WindowState();

		// Window creation and management
		bool CreateGLFWWindow(const WindowConfig& config);
		bool CreateGLFWWindow(); // Overload for default config
		void DestroyWindow();
		bool IsWindowValid() const { return window != nullptr; }
		GLFWwindow* GetWindow() const { return window; }

		// Window state queries
		int GetWidth() const { return currentWidth; }
		int GetHeight() const { return currentHeight; }
		int GetPosX() const { return currentPosX; }
		int GetPosY() const { return currentPosY; }
		bool IsMaximized() const { return isMaximized; }
		bool IsFullscreen() const { return isFullscreen; }
		bool IsMinimized() const;
		bool IsVisible() const;
		bool HasFocus() const;

		// Window state modification
		void SetSize(int width, int height);
		void SetPosition(int x, int y);
		void SetTitle(const std::string& title);
		void Maximize();
		void Restore();
		void Minimize();
		void ToggleFullscreen();
		void SetFullscreen(bool fullscreen);
		void SetVSync(bool enable);
		void CenterWindow();

		// Monitor information
		void GetMonitorInfo(int& monitorWidth, int& monitorHeight) const;
		void GetMonitorWorkArea(int& x, int& y, int& width, int& height) const;

		// Callback registration
		void SetResizeCallback(ResizeCallback callback);
		void SetPositionCallback(PositionCallback callback);
		void SetMaximizeCallback(StateChangeCallback callback);
		void SetMinimizeCallback(StateChangeCallback callback);
		void SetFocusCallback(StateChangeCallback callback);
		void SetCloseCallback(StateChangeCallback callback);

		// State persistence
		nlohmann::json Serialize() const;
		void Deserialize(const nlohmann::json& j);
		bool SaveToFile(const std::string& filepath) const;
		bool LoadFromFile(const std::string& filepath);

		// Global data path management
		void SetGlobalDataPath(const std::string& globalDataPath);

		// Configuration methods
		void ConfigureFromSavedState();
		void ConfigureForGlobalState();
		void SaveCurrentState();

		// Debug methods
		void DebugPrintCurrentState() const;
		void DebugPrintPaths() const;

		// Public state update method
		void UpdateCurrentState();
		void ApplyCurrentStateToWindow();

		// Update state (call this in your main loop)
		void Update();

		// Static utility functions
		static void SetWindowHints();
		static bool InitializeGLFW();
		static void TerminateGLFW();

	private:
		GLFWwindow* window;

		// Current state
		int currentWidth;
		int currentHeight;
		int currentPosX;
		int currentPosY;
		bool isMaximized;
		bool isFullscreen;
		bool vsyncEnabled;
		std::string windowTitle;

		// Persistence paths
		std::string globalDataPath;

		// Pre-fullscreen state for restoration
		int preFullscreenWidth;
		int preFullscreenHeight;
		int preFullscreenPosX;
		int preFullscreenPosY;
		bool preFullscreenMaximized;

		// Callbacks
		ResizeCallback onResize;
		PositionCallback onPosition;
		StateChangeCallback onMaximize;
		StateChangeCallback onMinimize;
		StateChangeCallback onFocus;
		StateChangeCallback onClose;

		// Internal state tracking
		bool stateChanged;
		double lastUpdateTime;

		// GLFW callbacks (static functions)
		static void GLFWWindowSizeCallback(GLFWwindow* window, int width, int height);
		static void GLFWWindowPosCallback(GLFWwindow* window, int xpos, int ypos);
		static void GLFWWindowMaximizeCallback(GLFWwindow* window, int maximized);
		static void GLFWWindowIconifyCallback(GLFWwindow* window, int iconified);
		static void GLFWWindowFocusCallback(GLFWwindow* window, int focused);
		static void GLFWWindowCloseCallback(GLFWwindow* window);

		// Helper methods
		void SavePreFullscreenState();
		void RestorePreFullscreenState();
		bool IsValidSize(int width, int height) const;
		void ClampToMonitorBounds(int& x, int& y, int width, int height) const;
		std::string GetGlobalStatePath() const;
		void EnsureDataDirectoryExists(const std::string& path) const;
	};

	// Singleton access for global window state
	WindowState& GetGlobalWindowState();

} // namespace Utils