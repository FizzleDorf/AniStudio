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
#include <zep/editor.h>
#include <zep/imgui/editor_imgui.h>
#include <zep/mode_standard.h>
#include <zep/mode_vim.h>
#include <zep/theme.h>
#include <memory>
#include <string>
#include <filesystem>
#include <imgui.h>
#include <iostream>
#include <regex>
#include <algorithm>
#include <vector>
#include <fstream>
#include <ImGuiFileDialog.h>

namespace Utils {

	class ZepFocusTracker {
	private:
		static uintptr_t& GetFocusedInstance() {
			static uintptr_t focusedInstance = 0;
			return focusedInstance;
		}

	public:
		static void SetFocused(uintptr_t id) {
			GetFocusedInstance() = id;
		}

		static bool IsFocused(uintptr_t id) {
			return GetFocusedInstance() == id;
		}

		static void ClearFocus() {
			GetFocusedInstance() = 0;
		}
	};

	class ZepTextEditor : public Zep::IZepComponent {
	public:
		ZepTextEditor() : instanceId(reinterpret_cast<uintptr_t>(this)), isInitialized(false) {
		}

		~ZepTextEditor() {
			if (editor) {
				editor->UnRegisterCallback(this);
			}
		}

		bool Initialize() {
			if (isInitialized) {
				return true;
			}

			try {
				auto rootPath = std::filesystem::current_path();

				editor = std::make_unique<Zep::ZepEditor_ImGui>(
					rootPath,
					Zep::NVec2f(1.0f, 1.0f)
					);

				// Register callback
				editor->RegisterCallback(this);

				// Initialize with default buffer from data/defaults/buffer.txt
				LoadDefaultBuffer();

				// Force standard mode by default
				editor->SetGlobalMode(Zep::ZepMode_Standard::StaticName());

				ApplyCurrentConfiguration();
				isInitialized = true;
				return true;
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in ZepTextEditor::Initialize(): " << e.what() << std::endl;
				return false;
			}
		}

		// Enhanced Render method with optional child window creation
		void Render(ImVec2 position, ImVec2 size, bool createChildWindow = false) {
			if (!editor || !isInitialized) {
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Editor not initialized");
				return;
			}

			if (createChildWindow) {
				// Schema rendering path - create child window with menu bar support
				std::string childId = "ZepEditor##" + std::to_string(instanceId);

				ImGuiWindowFlags childFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
				if (showMenuBar) {
					childFlags |= ImGuiWindowFlags_MenuBar;
				}

				ImVec2 availableSize = size;
				if (availableSize.x <= 0 || availableSize.y <= 0) {
					availableSize = ImGui::GetContentRegionAvail();
				}
				availableSize.x = std::max(100.0f, availableSize.x);
				availableSize.y = std::max(100.0f, availableSize.y);

				if (ImGui::BeginChild(childId.c_str(), availableSize, true, childFlags)) {
					RenderEditor();
				}
				ImGui::EndChild();
			}
			else {
				// ZepView path - no child window, render directly
				RenderEditor();
			}

			// Handle file dialogs
			HandleFileDialogs();
		}

		// Configuration methods
		void SetShowLineNumbers(bool show) {
			showLineNumbers = show;
			ApplyCurrentConfiguration();
		}

		void SetWordWrap(bool wrap) {
			wordWrap = wrap;
			ApplyCurrentConfiguration();
		}

		void SetReadOnly(bool readonly) {
			readOnly = readonly;
		}

		void SetShowWhitespace(bool show) {
			showWhitespace = show;
			ApplyCurrentConfiguration();
		}

		void SetSyntaxHighlighting(bool enable) {
			enableSyntaxHighlighting = enable;
			ApplyCurrentConfiguration();
		}

		void SetAutoIndent(bool enable) {
			autoIndent = enable;
		}

		void SetTheme(const std::string& theme) {
			if (currentTheme != theme) {
				currentTheme = theme;
				ApplyTheme();
			}
		}

		void SetMode(const std::string& mode) {
			if (editor && isInitialized) {
				currentMode = mode;
				try {
					if (mode == "vim") {
						editor->SetGlobalMode(Zep::ZepMode_Vim::StaticName());
					}
					else {
						editor->SetGlobalMode(Zep::ZepMode_Standard::StaticName());
					}
				}
				catch (const std::exception& e) {
					std::cerr << "Error setting editor mode to " << mode << ": " << e.what() << std::endl;
					editor->SetGlobalMode(Zep::ZepMode_Standard::StaticName());
					currentMode = "standard";
				}
			}
		}

		void SetShowMenuBar(bool show) {
			showMenuBar = show;
		}

		void SetFontSize(float size) {
			if (editor && isInitialized) {
				currentFontSize = size;

				try {
					auto& display = editor->GetDisplay();
					auto& font = display.GetFont(Zep::ZepTextType::Text);
					font.SetPixelHeight(static_cast<int>(size));

					auto& uiFont = display.GetFont(Zep::ZepTextType::UI);
					uiFont.SetPixelHeight(static_cast<int>(size));

					editor->RequestRefresh();
				}
				catch (const std::exception& e) {
					std::cerr << "Error setting font size: " << e.what() << std::endl;
				}
			}
		}

		// Set the file type for syntax highlighting
		void SetFileType(const std::string& extension) {
			if (!editor || !isInitialized) return;

			try {
				auto* buffer = editor->GetActiveBuffer();
				if (buffer) {
					currentFileType = extension;
					std::string filename = "buffer" + (extension.front() == '.' ? extension : "." + extension);
					buffer->SetFilePath(std::filesystem::path(filename));

					if (enableSyntaxHighlighting) {
						editor->RequestRefresh();
						ApplySyntaxHighlighting();
					}
				}
			}
			catch (const std::exception& e) {
				std::cerr << "Error setting file type: " << e.what() << std::endl;
			}
		}

		// Get the current file type
		std::string GetFileType() const {
			return currentFileType;
		}

		// Getters
		bool GetShowLineNumbers() const { return showLineNumbers; }
		bool GetWordWrap() const { return wordWrap; }
		bool GetReadOnly() const { return readOnly; }
		bool GetShowWhitespace() const { return showWhitespace; }
		bool GetSyntaxHighlighting() const { return enableSyntaxHighlighting; }
		bool GetAutoIndent() const { return autoIndent; }
		std::string GetTheme() const { return currentTheme; }
		std::string GetMode() const { return currentMode; }
		bool GetShowMenuBar() const { return showMenuBar; }
		float GetFontSize() const { return currentFontSize; }

