#include "ZepView.hpp"
#include <algorithm>
#include <iostream>
#include "Events.hpp"

namespace GUI {

	ZepView::ZepView(ECS::EntityManager& entityMgr)
		: BaseView(entityMgr), pythonEntity(0), showVirtualEnvSettings(false) {
		viewName = "ZepView";
		textEditor = std::make_unique<Utils::ZepTextEditor>();
		venvPathBuffer[0] = '\0';
	}

	void ZepView::Init() {
		if (!textEditor->Initialize()) {
			std::cerr << "Failed to initialize ZepTextEditor" << std::endl;
		}

		// Load default Python script from file paths
		LoadDefaultPythonScript();
	}

	void ZepView::Update(const float deltaT) {
		// Only check for Python execution results if we have a Python file loaded
		if (IsPythonFile() && pythonEntity != 0 && mgr.HasComponent<ECS::PythonComponent>(pythonEntity)) {
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

	void ZepView::Render() {

		ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);

		if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen, ImGuiWindowFlags_MenuBar)) {
			if (!windowOpen) {
				ANI::Events::Ref().RequestRemoveView(GetID(), viewName);
			}

			// Check if the Zep editor should capture input
			bool zepShouldCaptureInput = ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive();

			// Menu bar
			if (ImGui::BeginMenuBar()) {
				RenderFileMenu();
				RenderEditMenu();

				// Only show Python menu if a Python file is loaded
				if (IsPythonFile()) {
					RenderPythonMenu();
				}

				ImGui::EndMenuBar();
			}

			// Virtual Environment Settings Dialog (only if Python file is loaded)
			if (IsPythonFile() && showVirtualEnvSettings) {
				RenderVirtualEnvDialog();
			}

			// Handle file dialogs
			HandleFileDialogs();

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

	std::string ZepView::GetText() const {
		return textEditor->GetText();
	}

	void ZepView::SetText(const std::string& text) {
		textEditor->SetText(text);
	}

	bool ZepView::LoadFile(const std::string& filePath) {
		bool success = textEditor->LoadFile(filePath);
		if (success) {
			currentFilePath = filePath;

			// If this is a Python file and we don't have a Python entity yet, create one
			if (IsPythonFile() && pythonEntity == 0) {
				InitializePythonEntity();
			}
		}
		return success;
	}

	bool ZepView::SaveFile(const std::string& filePath) {
		bool success = textEditor->SaveFile(filePath);
		if (success) {
			currentFilePath = filePath;
		}
		return success;
	}

	bool ZepView::IsPythonFile() const {
		if (currentFilePath.empty()) {
			return false;
		}

		std::filesystem::path path(currentFilePath);
		std::string extension = path.extension().string();
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
		return extension == ".py";
	}

	void ZepView::InitializePythonEntity() {
		// Register PythonSystem if not already registered
		if (!mgr.GetSystem<ECS::PythonSystem>()) {
			mgr.RegisterSystem<ECS::PythonSystem>();
			std::cout << "PythonSystem registered" << std::endl;
		}

		// Create Python entity for script execution
		pythonEntity = mgr.AddNewEntity();
		mgr.AddComponent<ECS::PythonComponent>(pythonEntity);
		std::cout << "Python entity created for script execution" << std::endl;
	}

	void ZepView::RenderFileMenu() {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New", "Ctrl+N")) {
				SetText("");
				currentFilePath.clear();
			}
			if (ImGui::MenuItem("Open", "Ctrl+O")) {
				OpenFileDialog();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Save", "Ctrl+S")) {
				if (currentFilePath.empty()) {
					SaveAsFileDialog();
				}
				else {
					SaveFile(currentFilePath);
				}
			}
			if (ImGui::MenuItem("Save As", "Ctrl+Shift+S")) {
				SaveAsFileDialog();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Load Default Python Script")) {
				LoadDefaultPythonScript();
			}
			ImGui::EndMenu();
		}
	}

	void ZepView::RenderEditMenu() {
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
	}

	void ZepView::RenderPythonMenu() {
		if (ImGui::BeginMenu("Python")) {
			if (ImGui::MenuItem("Execute Script", "F5")) {
				ExecuteCurrentScript();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Clear Output")) {
				ClearPythonOutput();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Virtual Environment Settings")) {
				showVirtualEnvSettings = true;
			}

			// Show current virtual environment status in menu
			ImGui::Separator();
			if (pythonEntity != 0 && mgr.HasComponent<ECS::PythonComponent>(pythonEntity)) {
				auto& pythonComp = mgr.GetComponent<ECS::PythonComponent>(pythonEntity);

				if (pythonComp.useVirtualEnv) {
					std::string venvName = pythonComp.virtualEnvPath.empty() ?
						"Default" : std::filesystem::path(pythonComp.virtualEnvPath).filename().string();
					ImGui::MenuItem(("Virtual Env: " + venvName).c_str(), nullptr, false, false);
				}
				else {
					ImGui::MenuItem("Using: System Python", nullptr, false, false);
				}
			}

			ImGui::EndMenu();
		}
	}

	void ZepView::RenderVirtualEnvDialog() {
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

	void ZepView::LoadDefaultPythonScript() {
		// Construct path to default Python script using FilePaths
		std::string defaultScriptPath = Utils::FilePaths::defaultProjectPath + "/scripts/default_script.py";

		try {
			// Create scripts directory if it doesn't exist
			std::string scriptsDir = Utils::FilePaths::defaultProjectPath + "/scripts";
			std::filesystem::create_directories(scriptsDir);

			if (std::filesystem::exists(defaultScriptPath)) {
				if (LoadFile(defaultScriptPath)) {
					std::cout << "Default Python script loaded from: " << defaultScriptPath << std::endl;
					return;
				}
			}
			else {
				// Create the default script file if it doesn't exist
				CreateDefaultPythonScript(defaultScriptPath);
				if (LoadFile(defaultScriptPath)) {
					std::cout << "Created and loaded default Python script: " << defaultScriptPath << std::endl;
					return;
				}
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Failed to load default Python script: " << e.what() << std::endl;
		}

		// Fallback: Set default Python content directly
		SetDefaultPythonContent();
		currentFilePath = defaultScriptPath; // Set as if we loaded the file

		// Initialize Python entity since we have Python content
		if (pythonEntity == 0) {
			InitializePythonEntity();
		}

		std::cout << "Loaded default Python script content" << std::endl;
	}

	void ZepView::CreateDefaultPythonScript(const std::string& filePath) {
		try {
			std::ofstream file(filePath);
			if (file.is_open()) {
				file << GetDefaultPythonContent();
				file.close();
				std::cout << "Created default Python script at: " << filePath << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Failed to create default Python script: " << e.what() << std::endl;
		}
	}

	void ZepView::SetDefaultPythonContent() {
		SetText(GetDefaultPythonContent());
	}

	std::string ZepView::GetDefaultPythonContent() const {
		return R"(# AniStudio Default Python Script
# This script demonstrates Python integration with AniStudio

import math
import random
import sys
from datetime import datetime

def main():
    """Main function demonstrating AniStudio Python integration"""
    
    print("=" * 50)
    print("Welcome to AniStudio Python Environment!")
    print("=" * 50)
    
    # System Information
    print(f"\nPython Version: {sys.version}")
    print(f"Executable: {sys.executable}")
    print(f"Current Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    
    # Math Demonstrations
    print(f"\nMathematical Constants:")
    print(f"   Pi: {math.pi:.6f}")
    print(f"   e (Euler): {math.e:.6f}")
    
    # Random Number Generation
    print(f"\nRandom Number Generation:")
    random_numbers = [random.randint(1, 100) for _ in range(5)]
    print(f"   Random integers (1-100): {random_numbers}")
    print(f"   Average: {sum(random_numbers) / len(random_numbers):.2f}")
    
    # String Manipulations
    print(f"\nString Manipulations:")
    message = "AniStudio: Media Creation Made Easy!"
    print(f"   Original: {message}")
    print(f"   Reversed: {message[::-1]}")
    print(f"   Word Count: {len(message.split())} words")
    
    # Media Creation Workflow Example
    print(f"\nMedia Creation Workflow Example:")
    project_settings = {
        "resolution": "1920x1080",
        "framerate": 30,
        "format": "MP4",
        "quality": "High"
    }
    
    print("   Project Settings:")
    for key, value in project_settings.items():
        print(f"     {key.capitalize()}: {value}")
    
    print(f"\nScript execution completed successfully!")
    print("=" * 50)

if __name__ == "__main__":
    main()
    print("\nReady for your AniStudio projects!")
)";
	}

	void ZepView::ExecuteCurrentScript() {
		if (!IsPythonFile()) {
			std::cout << "Current file is not a Python script!" << std::endl;
			return;
		}

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

	void ZepView::ClearPythonOutput() {
		if (pythonEntity != 0 && mgr.HasComponent<ECS::PythonComponent>(pythonEntity)) {
			auto& pythonComp = mgr.GetComponent<ECS::PythonComponent>(pythonEntity);
			pythonComp.output.clear();
			pythonComp.error.clear();
			lastDisplayedOutput.clear();
			lastDisplayedError.clear();
			std::cout << "Python output cleared" << std::endl;
		}
	}

	void ZepView::OpenFileDialog() {
		IGFD::FileDialogConfig config;
		config.path = Utils::FilePaths::defaultProjectPath;
		ImGuiFileDialog::Instance()->OpenDialog("OpenCodeFile", "Choose Code File", ".py,.cpp,.hpp,.h,.c,.txt,.md", config);
	}

	void ZepView::SaveFileDialog() {
		IGFD::FileDialogConfig config;
		config.path = Utils::FilePaths::defaultProjectPath;

		// Set default extension based on current file
		std::string filter = ".txt";
		if (IsPythonFile()) {
			filter = ".py";
		}
		else if (!currentFilePath.empty()) {
			std::filesystem::path path(currentFilePath);
			std::string ext = path.extension().string();
			if (!ext.empty()) {
				filter = ext;
			}
		}

		ImGuiFileDialog::Instance()->OpenDialog("SaveCodeFile", "Save Code File", filter.c_str(), config);
	}

	void ZepView::SaveAsFileDialog() {
		IGFD::FileDialogConfig config;
		config.path = Utils::FilePaths::defaultProjectPath;
		ImGuiFileDialog::Instance()->OpenDialog("SaveAsCodeFile", "Save Code File As", ".py,.cpp,.hpp,.h,.c,.txt,.md", config);
	}

	void ZepView::HandleFileDialogs() {
		// Handle open file dialog
		if (ImGuiFileDialog::Instance()->Display("OpenCodeFile", 32, ImVec2(700, 400))) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
				LoadFile(filePath);
			}
			ImGuiFileDialog::Instance()->Close();
		}

		// Handle save file dialog
		if (ImGuiFileDialog::Instance()->Display("SaveCodeFile", 32, ImVec2(700, 400))) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
				SaveFile(filePath);
			}
			ImGuiFileDialog::Instance()->Close();
		}

		// Handle save as file dialog
		if (ImGuiFileDialog::Instance()->Display("SaveAsCodeFile", 32, ImVec2(700, 400))) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
				SaveFile(filePath);
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
	}

}