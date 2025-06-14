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
#include <memory>
#include <string>
#include <filesystem>
#include <imgui.h>
#include <iostream>
#include <regex>
#include <algorithm>
#include <vector>

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

				// Initialize with text
				editor->InitWithText("buffer.txt", "");

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
			ApplyCurrentConfiguration();
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
			ApplyCurrentConfiguration();
		}

		void SetTheme(const std::string& theme) {
			currentTheme = theme;
			ApplyCurrentConfiguration();
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
				}
			}
		}

		// File operations
		bool LoadFile(const std::string& filePath) {
			// TODO: Implement
			return false;
		}

		bool SaveFile(const std::string& filePath) {
			// TODO: Implement
			return false;
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
			if (!show) {
				ClearSearch();
			}
		}

		bool IsSearchBoxVisible() const { return showSearchBox; }

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
		// Internal render method that handles the actual editor rendering
		void RenderEditor() {
			// Handle focus
			bool isFocused = ZepFocusTracker::IsFocused(instanceId);
			if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
				ZepFocusTracker::SetFocused(instanceId);
				isFocused = true;
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

			// Render search box if enabled
			if (showSearchBox) {
				RenderSearchBox();
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

			// Handle input capture for hotkeys
			if (isFocused) {
				auto& io = ImGui::GetIO();
				io.WantCaptureKeyboard = false;
				io.WantCaptureMouse = false;
			}

			// Render Zep editor
			editor->Display();
			if (isFocused) {
				editor->HandleInput();
			}

			// Restore input capture
			if (isFocused) {
				auto& io = ImGui::GetIO();
				io.WantCaptureKeyboard = true;
				io.WantCaptureMouse = true;
			}
		}

		// Add search box rendering
		void RenderSearchBox() {
			if (ImGui::CollapsingHeader("Search")) {
				char searchBuffer[256];
				std::string currentSearch = GetSearchTerm();
				strncpy(searchBuffer, currentSearch.c_str(), sizeof(searchBuffer) - 1);
				searchBuffer[sizeof(searchBuffer) - 1] = '\0';

				if (ImGui::InputText("Find", searchBuffer, sizeof(searchBuffer))) {
					SetSearchTerm(searchBuffer);
				}

				ImGui::SameLine();
				if (ImGui::Button("Next")) {
					FindNext();
				}
				ImGui::SameLine();
				if (ImGui::Button("Prev")) {
					FindPrevious();
				}

				// Show results
				int resultCount = GetSearchResultCount();
				int currentIndex = GetCurrentSearchIndex();
				if (resultCount > 0) {
					ImGui::Text("Result %d of %d", currentIndex + 1, resultCount);
				}
			}
		}

		// Menu Bar
		void RenderMenuBar() {
			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu("File")) {
					if (ImGui::MenuItem("New", "Ctrl+N")) {
						SetText("");
					}
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
					if (ImGui::MenuItem("Auto Indent", nullptr, &autoIndent)) {
						ApplyCurrentConfiguration();
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

					// Font size options
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
					// Custom font size slider
					float customFontSize = currentFontSize;
					if (ImGui::SliderFloat("Custom Size", &customFontSize, 8.0f, 24.0f, "%.1f px")) {
						SetFontSize(customFontSize);
					}
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Search")) {
					// Search input
					char searchBuffer[256];
					std::string currentSearch = GetSearchTerm();
					strncpy(searchBuffer, currentSearch.c_str(), sizeof(searchBuffer) - 1);
					searchBuffer[sizeof(searchBuffer) - 1] = '\0';

					if (ImGui::InputText("Find", searchBuffer, sizeof(searchBuffer))) {
						SetSearchTerm(searchBuffer);
					}

					// Search options
					bool caseSensitive = GetSearchCaseSensitive();
					if (ImGui::Checkbox("Case Sensitive", &caseSensitive)) {
						SetSearchCaseSensitive(caseSensitive);
					}

					bool useRegex = GetSearchRegex();
					if (ImGui::Checkbox("Use Regex", &useRegex)) {
						SetSearchRegex(useRegex);
					}

					ImGui::Separator();

					// Search navigation
					if (ImGui::MenuItem("Find Next", "F3")) {
						FindNext();
					}

					if (ImGui::MenuItem("Find Previous", "Shift+F3")) {
						FindPrevious();
					}

					// Show search results count
					int resultCount = GetSearchResultCount();
					int currentIndex = GetCurrentSearchIndex();
					if (resultCount > 0) {
						ImGui::Text("Result %d of %d", currentIndex + 1, resultCount);
					}
					else if (!currentSearch.empty()) {
						ImGui::Text("No results found");
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

			// Quick edit actions
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

			// Quick toggles
			if (ImGui::MenuItem("Line Numbers", nullptr, &showLineNumbers)) {
				ApplyCurrentConfiguration();
			}
			if (ImGui::MenuItem("Word Wrap", nullptr, &wordWrap)) {
				ApplyCurrentConfiguration();
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

			// Convert to lowercase for case-insensitive search
			if (!searchCaseSensitive) {
				std::transform(searchText.begin(), searchText.end(), searchText.begin(), ::tolower);
				std::transform(sourceText.begin(), sourceText.end(), sourceText.begin(), ::tolower);
			}

			size_t pos = 0;
			while ((pos = sourceText.find(searchText, pos)) != std::string::npos) {
				searchResults.push_back({ pos, pos + searchText.length() });
				pos += 1; // Move past this match to find overlapping matches
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
					// Get the buffer and try to set cursor position
					auto* buffer = editor->GetActiveBuffer();
					if (buffer) {
						// Convert byte position to Zep's iterator system
						auto startIter = buffer->Begin() + static_cast<long>(result.first);
						auto endIter = buffer->Begin() + static_cast<long>(result.second);

						// Set cursor to start of match
						if (auto* window = editor->GetActiveWindow()) {
							window->SetBufferCursor(startIter);
							// Set selection using GlyphRange
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

			// Clear any selection
			if (editor && isInitialized) {
				auto* buffer = editor->GetActiveBuffer();
				if (buffer) {
					buffer->ClearSelection();
				}
			}
		}

		// Mouse highlighting
		void HandleMouseSelection() {
			auto& io = ImGui::GetIO();
			auto* buffer = editor->GetActiveBuffer();
			if (!buffer) return;

			// Get current mouse position in screen coordinates
			Zep::NVec2f mousePos = Zep::NVec2f(io.MousePos.x, io.MousePos.y);
			double currentTime = ImGui::GetTime();

			// Handle mouse clicks
			if (ImGui::IsMouseClicked(0)) {
				HandleMouseClick(mousePos, currentTime);
			}

			// Handle mouse drag - only if we actually started dragging
			if (ImGui::IsMouseDragging(0, 2.0f) && !isDoubleClickPending && !isTripleClickPending) {
				HandleMouseDrag(mousePos);
			}

			// Handle mouse release
			if (ImGui::IsMouseReleased(0)) {
				HandleMouseRelease();
			}
		}

		void HandleMouseClick(const Zep::NVec2f& mousePos, double currentTime) {
			auto* buffer = editor->GetActiveBuffer();
			if (!buffer) return;

			// Check if this is a consecutive click in the same area
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

			// Let Zep handle the mouse click first to get proper cursor positioning
			if (editor->OnMouseDown(mousePos, Zep::ZepMouseButton::Left)) {
				// Zep handled the click, now get the updated cursor position
				auto* window = editor->GetActiveWindow();
				if (window) {
					Zep::GlyphIterator clickPos = window->GetBufferCursor();

					switch (consecutiveClicks) {
					case 1:
						// Single click - start drag selection or place cursor
						HandleSingleClick(clickPos);
						break;
					case 2:
						// Double click - select word
						HandleDoubleClick(clickPos);
						break;
					case 3:
					default:
						// Triple click - select line
						HandleTripleClick(clickPos);
						consecutiveClicks = 3; // Cap at triple click
						break;
					}
				}
			}
			else {
				// Fallback if Zep doesn't handle the click
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

			// Set cursor position
			window->SetBufferCursor(clickPos);

			// Clear any existing selection only if we're not starting a potential drag
			auto* buffer = editor->GetActiveBuffer();
			if (buffer) {
				buffer->ClearSelection();
			}

			// Initialize drag operation
			dragStartPos = clickPos;
			dragCurrentPos = clickPos;
			isDragging = false; // Will be set to true on first mouse move
		}

		void HandleDoubleClick(const Zep::GlyphIterator& clickPos) {
			auto* buffer = editor->GetActiveBuffer();
			if (!buffer) return;

			// Find word boundaries
			auto wordStart = FindWordStart(clickPos);
			auto wordEnd = FindWordEnd(clickPos);

			// Select the word
			if (wordStart != wordEnd) {
				buffer->SetSelection(Zep::GlyphRange(wordStart, wordEnd));
			}

			// Set cursor to end of selection
			auto* window = editor->GetActiveWindow();
			if (window) {
				window->SetBufferCursor(wordEnd);
			}

			isDoubleClickPending = true;
		}

		void HandleTripleClick(const Zep::GlyphIterator& clickPos) {
			auto* buffer = editor->GetActiveBuffer();
			if (!buffer) return;

			// Find line boundaries
			auto lineStart = FindLineStart(clickPos);
			auto lineEnd = FindLineEnd(clickPos);

			// Select the entire line
			if (lineStart != lineEnd) {
				buffer->SetSelection(Zep::GlyphRange(lineStart, lineEnd));
			}

			// Set cursor to end of selection
			auto* window = editor->GetActiveWindow();
			if (window) {
				window->SetBufferCursor(lineEnd);
			}

			isTripleClickPending = true;
		}

		void HandleMouseDrag(const Zep::NVec2f& mousePos) {
			if (isDoubleClickPending || isTripleClickPending) {
				return; // Don't drag if we just did a multi-click
			}

			auto* buffer = editor->GetActiveBuffer();
			if (!buffer) return;

			isDragging = true;

			// Get current mouse position in buffer coordinates
			Zep::GlyphIterator currentPos = GetIteratorFromMousePos(mousePos);
			dragCurrentPos = currentPos;

			// Create selection from drag start to current position
			Zep::GlyphIterator selStart = dragStartPos;
			Zep::GlyphIterator selEnd = dragCurrentPos;

			// Ensure selection is in the right order (start should be before end)
			if (selEnd < selStart) {
				std::swap(selStart, selEnd);
			}

			// Always set selection, even if start == end (which clears selection)
			buffer->SetSelection(Zep::GlyphRange(selStart, selEnd));

			// Set cursor to the actual end position (where the mouse is)
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

		// Helper function to convert mouse position to buffer iterator
		Zep::GlyphIterator GetIteratorFromMousePos(const Zep::NVec2f& mousePos) {
			auto* buffer = editor->GetActiveBuffer();
			if (!buffer) return buffer->Begin();

			auto* window = editor->GetActiveWindow();
			if (!window) return buffer->Begin();

			// Try to use Zep's coordinate conversion if available
			try {
				// Get the current cursor position as a fallback
				auto currentCursor = window->GetBufferCursor();

				// For now, we'll use a simple approach based on relative mouse movement
				// In a real implementation, you'd want to use Zep's actual coordinate conversion

				// Simple approximation: assume uniform character width
				// This is not perfect but will work better than just returning cursor position
				auto textStart = buffer->Begin();
				auto textEnd = buffer->End();

				// Calculate approximate character position based on mouse
				// This is a simplified implementation - you may need to enhance this
				// based on the actual Zep coordinate system

				return currentCursor; // For now, return cursor position
			}
			catch (const std::exception& e) {
				std::cerr << "Error converting mouse position: " << e.what() << std::endl;
				return buffer->Begin();
			}
		}

		// Word boundary detection - USING SAFE ITERATOR OPERATIONS
		Zep::GlyphIterator FindWordStart(const Zep::GlyphIterator& pos) {
			auto* buffer = editor->GetActiveBuffer();
			if (!buffer) return pos;

			auto iter = pos;

			// Move backwards while we're in a word
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

			// Move forwards while we're in a word
			while (iter < buffer->End()) {
				char ch = *iter;

				if (!IsWordCharacter(ch)) {
					break;
				}
				iter.Move(1);
			}

			return iter;
		}

		// Line boundary detection
		Zep::GlyphIterator FindLineStart(const Zep::GlyphIterator& pos) {
			auto* buffer = editor->GetActiveBuffer();
			if (!buffer) return pos;

			auto iter = pos;

			// Move backwards to start of line
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

			// Move forwards to end of line (including the newline)
			while (iter < buffer->End()) {
				if (*iter == '\n') {
					iter.Move(1); // Include the newline
					break;
				}
				iter.Move(1);
			}

			return iter;
		}

		// Character classification
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
		std::string currentSearchTerm;
		float currentFontSize = 14.0f;

		// Search state
		bool searchCaseSensitive = false;
		bool searchRegex = false;
		std::vector<std::pair<size_t, size_t>> searchResults;
		int currentSearchIndex = -1;

		// Mouse selection state - RESTORED
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