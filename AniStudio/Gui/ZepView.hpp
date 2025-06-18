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

#include "Base/BaseView.hpp"
#include "EntityManager.hpp"
#include "ZepUtils.hpp"
#include "PythonComponent.hpp"
#include "PythonSystem.hpp"
#include "ImGuiFileDialog.h"
#include <memory>
#include <string>
#include <filesystem>
#include <fstream>

namespace GUI {
	class ZepView : public BaseView {
	public:
		ZepView(ECS::EntityManager& entityMgr) : BaseView(entityMgr) {
			viewName = "Python Editor";
			textEditor = std::make_unique<Utils::ZepTextEditor>();
		}

		void Init() override {
			if (!textEditor->Initialize()) {
				std::cerr << "Failed to initialize ZepTextEditor" << std::endl;
			}

			// Register PythonSystem if not already registered
			if (!mgr.GetSystem<ECS::PythonSystem>()) {
				mgr.RegisterSystem<ECS::PythonSystem>();
				std::cout << "PythonSystem registered" << std::endl;
			}

			// Create Python entity for script execution
			pythonEntity = mgr.AddNewEntity();
			mgr.AddComponent<ECS::PythonComponent>(pythonEntity);

			// Load default script from file
			LoadDefaultScript();
		}

		void Update(const float deltaT) override {
			// Check for Python execution results
			if (pythonEntity != 0 && mgr.HasComponent<ECS::PythonComponent>(pythonEntity)) {
				auto& pythonComp = mgr.GetComponent<ECS::PythonComponent>(pythonEntity);

				// Display any new output or errors in console
				if (!pythonComp.output.empty() && pythonComp.output != lastDisplayedOutput) {
					std::cout << "Python Output: " << pythonComp.output << std::endl;
					lastDisplayedOutput = pythonComp.output;
				}

				if (!pythonComp.error.empty() && pythonComp.error != lastDisplayedError) {
					std::cerr << "Python Error: " << pythonComp.error << std::endl;
					lastDisplayedError = pythonComp.error;
				}
			}
		}

		void Render() override {
			ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);

			if (ImGui::Begin(viewName.c_str(), nullptr, ImGuiWindowFlags_MenuBar)) {

				// Check if the Zep editor should capture input
				bool zepShouldCaptureInput = ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive();

				// Menu bar
				if (ImGui::BeginMenuBar()) {
					if (ImGui::BeginMenu("File")) {
						if (ImGui::MenuItem("New", "Ctrl+N")) {
							SetText("");
						}
						if (ImGui::MenuItem("Open", "Ctrl+O")) {
							OpenFileDialog();
						}
						ImGui::Separator();
						if (ImGui::MenuItem("Save", "Ctrl+S")) {
							SaveFileDialog();
						}
						if (ImGui::MenuItem("Save As", "Ctrl+Shift+S")) {
							SaveAsFileDialog();
						}
						ImGui::EndMenu();
					}

					if (ImGui::BeginMenu("Edit")) {
						if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
							// TODO: Implement undo
						}
						if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
							// TODO: Implement redo
						}
						ImGui::Separator();
						if (ImGui::MenuItem("Clear All")) {
							SetText("");
						}
						ImGui::EndMenu();
					}

					if (ImGui::BeginMenu("Python")) {
						if (ImGui::MenuItem("Execute Script", "F5")) {
							ExecuteCurrentScript();
						}
						ImGui::Separator();
						if (ImGui::MenuItem("Load Default Script")) {
							LoadDefaultScript();
						}
						if (ImGui::MenuItem("Virtual Environment Settings")) {
							showVirtualEnvSettings = true;
						}
						ImGui::EndMenu();
					}
					ImGui::EndMenuBar();
				}

				// Top toolbar buttons - moved above the editor
				RenderToolbar();

				// Virtual Environment Settings Dialog
				if (showVirtualEnvSettings) {
					RenderVirtualEnvDialog();
				}

				// Handle file dialogs
				HandleFileDialogs();

				// Separator between toolbar and editor
				ImGui::Separator();

				// Get remaining space for editor
				ImVec2 contentSize = ImGui::GetContentRegionAvail();
				ImVec2 editorPos = ImGui::GetCursorScreenPos();

				// Ensure minimum size
				if (contentSize.x < 200) contentSize.x = 200;
				if (contentSize.y < 100) contentSize.y = 100;

				// Disable ImGui keyboard navigation when editor area is focused
				if (zepShouldCaptureInput) {
					ImGui::GetIO().WantCaptureKeyboard = false;
					ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
				}

				// Create an invisible button to capture focus for the editor area
				ImGui::PushID("ZepEditorArea");
				ImGui::InvisibleButton("##ZepEditorFocus", contentSize);
				bool editorAreaHovered = ImGui::IsItemHovered();
				bool editorAreaClicked = ImGui::IsItemClicked();
				ImGui::PopID();

				// Reset cursor position to render editor over the invisible button
				ImGui::SetCursorScreenPos(editorPos);