		// Text methods
		std::string GetText() const {
			if (editor && isInitialized) {
				auto* buffer = editor->GetActiveBuffer();
				if (buffer) {
					return buffer->GetBufferText(buffer->Begin(), buffer->End());
				}
			}
			return "";
		}

		void SetText(const std::string& text) {
			if (editor && isInitialized) {
				auto* buffer = editor->GetActiveBuffer();
				if (buffer) {
					buffer->SetText(text);
					ApplySyntaxHighlighting();
				}
			}
		}

		// File operations
		bool LoadFile(const std::string& filePath) {
			if (editor && isInitialized) {
				try {
					auto* buffer = editor->GetActiveBuffer();
					if (buffer) {
						buffer->Load(std::filesystem::path(filePath));

						// Auto-detect file type from extension
						std::filesystem::path path(filePath);
						if (path.has_extension()) {
							currentFileType = path.extension().string();
						}

						ApplySyntaxHighlighting();
						currentFilePath = filePath;
						return true;
					}
				}
				catch (const std::exception& e) {
					std::cerr << "Error loading file: " << e.what() << std::endl;
				}
			}
			return false;
		}

		// Create a new file with the specified path
		bool CreateNewFile(const std::string& filePath) {
			if (editor && isInitialized) {
				try {
					// Clear the editor content
					auto* buffer = editor->GetActiveBuffer();
					if (buffer) {
						buffer->SetText("");

						// Set the file path and detect file type
						std::filesystem::path path(filePath);
						currentFilePath = filePath;

						if (path.has_extension()) {
							currentFileType = path.extension().string();
							SetFileType(currentFileType);
						}
						else {
							SetFileType(".txt"); // Default to plain text
						}

						// Create the directory if it doesn't exist
						std::filesystem::create_directories(path.parent_path());

						// Create the empty file
						std::ofstream file(filePath);
						if (file.is_open()) {
							file.close();
							std::cout << "Created new file: " << filePath << std::endl;
							return true;
						}
						else {
							std::cerr << "Failed to create file: " << filePath << std::endl;
							return false;
						}
					}
				}
				catch (const std::exception& e) {
					std::cerr << "Error creating new file: " << e.what() << std::endl;
				}
			}
			return false;
		}

		bool SaveFile(const std::string& filePath = "") {
			if (editor && isInitialized) {
				try {
					auto* buffer = editor->GetActiveBuffer();
					if (buffer) {
						std::string pathToUse = filePath.empty() ? currentFilePath : filePath;
						if (pathToUse.empty()) {
							// No path specified, need to open save dialog
							OpenSaveDialog();
							return false; // Will save when dialog completes
						}

						buffer->SetFilePath(std::filesystem::path(pathToUse));
						int64_t size;
						bool result = buffer->Save(size);
						if (result) {
							currentFilePath = pathToUse;
						}
						return result;
					}
				}
				catch (const std::exception& e) {
					std::cerr << "Error saving file: " << e.what() << std::endl;
				}
			}
			return false;
		}

		// Create a new file
		void NewFile() {
			IGFD::FileDialogConfig config;
			config.path = "data/defaults";
			ImGuiFileDialog::Instance()->OpenDialog("NewTextFileDialog", "Create New File",
				".txt,.cpp,.hpp,.h,.c,.cc,.cxx,.py,.js,.ts,.java,.cs,.rs,.go,.json,.xml,.html,.css,.md,.yml,.yaml,.toml,.vert,.frag,.hlsl,.bat,.sh,.log,.ini,.cfg,.conf",
				config);
		}

		// Open file dialog
		void OpenLoadDialog() {
			IGFD::FileDialogConfig config;
			config.path = "data/defaults";
			ImGuiFileDialog::Instance()->OpenDialog("LoadTextFileDialog", "Load Text File",
				".*,.txt,.cpp,.hpp,.h,.c,.cc,.cxx,.py,.js,.ts,.java,.cs,.rs,.go,.json,.xml,.html,.css,.md,.yml,.yaml,.toml,.vert,.frag,.hlsl,.bat,.sh,.log,.ini,.cfg,.conf",
				config);
		}

		// Open save dialog
		void OpenSaveDialog() {
			IGFD::FileDialogConfig config;
			config.path = "data/defaults";
			ImGuiFileDialog::Instance()->OpenDialog("SaveTextFileDialog", "Save Text File",
				".txt,.cpp,.hpp,.h,.c,.cc,.cxx,.py,.js,.ts,.java,.cs,.rs,.go,.json,.xml,.html,.css,.md,.yml,.yaml,.toml,.vert,.frag,.hlsl,.bat,.sh,.log,.ini,.cfg,.conf",
				config);
		}

		// Search functionality
		void SetSearchTerm(const std::string& term) {
			currentSearchTerm = term;
			PerformSearch();
		}

		std::string GetSearchTerm() const { return currentSearchTerm; }

		void SetSearchCaseSensitive(bool caseSensitive) {
			searchCaseSensitive = caseSensitive;
			if (!currentSearchTerm.empty()) {
				PerformSearch();
			}
		}

		bool GetSearchCaseSensitive() const { return searchCaseSensitive; }

		void SetSearchRegex(bool regex) {
			searchRegex = regex;
			if (!currentSearchTerm.empty()) {
				PerformSearch();
			}
		}

		bool GetSearchRegex() const { return searchRegex; }

		void ShowSearchBox(bool show) {
			showSearchBox = show;
			if (show) {
				searchBoxJustOpened = true;
			}
			if (!show) {
				ClearSearch();
			}
		}

		bool IsSearchBoxVisible() const { return showSearchBox; }

		void ToggleSearchBox() {
			ShowSearchBox(!showSearchBox);
		}

		bool FindNext() {
			if (searchResults.empty()) return false;

			currentSearchIndex++;
			if (currentSearchIndex >= static_cast<int>(searchResults.size())) {
				currentSearchIndex = 0;
			}

			if (currentSearchIndex >= 0 && currentSearchIndex < static_cast<int>(searchResults.size())) {
				JumpToSearchResult(currentSearchIndex);
				return true;
			}
			return false;
		}

