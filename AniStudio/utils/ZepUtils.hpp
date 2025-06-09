#pragma once
#include <zep/editor.h>
#include <zep/imgui/editor_imgui.h>
#include <memory>
#include <string>
#include <filesystem>
#include <imgui.h>
#include <iostream>

namespace Utils {

	class ZepTextEditor : public Zep::IZepComponent {
	public:
		ZepTextEditor() = default;
		~ZepTextEditor() = default;

		bool Initialize() {
			try {
				std::cout << "ZepTextEditor::Initialize() starting..." << std::endl;

				// Get the file system path - use current directory if no specific path needed
				auto rootPath = std::filesystem::current_path();

				// Initialize Zep editor with proper settings
				editor = std::make_shared<Zep::ZepEditor_ImGui>(
					rootPath,                                     // root path
					Zep::NVec2f(1.0f, 1.0f),                     // pixel scale
					Zep::ZepEditorFlags::DisableThreads,         // flags
					nullptr                                       // file system (use default)
					);

				std::cout << "ZepEditor_ImGui created successfully" << std::endl;

				// Set up clipboard callbacks for copy/paste functionality
				SetupClipboardCallbacks();

				// Create initial buffer and window setup
				SetupInitialBuffer();

				std::cout << "ZepTextEditor::Initialize() completed successfully" << std::endl;
				return true;
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in ZepTextEditor::Initialize(): " << e.what() << std::endl;
				return false;
			}
			catch (...) {
				std::cerr << "Unknown exception in ZepTextEditor::Initialize()" << std::endl;
				return false;
			}
		}

		void Render(ImVec2 position, ImVec2 size) {
			if (!editor) {
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Editor not initialized");
				return;
			}

			try {
				// Set the display region for Zep using screen coordinates
				editor->SetDisplayRegion(
					Zep::NVec2f(position.x, position.y),
					Zep::NVec2f(position.x + size.x, position.y + size.y)
				);

				// Reserve the space for Zep
				ImGui::Dummy(size);

				// Handle mouse input manually
				HandleMouseInput(position, size);

				// Handle input - this should be called after setting display region
				editor->HandleInput();

				// Display the editor
				editor->Display();
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in ZepTextEditor::Render(): " << e.what() << std::endl;
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Render Error: %s", e.what());
			}
			catch (...) {
				std::cerr << "Unknown exception in ZepTextEditor::Render()" << std::endl;
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Unknown render error");
			}
		}

		void SetMode(const std::string& mode) {
			if (editor) {
				editor->SetGlobalMode(mode);
				std::cout << "Switched to " << mode << " mode" << std::endl;
			}
		}

		std::string GetText() const {
			if (editor) {
				auto* tabWindow = editor->GetActiveTabWindow();
				if (tabWindow) {
					auto* window = tabWindow->GetActiveWindow();
					if (window) {
						auto& buffer = window->GetBuffer();
						return buffer.GetWorkingBuffer().string();
					}
				}
			}
			return "";
		}

		void SetText(const std::string& text) {
			if (editor) {
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

		bool LoadFile(const std::string& filePath) {
			if (editor) {
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
			if (editor) {
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

		// Override the Notify method to handle clipboard messages from Zep
		void Notify(std::shared_ptr<Zep::ZepMessage> message) override {
			if (message->messageId == Zep::Msg::GetClipBoard) {
				// Get clipboard content from ImGui/GLFW
				const char* clipboardText = ImGui::GetClipboardText();
				if (clipboardText) {
					message->str = std::string(clipboardText);
					message->handled = true;
					std::cout << "Clipboard read: " << message->str.substr(0, 50) << (message->str.length() > 50 ? "..." : "") << std::endl;
				}
			}
			else if (message->messageId == Zep::Msg::SetClipBoard) {
				// Set clipboard content through ImGui/GLFW
				ImGui::SetClipboardText(message->str.c_str());
				message->handled = true;
				std::cout << "Clipboard set: " << message->str.substr(0, 50) << (message->str.length() > 50 ? "..." : "") << std::endl;
			}
		}

	private:
		std::shared_ptr<Zep::ZepEditor_ImGui> editor;

		void SetupClipboardCallbacks() {
			// Set up clipboard integration for Zep
			if (editor) {
				// Register clipboard callbacks with Zep
				editor->RegisterCallback(this);

				std::cout << "Clipboard callbacks registered with Zep" << std::endl;

				// Test if ImGui clipboard is working
				ImGuiIO& io = ImGui::GetIO();
				if (io.GetClipboardTextFn && io.SetClipboardTextFn) {
					std::cout << "ImGui clipboard integration confirmed" << std::endl;
				}
				else {
					std::cout << "WARNING: ImGui clipboard not available" << std::endl;
				}
			}
		}

		void SetupInitialBuffer() {
			try {
				// Ensure we have a tab window first
				std::cout << "Creating/ensuring tab window..." << std::endl;
				auto* pNewTabWindow = editor->AddTabWindow();
				std::cout << "Tab window created: " << pNewTabWindow << std::endl;

				// Create a buffer with initial text
				auto* pBuffer = editor->GetEmptyBuffer("welcome.cpp");
				std::cout << "Got empty buffer: " << pBuffer << std::endl;

				pBuffer->SetText("// Welcome to the Zep text editor!\n// This is integrated with your ECS system\n\nvoid main() {\n    // Start coding...\n}");
				std::cout << "Set buffer text" << std::endl;

				// Set the active buffer
				std::cout << "Getting active tab window..." << std::endl;
				auto* pTabWindow = editor->GetActiveTabWindow();
				std::cout << "Got active tab window: " << pTabWindow << std::endl;

				if (pTabWindow) {
					std::cout << "Getting active window from tab..." << std::endl;
					auto* pWindow = pTabWindow->GetActiveWindow();
					std::cout << "Got active window: " << pWindow << std::endl;

					if (pWindow) {
						pWindow->SetBuffer(pBuffer);
						std::cout << "Set buffer to window" << std::endl;
					}
					else {
						std::cout << "ERROR: Active window is null!" << std::endl;
					}
				}
				else {
					std::cout << "ERROR: Active tab window is null!" << std::endl;
				}
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in SetupInitialBuffer: " << e.what() << std::endl;
			}
		}

		void HandleMouseInput(ImVec2 position, ImVec2 size) {
			// Check if mouse is over our region
			ImVec2 mousePos = ImGui::GetMousePos();
			bool mouseInRegion = (mousePos.x >= position.x && mousePos.x <= position.x + size.x &&
				mousePos.y >= position.y && mousePos.y <= position.y + size.y);

			// Handle mouse input manually
			if (mouseInRegion) {
				ImGuiIO& io = ImGui::GetIO();

				// Convert mouse position to Zep coordinates
				Zep::NVec2f zepMouse(mousePos.x - position.x, mousePos.y - position.y);

				// Handle mouse events
				if (ImGui::IsMouseClicked(0)) {
					editor->OnMouseDown(zepMouse, Zep::ZepMouseButton::Left);
					std::cout << "Mouse down at: " << zepMouse.x << ", " << zepMouse.y << std::endl;
				}
				if (ImGui::IsMouseReleased(0)) {
					editor->OnMouseUp(zepMouse, Zep::ZepMouseButton::Left);
				}
				if (ImGui::IsMouseDragging(0)) {
					editor->OnMouseMove(zepMouse);
				}

				// Handle scroll wheel
				if (io.MouseWheel != 0.0f) {
					editor->OnMouseWheel(zepMouse, io.MouseWheel);
				}
			}
		}
	};

} // namespace Utils