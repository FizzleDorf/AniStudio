#pragma once
#include <zep/editor.h>
#include <zep/imgui/editor_imgui.h>
#include <memory>
#include <string>
#include <filesystem>
#include <imgui.h>
#include <iostream>

namespace Utils {

	// Simple focus tracker - just tracks which instance is currently focused
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
	private:
		std::shared_ptr<Zep::ZepEditor_ImGui> editor;
		uintptr_t instanceId;
		bool isInitialized;
		std::string lastKnownText;

		// Configuration state
		bool showLineNumbers = true;
		bool wordWrap = false;
		bool readOnly = false;
		bool showWhitespace = false;
		bool enableSyntaxHighlighting = true;
		bool autoIndent = true;
		std::string currentTheme = "dark";

		// Search state
		std::string currentSearchTerm;
		bool searchCaseSensitive = false;
		bool searchRegex = false;
		bool showSearchBox = false;

	public:
		ZepTextEditor() : instanceId(reinterpret_cast<uintptr_t>(this)), isInitialized(false) {
		}

		~ZepTextEditor() = default;

		bool Initialize() {
			if (isInitialized) {
				return true;
			}

			try {
				auto rootPath = std::filesystem::current_path();

				// Create completely independent Zep editor instance
				editor = std::make_shared<Zep::ZepEditor_ImGui>(
					rootPath,
					Zep::NVec2f(1.0f, 1.0f),
					Zep::ZepEditorFlags::DisableThreads,
					nullptr
					);

				SetupClipboardCallbacks();
				SetupInitialBuffer();

				// FORCE Standard mode - Zep defaults to Vim! Use exact string from ZepMode_Standard::StaticName()
				editor->SetGlobalMode("Standard");  // Capital S!

				std::cout << "Forced Standard mode during initialization" << std::endl;

				// Verify we're in Standard mode
				auto* mode = editor->GetGlobalMode();
				if (mode) {
					std::cout << "Current mode after init: " << mode->Name() << std::endl;
					if (mode->Name() != "Standard") {
						std::cerr << "ERROR: Still not in Standard mode after initialization!" << std::endl;
					}
				}

				// Apply default configuration
				ApplyCurrentConfiguration();

				isInitialized = true;
				return true;
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in ZepTextEditor::Initialize(): " << e.what() << std::endl;
				return false;
			}
		}