		bool FindPrevious() {
			if (searchResults.empty()) return false;

			currentSearchIndex--;
			if (currentSearchIndex < 0) {
				currentSearchIndex = static_cast<int>(searchResults.size()) - 1;
			}

			if (currentSearchIndex >= 0 && currentSearchIndex < static_cast<int>(searchResults.size())) {
				JumpToSearchResult(currentSearchIndex);
				return true;
			}
			return false;
		}

		int GetSearchResultCount() const {
			return static_cast<int>(searchResults.size());
		}

		int GetCurrentSearchIndex() const {
			return currentSearchIndex;
		}

		// Required by IZepComponent interface
		Zep::ZepEditor& GetEditor() const override {
			return *editor;
		}

		uintptr_t GetInstanceId() const { return instanceId; }

		// Handle clipboard messages
		void Notify(std::shared_ptr<Zep::ZepMessage> message) override {
			if (message->messageId == Zep::Msg::GetClipBoard) {
				const char* clipboardText = ImGui::GetClipboardText();
				if (clipboardText) {
					message->str = std::string(clipboardText);
					message->handled = true;
				}
			}
			else if (message->messageId == Zep::Msg::SetClipBoard) {
				ImGui::SetClipboardText(message->str.c_str());
				message->handled = true;
			}
		}

	private:
		// Load default buffer from data/defaults/buffer.txt
		void LoadDefaultBuffer() {
			std::filesystem::path defaultPath = std::filesystem::current_path() / ".." / "data" / "defaults" / "buffer.txt";

			try {
				// Ensure the directory exists
				std::filesystem::create_directories(defaultPath.parent_path());

				std::string defaultText;
				if (std::filesystem::exists(defaultPath)) {
					// Load from existing file
					std::ifstream file(defaultPath);
					if (file.is_open()) {
						defaultText = std::string((std::istreambuf_iterator<char>(file)),
							std::istreambuf_iterator<char>());
						file.close();
					}
				}
				else {
					// Create default content
					defaultText = "// Welcome to AniStudio Text Editor\n";

					// Save the default content
					std::ofstream file(defaultPath);
					if (file.is_open()) {
						file << defaultText;
						file.close();
					}
				}

				// Initialize with the default text
				editor->InitWithText("buffer.txt", defaultText);
				SetFileType(".txt"); // Default file type
			}
			catch (const std::exception& e) {
				std::cerr << "Error loading default buffer: " << e.what() << std::endl;
				// Fallback to empty buffer
				editor->InitWithText("buffer.txt", "");
				SetFileType(".txt");
			}
		}

		// Handle file dialogs
		void HandleFileDialogs() {
			// Handle new file dialog
			if (ImGuiFileDialog::Instance()->Display("NewTextFileDialog", 32, ImVec2(700, 400))) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
					CreateNewFile(filePath);
				}
				ImGuiFileDialog::Instance()->Close();
			}

