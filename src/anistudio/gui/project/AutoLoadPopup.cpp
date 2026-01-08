#include "AutoLoadPopup.hpp"
#include <filesystem>
#include <iostream>

namespace GUI {

	bool AutoLoadPopup::ShouldShow(const std::string& lastProjectPath) {
		if (lastProjectPath.empty()) {
			return false;
		}

		// Check if the project actually exists
		std::filesystem::path projectFile = std::filesystem::path(lastProjectPath) / "project.ani";
		return std::filesystem::exists(projectFile);
	}

	std::string AutoLoadPopup::GetProjectNameFromPath(const std::string& path) {
		std::filesystem::path fsPath(path);
		std::string name = fsPath.filename().string();

		if (name.empty() && !path.empty()) {
			// Try to get parent directory name
			fsPath = fsPath.parent_path();
			name = fsPath.filename().string();
		}

		return name.empty() ? "Untitled Project" : name;
	}

	bool AutoLoadPopup::Show(AutoLoadPopupState& state) {
		if (!state.showPopup) {
			return false;
		}

		bool result = false;

		// Center the popup
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_Appearing);

		// Modal popup flags
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_Modal;

		if (ImGui::BeginPopupModal("Auto-load Project?##AutoLoadPopup", &state.showPopup, flags)) {
			ImGui::Text("Welcome to AniStudio");
			ImGui::Separator();

			ImGui::Spacing();
			ImGui::Text("You have a previously opened project:");

			// Project info box
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.5f));
			if (ImGui::BeginChild("ProjectInfo", ImVec2(0, 80), true)) {
				ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "%s", state.lastProjectName.c_str());
				ImGui::TextWrapped("%s", state.lastProjectPath.c_str());
			}
			ImGui::EndChild();
			ImGui::PopStyleColor();

			ImGui::Spacing();
			ImGui::Text("Would you like to automatically load this project?");
			ImGui::TextDisabled("(You can change this behavior in Settings)");

			ImGui::Spacing();
			ImGui::Separator();

			// Buttons
			float buttonWidth = 150.0f;
			float buttonHeight = 35.0f;
			float spacing = 10.0f;
			float totalWidth = buttonWidth * 2 + spacing;
			float startX = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;

			ImGui::SetCursorPosX(startX);

			// Yes button (auto-load)
			if (ImGui::Button("Yes, Load Project", ImVec2(buttonWidth, buttonHeight))) {
				state.shouldAutoLoad = true;
				state.userChoiceMade = true;
				state.showPopup = false;
				result = true; // User chose to auto-load
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine(0, spacing);

			// No button (show startup view)
			if (ImGui::Button("No, Show Projects", ImVec2(buttonWidth, buttonHeight))) {
				state.shouldAutoLoad = false;
				state.userChoiceMade = true;
				state.showPopup = false;
				result = false; // User chose not to auto-load
				ImGui::CloseCurrentPopup();
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::EndPopup();
		}

		// Handle ESC key
		if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			state.shouldAutoLoad = false;
			state.userChoiceMade = true;
			state.showPopup = false;
			result = false;
		}

		return result;
	}

} // namespace GUI