				// Manage input focus
				if (editorAreaClicked || (editorAreaHovered && zepShouldCaptureInput)) {
					// Disable ImGui input capture when Zep should have focus
					ImGui::GetIO().WantCaptureKeyboard = false;
					ImGui::GetIO().WantCaptureMouse = false;
				}

				// Render the Zep editor in the remaining space
				textEditor->Render(editorPos, contentSize, false);

				// Restore ImGui input capture if we're not in the editor area
				if (!editorAreaHovered || !zepShouldCaptureInput) {
					ImGui::GetIO().WantCaptureKeyboard = true;
					ImGui::GetIO().WantCaptureMouse = true;
					ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
				}
			}
			ImGui::End();
		}

		// Helper methods for text operations
		std::string GetText() const {
			return textEditor->GetText();
		}

		void SetText(const std::string& text) {
			textEditor->SetText(text);
		}

		bool LoadFile(const std::string& filePath) {
			return textEditor->LoadFile(filePath);
		}

		bool SaveFile(const std::string& filePath) {
			return textEditor->SaveFile(filePath);
		}

	private:
		std::unique_ptr<Utils::ZepTextEditor> textEditor;
		ECS::EntityID pythonEntity = 0;
		std::string lastDisplayedOutput;
		std::string lastDisplayedError;
		std::string currentFilePath;
		bool showVirtualEnvSettings = false;

		// File dialog state
		char filePathBuffer[512] = "";
		char venvPathBuffer[512] = "";

		void RenderToolbar() {
			// First row of buttons
			if (ImGui::Button("New")) {
				SetText("");
				currentFilePath.clear();
			}
			ImGui::SameLine();

			if (ImGui::Button("Open")) {
				OpenFileDialog();
			}
			ImGui::SameLine();

			if (ImGui::Button("Save")) {
				if (currentFilePath.empty()) {
					SaveAsFileDialog();
				}
				else {
					SaveFile(currentFilePath);
				}
			}
			ImGui::SameLine();

			if (ImGui::Button("Save As")) {
				SaveAsFileDialog();
			}
			ImGui::SameLine();

			// Add some spacing
			ImGui::SameLine(0, 20);

			if (ImGui::Button("Load Default")) {
				LoadDefaultScript();
			}
			ImGui::SameLine();

			if (ImGui::Button("Execute")) {
				ExecuteCurrentScript();
			}
			ImGui::SameLine();

			if (ImGui::Button("Clear")) {
				SetText("");
			}
			ImGui::SameLine();

			if (ImGui::Button("Clear Results")) {
				if (pythonEntity != 0 && mgr.HasComponent<ECS::PythonComponent>(pythonEntity)) {
					auto& pythonComp = mgr.GetComponent<ECS::PythonComponent>(pythonEntity);
					pythonComp.output.clear();
					pythonComp.error.clear();
					lastDisplayedOutput.clear();
					lastDisplayedError.clear();
				}
			}

			// Second row - Virtual environment settings
			ImGui::Text("Virtual Environment:");
			ImGui::SameLine();

			if (ImGui::Button("Settings")) {
				showVirtualEnvSettings = true;
			}
			ImGui::SameLine();

			// Show current venv status
			if (pythonEntity != 0 && mgr.HasComponent<ECS::PythonComponent>(pythonEntity)) {
				auto& pythonComp = mgr.GetComponent<ECS::PythonComponent>(pythonEntity);

				if (pythonComp.useVirtualEnv) {
					ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f),
						"Active: %s",
						pythonComp.virtualEnvPath.empty() ? Utils::FilePaths::virtualEnvPath :
						std::filesystem::path(pythonComp.virtualEnvPath).filename().string().c_str());
				}
				else {
					ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "System Python");
				}
			}
		}

		void RenderVirtualEnvDialog() {
			ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Virtual Environment Settings", &showVirtualEnvSettings)) {

				if (pythonEntity != 0 && mgr.HasComponent<ECS::PythonComponent>(pythonEntity)) {
					auto& pythonComp = mgr.GetComponent<ECS::PythonComponent>(pythonEntity);

					ImGui::Text("Python Virtual Environment Configuration");
					ImGui::Separator();

					// Use virtual environment checkbox
					ImGui::Checkbox("Use Virtual Environment", &pythonComp.useVirtualEnv);

					if (pythonComp.useVirtualEnv) {
						ImGui::Text("Virtual Environment Path:");

						// Initialize buffer if empty
						if (strlen(venvPathBuffer) == 0) {
							strncpy(venvPathBuffer, pythonComp.virtualEnvPath.c_str(), sizeof(venvPathBuffer) - 1);
						}

						if (ImGui::InputText("##VenvPath", venvPathBuffer, sizeof(venvPathBuffer))) {
							pythonComp.virtualEnvPath = venvPathBuffer;
							pythonComp.UpdateVirtualEnvPaths();
						}

						ImGui::SameLine();
						if (ImGui::Button("Browse##Venv")) {
							IGFD::FileDialogConfig config;
							config.path = Utils::FilePaths::defaultProjectPath;
							ImGuiFileDialog::Instance()->OpenDialog("ChooseVenvDialog", "Choose Virtual Environment Directory", nullptr, config);
						}

						if (ImGui::Button("Reset to Default")) {
							pythonComp.virtualEnvPath = Utils::FilePaths::virtualEnvPath;
							pythonComp.UpdateVirtualEnvPaths();
							strncpy(venvPathBuffer, pythonComp.virtualEnvPath.c_str(), sizeof(venvPathBuffer) - 1);
						}

						ImGui::Text("Python Executable: %s", pythonComp.pythonExecutable.c_str());
						ImGui::Text("Site Packages: %s", pythonComp.sitePackagesPath.c_str());

						// Check if virtual environment exists
						if (std::filesystem::exists(pythonComp.pythonExecutable)) {
							ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Virtual environment found");
						}
						else {
							ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Virtual environment not found");
							ImGui::Text("Create virtual environment with: python -m venv %s", pythonComp.virtualEnvPath.c_str());
						}
					}
					else {
						ImGui::Text("Using system Python interpreter");
					}

					ImGui::Separator();
					if (ImGui::Button("Close")) {
						showVirtualEnvSettings = false;
					}
				}
			}
			ImGui::End();
		}

		void LoadDefaultScript() {
			std::string defaultScriptPath = Utils::FilePaths::defaultScriptsPath + "/default_script.py";

			try {
				if (std::filesystem::exists(defaultScriptPath)) {
					std::ifstream file(defaultScriptPath);
					if (file.is_open()) {
						std::string content((std::istreambuf_iterator<char>(file)),
							std::istreambuf_iterator<char>());
						SetText(content);
						std::cout << "Default script loaded from: " << defaultScriptPath << std::endl;
						return;
					}
				}
			}
			catch (const std::exception& e) {
				std::cerr << "Failed to load default script: " << e.what() << std::endl;
			}

			// If file loading fails, use empty string
			SetText("");
			std::cout << "Default script file not found, starting with empty editor" << std::endl;
		}

		void ExecuteCurrentScript() {
			std::string scriptText = GetText();
			if (!scriptText.empty() && pythonEntity != 0) {
				auto& pythonComp = mgr.GetComponent<ECS::PythonComponent>(pythonEntity);

				// Clear previous results
				pythonComp.output.clear();
				pythonComp.error.clear();
				lastDisplayedOutput.clear();
				lastDisplayedError.clear();

				// Set new script and execute
				pythonComp.script = scriptText;
				pythonComp.isFile = false;
				pythonComp.execute = true;

				std::cout << "Executing Python script through PythonSystem..." << std::endl;
			}
			else {
				std::cout << "No script to execute or Python entity not initialized!" << std::endl;
			}
		}

		void OpenFileDialog() {
			IGFD::FileDialogConfig config;
			config.path = Utils::FilePaths::defaultScriptsPath;
			ImGuiFileDialog::Instance()->OpenDialog("OpenPythonFile", "Choose Python File", ".py", config);
		}

		void SaveFileDialog() {
			IGFD::FileDialogConfig config;
			config.path = Utils::FilePaths::defaultScriptsPath;
			ImGuiFileDialog::Instance()->OpenDialog("SavePythonFile", "Save Python File", ".py", config);
		}

		void SaveAsFileDialog() {
			IGFD::FileDialogConfig config;
			config.path = Utils::FilePaths::defaultScriptsPath;
			ImGuiFileDialog::Instance()->OpenDialog("SaveAsPythonFile", "Save Python File As", ".py", config);
		}

		void HandleFileDialogs() {
			// Handle open file dialog
			if (ImGuiFileDialog::Instance()->Display("OpenPythonFile", 32, ImVec2(700, 400))) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
					if (LoadFile(filePath)) {
						currentFilePath = filePath;
					}
				}
				ImGuiFileDialog::Instance()->Close();
			}

			// Handle save file dialog
			if (ImGuiFileDialog::Instance()->Display("SavePythonFile", 32, ImVec2(700, 400))) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
					if (SaveFile(filePath)) {
						currentFilePath = filePath;
					}
				}
				ImGuiFileDialog::Instance()->Close();
			}

			// Handle save as file dialog
			if (ImGuiFileDialog::Instance()->Display("SaveAsPythonFile", 32, ImVec2(700, 400))) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
					if (SaveFile(filePath)) {
						currentFilePath = filePath;
					}
				}
				ImGuiFileDialog::Instance()->Close();
			}

			// Handle virtual environment directory selection
			if (ImGuiFileDialog::Instance()->Display("ChooseVenvDialog", 32, ImVec2(700, 400))) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string venvPath = ImGuiFileDialog::Instance()->GetCurrentPath();
					if (pythonEntity != 0 && mgr.HasComponent<ECS::PythonComponent>(pythonEntity)) {
						auto& pythonComp = mgr.GetComponent<ECS::PythonComponent>(pythonEntity);
						pythonComp.virtualEnvPath = venvPath;
						pythonComp.UpdateVirtualEnvPaths();
						strncpy(venvPathBuffer, venvPath.c_str(), sizeof(venvPathBuffer) - 1);
					}
				}
				ImGuiFileDialog::Instance()->Close();
			}
		}	};
}