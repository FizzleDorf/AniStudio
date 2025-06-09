#pragma once
#include "Base/BaseView.hpp"
#include "EntityManager.hpp"
#include "ZepUtils.hpp" // Include our utility
#include <memory>
#include <string>

namespace GUI {

	class ZepView : public BaseView {
	public:
		ZepView(ECS::EntityManager& entityMgr) : BaseView(entityMgr) {
			viewName = "Text Editor";
			textEditor = std::make_unique<Utils::ZepTextEditor>();
		}

		void Init() override {
			if (!textEditor->Initialize()) {
				std::cerr << "Failed to initialize ZepTextEditor" << std::endl;
			}
		}

		void Update(const float deltaT) override {
			// Update method should only handle logic updates, not rendering
			// Zep handles its own updates during the Display call
		}

		void Render() override {
			if (ImGui::Begin(viewName.c_str())) {
				// Control buttons
				RenderControls();

				// Get the available content region for the editor
				ImVec2 contentSize = ImGui::GetContentRegionAvail();
				ImVec2 editorPos = ImGui::GetCursorScreenPos();

				// Ensure we have a minimum size
				if (contentSize.x < 100) contentSize.x = 100;
				if (contentSize.y < 100) contentSize.y = 100;

				// Render the text editor
				textEditor->Render(editorPos, contentSize);

				// Show help text
				RenderHelpText();
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

		void RenderControls() {
			// Mode switching buttons
			if (ImGui::Button("Standard Mode")) {
				textEditor->SetMode("Standard");
			}
			ImGui::SameLine();
			if (ImGui::Button("Vim Mode")) {
				textEditor->SetMode("Vim");
			}
			ImGui::SameLine();
			if (ImGui::Button("Test Clipboard")) {
				textEditor->TestClipboard();
			}

			// File operations
			ImGui::SameLine();
			if (ImGui::Button("Clear Text")) {
				textEditor->SetText("");
			}

			ImGui::Separator();
		}

		void RenderHelpText() {
			// Position help text at the bottom
			ImVec2 windowSize = ImGui::GetWindowSize();
			ImGui::SetCursorPos(ImVec2(5, windowSize.y - 25));
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
				"Standard: Ctrl+A/C/V | Vim: v+arrows, y, p | Click+drag to select");
		}
	};

} // namespace GUI