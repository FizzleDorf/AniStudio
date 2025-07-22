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

#pragma once

#include <imgui.h>
#include <string>
#include <filesystem>
#include <iostream>

namespace Utils {

	class ImGuiStateUtils {
	public:
		// Load default ImGui layout from defaults directory
		static void LoadDefaultImGuiLayout() {
			std::cout << "[ImGuiStateUtils] Loading default ImGui layout..." << std::endl;

			// Load the default imgui.ini file by changing ImGui's IniFilename
			std::string defaultLayoutPath = "../data/defaults/imgui.ini";
			if (std::filesystem::exists(defaultLayoutPath)) {
				ImGuiIO& io = ImGui::GetIO();

				// Change the ini filename - ImGui will automatically load it
				static std::string absolutePath = std::filesystem::absolute(defaultLayoutPath).string();
				io.IniFilename = absolutePath.c_str();

				// Force ImGui to reload the ini file
				ImGui::LoadIniSettingsFromDisk(absolutePath.c_str());

				std::cout << "[ImGuiStateUtils] Loaded default ImGui layout from: " << absolutePath << std::endl;
			}
			else {
				std::cout << "[ImGuiStateUtils] No default ImGui layout found at: " << defaultLayoutPath << std::endl;
			}
		}

		// Load project-specific ImGui layout
		static void LoadProjectImGuiLayout(const std::string& projectPath) {
			std::cout << "[ImGuiStateUtils] Loading project ImGui layout..." << std::endl;

			std::string projectLayoutPath = projectPath + "/data/imgui.ini";
			if (std::filesystem::exists(projectLayoutPath)) {
				ImGuiIO& io = ImGui::GetIO();

				// Change the ini filename to project-specific path
				static std::string absolutePath = std::filesystem::absolute(projectLayoutPath).string();
				io.IniFilename = absolutePath.c_str();

				// Force ImGui to reload the ini file
				ImGui::LoadIniSettingsFromDisk(absolutePath.c_str());

				std::cout << "[ImGuiStateUtils] Loaded project ImGui layout from: " << absolutePath << std::endl;
			}
			else {
				std::cout << "[ImGuiStateUtils] No project ImGui layout found, using default" << std::endl;
				LoadDefaultImGuiLayout();
			}
		}

		// Save project-specific ImGui layout
		static void SaveProjectImGuiLayout(const std::string& projectPath) {
			std::cout << "[ImGuiStateUtils] Saving project ImGui layout..." << std::endl;

			// Ensure project data directory exists
			std::string dataPath = projectPath + "/data";
			std::filesystem::create_directories(dataPath);

			std::string projectLayoutPath = dataPath + "/imgui.ini";
			std::string absolutePath = std::filesystem::absolute(projectLayoutPath).string();

			// Change ImGui's ini filename to the project path
			ImGuiIO& io = ImGui::GetIO();
			static std::string savedPath = absolutePath;
			io.IniFilename = savedPath.c_str();

			// ImGui will automatically save to this location when it shuts down
			// But we can force a save now too
			ImGui::SaveIniSettingsToDisk(absolutePath.c_str());

			std::cout << "[ImGuiStateUtils] Project ImGui layout will be saved to: " << absolutePath << std::endl;
		}

		// Setup project UI when a project is loaded
		static void OnProjectLoaded(const std::string& projectPath) {
			std::cout << "[ImGuiStateUtils] Project loaded - setting up UI..." << std::endl;

			// Load project-specific ImGui layout
			LoadProjectImGuiLayout(projectPath);

			std::cout << "[ImGuiStateUtils] Project UI setup complete" << std::endl;
		}

		// Setup project UI when a new project is created
		static void OnProjectCreated(const std::string& projectPath) {
			std::cout << "[ImGuiStateUtils] New project created - setting up UI..." << std::endl;

			// Create default project layout files by copying from default
			std::string defaultLayoutPath = "../data/defaults/imgui.ini";
			std::string projectLayoutPath = projectPath + "/data/imgui.ini";

			try {
				std::filesystem::create_directories(projectPath + "/data");

				if (std::filesystem::exists(defaultLayoutPath)) {
					std::filesystem::copy_file(defaultLayoutPath, projectLayoutPath,
						std::filesystem::copy_options::overwrite_existing);
					std::cout << "[ImGuiStateUtils] Copied default layout to project" << std::endl;
				}

				// Load the project layout
				LoadProjectImGuiLayout(projectPath);
			}
			catch (const std::exception& e) {
				std::cerr << "[ImGuiStateUtils] Failed to set up project layout: " << e.what() << std::endl;
			}

			std::cout << "[ImGuiStateUtils] New project UI setup complete" << std::endl;
		}

		// Cleanup UI when a project is closed
		static void OnProjectClosed() {
			std::cout << "[ImGuiStateUtils] Project closed - reverting to startup..." << std::endl;

			// Load default layout for startup
			LoadDefaultImGuiLayout();

			std::cout << "[ImGuiStateUtils] Reverted to startup UI" << std::endl;
		}
	};

} // namespace Utils