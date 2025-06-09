#pragma once
#include "Base/BaseView.hpp"
#include "EntityManager.hpp"
#include <zep/editor.h>
#include <zep/imgui/editor_imgui.h>
#include <memory>
#include <string>
#include <filesystem>

namespace GUI {

	class ZepView : public BaseView {
	public:
		ZepView(ECS::EntityManager& entityMgr) : BaseView(entityMgr) {
			viewName = "Text Editor";
		}

		void Init() override {
			try {
				std::cout << "ZepView::Init() starting..." << std::endl;

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

				// Create initial buffer and window setup
				SetupInitialBuffer();

				std::cout << "ZepView::Init() completed successfully" << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in ZepView::Init(): " << e.what() << std::endl;
			}
			catch (...) {
				std::cerr << "Unknown exception in ZepView::Init()" << std::endl;
			}
		}

		void Update(const float deltaT) override {
			// Update method should only handle logic updates, not rendering
			// Zep handles its own updates during the Display call
		}

		void Render() override {
			try {
				if (ImGui::Begin(viewName.c_str())) {
					if (editor) {
						// Get the available content region
						ImVec2 contentSize = ImGui::GetContentRegionAvail();
						ImVec2 windowPos = ImGui::GetCursorScreenPos();

						// Ensure we have a minimum size
						if (contentSize.x < 100) contentSize.x = 100;
						if (contentSize.y < 100) contentSize.y = 100;

						// Set the display region for Zep - this is the correct API
						editor->SetDisplayRegion(
							Zep::NVec2f(windowPos.x, windowPos.y),
							Zep::NVec2f(windowPos.x + contentSize.x, windowPos.y + contentSize.y)
						);

						// Handle input first
						editor->HandleInput();

						// Then display the editor
						editor->Display();
					}
					else {
						ImGui::Text("Editor not initialized");
					}
				}
				ImGui::End();
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in ZepView::Render(): " << e.what() << std::endl;
				if (ImGui::Begin(viewName.c_str())) {
					ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Render Error: %s", e.what());
				}
				ImGui::End();
			}
			catch (...) {
				std::cerr << "Unknown exception in ZepView::Render()" << std::endl;
				if (ImGui::Begin(viewName.c_str())) {
					ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Unknown render error");
				}
				ImGui::End();
			}
		}

		// Helper methods for getting/setting text
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

		void LoadFile(const std::string& filePath) {
			if (editor) {
				auto* buffer = editor->GetFileBuffer(filePath);
				auto* tabWindow = editor->GetActiveTabWindow();
				if (tabWindow) {
					auto* window = tabWindow->GetActiveWindow();
					if (window && buffer) {
						window->SetBuffer(buffer);
					}
				}
			}
		}

		void SaveFile(const std::string& filePath) {
			if (editor) {
				auto* tabWindow = editor->GetActiveTabWindow();
				if (tabWindow) {
					auto* window = tabWindow->GetActiveWindow();
					if (window) {
						auto& buffer = window->GetBuffer();
						buffer.SetFilePath(filePath);
						int64_t size = 0;
						buffer.Save(size); // Pass size by reference as required
					}
				}
			}
		}

	private:
		std::shared_ptr<Zep::ZepEditor_ImGui> editor;

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
	};

} // namespace GUI