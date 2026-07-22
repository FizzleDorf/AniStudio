#pragma once

#include "BaseView.hpp"
#include "SDcppSystem.hpp"
#include <imgui.h>
#include <string>
#include <vector>
#include <algorithm>

namespace GUI {

	class ModelCacheView : public BaseView {
	public:
		ModelCacheView(ECS::EntityManager& m_entityManager, ViewManager& vm)
			: BaseView(m_entityManager,vm)
			, maxCacheSize(3)
			, selectedModelIndex(-1)
			, showConfirmDialog(false)
			, confirmAction(ConfirmAction::None) {
			viewName = "Model Cache";
			strcpy(maxCacheInput, "3");
		}

		static constexpr const char* GetMetadataJSON() {
			return R"({
                "displayName": "Model Cache",
                "category": "System", 
                "description": "Manage loaded models and cache settings."
            })";
		}

		void Init() override {
			// Get initial cache size
			auto sdcppSystem = m_entityManager.GetSystem<ECS::SDCPPSystem>();
			if (sdcppSystem) {
				maxCacheSize = sdcppSystem->GetMaxModelCache();
				strcpy(maxCacheInput, std::to_string(maxCacheSize).c_str());
			}
		}

		void Update(float deltaT) override {
			// Refresh model list periodically (every 2 seconds)
			static float refreshTimer = 0.0f;
			refreshTimer += deltaT;
			if (refreshTimer >= 2.0f) {
				RefreshModelList();
				refreshTimer = 0.0f;
			}
		}

		void Render() override {
			if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen)) {
				RenderContent();
			}
			ImGui::End();

			// Render confirmation dialog if needed
			if (showConfirmDialog) {
				RenderConfirmationDialog();
			}
		}

		nlohmann::json Serialize() const override {
			auto j = BaseView::Serialize();
			j["maxCacheSize"] = maxCacheSize;
			j["selectedModelIndex"] = selectedModelIndex;
			return j;
		}

		void Deserialize(const nlohmann::json& j) override {
			BaseView::Deserialize(j);
			if (j.contains("maxCacheSize")) {
				maxCacheSize = j["maxCacheSize"];
				strcpy(maxCacheInput, std::to_string(maxCacheSize).c_str());
			}
			if (j.contains("selectedModelIndex")) {
				selectedModelIndex = j["selectedModelIndex"];
			}
		}

	private:
		enum class ConfirmAction {
			None,
			ClearAll,
			ClearSelected,
			UnloadAll
		};

		size_t maxCacheSize;
		std::vector<std::string> loadedModels;
		int selectedModelIndex;
		char maxCacheInput[16];
		bool showConfirmDialog;
		ConfirmAction confirmAction;
		std::string confirmMessage;

		void RefreshModelList() {
			auto sdcppSystem = m_entityManager.GetSystem<ECS::SDCPPSystem>();
			if (sdcppSystem) {
				loadedModels = sdcppSystem->GetLoadedModels();

				// Ensure selected index is valid
				if (selectedModelIndex >= static_cast<int>(loadedModels.size())) {
					selectedModelIndex = -1;
				}
			}
		}

		void RenderContent() {
			auto sdcppSystem = m_entityManager.GetSystem<ECS::SDCPPSystem>();
			if (!sdcppSystem) {
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
					"SDCPPSystem not available!");
				return;
			}

			ImGui::Separator();

			// Cache Configuration Section
			RenderCacheConfig(sdcppSystem);

			ImGui::Separator();

			// Loaded Models Section
			RenderLoadedModels(sdcppSystem);

			ImGui::Separator();

			// Cache Actions Section
			RenderCacheActions(sdcppSystem);
		}

		void RenderCacheConfig(std::shared_ptr<ECS::SDCPPSystem> sdcppSystem) {
			ImGui::Text("Cache Configuration");

			// Max cache size input
			ImGui::SetNextItemWidth(100);
			if (ImGui::InputText("Max Cache Size", maxCacheInput, sizeof(maxCacheInput),
				ImGuiInputTextFlags_CharsDecimal)) {
				// Validate input
				try {
					size_t newSize = std::stoul(maxCacheInput);
					if (newSize > 0 && newSize <= 20) { // Reasonable limit
						maxCacheSize = newSize;
					}
					else if (newSize > 20) {
						strcpy(maxCacheInput, "20");
						maxCacheSize = 20;
					}
				}
				catch (...) {
					// Keep current value on invalid input
					strcpy(maxCacheInput, std::to_string(maxCacheSize).c_str());
				}
			}
			ImGui::SameLine(); HelpMarker("Maximum number of models to keep in cache (1-20)");

			// Apply button
			ImGui::SameLine();
			if (ImGui::Button("Apply##CacheSize")) {
				sdcppSystem->SetMaxModelCache(maxCacheSize);
				ImGui::OpenPopup("CacheSizeApplied");
			}

			// Success popup
			if (ImGui::BeginPopup("CacheSizeApplied")) {
				ImGui::Text("Cache size updated to %zu", maxCacheSize);
				if (ImGui::Button("OK")) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			// Quick preset buttons
			ImGui::Text("Presets:");
			ImGui::SameLine();
			if (ImGui::Button("1 Model")) {
				maxCacheSize = 1;
				strcpy(maxCacheInput, "1");
				sdcppSystem->SetMaxModelCache(1);
			}
			ImGui::SameLine();
			if (ImGui::Button("3 Models")) {
				maxCacheSize = 3;
				strcpy(maxCacheInput, "3");
				sdcppSystem->SetMaxModelCache(3);
			}
			ImGui::SameLine();
			if (ImGui::Button("5 Models")) {
				maxCacheSize = 5;
				strcpy(maxCacheInput, "5");
				sdcppSystem->SetMaxModelCache(5);
			}
		}

		void RenderLoadedModels(std::shared_ptr<ECS::SDCPPSystem> sdcppSystem) {
			ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Loaded Models");

			if (loadedModels.empty()) {
				ImGui::Text("No models currently loaded");
				return;
			}

			// Model list with selectable items
			ImGui::BeginChild("ModelList", ImVec2(0, 200), true);

			for (size_t i = 0; i < loadedModels.size(); ++i) {
				// Create a selectable item for each model
				bool isSelected = (selectedModelIndex == static_cast<int>(i));

				// Display model with status indicator
				std::string displayText = loadedModels[i];

				// Add status prefix
				std::string statusPrefix = "  ";
				ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default white

				// Check if this model is currently in use (simplified check)
				if (displayText.find("[In Use]") != std::string::npos ||
					displayText.find("processing") != std::string::npos) {
					color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); // Green for active
					statusPrefix = "> ";
				}
				// Check if loading (simplified)
				else if (displayText.find("[Loading]") != std::string::npos) {
					color = ImVec4(1.0f, 0.8f, 0.4f, 1.0f); // Yellow for loading
					statusPrefix = "~ ";
				}

				ImGui::PushStyleColor(ImGuiCol_Text, color);
				if (ImGui::Selectable((statusPrefix + displayText).c_str(), isSelected)) {
					selectedModelIndex = static_cast<int>(i);
				}
				ImGui::PopStyleColor();

				// Context menu for each model
				if (ImGui::BeginPopupContextItem()) {
					if (ImGui::MenuItem("Unload Model")) {
						// Extract model path from display text
						std::string modelPath = ExtractModelPath(loadedModels[i]);
						if (!modelPath.empty()) {
							sdcppSystem->UnloadModel(modelPath);
							RefreshModelList();
						}
					}

					if (ImGui::MenuItem("Force Reload")) {
						// Force reload would require more context about which entity uses this model
						// For now, just unload and let it reload when needed
						std::string modelPath = ExtractModelPath(loadedModels[i]);
						if (!modelPath.empty()) {
							sdcppSystem->UnloadModel(modelPath);
							sdcppSystem->ForceModelReload();
							RefreshModelList();
						}
					}

					ImGui::EndPopup();
				}

				// Tooltip with more info
				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::Text("Model: %s", loadedModels[i].c_str());
					ImGui::Text("Status: %s",
						color.x == 0.4f ? "Active" :
						color.x == 1.0f ? "Idle" : "Loading");
					ImGui::EndTooltip();
				}
			}

			ImGui::EndChild();

			// Selected model info
			if (selectedModelIndex >= 0 && selectedModelIndex < static_cast<int>(loadedModels.size())) {
				ImGui::Text("Selected: %s", loadedModels[selectedModelIndex].c_str());
			}
			else {
				ImGui::Text("No model selected");
			}
		}

		void RenderCacheActions(std::shared_ptr<ECS::SDCPPSystem> sdcppSystem) {
			ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Cache Actions");

			// Unload selected model button
			if (ImGui::Button("Unload Selected Model")) {
				if (selectedModelIndex >= 0 && selectedModelIndex < static_cast<int>(loadedModels.size())) {
					std::string modelPath = ExtractModelPath(loadedModels[selectedModelIndex]);
					if (!modelPath.empty()) {
						sdcppSystem->UnloadModel(modelPath);
						RefreshModelList();
						selectedModelIndex = -1;
					}
				}
			}
			ImGui::SameLine(); HelpMarker("Unloads the selected model if not currently in use");

			// Unload all models button
			ImGui::SameLine();
			if (ImGui::Button("Unload All Models")) {
				ShowConfirmationDialog(
					ConfirmAction::UnloadAll,
					"Are you sure you want to unload ALL models?\n\n"
					"This will remove all cached models from memory.\n"
					"Any running tasks will continue with their current model."
				);
			}
			ImGui::SameLine(); HelpMarker("Unloads all models that are not currently in use");

			// Force unload idle models button
			if (ImGui::Button("Force Unload Idle Models")) {
				ShowConfirmationDialog(
					ConfirmAction::ClearAll,
					"Are you sure you want to force unload ALL idle models?\n\n"
					"This will remove ALL cached contexts, even if they're\n"
					"marked as in use (except for currently running tasks).\n"
					"Use with caution!"
				);
			}
			ImGui::SameLine(); HelpMarker("Forcefully unloads all idle models (aggressive cleanup)");

			// List contexts button
			ImGui::SameLine();
			if (ImGui::Button("List Contexts")) {
				sdcppSystem->ListSDContexts();
			}
			ImGui::SameLine(); HelpMarker("Prints detailed cache info to console");

			// Refresh button
			if (ImGui::Button("Refresh List")) {
				RefreshModelList();
			}
		}

		void RenderConfirmationDialog() {
			if (!showConfirmDialog) return;

			// Center the confirmation dialog
			ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Appearing);

			if (ImGui::Begin("Confirm Action", &showConfirmDialog,
				ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {

				ImGui::TextWrapped("%s", confirmMessage.c_str());
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.5f - 100);
				if (ImGui::Button("Confirm", ImVec2(100, 0))) {
					ExecuteConfirmedAction();
					showConfirmDialog = false;
				}

				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(100, 0))) {
					showConfirmDialog = false;
				}

				ImGui::End();
			}
		}

		void ShowConfirmationDialog(ConfirmAction action, const std::string& message) {
			confirmAction = action;
			confirmMessage = message;
			showConfirmDialog = true;
		}

		void ExecuteConfirmedAction() {
			auto sdcppSystem = m_entityManager.GetSystem<ECS::SDCPPSystem>();
			if (!sdcppSystem) return;

			switch (confirmAction) {
			case ConfirmAction::ClearAll:
				sdcppSystem->ForceUnloadIdleModels();
				break;
			case ConfirmAction::UnloadAll:
				sdcppSystem->UnloadAllModels();
				break;
			case ConfirmAction::ClearSelected:
				if (selectedModelIndex >= 0 && selectedModelIndex < static_cast<int>(loadedModels.size())) {
					std::string modelPath = ExtractModelPath(loadedModels[selectedModelIndex]);
					if (!modelPath.empty()) {
						sdcppSystem->UnloadModel(modelPath);
					}
				}
				break;
			default:
				break;
			}

			RefreshModelList();
		}

		std::string ExtractModelPath(const std::string& displayText) {
			// Extract model path from display text
			// Format: "model_path [Cache: key]" or "model_name [Cache: key]"

			// Find the cache bracket
			size_t cachePos = displayText.find(" [Cache:");
			if (cachePos != std::string::npos) {
				return displayText.substr(0, cachePos);
			}

			// If no cache info, return the whole string
			return displayText;
		}

		void HelpMarker(const char* desc) {
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
				ImGui::TextUnformatted(desc);
				ImGui::PopTextWrapPos();
				ImGui::EndTooltip();
			}
		}
	};

} // namespace GUI