			// Handle load dialog
			if (ImGuiFileDialog::Instance()->Display("LoadTextFileDialog", 32, ImVec2(700, 400))) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
					LoadFile(filePath);
				}
				ImGuiFileDialog::Instance()->Close();
			}

			// Handle save dialog
			if (ImGuiFileDialog::Instance()->Display("SaveTextFileDialog", 32, ImVec2(700, 400))) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
					SaveFile(filePath);
				}
				ImGuiFileDialog::Instance()->Close();
			}
		}

		// Apply theme using Zep's actual theme system
		void ApplyTheme() {
			if (!editor || !isInitialized) return;

			try {
				auto& theme = editor->GetTheme();

				if (currentTheme == "dark") {
					// Set dark theme colors
					theme.SetThemeType(Zep::ThemeType::Dark);
					theme.SetColor(Zep::ThemeColor::Background, Zep::NVec4f(0.1f, 0.1f, 0.1f, 1.0f));
					theme.SetColor(Zep::ThemeColor::Text, Zep::NVec4f(0.9f, 0.9f, 0.9f, 1.0f));
					theme.SetColor(Zep::ThemeColor::Comment, Zep::NVec4f(0.5f, 0.7f, 0.5f, 1.0f));
					theme.SetColor(Zep::ThemeColor::Keyword, Zep::NVec4f(0.3f, 0.7f, 1.0f, 1.0f));
					theme.SetColor(Zep::ThemeColor::Identifier, Zep::NVec4f(0.8f, 0.8f, 0.8f, 1.0f));
					theme.SetColor(Zep::ThemeColor::Number, Zep::NVec4f(1.0f, 0.7f, 0.3f, 1.0f));
					theme.SetColor(Zep::ThemeColor::String, Zep::NVec4f(1.0f, 0.8f, 0.5f, 1.0f));
					theme.SetColor(Zep::ThemeColor::LineNumber, Zep::NVec4f(0.5f, 0.5f, 0.5f, 1.0f));
					theme.SetColor(Zep::ThemeColor::LineNumberActive, Zep::NVec4f(0.8f, 0.8f, 0.8f, 1.0f));
					theme.SetColor(Zep::ThemeColor::CursorNormal, Zep::NVec4f(1.0f, 1.0f, 1.0f, 1.0f));
					theme.SetColor(Zep::ThemeColor::CursorInsert, Zep::NVec4f(1.0f, 1.0f, 1.0f, 1.0f));
				}
				else if (currentTheme == "light") {
					// Set light theme colors
					theme.SetThemeType(Zep::ThemeType::Light);
					theme.SetColor(Zep::ThemeColor::Background, Zep::NVec4f(1.0f, 1.0f, 1.0f, 1.0f));
					theme.SetColor(Zep::ThemeColor::Text, Zep::NVec4f(0.0f, 0.0f, 0.0f, 1.0f));
					theme.SetColor(Zep::ThemeColor::Comment, Zep::NVec4f(0.0f, 0.5f, 0.0f, 1.0f));
					theme.SetColor(Zep::ThemeColor::Keyword, Zep::NVec4f(0.0f, 0.0f, 1.0f, 1.0f));
					theme.SetColor(Zep::ThemeColor::Identifier, Zep::NVec4f(0.2f, 0.2f, 0.2f, 1.0f));
					theme.SetColor(Zep::ThemeColor::Number, Zep::NVec4f(0.8f, 0.4f, 0.0f, 1.0f));
					theme.SetColor(Zep::ThemeColor::String, Zep::NVec4f(0.6f, 0.2f, 0.2f, 1.0f));
					theme.SetColor(Zep::ThemeColor::LineNumber, Zep::NVec4f(0.6f, 0.6f, 0.6f, 1.0f));
					theme.SetColor(Zep::ThemeColor::LineNumberActive, Zep::NVec4f(0.2f, 0.2f, 0.2f, 1.0f));
					theme.SetColor(Zep::ThemeColor::CursorNormal, Zep::NVec4f(0.0f, 0.0f, 0.0f, 1.0f));
					theme.SetColor(Zep::ThemeColor::CursorInsert, Zep::NVec4f(0.0f, 0.0f, 0.0f, 1.0f));
				}
				else if (currentTheme == "classic") {
					// Set classic/retro theme colors
					theme.SetThemeType(Zep::ThemeType::Dark);
					theme.SetColor(Zep::ThemeColor::Background, Zep::NVec4f(0.0f, 0.0f, 0.3f, 1.0f));
					theme.SetColor(Zep::ThemeColor::Text, Zep::NVec4f(0.9f, 0.9f, 0.9f, 1.0f));
					theme.SetColor(Zep::ThemeColor::Comment, Zep::NVec4f(0.4f, 0.8f, 0.4f, 1.0f));
					theme.SetColor(Zep::ThemeColor::Keyword, Zep::NVec4f(1.0f, 1.0f, 0.0f, 1.0f));
					theme.SetColor(Zep::ThemeColor::Identifier, Zep::NVec4f(0.8f, 0.8f, 1.0f, 1.0f));
					theme.SetColor(Zep::ThemeColor::Number, Zep::NVec4f(1.0f, 0.5f, 1.0f, 1.0f));
					theme.SetColor(Zep::ThemeColor::String, Zep::NVec4f(1.0f, 0.8f, 0.8f, 1.0f));
					theme.SetColor(Zep::ThemeColor::LineNumber, Zep::NVec4f(0.6f, 0.6f, 0.8f, 1.0f));
					theme.SetColor(Zep::ThemeColor::LineNumberActive, Zep::NVec4f(1.0f, 1.0f, 1.0f, 1.0f));
					theme.SetColor(Zep::ThemeColor::CursorNormal, Zep::NVec4f(1.0f, 1.0f, 0.0f, 1.0f));
					theme.SetColor(Zep::ThemeColor::CursorInsert, Zep::NVec4f(1.0f, 1.0f, 0.0f, 1.0f));
				}

				editor->RequestRefresh();
			}
			catch (const std::exception& e) {
				std::cerr << "Error applying theme: " << e.what() << std::endl;
			}
		}

		// Apply syntax highlighting using Zep's actual API
		void ApplySyntaxHighlighting() {
			if (!editor || !isInitialized) return;

			try {
				auto* buffer = editor->GetActiveBuffer();
				if (buffer) {
					if (enableSyntaxHighlighting && !currentFileType.empty()) {
						// Set file path with the correct extension for syntax detection
						std::string filename = "buffer" + currentFileType;
						buffer->SetFilePath(std::filesystem::path(filename));
						editor->RequestRefresh();
					}
					else {
						// Disable syntax highlighting by using no extension
						buffer->SetFilePath(std::filesystem::path("buffer"));
						editor->RequestRefresh();
					}
				}
			}
			catch (const std::exception& e) {
				std::cerr << "Error applying syntax highlighting: " << e.what() << std::endl;
			}
		}

		// Internal render method that handles the actual editor rendering
		void RenderEditor() {
			// Handle focus
			bool isFocused = ZepFocusTracker::IsFocused(instanceId);
			if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
				ZepFocusTracker::SetFocused(instanceId);
				isFocused = true;
			}

			// Handle Ctrl+F hotkey for search toggle (only when editor is focused and search input is not active)
			if (isFocused && !searchInputFocused && ImGui::IsKeyPressed(ImGuiKey_F) && ImGui::GetIO().KeyCtrl) {
				ToggleSearchBox();
			}

			// Handle right-click context menu
			if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1)) {
				ImGui::OpenPopup("ZepContextMenu");
			}

			if (ImGui::BeginPopup("ZepContextMenu")) {
				RenderContextMenu();
				ImGui::EndPopup();
			}

			// Render menu bar if enabled
			if (showMenuBar) {
				RenderMenuBar();
			}

			// Render search panel if enabled (between menu bar and editor)
			if (showSearchBox) {
				RenderSearchPanel();
			}

			editor->isFocused = isFocused;

			// Process mouse selection if focused
			if (isFocused) {
				HandleMouseSelection();
			}

			// Get display region
			auto min = ImGui::GetCursorScreenPos();
			auto max = ImGui::GetContentRegionAvail();
			max.x = std::max(1.0f, max.x);
			max.y = std::max(1.0f, max.y);

			max.x = min.x + max.x;
			max.y = min.y + max.y;
			editor->SetDisplayRegion(Zep::NVec2f(min.x, min.y), Zep::NVec2f(max.x, max.y));

			// Handle input capture for hotkeys - but only when search input is not focused
			if (isFocused && !searchInputFocused) {
				auto& io = ImGui::GetIO();
				io.WantCaptureKeyboard = false;
				io.WantCaptureMouse = false;
			}

			// Render Zep editor
			editor->Display();
			if (isFocused && !searchInputFocused) {
				editor->HandleInput();
			}

			// Restore input capture - but only when search input is not focused
			if (isFocused && !searchInputFocused) {
				auto& io = ImGui::GetIO();
				io.WantCaptureKeyboard = true;
				io.WantCaptureMouse = true;
			}
		}

		// Search panel rendering
		void RenderSearchPanel() {
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));

			float panelHeight = ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().FramePadding.y * 4;

			if (ImGui::BeginChild("SearchPanel", ImVec2(0, panelHeight), true, ImGuiWindowFlags_NoScrollbar)) {
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Find:");
				ImGui::SameLine();

				char searchBuffer[256];
				std::string currentSearch = GetSearchTerm();
				strncpy(searchBuffer, currentSearch.c_str(), sizeof(searchBuffer) - 1);
				searchBuffer[sizeof(searchBuffer) - 1] = '\0';

				ImGui::SetNextItemWidth(200.0f);

				if (showSearchBox && searchBoxJustOpened) {
					ImGui::SetKeyboardFocusHere(0);
					searchBoxJustOpened = false;
				}

				bool inputChanged = ImGui::InputText("##SearchInput", searchBuffer, sizeof(searchBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
				bool searchInputActive = ImGui::IsItemActive();

				searchInputFocused = searchInputActive;

				if (inputChanged) {
					SetSearchTerm(searchBuffer);
					if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
						FindNext();
					}
				}

				if (!inputChanged && searchBuffer != currentSearch) {
					SetSearchTerm(searchBuffer);
				}

				ImGui::SameLine();

				if (ImGui::Button("Prev")) {
					FindPrevious();
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Find Previous (Shift+F3)");
				}

				ImGui::SameLine();
				if (ImGui::Button("Next")) {
					FindNext();
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Find Next (F3)");
				}

				ImGui::SameLine();

				if (ImGui::Checkbox("Aa", &searchCaseSensitive)) {
					SetSearchCaseSensitive(searchCaseSensitive);
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Case Sensitive");
				}

				ImGui::SameLine();
				if (ImGui::Checkbox("Regex", &searchRegex)) {
					SetSearchRegex(searchRegex);
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Use Regular Expressions");
				}

				ImGui::SameLine();

				int resultCount = GetSearchResultCount();
				int currentIndex = GetCurrentSearchIndex();
				if (resultCount > 0) {
					ImGui::Text("%d/%d", currentIndex + 1, resultCount);
				}
				else if (!currentSearch.empty()) {
					ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "No results");
				}

				ImGui::SameLine();
				float closeButtonX = ImGui::GetContentRegionAvail().x - 20.0f;
				if (closeButtonX > 0) {
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + closeButtonX);
				}
				if (ImGui::Button("X")) {
					ShowSearchBox(false);
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Close Search (Escape)");
				}

				if (ImGui::IsKeyPressed(ImGuiKey_Escape) && !searchInputActive) {
					ShowSearchBox(false);
				}

				if (!searchInputActive && ImGui::IsKeyPressed(ImGuiKey_F3)) {
					if (ImGui::GetIO().KeyShift) {
						FindPrevious();
					}
					else {
						FindNext();
					}
				}
			}
			ImGui::EndChild();

			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
		}

		// Enhanced Menu Bar with file operations and syntax highlighting submenu
		void RenderMenuBar() {
			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu("File")) {
					if (ImGui::MenuItem("New", "Ctrl+N")) {
						NewFile();
					}
					ImGui::Separator();
					if (ImGui::MenuItem("Open...", "Ctrl+O")) {
						OpenLoadDialog();
					}
					if (ImGui::MenuItem("Save", "Ctrl+S")) {
						SaveFile();
					}
					if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
						OpenSaveDialog();
					}
					ImGui::Separator();
					if (ImGui::MenuItem("Clear")) {
						SetText("");
					}
					ImGui::Separator();
					if (ImGui::MenuItem("Copy All", "Ctrl+A, Ctrl+C")) {
						ImGui::SetClipboardText(GetText().c_str());
					}
					if (ImGui::MenuItem("Paste All", "Ctrl+V")) {
						const char* clipText = ImGui::GetClipboardText();
						if (clipText) {
							SetText(clipText);
						}
					}
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Edit")) {
					if (ImGui::MenuItem("Copy", "Ctrl+C")) {
						auto* buffer = editor->GetActiveBuffer();
						if (buffer && buffer->HasSelection()) {
							auto selection = buffer->GetInclusiveSelection();
							std::string selectedText = buffer->GetBufferText(selection.first, selection.second);
							ImGui::SetClipboardText(selectedText.c_str());
						}
					}
					if (ImGui::MenuItem("Paste", "Ctrl+V")) {
						const char* clipText = ImGui::GetClipboardText();
						if (clipText) {
							auto* buffer = editor->GetActiveBuffer();
							auto* window = editor->GetActiveWindow();
							if (buffer && window) {
								auto cursorPos = window->GetBufferCursor();
								Zep::ChangeRecord record;
								buffer->Insert(cursorPos, std::string(clipText), record);
							}
						}
					}
					if (ImGui::MenuItem("Select All", "Ctrl+A")) {
						auto* buffer = editor->GetActiveBuffer();
						if (buffer) {
							buffer->SetSelection(Zep::GlyphRange(buffer->Begin(), buffer->End()));
						}
					}
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("View")) {
					if (ImGui::MenuItem("Line Numbers", nullptr, &showLineNumbers)) {
						ApplyCurrentConfiguration();
					}
					if (ImGui::MenuItem("Word Wrap", nullptr, &wordWrap)) {
						ApplyCurrentConfiguration();
					}
					if (ImGui::MenuItem("Show Whitespace", nullptr, &showWhitespace)) {
						ApplyCurrentConfiguration();
					}
					if (ImGui::MenuItem("Syntax Highlighting", nullptr, &enableSyntaxHighlighting)) {
						ApplyCurrentConfiguration();
					}
					ImGui::Separator();
					if (ImGui::MenuItem("Show Search", "Ctrl+F", &showSearchBox)) {
						ShowSearchBox(showSearchBox);
						if (showSearchBox) {
							searchBoxJustOpened = true;
						}
					}
					ImGui::EndMenu();
				}

				// Syntax Highlighting submenu
				if (ImGui::BeginMenu("Syntax")) {
					// Display current file type
					ImGui::Text("Current: %s", currentFileType.empty() ? "Plain Text" : currentFileType.c_str());
					ImGui::Separator();

					// Common file types
					if (ImGui::MenuItem("Plain Text", nullptr, currentFileType == ".txt")) {
						SetFileType(".txt");
					}

					ImGui::Separator();
					ImGui::Text("Programming Languages:");

					if (ImGui::MenuItem("C/C++", nullptr, currentFileType == ".cpp" || currentFileType == ".c")) {
						SetFileType(".cpp");
					}
					if (ImGui::MenuItem("C/C++ Header", nullptr, currentFileType == ".h" || currentFileType == ".hpp")) {
						SetFileType(".hpp");
					}
					if (ImGui::MenuItem("Python", nullptr, currentFileType == ".py")) {
						SetFileType(".py");
					}
					if (ImGui::MenuItem("JavaScript", nullptr, currentFileType == ".js")) {
						SetFileType(".js");
					}
					if (ImGui::MenuItem("TypeScript", nullptr, currentFileType == ".ts")) {
						SetFileType(".ts");
					}
					if (ImGui::MenuItem("Java", nullptr, currentFileType == ".java")) {
						SetFileType(".java");
					}
					if (ImGui::MenuItem("C#", nullptr, currentFileType == ".cs")) {
						SetFileType(".cs");
					}
					if (ImGui::MenuItem("Rust", nullptr, currentFileType == ".rs")) {
						SetFileType(".rs");
					}
					if (ImGui::MenuItem("Go", nullptr, currentFileType == ".go")) {
						SetFileType(".go");
					}

					ImGui::Separator();
					ImGui::Text("Markup & Config:");

					if (ImGui::MenuItem("JSON", nullptr, currentFileType == ".json")) {
						SetFileType(".json");
					}
					if (ImGui::MenuItem("XML", nullptr, currentFileType == ".xml")) {
						SetFileType(".xml");
					}
					if (ImGui::MenuItem("HTML", nullptr, currentFileType == ".html")) {
						SetFileType(".html");
					}
					if (ImGui::MenuItem("CSS", nullptr, currentFileType == ".css")) {
						SetFileType(".css");
					}
					if (ImGui::MenuItem("Markdown", nullptr, currentFileType == ".md")) {
						SetFileType(".md");
					}
					if (ImGui::MenuItem("YAML", nullptr, currentFileType == ".yml" || currentFileType == ".yaml")) {
						SetFileType(".yml");
					}
					if (ImGui::MenuItem("TOML", nullptr, currentFileType == ".toml")) {
						SetFileType(".toml");
					}

					ImGui::Separator();
					ImGui::Text("Shaders:");

					if (ImGui::MenuItem("GLSL Vertex", nullptr, currentFileType == ".vert")) {
						SetFileType(".vert");
					}
					if (ImGui::MenuItem("GLSL Fragment", nullptr, currentFileType == ".frag")) {
						SetFileType(".frag");
					}
					if (ImGui::MenuItem("HLSL", nullptr, currentFileType == ".hlsl")) {
						SetFileType(".hlsl");
					}

					ImGui::Separator();
					ImGui::Text("Other:");

					if (ImGui::MenuItem("Batch Script", nullptr, currentFileType == ".bat")) {
						SetFileType(".bat");
					}
					if (ImGui::MenuItem("Shell Script", nullptr, currentFileType == ".sh")) {
						SetFileType(".sh");
					}
					if (ImGui::MenuItem("Log File", nullptr, currentFileType == ".log")) {
						SetFileType(".log");
					}

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Mode")) {
					if (ImGui::MenuItem("Standard Mode", nullptr, currentMode == "standard")) {
						SetMode("standard");
					}
					if (ImGui::MenuItem("Vim Mode", nullptr, currentMode == "vim")) {
						SetMode("vim");
					}
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Style")) {
					if (ImGui::MenuItem("Dark Theme", nullptr, currentTheme == "dark")) {
						SetTheme("dark");
					}
					if (ImGui::MenuItem("Light Theme", nullptr, currentTheme == "light")) {
						SetTheme("light");
					}
					if (ImGui::MenuItem("Classic Theme", nullptr, currentTheme == "classic")) {
						SetTheme("classic");
					}
					ImGui::Separator();

					ImGui::Text("Font Size:");
					if (ImGui::MenuItem("Small (12px)", nullptr, currentFontSize == 12.0f)) {
						SetFontSize(12.0f);
					}
					if (ImGui::MenuItem("Medium (14px)", nullptr, currentFontSize == 14.0f)) {
						SetFontSize(14.0f);
					}
					if (ImGui::MenuItem("Large (16px)", nullptr, currentFontSize == 16.0f)) {
						SetFontSize(16.0f);
					}
					if (ImGui::MenuItem("Extra Large (18px)", nullptr, currentFontSize == 18.0f)) {
						SetFontSize(18.0f);
					}

					ImGui::Separator();
					float customFontSize = currentFontSize;
					if (ImGui::SliderFloat("Custom Size", &customFontSize, 8.0f, 24.0f, "%.1f px")) {
						SetFontSize(customFontSize);
					}
					ImGui::EndMenu();
				}

				ImGui::EndMenuBar();
			}
		}

		// Context Menu
		void RenderContextMenu() {
			if (ImGui::MenuItem("Show Menu Bar", nullptr, &showMenuBar)) {
				// Toggle handled automatically by reference
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Copy", "Ctrl+C")) {
				auto* buffer = editor->GetActiveBuffer();
				if (buffer && buffer->HasSelection()) {
					auto selection = buffer->GetInclusiveSelection();
					std::string selectedText = buffer->GetBufferText(selection.first, selection.second);
					ImGui::SetClipboardText(selectedText.c_str());
				}
			}

			if (ImGui::MenuItem("Paste", "Ctrl+V")) {
				const char* clipText = ImGui::GetClipboardText();
				if (clipText) {
					auto* buffer = editor->GetActiveBuffer();
					auto* window = editor->GetActiveWindow();
					if (buffer && window) {
						auto cursorPos = window->GetBufferCursor();
						Zep::ChangeRecord record;
						buffer->Insert(cursorPos, std::string(clipText), record);
					}
				}
			}

			if (ImGui::MenuItem("Select All", "Ctrl+A")) {
				auto* buffer = editor->GetActiveBuffer();
				if (buffer) {
					buffer->SetSelection(Zep::GlyphRange(buffer->Begin(), buffer->End()));
				}
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Line Numbers", nullptr, &showLineNumbers)) {
				ApplyCurrentConfiguration();
			}
			if (ImGui::MenuItem("Word Wrap", nullptr, &wordWrap)) {
				ApplyCurrentConfiguration();
			}
			if (ImGui::MenuItem("Show Search", "Ctrl+F", &showSearchBox)) {
				ShowSearchBox(showSearchBox);
				if (showSearchBox) {
					searchBoxJustOpened = true;
				}
			}
		}

		void ApplyCurrentConfiguration() {
			if (!editor || !isInitialized) return;

			try {
				auto* buffer = editor->GetActiveBuffer();
				if (buffer) {
					auto* tabWindow = editor->GetActiveTabWindow();
					if (tabWindow) {
						auto* window = tabWindow->GetActiveWindow();
						if (window) {
							uint32_t flags = window->GetWindowFlags();

							if (showLineNumbers) {
								flags |= Zep::WindowFlags::ShowLineNumbers;
							}
							else {
								flags &= ~Zep::WindowFlags::ShowLineNumbers;
							}

							if (showWhitespace) {
								flags |= Zep::WindowFlags::ShowWhiteSpace;
							}
							else {
								flags &= ~Zep::WindowFlags::ShowWhiteSpace;
							}

							if (wordWrap) {
								flags |= Zep::WindowFlags::WrapText;
							}
							else {
								flags &= ~Zep::WindowFlags::WrapText;
							}

							window->SetWindowFlags(flags);
						}
					}

					// Apply syntax highlighting when configuration changes
					ApplySyntaxHighlighting();
				}
			}
			catch (const std::exception& e) {
				std::cerr << "Exception applying configuration: " << e.what() << std::endl;
			}
		}

		void PerformSearch() {
			searchResults.clear();
			currentSearchIndex = -1;

			if (currentSearchTerm.empty() || !editor || !isInitialized) {
				return;
			}

			std::string text = GetText();
			if (text.empty()) return;

			try {
				if (searchRegex) {
					PerformRegexSearch(text);
				}
				else {
					PerformPlainSearch(text);
				}
			}
			catch (const std::exception& e) {
				std::cerr << "Search error: " << e.what() << std::endl;
			}
		}

		void PerformPlainSearch(const std::string& text) {
			std::string searchText = currentSearchTerm;
			std::string sourceText = text;

			if (!searchCaseSensitive) {
				std::transform(searchText.begin(), searchText.end(), searchText.begin(), ::tolower);
				std::transform(sourceText.begin(), sourceText.end(), sourceText.begin(), ::tolower);
			}

			size_t pos = 0;
			while ((pos = sourceText.find(searchText, pos)) != std::string::npos) {
				searchResults.push_back({ pos, pos + searchText.length() });
				pos += 1;
			}
		}

		void PerformRegexSearch(const std::string& text) {
			try {
				std::regex::flag_type flags = std::regex::ECMAScript;
				if (!searchCaseSensitive) {
					flags |= std::regex::icase;
				}

				std::regex pattern(currentSearchTerm, flags);
				std::sregex_iterator iter(text.begin(), text.end(), pattern);
				std::sregex_iterator end;

				for (; iter != end; ++iter) {
					const std::smatch& match = *iter;
					searchResults.push_back({
						static_cast<size_t>(match.position()),
						static_cast<size_t>(match.position() + match.length())
						});
				}
			}
			catch (const std::regex_error& e) {
				std::cerr << "Regex error: " << e.what() << std::endl;
			}
		}

		void JumpToSearchResult(int index) {
			if (index < 0 || index >= static_cast<int>(searchResults.size())) return;

			auto& result = searchResults[index];

			if (editor && isInitialized) {
				try {
					auto* buffer = editor->GetActiveBuffer();
					if (buffer) {
						auto startIter = buffer->Begin() + static_cast<long>(result.first);
						auto endIter = buffer->Begin() + static_cast<long>(result.second);

						if (auto* window = editor->GetActiveWindow()) {
							window->SetBufferCursor(startIter);
							buffer->SetSelection(Zep::GlyphRange(startIter, endIter));
						}
					}
				}
				catch (const std::exception& e) {
					std::cerr << "Error jumping to search result: " << e.what() << std::endl;
				}
			}
		}

		void ClearSearch() {
			searchResults.clear();
			currentSearchIndex = -1;
			currentSearchTerm.clear();

			if (editor && isInitialized) {
				auto* buffer = editor->GetActiveBuffer();
				if (buffer) {
					buffer->ClearSelection();
				}
			}
		}

		void HandleMouseSelection() {
			auto& io = ImGui::GetIO();
			auto* buffer = editor->GetActiveBuffer();
			if (!buffer) return;

			Zep::NVec2f mousePos = Zep::NVec2f(io.MousePos.x, io.MousePos.y);
			double currentTime = ImGui::GetTime();

			if (ImGui::IsMouseClicked(0)) {
				HandleMouseClick(mousePos, currentTime);
			}

			if (ImGui::IsMouseDragging(0, 2.0f) && !isDoubleClickPending && !isTripleClickPending) {
				HandleMouseDrag(mousePos);
			}

			if (ImGui::IsMouseReleased(0)) {
				HandleMouseRelease();
			}
		}

		void HandleMouseClick(const Zep::NVec2f& mousePos, double currentTime) {
			auto* buffer = editor->GetActiveBuffer();
			if (!buffer) return;

			bool isConsecutiveClick = false;
			float distance = std::sqrt(
				(mousePos.x - lastClickPosition.x) * (mousePos.x - lastClickPosition.x) +
				(mousePos.y - lastClickPosition.y) * (mousePos.y - lastClickPosition.y)
			);

			if (currentTime - lastClickTime < DOUBLE_CLICK_TIME && distance < CLICK_POSITION_TOLERANCE) {
				consecutiveClicks++;
				isConsecutiveClick = true;
			}
			else {
				consecutiveClicks = 1;
			}

			lastClickTime = currentTime;
			lastClickPosition = mousePos;

			if (editor->OnMouseDown(mousePos, Zep::ZepMouseButton::Left)) {
				auto* window = editor->GetActiveWindow();
				if (window) {
					Zep::GlyphIterator clickPos = window->GetBufferCursor();

					switch (consecutiveClicks) {
					case 1:
						HandleSingleClick(clickPos);
						break;
					case 2:
						HandleDoubleClick(clickPos);
						break;
					case 3:
					default:
						HandleTripleClick(clickPos);
						consecutiveClicks = 3;
						break;
					}
				}
			}
			else {
				auto* window = editor->GetActiveWindow();
				if (window) {
					Zep::GlyphIterator clickPos = GetIteratorFromMousePos(mousePos);

					switch (consecutiveClicks) {
					case 1:
						HandleSingleClick(clickPos);
						break;
					case 2:
						HandleDoubleClick(clickPos);
						break;
					case 3:
					default:
						HandleTripleClick(clickPos);
						consecutiveClicks = 3;
						break;
					}
				}
			}
		}

		void HandleSingleClick(const Zep::GlyphIterator& clickPos) {
			auto* window = editor->GetActiveWindow();
			if (!window) return;

			window->SetBufferCursor(clickPos);

			auto* buffer = editor->GetActiveBuffer();
			if (buffer) {
				buffer->ClearSelection();
			}

			dragStartPos = clickPos;
			dragCurrentPos = clickPos;
			isDragging = false;
		}

		void HandleDoubleClick(const Zep::GlyphIterator& clickPos) {
			auto* buffer = editor->GetActiveBuffer();
			if (!buffer) return;

			auto wordStart = FindWordStart(clickPos);
			auto wordEnd = FindWordEnd(clickPos);

			if (wordStart != wordEnd) {
				buffer->SetSelection(Zep::GlyphRange(wordStart, wordEnd));
			}

			auto* window = editor->GetActiveWindow();
			if (window) {
				window->SetBufferCursor(wordEnd);
			}

			isDoubleClickPending = true;
		}

		void HandleTripleClick(const Zep::GlyphIterator& clickPos) {
			auto* buffer = editor->GetActiveBuffer();
			if (!buffer) return;

			auto lineStart = FindLineStart(clickPos);
			auto lineEnd = FindLineEnd(clickPos);

			if (lineStart != lineEnd) {
				buffer->SetSelection(Zep::GlyphRange(lineStart, lineEnd));
			}

			auto* window = editor->GetActiveWindow();
			if (window) {
				window->SetBufferCursor(lineEnd);
			}

			isTripleClickPending = true;
		}

		void HandleMouseDrag(const Zep::NVec2f& mousePos) {
			if (isDoubleClickPending || isTripleClickPending) {
				return;
			}

			auto* buffer = editor->GetActiveBuffer();
			if (!buffer) return;

			isDragging = true;

			Zep::GlyphIterator currentPos = GetIteratorFromMousePos(mousePos);
			dragCurrentPos = currentPos;

			Zep::GlyphIterator selStart = dragStartPos;
			Zep::GlyphIterator selEnd = dragCurrentPos;

			if (selEnd < selStart) {
				std::swap(selStart, selEnd);
			}

			buffer->SetSelection(Zep::GlyphRange(selStart, selEnd));

			auto* window = editor->GetActiveWindow();
			if (window) {
				window->SetBufferCursor(dragCurrentPos);
			}
		}

		void HandleMouseRelease() {
			isDragging = false;
			isDoubleClickPending = false;
			isTripleClickPending = false;
		}

		Zep::GlyphIterator GetIteratorFromMousePos(const Zep::NVec2f& mousePos) {
			auto* buffer = editor->GetActiveBuffer();
			if (!buffer) return buffer->Begin();

			auto* window = editor->GetActiveWindow();
			if (!window) return buffer->Begin();

			try {
				auto currentCursor = window->GetBufferCursor();
				return currentCursor;
			}
			catch (const std::exception& e) {
				std::cerr << "Error converting mouse position: " << e.what() << std::endl;
				return buffer->Begin();
			}
		}

		Zep::GlyphIterator FindWordStart(const Zep::GlyphIterator& pos) {
			auto* buffer = editor->GetActiveBuffer();
			if (!buffer) return pos;

			auto iter = pos;

			while (iter > buffer->Begin()) {
				auto prevIter = iter;
				prevIter.Move(-1);
				char ch = *prevIter;

				if (!IsWordCharacter(ch)) {
					break;
				}
				iter = prevIter;
			}

			return iter;
		}

		Zep::GlyphIterator FindWordEnd(const Zep::GlyphIterator& pos) {
			auto* buffer = editor->GetActiveBuffer();
			if (!buffer) return pos;

			auto iter = pos;

			while (iter < buffer->End()) {
				char ch = *iter;

				if (!IsWordCharacter(ch)) {
					break;
				}
				iter.Move(1);
			}

			return iter;
		}

		Zep::GlyphIterator FindLineStart(const Zep::GlyphIterator& pos) {
			auto* buffer = editor->GetActiveBuffer();
			if (!buffer) return pos;

			auto iter = pos;

			while (iter > buffer->Begin()) {
				auto prevIter = iter;
				prevIter.Move(-1);
				if (*prevIter == '\n') {
					break;
				}
				iter = prevIter;
			}

			return iter;
		}

		Zep::GlyphIterator FindLineEnd(const Zep::GlyphIterator& pos) {
			auto* buffer = editor->GetActiveBuffer();
			if (!buffer) return pos;

			auto iter = pos;

			while (iter < buffer->End()) {
				if (*iter == '\n') {
					iter.Move(1);
					break;
				}
				iter.Move(1);
			}

			return iter;
		}

		bool IsWordCharacter(char ch) {
			return std::isalnum(ch) || ch == '_';
		}

	private:
		std::unique_ptr<Zep::ZepEditor_ImGui> editor;
		uintptr_t instanceId;
		bool isInitialized;

		// Configuration state
		bool showLineNumbers = true;
		bool wordWrap = true;
		bool readOnly = false;
		bool showWhitespace = false;
		bool enableSyntaxHighlighting = true;
		bool autoIndent = true;
		bool showMenuBar = true;
		std::string currentTheme = "dark";
		std::string currentMode = "standard";
		bool showSearchBox = false;
		bool searchBoxJustOpened = false;
		bool searchInputFocused = false;
		std::string currentSearchTerm;
		float currentFontSize = 14.0f;

		// File management
		std::string currentFileType = ".txt";
		std::string currentFilePath;

		// Search state
		bool searchCaseSensitive = false;
		bool searchRegex = false;
		std::vector<std::pair<size_t, size_t>> searchResults;
		int currentSearchIndex = -1;

		// Mouse selection state
		bool isDragging = false;
		bool isDoubleClickPending = false;
		bool isTripleClickPending = false;
		Zep::GlyphIterator dragStartPos;
		Zep::GlyphIterator dragCurrentPos;
		double lastClickTime = 0.0;
		int consecutiveClicks = 0;
		Zep::NVec2f lastClickPosition;

		// Mouse selection timing constants
		static constexpr double DOUBLE_CLICK_TIME = 0.5;
		static constexpr double TRIPLE_CLICK_TIME = 0.5;
		static constexpr float CLICK_POSITION_TOLERANCE = 5.0f;
	};

} // namespace Utils