		void Render(ImVec2 position, ImVec2 size) {
			if (!editor || !isInitialized) {
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Editor not initialized");
				return;
			}

			// Render search box if enabled
			if (showSearchBox) {
				RenderSearchBox();
			}

			// Check if we're focused
			bool isFocused = ZepFocusTracker::IsFocused(instanceId);

			// Handle focus changes with a child window that captures input
			std::string childId = "##zep_child_" + std::to_string(instanceId);

			if (ImGui::BeginChild(childId.c_str(), size, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
				bool childFocused = ImGui::IsWindowFocused();
				bool childHovered = ImGui::IsWindowHovered();

				// Update focus state - but don't steal focus from text selection
				if (childHovered && ImGui::IsMouseClicked(0) && !ImGui::IsMouseDragging(0)) {
					ZepFocusTracker::SetFocused(instanceId);
					isFocused = true;
				}
				else if (isFocused && ImGui::IsMouseClicked(0) && !childHovered) {
					ZepFocusTracker::ClearFocus();
					isFocused = false;
				}

				// Set the editor's focus state (controls cursor blinking and input processing)
				editor->isFocused = isFocused;

				// IMPORTANT: Allow Zep to handle text selection properly
				if (isFocused) {
					ImGuiIO& io = ImGui::GetIO();

					// Let Zep handle mouse for text selection, but don't override keyboard shortcuts
					if (childHovered) {
						io.WantCaptureMouse = false;  // Let Zep handle mouse for text selection
					}

					// Only prevent keyboard capture for certain keys, allow Ctrl+C, Ctrl+V, etc.
					if (!io.KeyCtrl && !io.KeyAlt) {
						io.WantCaptureKeyboard = false;  // Let Zep handle most keyboard input
					}
				}

				// Handle search hotkeys - check BEFORE setting focus
				ImGuiIO& io = ImGui::GetIO();
				if (childHovered && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F)) {
					showSearchBox = !showSearchBox;
				}

				// Draw highlight border if focused
				if (isFocused) {
					ImDrawList* drawList = ImGui::GetWindowDrawList();
					ImVec2 childPos = ImGui::GetWindowPos();
					ImVec2 childSize = ImGui::GetWindowSize();
					drawList->AddRect(childPos, ImVec2(childPos.x + childSize.x, childPos.y + childSize.y),
						IM_COL32(100, 150, 255, 255), 0.0f, 0, 2.0f);
				}

				try {
					// Get the actual content area of the child window
					ImVec2 contentSize = ImGui::GetContentRegionAvail();
					ImVec2 cursorPos = ImGui::GetCursorScreenPos();

					// Set display region for Zep
					editor->SetDisplayRegion(
						Zep::NVec2f(cursorPos.x, cursorPos.y),
						Zep::NVec2f(cursorPos.x + contentSize.x, cursorPos.y + contentSize.y)
					);

					// CRITICAL: Only let the focused editor handle input, but allow standard text editing
					if (isFocused) {
						// FORCE Standard mode every frame - use correct string
						editor->SetGlobalMode("Standard");  // Capital S!

						// Handle search hotkey before Zep gets input
						ImGuiIO& io = ImGui::GetIO();
						if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F)) {
							showSearchBox = !showSearchBox;
							// Clear the key press so Zep doesn't also handle it
							io.KeysDown[ImGuiKey_F] = false;
						}
						else {
							// Let Zep handle ALL other input (keyboard + mouse)
							editor->HandleInput();
						}
					}

					// Always display the editor
					editor->Display();
				}
				catch (const std::exception& e) {
					ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Render Error: %s", e.what());
				}
			}
			ImGui::EndChild();
		}

		void SetMode(const std::string& mode) {
			// ALWAYS force Standard mode - use correct string
			if (editor && isInitialized) {
				editor->SetGlobalMode("Standard");  // Capital S!
			}
		}

		std::string GetText() const {
			if (editor && isInitialized) {
				auto* tabWindow = editor->GetActiveTabWindow();
				if (tabWindow) {
					auto* window = tabWindow->GetActiveWindow();
					if (window) {
						auto& buffer = window->GetBuffer();
						return buffer.GetWorkingBuffer().string();
					}
				}
			}
			return lastKnownText;
		}

		void SetText(const std::string& text) {
			lastKnownText = text;
			if (editor && isInitialized) {
				auto* tabWindow = editor->GetActiveTabWindow();
				if (tabWindow) {
					auto* window = tabWindow->GetActiveWindow();
					if (window) {
						auto& buffer = window->GetBuffer();
						buffer.SetText(text);
					}
				}
			}
		}

		// Configuration methods (store state but don't apply to Zep directly)
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

		// Configuration getters
		bool GetShowLineNumbers() const { return showLineNumbers; }
		bool GetWordWrap() const { return wordWrap; }
		bool GetReadOnly() const { return readOnly; }
		bool GetShowWhitespace() const { return showWhitespace; }
		bool GetSyntaxHighlighting() const { return enableSyntaxHighlighting; }
		bool GetAutoIndent() const { return autoIndent; }
		std::string GetTheme() const { return currentTheme; }

		// Search functionality
		void SetSearchTerm(const std::string& term) { currentSearchTerm = term; }
		std::string GetSearchTerm() const { return currentSearchTerm; }

		void SetSearchCaseSensitive(bool caseSensitive) { searchCaseSensitive = caseSensitive; }
		bool GetSearchCaseSensitive() const { return searchCaseSensitive; }

		void SetSearchRegex(bool regex) { searchRegex = regex; }
		bool GetSearchRegex() const { return searchRegex; }

		void ShowSearchBox(bool show) { showSearchBox = show; }
		bool IsSearchBoxVisible() const { return showSearchBox; }

		// Copy selected text to clipboard (simplified version using existing working methods)
		bool CopySelection() {
			if (!editor || !isInitialized) {
				return false;
			}

			try {
				// For now, just copy current text since we don't have selection API access
				// This will be enhanced once proper selection API is available
				std::string currentText = GetText();
				if (!currentText.empty()) {
					ImGui::SetClipboardText(currentText.c_str());
					return true;
				}
			}
			catch (const std::exception& e) {
				std::cerr << "Error in CopySelection: " << e.what() << std::endl;
			}
			return false;
		}

		// Paste text from clipboard (simplified version)
		bool PasteFromClipboard() {
			if (!editor || !isInitialized) {
				return false;
			}

			try {
				const char* clipboardText = ImGui::GetClipboardText();
				if (!clipboardText || strlen(clipboardText) == 0) {
					return false;
				}

				// For now, append to existing text
				// This will be enhanced once proper insertion API is available
				std::string currentText = GetText();
				std::string newText = currentText + std::string(clipboardText);
				SetText(newText);
				return true;
			}
			catch (const std::exception& e) {
				std::cerr << "Error in PasteFromClipboard: " << e.what() << std::endl;
			}
			return false;
		}
		bool FindNext() {
			if (!editor || !isInitialized || currentSearchTerm.empty()) {
				return false;
			}

			try {
				// Simple string search implementation since Zep's search API isn't available
				std::string text = GetText();
				auto* tabWindow = editor->GetActiveTabWindow();
				if (!tabWindow) return false;

				auto* window = tabWindow->GetActiveWindow();
				if (!window) return false;

				auto cursor = window->GetBufferCursor();
				long currentPos = cursor.Index();

				// Search forward from current position
				std::string searchText = currentSearchTerm;
				if (!searchCaseSensitive) {
					// Convert to lowercase for case-insensitive search
					std::transform(searchText.begin(), searchText.end(), searchText.begin(), ::tolower);
					std::transform(text.begin(), text.end(), text.begin(), ::tolower);
				}

				size_t found = text.find(searchText, currentPos + 1);
				if (found != std::string::npos) {
					// Move cursor to found position
					auto& buffer = window->GetBuffer();
					auto foundIter = buffer.Begin() + found;
					window->SetBufferCursor(foundIter);
					return true;
				}

				// If not found forward, search from beginning
				found = text.find(searchText, 0);
				if (found != std::string::npos && found < currentPos) {
					auto& buffer = window->GetBuffer();
					auto foundIter = buffer.Begin() + found;
					window->SetBufferCursor(foundIter);
					return true;
				}
			}
			catch (const std::exception& e) {
				std::cerr << "Error in FindNext: " << e.what() << std::endl;
			}
			return false;
		}

		bool FindPrevious() {
			if (!editor || !isInitialized || currentSearchTerm.empty()) {
				return false;
			}

			try {
				// Simple string search implementation
				std::string text = GetText();
				auto* tabWindow = editor->GetActiveTabWindow();
				if (!tabWindow) return false;

				auto* window = tabWindow->GetActiveWindow();
				if (!window) return false;

				auto cursor = window->GetBufferCursor();
				long currentPos = cursor.Index();

				// Search backward from current position
				std::string searchText = currentSearchTerm;
				if (!searchCaseSensitive) {
					std::transform(searchText.begin(), searchText.end(), searchText.begin(), ::tolower);
					std::transform(text.begin(), text.end(), text.begin(), ::tolower);
				}

				// Find last occurrence before current position
				size_t found = std::string::npos;
				size_t pos = 0;
				while ((pos = text.find(searchText, pos)) != std::string::npos && pos < currentPos) {
					found = pos;
					pos++;
				}

				if (found != std::string::npos) {
					auto& buffer = window->GetBuffer();
					auto foundIter = buffer.Begin() + found;
					window->SetBufferCursor(foundIter);
					return true;
				}

				// If not found backward, search from end
				found = text.rfind(searchText);
				if (found != std::string::npos && found > currentPos) {
					auto& buffer = window->GetBuffer();
					auto foundIter = buffer.Begin() + found;
					window->SetBufferCursor(foundIter);
					return true;
				}
			}
			catch (const std::exception& e) {
				std::cerr << "Error in FindPrevious: " << e.what() << std::endl;
			}
			return false;
		}

		bool LoadFile(const std::string& filePath) {
			if (editor && isInitialized) {
				try {
					auto* buffer = editor->GetFileBuffer(filePath);
					auto* tabWindow = editor->GetActiveTabWindow();
					if (tabWindow) {
						auto* window = tabWindow->GetActiveWindow();
						if (window && buffer) {
							window->SetBuffer(buffer);
							return true;
						}
					}
				}
				catch (const std::exception& e) {
					std::cerr << "Error loading file: " << e.what() << std::endl;
				}
			}
			return false;
		}

		bool SaveFile(const std::string& filePath) {
			if (editor && isInitialized) {
				try {
					auto* tabWindow = editor->GetActiveTabWindow();
					if (tabWindow) {
						auto* window = tabWindow->GetActiveWindow();
						if (window) {
							auto& buffer = window->GetBuffer();
							buffer.SetFilePath(filePath);
							int64_t size = 0;
							buffer.Save(size);
							return true;
						}
					}
				}
				catch (const std::exception& e) {
					std::cerr << "Error saving file: " << e.what() << std::endl;
				}
			}
			return false;
		}

		void TestClipboard() {
			const char* text = ImGui::GetClipboardText();
			std::cout << "Clipboard contains: " << (text ? text : "NULL") << std::endl;
		}

		// Required by IZepComponent interface
		Zep::ZepEditor& GetEditor() const override {
			return *editor;
		}

		uintptr_t GetInstanceId() const { return instanceId; }

		// Handle clipboard messages - only if focused
		void Notify(std::shared_ptr<Zep::ZepMessage> message) override {
			if (!ZepFocusTracker::IsFocused(instanceId)) {
				return;
			}

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
		void SetupClipboardCallbacks() {
			if (editor) {
				editor->RegisterCallback(this);
			}
		}

		void SetupInitialBuffer() {
			try {
				auto* pNewTabWindow = editor->AddTabWindow();
				std::string bufferName = "buffer_" + std::to_string(instanceId) + ".txt";
				auto* pBuffer = editor->GetEmptyBuffer(bufferName);
				pBuffer->SetText(lastKnownText.empty() ? "" : lastKnownText);

				auto* pTabWindow = editor->GetActiveTabWindow();
				if (pTabWindow) {
					auto* pWindow = pTabWindow->GetActiveWindow();
					if (pWindow) {
						pWindow->SetBuffer(pBuffer);
					}
				}
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in SetupInitialBuffer: " << e.what() << std::endl;
			}
		}

		void ApplyCurrentConfiguration() {
			if (!editor || !isInitialized) {
				return;
			}

			try {
				// Apply window flags based on configuration
				auto* tabWindow = editor->GetActiveTabWindow();
				if (tabWindow) {
					auto* window = tabWindow->GetActiveWindow();
					if (window) {
						uint32_t flags = window->GetWindowFlags();

						// Apply line numbers
						if (showLineNumbers) {
							flags |= Zep::WindowFlags::ShowLineNumbers;
						}
						else {
							flags &= ~Zep::WindowFlags::ShowLineNumbers;
						}

						// Apply whitespace display
						if (showWhitespace) {
							flags |= Zep::WindowFlags::ShowWhiteSpace;
						}
						else {
							flags &= ~Zep::WindowFlags::ShowWhiteSpace;
						}

						// Apply word wrap
						if (wordWrap) {
							flags |= Zep::WindowFlags::WrapText;
						}
						else {
							flags &= ~Zep::WindowFlags::WrapText;
						}

						window->SetWindowFlags(flags);
					}
				}

				std::cout << "Applied configuration: LineNumbers=" << showLineNumbers
					<< ", WordWrap=" << wordWrap
					<< ", ReadOnly=" << readOnly
					<< ", Theme=" << currentTheme << std::endl;

			}
			catch (const std::exception& e) {
				std::cerr << "Exception applying configuration: " << e.what() << std::endl;
			}
		}

		void RenderSearchBox() {
			ImGui::PushID(("search_" + std::to_string(instanceId)).c_str());

			if (ImGui::BeginChild("SearchBox", ImVec2(0, 30), true, ImGuiWindowFlags_NoScrollbar)) {
				// Search input
				char searchBuffer[256];
				strncpy(searchBuffer, currentSearchTerm.c_str(), sizeof(searchBuffer) - 1);
				searchBuffer[sizeof(searchBuffer) - 1] = '\0';

				ImGui::SetNextItemWidth(200);
				if (ImGui::InputText("##search", searchBuffer, sizeof(searchBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
					currentSearchTerm = searchBuffer;
					FindNext();
				}

				ImGui::SameLine();

				// Find Next button
				if (ImGui::Button("Next")) {
					FindNext();
				}

				ImGui::SameLine();

				// Find Previous button
				if (ImGui::Button("Prev")) {
					FindPrevious();
				}

				ImGui::SameLine();

				// Case sensitive checkbox
				ImGui::Checkbox("Case", &searchCaseSensitive);

				ImGui::SameLine();

				// Regex checkbox
				ImGui::Checkbox("Regex", &searchRegex);

				ImGui::SameLine();

				// Close button
				if (ImGui::Button("X")) {
					showSearchBox = false;
				}
			}
			ImGui::EndChild();

			ImGui::PopID();
		}
	};

} // namespace Utils