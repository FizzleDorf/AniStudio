#include "ConvertView.hpp"
#include "Events.hpp"
#include "Constants.hpp"
#include "DiffusionCallbackUtils.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

using namespace ECS;
using namespace ANI;

namespace GUI {

	ConvertView::ConvertView(ECS::EntityManager &entityMgr, ImGuiContext* mainContext) : BaseView(entityMgr) {
		viewName = "ConvertView";
		windowOpen = true;
	}

	void ConvertView::Init() {
		GUI::DiffusionCallbackUtils::InitializeCallbacks();
	}

	void ConvertView::Render() {
		RenderQueueList();

		ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);

		if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen)) {
			if (ImGui::BeginTable("ModelLoaderTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("Model", ImGuiTableColumnFlags_WidthFixed, 52.0f);
				ImGui::TableSetupColumn("Load", ImGuiTableColumnFlags_WidthFixed, 52.0f);
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableHeadersRow();

				// Row for "Input Model"
				ImGui::TableNextColumn();
				ImGui::Text("Input Model");
				ImGui::TableNextColumn();
				if (ImGui::Button("...##j6")) {
					IGFD::FileDialogConfig config;
					config.path = Utils::FilePaths::checkpointDir;
					ImGuiFileDialog::Instance()->OpenDialog("ConvertModelDialog", "Choose Model",
						".safetensors, .ckpt, .pt, .gguf", config);
				}
				ImGui::SameLine();
				if (ImGui::Button("R##j6")) {
					modelComp.modelName = "";
					modelComp.modelPath = "";
				}
				ImGui::TableNextColumn();
				ImGui::Text("%s", modelComp.modelName.c_str());

				RenderVaeLoader();

				ImGui::EndTable();
			}

			if (ImGuiFileDialog::Instance()->Display("ConvertModelDialog", 32, ImVec2(700, 400))) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
					std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();

					modelComp.modelName = selectedFile;
					modelComp.modelPath = fullPath;
					std::cout << "Selected file: " << modelComp.modelName << std::endl;
					std::cout << "Full path: " << modelComp.modelPath << std::endl;
					std::cout << "New model path set: " << modelComp.modelPath << std::endl;
				}

				ImGuiFileDialog::Instance()->Close();
			}

			ImGui::Combo("Quant Type", reinterpret_cast<int *>(&samplerComp.current_type_method), type_method_items,
				type_method_item_count);

			if (ImGui::Button("Convert")) {
				Convert();
			}
		}
		ImGui::End();

		if (!windowOpen) {
			std::unordered_map<std::string, std::any> eventData;
			eventData["workspaceID"] = GetID();
			eventData["viewTypeName"] = viewName;
			ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
		}
	}

	void ConvertView::Convert() {
		auto sdSystem = mgr.GetSystem<ECS::SDCPPSystem>();
		if (!sdSystem) {
			mgr.RegisterSystem<ECS::SDCPPSystem>();
		}

		ECS::EntityID entity = mgr.AddNewEntity();
		mgr.AddComponent<ECS::ModelComponent>(entity);
		mgr.AddComponent<ECS::SamplerComponent>(entity);
		mgr.AddComponent<ECS::VaeComponent>(entity);

		mgr.GetComponent<ECS::ModelComponent>(entity) = modelComp;
		mgr.GetComponent<ECS::SamplerComponent>(entity) = samplerComp;
		mgr.GetComponent<ECS::VaeComponent>(entity) = vaeComp;

		auto taskData = std::make_pair(entity, ECS::SDCPPSystem::TaskType::Conversion);
		ANI::Events::Ref().QueueEventWithData("QueueDiffusionTask", taskData);
	}

	void ConvertView::RenderVaeLoader() {
		ImGui::TableNextColumn();
		ImGui::Text("Vae: ");
		ImGui::TableNextColumn();
		if (ImGui::Button("...##4b")) {
			IGFD::FileDialogConfig config;
			config.path = Utils::FilePaths::vaeDir;
			ImGuiFileDialog::Instance()->OpenDialog("ConvertVaeDialog", "Choose Model", ".safetensors, .ckpt, .pt, .gguf",
				config);
		}
		ImGui::SameLine();
		if (ImGui::Button("R##f7")) {
			vaeComp.modelName = "";
			vaeComp.modelPath = "";
		}
		ImGui::TableNextColumn();
		ImGui::Text("%s", vaeComp.modelName.c_str());

		if (ImGuiFileDialog::Instance()->Display("ConvertVaeDialog", 32, ImVec2(700, 400))) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
				std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();

				vaeComp.modelName = selectedFile;
				vaeComp.modelPath = fullPath;
				std::cout << "Selected file: " << vaeComp.modelName << std::endl;
				std::cout << "Full path: " << vaeComp.modelPath << std::endl;
				std::cout << "New model path set: " << vaeComp.modelPath << std::endl;
			}

			ImGuiFileDialog::Instance()->Close();
		}
	}

	void ConvertView::RenderQueueList() {
		ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Convert Queue")) {
			const auto& progressData = DiffusionCallbackUtils::GetProgressData();
			int currentStep = progressData.currentStep;
			int totalSteps = progressData.totalSteps;
			float time = progressData.currentTime;
			bool isProcessing = progressData.isProcessing;

			if (isProcessing && totalSteps > 0) {
				float progress = static_cast<float>(currentStep) / totalSteps;
				std::ostringstream ss;
				ss << "Processing: " << currentStep << "/" << totalSteps << " steps ("
					<< std::fixed << std::setprecision(1) << time << "s)";
				ImGui::Text("%s", ss.str().c_str());
				ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 0));
			}
			else {
				ImGui::Text("Waiting...");
				ImGui::ProgressBar(0.0f, ImVec2(-FLT_MIN, 0));
			}

			ImGui::Separator();

			// Queue count input
			static int numQueues = 1;
			if (ImGui::InputInt("Queue #", &numQueues, 1, 4)) {
				if (numQueues < 1) {
					numQueues = 1;
				}
			}

			if (ImGui::Button("Queue", ImVec2(-FLT_MIN, 0))) {
				for (int i = 0; i < numQueues; i++) {
					Convert();
				}
			}

			ImGui::Separator();

			// Control buttons
			static bool isPaused = false;
			if (isPaused) {
				if (ImGui::Button("Resume", ImVec2(-FLT_MIN, 0))) {
					ANI::Events::Ref().QueueEvent("ResumeDiffusionWorker");
					isPaused = false;
				}
			}
			else {
				if (ImGui::Button("Pause", ImVec2(-FLT_MIN, 0))) {
					ANI::Events::Ref().QueueEvent("PauseDiffusionWorker");
					isPaused = true;
				}
			}

			if (ImGui::Button("Stop", ImVec2(-FLT_MIN, 0))) {
				ANI::Events::Ref().QueueEvent("StopCurrentDiffusionTask");
			}

			if (ImGui::Button("Clear Queue", ImVec2(-FLT_MIN, 0))) {
				ANI::Events::Ref().QueueEvent("ClearDiffusionQueue");
			}

			ImGui::Separator();

			// Queue table with move/remove operations
			auto sdSystem = mgr.GetSystem<ECS::SDCPPSystem>();
			if (sdSystem && ImGui::BeginTable("Queue", 3,
				ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {

				ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 42.0f);
				ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 44.0f);
				ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableHeadersRow();

				auto queueItems = sdSystem->GetQueueSnapshot();
				for (size_t i = 0; i < queueItems.size(); i++) {
					const auto& item = queueItems[i];

					ImGui::TableNextRow();

					// ID column
					ImGui::TableNextColumn();
					ImGui::Text("%d", static_cast<int>(item.entityID));

					// Status column
					ImGui::TableNextColumn();
					if (item.processing) {
						ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Active");
					}
					else {
						ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "Queued");
					}

					// Controls column
					ImGui::TableNextColumn();
					if (!item.processing) {
						if (i > 0) {
							if (ImGui::ArrowButton(("up##" + std::to_string(i)).c_str(), ImGuiDir_Up)) {
								auto moveData = std::make_pair(i, i - 1);
								ANI::Events::Ref().QueueEventWithData("MoveInDiffusionQueue", moveData);
							}
							ImGui::SameLine();
						}

						if (i < queueItems.size() - 1) {
							if (ImGui::ArrowButton(("down##" + std::to_string(i)).c_str(), ImGuiDir_Down)) {
								auto moveData = std::make_pair(i, i + 1);
								ANI::Events::Ref().QueueEventWithData("MoveInDiffusionQueue", moveData);
							}
							ImGui::SameLine();
						}

						if (i > 0) {
							if (ImGui::Button(("Top##Top" + std::to_string(i)).c_str())) {
								size_t targetIndex = queueItems[0].processing ? 1 : 0;
								auto moveData = std::make_pair(i, targetIndex);
								ANI::Events::Ref().QueueEventWithData("MoveInDiffusionQueue", moveData);
							}
							ImGui::SameLine();
						}

						if (i < queueItems.size() - 1) {
							if (ImGui::Button(("Bottom##Bottom" + std::to_string(i)).c_str())) {
								auto moveData = std::make_pair(i, queueItems.size() - 1);
								ANI::Events::Ref().QueueEventWithData("MoveInDiffusionQueue", moveData);
							}
							ImGui::SameLine();
						}

						if (ImGui::Button(("X##Remove" + std::to_string(i)).c_str())) {
							ANI::Events::Ref().QueueEventWithData("RemoveFromDiffusionQueue", i);
						}
					}
				}
				ImGui::EndTable();
			}
		}
		ImGui::End();
	}

} // namespace GUI