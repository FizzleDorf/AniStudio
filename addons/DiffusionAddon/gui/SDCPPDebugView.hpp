#pragma once

#include "BaseView.hpp"
#include "SDcppSystem.hpp"
#include <imgui.h>
#include <vector>
#include <string>

namespace GUI {

	class SDCPPDebugView : public BaseView {
	public:
		SDCPPDebugView(ECS::EntityManager &entityMgr) : BaseView(entityMgr) {
			viewName = "SDCPP Debug";
		}

		~SDCPPDebugView() override = default;

		static constexpr const char* GetMetadataJSON() {
			return R"({
                "displayName": "SDCPP Debug",
                "category": "System",
                "description": "SDCPP System debugging and model management."
            })";
		}

		void Render() override {
			if (!windowOpen) return;

			ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
			if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen)) {
				auto sdSystem = mgr.GetSystem<ECS::SDCPPSystem>();
				if (!sdSystem) {
					ImGui::Text("SDCPPSystem not available!");
					ImGui::End();
					return;
				}

				RenderSystemStatus(sdSystem.get());
				ImGui::Separator();
				RenderModelManagement(sdSystem.get());
				ImGui::Separator();
				RenderQueueManagement(sdSystem.get());
				ImGui::Separator();
				RenderContextCache(sdSystem.get());
			}
			ImGui::End();
		}

	private:
		void RenderSystemStatus(ECS::SDCPPSystem* sdSystem) {
			if (ImGui::CollapsingHeader("System Status", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Text("Queue Size: %zu", sdSystem->GetQueueSize());
				ImGui::Text("Has Active Task: %s", sdSystem->HasActiveTask() ? "Yes" : "No");

				// Try to get context stats if available
				try {
					auto[totalCached, inUse, available] = sdSystem->GetSDContextStats();
					ImGui::Text("Cached Contexts: %zu", totalCached);
					ImGui::Text("In Use: %zu", inUse);
					ImGui::Text("Available: %zu", available);
				}
				catch (...) {
					ImGui::Text("Context stats not available");
				}

				// Try to get model loading stats if available
				try {
					auto[loadingCount, failedCount] = sdSystem->GetModelLoadingStats();
					ImGui::Text("Loading Models: %zu", loadingCount);
					ImGui::Text("Failed Loads: %zu", failedCount);
				}
				catch (...) {
					ImGui::Text("Model loading stats not available");
				}
			}
		}

		void RenderModelManagement(ECS::SDCPPSystem* sdSystem) {
			if (ImGui::CollapsingHeader("Model Management", ImGuiTreeNodeFlags_DefaultOpen)) {
				// Multiple models setting
				try {
					bool allowMultiple = sdSystem->GetAllowMultipleModels();
					if (ImGui::Checkbox("Allow Multiple Loaded Models", &allowMultiple)) {
						sdSystem->SetAllowMultipleModels(allowMultiple);
					}
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("When disabled, old models are automatically unloaded when new ones are requested.");
					}
				}
				catch (...) {
					// Method not available
				}

				// Auto-unload setting
				try {
					bool autoUnload = sdSystem->GetAutoUnloadOldModels();
					if (ImGui::Checkbox("Auto-unload Old Models", &autoUnload)) {
						sdSystem->SetAutoUnloadOldModels(autoUnload);
					}
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Automatically unload models when they're no longer in use.");
					}
				}
				catch (...) {
					// Method not available
				}

				// Max cache size
				try {
					int maxCache = static_cast<int>(sdSystem->GetMaxModelCache());
					if (ImGui::SliderInt("Max Model Cache", &maxCache, 1, 10)) {
						sdSystem->SetMaxModelCache(static_cast<size_t>(maxCache));
					}
				}
				catch (...) {
					// Method not available
				}

				ImGui::Separator();

				// Loaded models list
				try {
					auto loadedModels = sdSystem->GetLoadedModels();
					if (loadedModels.empty()) {
						ImGui::Text("No models loaded");
					}
					else {
						ImGui::Text("Loaded Models:");
						for (const auto& model : loadedModels) {
							ImGui::BulletText("%s", model.c_str());
						}
					}
				}
				catch (...) {
					ImGui::Text("Loaded models list not available");
				}

				ImGui::Separator();

				// Manual model unload controls
				static char modelPathToUnload[256] = "";
				ImGui::InputText("Model Path to Unload", modelPathToUnload, sizeof(modelPathToUnload));
				ImGui::SameLine();
				if (ImGui::Button("Unload Specific Model")) {
					if (strlen(modelPathToUnload) > 0) {
						try {
							sdSystem->UnloadModel(modelPathToUnload);
							modelPathToUnload[0] = '\0';
						}
						catch (...) {
							ImGui::SetTooltip("Failed to unload model");
						}
					}
				}

				if (ImGui::Button("Unload All Models")) {
					try {
						sdSystem->UnloadAllModels();
					}
					catch (...) {
						ImGui::SetTooltip("Failed to unload all models");
					}
				}

				ImGui::SameLine();
				if (ImGui::Button("Force Model Reload")) {
					try {
						sdSystem->ForceModelReload();
					}
					catch (...) {
						ImGui::SetTooltip("Failed to force model reload");
					}
				}
			}
		}

		void RenderQueueManagement(ECS::SDCPPSystem* sdSystem) {
			if (ImGui::CollapsingHeader("Queue Management")) {
				try {
					auto queueSnapshot = sdSystem->GetQueueSnapshot();

					if (queueSnapshot.empty()) {
						ImGui::Text("Queue is empty");
					}
					else {
						ImGui::Text("Queued Tasks: %zu", queueSnapshot.size());
						if (ImGui::BeginTable("TaskQueue", 4,
							ImGuiTableFlags_Borders |
							ImGuiTableFlags_RowBg |
							ImGuiTableFlags_ScrollY)) {
							ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed, 50.0f);
							ImGui::TableSetupColumn("Entity ID", ImGuiTableColumnFlags_WidthFixed, 80.0f);
							ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 100.0f);
							ImGui::TableSetupColumn("Task Type", ImGuiTableColumnFlags_WidthStretch);
							ImGui::TableHeadersRow();

							for (size_t i = 0; i < queueSnapshot.size(); ++i) {
								const auto& item = queueSnapshot[i];

								ImGui::TableNextRow();
								ImGui::TableSetColumnIndex(0);
								ImGui::Text("%zu", i);

								ImGui::TableSetColumnIndex(1);
								ImGui::Text("%u", item.entityID);

								ImGui::TableSetColumnIndex(2);
								if (item.processing) {
									ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Processing");
								}
								else {
									ImGui::Text("Queued");
								}

								ImGui::TableSetColumnIndex(3);
								ImGui::Text("%s", TaskTypeToString(item.taskType).c_str());
							}

							ImGui::EndTable();
						}
					}

					ImGui::Separator();

					if (ImGui::Button("Clear Queue")) {
						try {
							sdSystem->ClearQueue();
						}
						catch (...) {
							ImGui::SetTooltip("Failed to clear queue");
						}
					}

					ImGui::SameLine();

					if (sdSystem->HasActiveTask()) {
						if (ImGui::Button("Stop Current Task")) {
							try {
								sdSystem->StopCurrentTask();
							}
							catch (...) {
								ImGui::SetTooltip("Failed to stop current task");
							}
						}
					}
					else {
						ImGui::BeginDisabled();
						ImGui::Button("Stop Current Task");
						ImGui::EndDisabled();
					}

					ImGui::SameLine();
					if (ImGui::Button("Pause Worker")) {
						try {
							sdSystem->PauseWorker();
						}
						catch (...) {
							ImGui::SetTooltip("Failed to pause worker");
						}
					}

					ImGui::SameLine();
					if (ImGui::Button("Resume Worker")) {
						try {
							sdSystem->ResumeWorker();
						}
						catch (...) {
							ImGui::SetTooltip("Failed to resume worker");
						}
					}
				}
				catch (...) {
					ImGui::Text("Queue snapshot not available");
				}
			}
		}

		void RenderContextCache(ECS::SDCPPSystem* sdSystem) {
			if (ImGui::CollapsingHeader("Context Cache")) {
				if (ImGui::Button("List All Contexts")) {
					try {
						sdSystem->ListSDContexts();
					}
					catch (...) {
						ImGui::SetTooltip("Failed to list contexts");
					}
				}

				ImGui::SameLine();

				if (ImGui::Button("Clear All Contexts")) {
					try {
						sdSystem->ClearAllSDContexts();
					}
					catch (...) {
						ImGui::SetTooltip("Failed to clear contexts");
					}
				}

				ImGui::Separator();
				ImGui::Text("Thread Pool Stats:");

				try {
					ImGui::Text("  Threads: %zu", sdSystem->GetNumThreads());
				}
				catch (...) {
					ImGui::Text("  Threads: N/A");
				}

				try {
					ImGui::Text("  Queued Tasks: %zu", sdSystem->GetQueuedTaskCount());
				}
				catch (...) {
					ImGui::Text("  Queued Tasks: N/A");
				}

				try {
					ImGui::Text("  Active Tasks: %zu", sdSystem->GetActiveTaskCount());
				}
				catch (...) {
					ImGui::Text("  Active Tasks: N/A");
				}
			}
		}

		std::string TaskTypeToString(ECS::SDCPPSystem::TaskType type) {
			switch (type) {
			case ECS::SDCPPSystem::TaskType::Inference: return "txt2img";
			case ECS::SDCPPSystem::TaskType::Conversion: return "Conversion";
			case ECS::SDCPPSystem::TaskType::Img2Img: return "img2img";
			case ECS::SDCPPSystem::TaskType::Img2Vid: return "img2vid";
			case ECS::SDCPPSystem::TaskType::Edit: return "Edit";
			case ECS::SDCPPSystem::TaskType::Upscaling: return "Upscaling";
			default: return "Unknown";
			}
		}
	};
} // namespace GUI