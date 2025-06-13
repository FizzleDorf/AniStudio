#pragma once
#include <zep/editor.h>
#include <zep/imgui/editor_imgui.h>
#include <zep/mode_standard.h>
#include <memory>
#include <string>
#include <filesystem>
#include <imgui.h>
#include <iostream>

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
	private:
		std::unique_ptr<Zep::ZepEditor_ImGui> editor;
		uintptr_t instanceId;
		bool isInitialized;

		// Configuration state
		bool showLineNumbers = true;
		bool wordWrap = false;
		bool readOnly = false;
		bool showWhitespace = false;
		bool enableSyntaxHighlighting = true;
		bool autoIndent = true;
		std::string currentTheme = "dark";
		bool showSearchBox = false;
		std::string currentSearchTerm;

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

				// Register callback like demo
				editor->RegisterCallback(this);

				// Initialize with text like demo
				editor->InitWithText("buffer.txt", "");

				// Force standard mode like demo
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

		// EXACT demo pattern - no child windows, no bullshit
		void Render(ImVec2 position, ImVec2 size) {
			if (!editor || !isInitialized) {
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Editor not initialized");
				return;
			}

			// Handle focus
			bool isFocused = ZepFocusTracker::IsFocused(instanceId);
			if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
				ZepFocusTracker::SetFocused(instanceId);
				isFocused = true;
			}
			editor->isFocused = isFocused;

			// Get display region EXACTLY like demo
			auto min = ImGui::GetCursorScreenPos();
			auto max = ImGui::GetContentRegionAvail();
			max.x = std::max(1.0f, max.x);
			max.y = std::max(1.0f, max.y);

			// Fill the region EXACTLY like demo
			max.x = min.x + max.x;
			max.y = min.y + max.y;
			editor->SetDisplayRegion(Zep::NVec2f(min.x, min.y), Zep::NVec2f(max.x, max.y));

			// EXACT demo order: Display FIRST, HandleInput SECOND
			editor->Display();
			if (isFocused) {
				editor->HandleInput();
			}
		}

		// Simple menu bar
		void RenderMenuBar() {
			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu("File")) {
					if (ImGui::MenuItem("New", "Ctrl+N")) {
						SetText("");
					}
					if (ImGui::MenuItem("Save", "Ctrl+S")) {
						// TODO: Save functionality
					}
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Options")) {
					if (ImGui::MenuItem("Line Numbers", nullptr, showLineNumbers)) {
						SetShowLineNumbers(!showLineNumbers);
					}
					if (ImGui::MenuItem("Word Wrap", nullptr, wordWrap)) {
						SetWordWrap(!wordWrap);
					}
					if (ImGui::MenuItem("Show Whitespace", nullptr, showWhitespace)) {
						SetShowWhitespace(!showWhitespace);
					}
					if (ImGui::MenuItem("Syntax Highlighting", nullptr, enableSyntaxHighlighting)) {
						SetSyntaxHighlighting(!enableSyntaxHighlighting);
					}
					ImGui::EndMenu();
				}

				ImGui::EndMenuBar();
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

		void SetShowMenuBar(bool show) {
			// Compatibility method
		}

		// Getters
		bool GetShowLineNumbers() const { return showLineNumbers; }
		bool GetWordWrap() const { return wordWrap; }
		bool GetReadOnly() const { return readOnly; }
		bool GetShowWhitespace() const { return showWhitespace; }
		bool GetSyntaxHighlighting() const { return enableSyntaxHighlighting; }
		bool GetAutoIndent() const { return autoIndent; }
		std::string GetTheme() const { return currentTheme; }
		bool GetShowMenuBar() const { return true; }

		// Text methods
		std::string GetText() const {
			if (editor && isInitialized) {
				auto* buffer = editor->GetActiveBuffer();
				if (buffer) {
					return buffer->GetWorkingBuffer().string();
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

		void SetMode(const std::string& mode) {
			if (editor && isInitialized) {
				editor->SetGlobalMode(Zep::ZepMode_Standard::StaticName());
			}
		}

		// Search functionality
		void SetSearchTerm(const std::string& term) { currentSearchTerm = term; }
		std::string GetSearchTerm() const { return currentSearchTerm; }
		void SetSearchCaseSensitive(bool caseSensitive) { }
		bool GetSearchCaseSensitive() const { return false; }
		void SetSearchRegex(bool regex) { }
		bool GetSearchRegex() const { return false; }
		void ShowSearchBox(bool show) { showSearchBox = show; }
		bool IsSearchBoxVisible() const { return showSearchBox; }

		bool CopySelection() { return true; }
		bool PasteFromClipboard() { return true; }
		bool FindNext() { return false; }
		bool FindPrevious() { return false; }
		bool LoadFile(const std::string& filePath) { return false; }
		bool SaveFile(const std::string& filePath) { return false; }
		void TestClipboard() { }

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
	};

} // namespace Utils