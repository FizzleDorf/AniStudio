#pragma once

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace Utils {

	class ImGuiSettingsUtil {
	public:
		static void SerializeIO(const ImGuiIO& io, nlohmann::json& j) {
			// Window behavior
			j["ConfigWindowsResizeFromEdges"] = io.ConfigWindowsResizeFromEdges;
			j["ConfigWindowsMoveFromTitleBarOnly"] = io.ConfigWindowsMoveFromTitleBarOnly;
			j["ConfigDragClickToInputText"] = io.ConfigDragClickToInputText;

			// Navigation
			j["ConfigNavMoveSetMousePos"] = io.ConfigNavMoveSetMousePos;
			j["ConfigNavCaptureKeyboard"] = io.ConfigNavCaptureKeyboard;
			j["ConfigNavEscapeClearFocusItem"] = io.ConfigNavEscapeClearFocusItem;

			// Memory & Performance
			j["ConfigMemoryCompactTimer"] = io.ConfigMemoryCompactTimer;

			// Debug
			j["ConfigDebugHighlightIdConflicts"] = io.ConfigDebugHighlightIdConflicts;

			// Docking
			j["ConfigDockingWithShift"] = io.ConfigDockingWithShift;
			j["ConfigDockingAlwaysTabBar"] = io.ConfigDockingAlwaysTabBar;
			j["ConfigDockingTransparentPayload"] = io.ConfigDockingTransparentPayload;

			// Viewports
			j["ConfigViewportsNoAutoMerge"] = io.ConfigViewportsNoAutoMerge;
			j["ConfigViewportsNoTaskBarIcon"] = io.ConfigViewportsNoTaskBarIcon;
			j["ConfigViewportsNoDecoration"] = io.ConfigViewportsNoDecoration;
			j["ConfigViewportsNoDefaultParent"] = io.ConfigViewportsNoDefaultParent;

			// Platform
			j["ConfigMacOSXBehaviors"] = io.ConfigMacOSXBehaviors;

			// Input Text
			j["ConfigInputTextCursorBlink"] = io.ConfigInputTextCursorBlink;
			j["ConfigInputTextEnterKeepActive"] = io.ConfigInputTextEnterKeepActive;

			// ConfigFlags
			j["ConfigFlags"] = static_cast<int>(io.ConfigFlags);

			// Input timing
			j["MouseDoubleClickTime"] = io.MouseDoubleClickTime;
			j["MouseDoubleClickMaxDist"] = io.MouseDoubleClickMaxDist;
			j["MouseDragThreshold"] = io.MouseDragThreshold;
			j["KeyRepeatDelay"] = io.KeyRepeatDelay;
			j["KeyRepeatRate"] = io.KeyRepeatRate;

			// Display
			j["FontGlobalScale"] = io.FontGlobalScale;
			j["DisplayFramebufferScale"] = { io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y };
		}

		static void DeserializeIO(ImGuiIO& io, const nlohmann::json& j) {
			// Window behavior
			if (j.contains("ConfigWindowsResizeFromEdges"))
				io.ConfigWindowsResizeFromEdges = j["ConfigWindowsResizeFromEdges"];
			if (j.contains("ConfigWindowsMoveFromTitleBarOnly"))
				io.ConfigWindowsMoveFromTitleBarOnly = j["ConfigWindowsMoveFromTitleBarOnly"];
			if (j.contains("ConfigDragClickToInputText"))
				io.ConfigDragClickToInputText = j["ConfigDragClickToInputText"];

			// Navigation
			if (j.contains("ConfigNavMoveSetMousePos"))
				io.ConfigNavMoveSetMousePos = j["ConfigNavMoveSetMousePos"];
			if (j.contains("ConfigNavCaptureKeyboard"))
				io.ConfigNavCaptureKeyboard = j["ConfigNavCaptureKeyboard"];
			if (j.contains("ConfigNavEscapeClearFocusItem"))
				io.ConfigNavEscapeClearFocusItem = j["ConfigNavEscapeClearFocusItem"];

			// Memory & Performance
			if (j.contains("ConfigMemoryCompactTimer"))
				io.ConfigMemoryCompactTimer = j["ConfigMemoryCompactTimer"];

			// Debug
			if (j.contains("ConfigDebugHighlightIdConflicts"))
				io.ConfigDebugHighlightIdConflicts = j["ConfigDebugHighlightIdConflicts"];

			// Docking
			if (j.contains("ConfigDockingWithShift"))
				io.ConfigDockingWithShift = j["ConfigDockingWithShift"];
			if (j.contains("ConfigDockingAlwaysTabBar"))
				io.ConfigDockingAlwaysTabBar = j["ConfigDockingAlwaysTabBar"];
			if (j.contains("ConfigDockingTransparentPayload"))
				io.ConfigDockingTransparentPayload = j["ConfigDockingTransparentPayload"];

			// Viewports
			if (j.contains("ConfigViewportsNoAutoMerge"))
				io.ConfigViewportsNoAutoMerge = j["ConfigViewportsNoAutoMerge"];
			if (j.contains("ConfigViewportsNoTaskBarIcon"))
				io.ConfigViewportsNoTaskBarIcon = j["ConfigViewportsNoTaskBarIcon"];
			if (j.contains("ConfigViewportsNoDecoration"))
				io.ConfigViewportsNoDecoration = j["ConfigViewportsNoDecoration"];
			if (j.contains("ConfigViewportsNoDefaultParent"))
				io.ConfigViewportsNoDefaultParent = j["ConfigViewportsNoDefaultParent"];

			// Platform
			if (j.contains("ConfigMacOSXBehaviors"))
				io.ConfigMacOSXBehaviors = j["ConfigMacOSXBehaviors"];

			// Input Text
			if (j.contains("ConfigInputTextCursorBlink"))
				io.ConfigInputTextCursorBlink = j["ConfigInputTextCursorBlink"];
			if (j.contains("ConfigInputTextEnterKeepActive"))
				io.ConfigInputTextEnterKeepActive = j["ConfigInputTextEnterKeepActive"];

			// ConfigFlags
			if (j.contains("ConfigFlags"))
				io.ConfigFlags = static_cast<ImGuiConfigFlags>(j["ConfigFlags"].get<int>());

			// Input timing
			if (j.contains("MouseDoubleClickTime"))
				io.MouseDoubleClickTime = j["MouseDoubleClickTime"];
			if (j.contains("MouseDoubleClickMaxDist"))
				io.MouseDoubleClickMaxDist = j["MouseDoubleClickMaxDist"];
			if (j.contains("MouseDragThreshold"))
				io.MouseDragThreshold = j["MouseDragThreshold"];
			if (j.contains("KeyRepeatDelay"))
				io.KeyRepeatDelay = j["KeyRepeatDelay"];
			if (j.contains("KeyRepeatRate"))
				io.KeyRepeatRate = j["KeyRepeatRate"];

			// Display
			if (j.contains("FontGlobalScale"))
				io.FontGlobalScale = j["FontGlobalScale"];
			if (j.contains("DisplayFramebufferScale")) {
				auto scale = j["DisplayFramebufferScale"];
				io.DisplayFramebufferScale.x = scale[0];
				io.DisplayFramebufferScale.y = scale[1];
			}
		}

		static bool SaveToFile(const std::string& filePath, const ImGuiIO& io) {
			try {
				nlohmann::json j;
				SerializeIO(io, j);

				std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());
				std::ofstream file(filePath);
				if (!file.is_open()) {
					std::cerr << "[ImGuiSettingsUtil] Failed to open file for writing: " << filePath << std::endl;
					return false;
				}

				file << j.dump(4);
				file.close();
				return true;
			}
			catch (const std::exception& e) {
				std::cerr << "[ImGuiSettingsUtil] Save error: " << e.what() << std::endl;
				return false;
			}
		}

		static bool LoadFromFile(const std::string& filePath, ImGuiIO& io) {
			try {
				if (!std::filesystem::exists(filePath)) {
					std::cout << "[ImGuiSettingsUtil] File doesn't exist: " << filePath << std::endl;
					return false;
				}

				std::ifstream file(filePath);
				if (!file.is_open()) {
					std::cerr << "[ImGuiSettingsUtil] Failed to open file for reading: " << filePath << std::endl;
					return false;
				}

				nlohmann::json j;
				file >> j;
				file.close();

				DeserializeIO(io, j);
				return true;
			}
			catch (const std::exception& e) {
				std::cerr << "[ImGuiSettingsUtil] Load error: " << e.what() << std::endl;
				return false;
			}
		}

		static void LoadImGuiSettingsForApp(const std::string& dataPath) {
			std::cout << "[ImGuiSettingsUtil] Loading ImGui settings for application..." << std::endl;

			ImGuiIO& io = ImGui::GetIO();

			// Try to load user settings first
			std::string userSettingsPath = dataPath + "/settings/imgui_render_settings.json";
			if (LoadFromFile(userSettingsPath, io)) {
				std::cout << "[ImGuiSettingsUtil] Loaded user ImGui settings from: " << userSettingsPath << std::endl;
				return;
			}

			// Fall back to defaults
			std::string defaultSettingsPath = dataPath + "/defaults/imgui_render_defaults.json";
			if (LoadFromFile(defaultSettingsPath, io)) {
				std::cout << "[ImGuiSettingsUtil] Loaded default ImGui settings from: " << defaultSettingsPath << std::endl;
				return;
			}

			// If no files exist, create defaults and apply them
			std::cout << "[ImGuiSettingsUtil] No settings found, creating and applying defaults..." << std::endl;
			CreateDefaultSettings(dataPath);
			LoadFromFile(defaultSettingsPath, io);
		}

		static void CreateDefaultSettings(const std::string& dataPath) {
			// Create default ImGui settings
			ImGuiIO defaultIO = ImGui::GetIO(); // Start with current IO as base

			// Set optimal defaults for performance
			defaultIO.ConfigFlags = ImGuiConfigFlags_DockingEnable; // Enable docking by default, viewports OFF
			defaultIO.ConfigWindowsResizeFromEdges = true;
			defaultIO.ConfigWindowsMoveFromTitleBarOnly = false;
			defaultIO.ConfigDragClickToInputText = false;
			defaultIO.ConfigNavMoveSetMousePos = false;
			defaultIO.ConfigNavCaptureKeyboard = true;
			defaultIO.ConfigNavEscapeClearFocusItem = true;
			defaultIO.ConfigMemoryCompactTimer = -1.0f; // Disabled for performance
			defaultIO.ConfigDebugHighlightIdConflicts = false; // Disabled for performance
			defaultIO.ConfigDockingWithShift = false;
			defaultIO.ConfigDockingAlwaysTabBar = false;
			defaultIO.ConfigDockingTransparentPayload = false;
			defaultIO.ConfigViewportsNoAutoMerge = false;
			defaultIO.ConfigViewportsNoTaskBarIcon = false;
			defaultIO.ConfigViewportsNoDecoration = false;
			defaultIO.ConfigViewportsNoDefaultParent = false;
			defaultIO.ConfigMacOSXBehaviors = false;
			defaultIO.ConfigInputTextCursorBlink = true;
			defaultIO.ConfigInputTextEnterKeepActive = false;

			// Ensure the defaults directory exists
			std::filesystem::create_directories(dataPath + "/defaults");

			// Save defaults to correct path
			std::string defaultPath = dataPath + "/defaults/imgui_render_defaults.json";
			SaveToFile(defaultPath, defaultIO);
			std::cout << "[ImGuiSettingsUtil] Created default settings at: " << defaultPath << std::endl;
		}
	};

} // namespace Utils
