#include "DiffusionView.hpp"
#include "Events.hpp"
#include "Constants.hpp"
#include "UISchema.hpp"
#include "PngMetadataUtils.hpp"
#include "ContextMenuUtils.hpp"
#include "DiffusionCallbackUtils.hpp"
#include "utils.h"
#include "SDcppSystem.hpp"
#include <ImGuiFileDialog.h>
#include <png.h>
#include <exiv2/exiv2.hpp>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace ECS;
using namespace ANI;

namespace GUI {

	DiffusionView::DiffusionView(EntityManager& entityMgr, ImGuiContext* mainContext) : BaseView(entityMgr) {
		viewName = "DiffusionView";
		windowOpen = true;
		contextMenuUtils = std::make_unique<Utils::ContextMenuUtils>(entityMgr);
	}

	DiffusionView::~DiffusionView() {
		if (txt2imgEntity != 0) mgr.DestroyEntity(txt2imgEntity);
		if (img2imgEntity != 0) mgr.DestroyEntity(img2imgEntity);
		if (editEntity != 0) mgr.DestroyEntity(editEntity);
	}

	void DiffusionView::Init() {
		ResetEntities();
	}

	void DiffusionView::ResetEntities() {
		if (txt2imgEntity != 0) mgr.DestroyEntity(txt2imgEntity);
		if (img2imgEntity != 0) mgr.DestroyEntity(img2imgEntity);
		if (editEntity != 0) mgr.DestroyEntity(editEntity);

		// Create entities with core diffusion components
		txt2imgEntity = CreateEntityWithComponents(false);
		img2imgEntity = CreateEntityWithComponents(true);
		editEntity = CreateEntityWithComponents(true);

		// Set default output paths for ALL template entities
		for (EntityID entity : {txt2imgEntity, img2imgEntity, editEntity}) {
			if (mgr.HasComponent<OutputImageComponent>(entity)) {
				auto& output = mgr.GetComponent<OutputImageComponent>(entity);
				output.filePath = Utils::FilePaths::defaultProjectPath;
				output.fileName = "AniStudio.png";
			}

			if (mgr.HasComponent<LoraComponent>(entity)) {
				auto& lora = mgr.GetComponent<LoraComponent>(entity);
				lora.modelPath = Utils::FilePaths::loraDir;
			}
		}

		// Set default denoise for img2img and edit
		if (mgr.HasComponent<SamplerComponent>(img2imgEntity)) {
			mgr.GetComponent<SamplerComponent>(img2imgEntity).denoise = 0.6f;
		}
		if (mgr.HasComponent<SamplerComponent>(editEntity)) {
			mgr.GetComponent<SamplerComponent>(editEntity).denoise = 0.6f;
		}
	}

	ECS::EntityID DiffusionView::CreateEntityWithComponents(bool includeInputImage) {
		EntityID entity = mgr.AddNewEntity();

		// Add core diffusion components (NO ESRGAN)
		mgr.AddComponent<ModelComponent>(entity);
		mgr.AddComponent<ClipLComponent>(entity);
		mgr.AddComponent<ClipGComponent>(entity);
		mgr.AddComponent<T5XXLComponent>(entity);
		mgr.AddComponent<DiffusionModelComponent>(entity);
		mgr.AddComponent<VaeComponent>(entity);
		mgr.AddComponent<LoraComponent>(entity);
		mgr.AddComponent<TaesdComponent>(entity);
		mgr.AddComponent<LatentComponent>(entity);
		mgr.AddComponent<SamplerComponent>(entity);
		mgr.AddComponent<GuidanceComponent>(entity);
		mgr.AddComponent<ClipSkipComponent>(entity);
		mgr.AddComponent<PromptComponent>(entity);
		mgr.AddComponent<LayerSkipComponent>(entity);
		mgr.AddComponent<OutputImageComponent>(entity);
		mgr.AddComponent<ControlNetComponent>(entity);
		mgr.AddComponent<EmbeddingComponent>(entity);
		mgr.AddComponent<ChromaComponent>(entity);

		if (includeInputImage) {
			mgr.AddComponent<InputImageComponent>(entity);
		}

		return entity;
	}

	bool DiffusionView::IsEntitySafeToUse(ECS::EntityID entity) const {
		return mgr.IsEntityValid(entity);
	}

	std::vector<std::string> DiffusionView::GetCategoryRenderOrder() const {
		return {
			"Models",
			"Sampling",
			"Image",
			"Advanced"
		};
	}

	void DiffusionView::RenderEntityComponents(const EntityID entity) {
		if (entity == 0 || !IsEntitySafeToUse(entity)) return;

		// Get all components and organize by category
		auto componentIds = mgr.GetEntityComponents(entity);
		std::map<std::string, std::vector<std::pair<ComponentTypeID, std::string>>> categorizedComponents;

		for (ComponentTypeID compId : componentIds) {
			std::string componentName = mgr.GetComponentNameById(compId);

			// Skip InputImageComponent for txt2img mode
			if (componentName == "InputImage" && currentMode == 0) {
				continue;
			}

			auto* component = mgr.GetComponentByIdConst(entity, compId);
			if (!component) continue;

			std::string category = component->compCategory.empty() ? "Uncategorized" : component->compCategory;
			categorizedComponents[category].emplace_back(compId, componentName);
		}

		// Special handling for Models category with Full/Split tabs
		auto modelsIt = categorizedComponents.find("Models");
		if (modelsIt != categorizedComponents.end()) {
			if (ImGui::CollapsingHeader("Model Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
				if (ImGui::BeginTabBar("ModelTabs")) {
					if (ImGui::BeginTabItem("Full")) {
						// Render checkpoint and VAE
						for (const auto&[compId, componentName] : modelsIt->second) {
							if (componentName == "Model" || componentName == "Vae") {
								RenderComponent(entity, compId, componentName);
							}
						}
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("Split")) {
						// UNet
						for (const auto&[compId, componentName] : modelsIt->second) {
							if (componentName == "DiffusionModel") {
								RenderComponent(entity, compId, componentName);
							}
						}

						// Text Encoders subcategory
						if (ImGui::CollapsingHeader("Text Encoders", ImGuiTreeNodeFlags_DefaultOpen)) {
							for (const auto&[compId, componentName] : modelsIt->second) {
								if (componentName == "ClipL" || componentName == "ClipG" || componentName == "T5XXL") {
									RenderComponent(entity, compId, componentName);
								}
							}
						}

						// VAE
						for (const auto&[compId, componentName] : modelsIt->second) {
							if (componentName == "Vae") {
								RenderComponent(entity, compId, componentName);
							}
						}
						ImGui::EndTabItem();
					}
					ImGui::EndTabBar();
				}
			}

			// Remove Models from categorizedComponents so it doesn't render again
			categorizedComponents.erase(modelsIt);
		}

		// Get the desired render order
		std::vector<std::string> renderOrder = GetCategoryRenderOrder();

		// First render categories in the specified order
		for (const auto& category : renderOrder) {
			auto it = categorizedComponents.find(category);
			if (it != categorizedComponents.end()) {
				if (ImGui::CollapsingHeader(category.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
					for (const auto&[compId, componentName] : it->second) {
						RenderComponent(entity, compId, componentName);
					}
				}
				// Remove from map so it doesn't render again
				categorizedComponents.erase(it);
			}
		}

		// Render remaining categories that weren't in the ordered list
		for (const auto&[category, components] : categorizedComponents) {
			if (ImGui::CollapsingHeader(category.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
				for (const auto&[compId, componentName] : components) {
					RenderComponent(entity, compId, componentName);
				}
			}
		}
	}

	void DiffusionView::RenderComponent(EntityID entity, ComponentTypeID compId, const std::string& componentName) {
		// Component visibility checkbox
		if (componentVisibility.find(componentName) == componentVisibility.end()) {
			componentVisibility[componentName] = true;
		}

		bool isVisible = componentVisibility[componentName];
		if (ImGui::Checkbox(componentName.c_str(), &isVisible)) {
			componentVisibility[componentName] = isVisible;
		}

		if (!isVisible) return;

		ImGui::Indent();

		// Store cursor position for context menu
		ImVec2 contentStart = ImGui::GetCursorScreenPos();

		// Get component and render its UI
		auto* component = mgr.GetComponentById(entity, compId);
		if (component && !component->schema.empty()) {
			try {
				auto properties = component->GetPropertyMap();

				// Special handling for specific components
				if (componentName == "Latent") {
					UISchema::RenderSchema(component->schema, properties);

					// Latent-specific features
					if (mgr.HasComponent<LatentComponent>(entity)) {
						auto& latentComp = mgr.GetComponent<LatentComponent>(entity);

						// Enforce 64-pixel multiples, minimum 64x64
						int validWidth = std::max(64, ((latentComp.latentWidth + 32) / 64) * 64);
						int validHeight = std::max(64, ((latentComp.latentHeight + 32) / 64) * 64);

						if (validWidth != latentComp.latentWidth || validHeight != latentComp.latentHeight) {
							latentComp.latentWidth = validWidth;
							latentComp.latentHeight = validHeight;
						}

						// Display current dimensions
						ImGui::Text("Current: %dx%d", latentComp.latentWidth, latentComp.latentHeight);

						// Swap width/height button
						if (ImGui::Button("Swap Width/Height", ImVec2(-1.0f, 0))) {
							int temp = latentComp.latentWidth;
							latentComp.latentWidth = latentComp.latentHeight;
							latentComp.latentHeight = temp;
						}
					}
				}
				else if (componentName == "InputImage") {
					// Render the file selector from schema
					UISchema::RenderSchema(component->schema, properties);

					// Display current file path if set
					if (mgr.HasComponent<InputImageComponent>(entity)) {
						auto& inputComp = mgr.GetComponent<InputImageComponent>(entity);

						if (!inputComp.inputFilePath.empty()) {
							ImGui::Text("Selected: %s", inputComp.inputFilePath.c_str());

							// Button to set latent dimensions to image size
							if (ImGui::Button("Set Latent to Image Size", ImVec2(-1.0f, 0))) {
								if (mgr.HasComponent<LatentComponent>(entity)) {
									auto& latentComp = mgr.GetComponent<LatentComponent>(entity);

									// Try to get image dimensions from InputImageComponent
									if (inputComp.width > 0 && inputComp.height > 0) {
										// Round to nearest multiple of 64, minimum 64
										int roundedWidth = std::max(64, ((inputComp.width + 32) / 64) * 64);
										int roundedHeight = std::max(64, ((inputComp.height + 32) / 64) * 64);

										latentComp.latentWidth = roundedWidth;
										latentComp.latentHeight = roundedHeight;

										std::cout << "Set latent dimensions to: " << roundedWidth
											<< "x" << roundedHeight
											<< " (from image: " << inputComp.width
											<< "x" << inputComp.height << ")" << std::endl;
									}
									else {
										std::cerr << "Input image dimensions not available!" << std::endl;
									}
								}
							}
						}
						else {
							ImGui::TextDisabled("No image selected");
						}
					}
				}
				else {
					// Standard component rendering
					UISchema::RenderSchema(component->schema, properties);
				}
			}
			catch (const std::exception& e) {
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering %s: %s",
					componentName.c_str(), e.what());
			}
		}

		// Add context menu for component
		ImVec2 contentEnd = ImGui::GetCursorScreenPos();
		ImVec2 contentSize = ImVec2(ImGui::GetContentRegionAvail().x, contentEnd.y - contentStart.y);

		ImGui::SetCursorScreenPos(contentStart);
		ImGui::InvisibleButton(("##component_context_" + componentName).c_str(), contentSize);

		std::string popupId = "ComponentContextMenu##" + componentName + "_" + std::to_string(entity);
		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			ImGui::OpenPopup(popupId.c_str());
		}

		if (ImGui::BeginPopup(popupId.c_str())) {
			ImGui::Text("Component: %s", componentName.c_str());
			ImGui::Text("Entity: %zu", entity);
			ImGui::Separator();

			if (ImGui::MenuItem("Copy Component")) {
				contextMenuUtils->CopyComponent(entity, compId);
			}

			if (ImGui::MenuItem("Copy Entity")) {
				contextMenuUtils->CopyEntity(entity);
			}

			ImGui::Separator();

			if (contextMenuUtils->HasValidClipboardData()) {
				if (contextMenuUtils->CanPasteComponent()) {
					if (ImGui::MenuItem("Paste Component")) {
						contextMenuUtils->PasteComponent(entity);
					}
				}
				if (contextMenuUtils->CanPasteEntity()) {
					if (ImGui::MenuItem("Paste Entity")) {
						nlohmann::json clipboardData = contextMenuUtils->GetClipboardData();
						if (clipboardData.contains("data")) {
							mgr.DeserializeEntity(clipboardData["data"], entity);
						}
					}
				}
			}
			else {
				ImGui::TextDisabled("Nothing to paste");
			}

			ImGui::EndPopup();
		}

		ImGui::SetCursorScreenPos(contentEnd);
		ImGui::Unindent();
	}

	void DiffusionView::RenderMainContextMenu() {
		if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() &&
			ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			ImGui::OpenPopup("DiffusionMainContext");
		}

		if (ImGui::BeginPopup("DiffusionMainContext")) {
			ImGui::Text("Diffusion View");
			ImGui::Separator();

			EntityID currentEntity = GetCurrentEntity();

			if (ImGui::MenuItem("Copy Current Entity")) {
				contextMenuUtils->CopyEntity(currentEntity);
			}

			ImGui::Separator();

			if (contextMenuUtils->HasValidClipboardData()) {
				if (contextMenuUtils->CanPasteComponent()) {
					if (ImGui::MenuItem("Paste Component")) {
						contextMenuUtils->PasteComponent(currentEntity);
					}
				}
				if (contextMenuUtils->CanPasteEntity()) {
					if (ImGui::MenuItem("Paste Entity")) {
						nlohmann::json clipboardData = contextMenuUtils->GetClipboardData();
						if (clipboardData.contains("data")) {
							mgr.DeserializeEntity(clipboardData["data"], currentEntity);
						}
					}
				}

				ImGui::Separator();
				ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Clipboard: %s",
					contextMenuUtils->GetClipboardPreview().c_str());
			}
			else {
				ImGui::TextDisabled("Nothing to paste");
			}

			ImGui::EndPopup();
		}
	}

	void DiffusionView::HandleT2IEvent() {
		std::cout << "Adding new T2I entity..." << std::endl;

		// Clone the entity
		nlohmann::json entityData = mgr.SerializeEntity(txt2imgEntity);
		EntityID newEntity = mgr.DeserializeEntity(entityData);

		if (newEntity == 0) {
			std::cerr << "Failed to create new entity via serialization!" << std::endl;
			return;
		}

		std::cout << "Successfully cloned entity " << txt2imgEntity << " to " << newEntity << " via serialization" << std::endl;

		// Get the SDCPPSystem directly and queue the task
		auto sdSystem = mgr.GetSystem<ECS::SDCPPSystem>();
		if (sdSystem) {
			sdSystem->QueueTask(newEntity, ECS::SDCPPSystem::TaskType::Inference);
			std::cout << "Task queued directly to SDCPPSystem" << std::endl;
		}
		else {
			std::cerr << "SDCPPSystem not found!" << std::endl;
			mgr.DestroyEntity(newEntity); // Clean up if system not available
		}
	}

	void DiffusionView::HandleI2IEvent() {
		std::cout << "Adding new I2I entity..." << std::endl;

		// SERIALIZE AND DESERIALIZE TO COPY THE TEMPLATE ENTITY
		nlohmann::json entityData = mgr.SerializeEntity(img2imgEntity);
		EntityID newEntity = mgr.DeserializeEntity(entityData);

		if (newEntity == 0) {
			std::cerr << "Failed to create new entity via serialization!" << std::endl;
			return;
		}

		std::cout << "Successfully cloned entity " << img2imgEntity << " to " << newEntity << " via serialization" << std::endl;

		// Configure the cloned entity
		if (mgr.HasComponent<OutputImageComponent>(newEntity)) {
			auto& outputComp = mgr.GetComponent<OutputImageComponent>(newEntity);
			if (outputComp.filePath.empty()) {
				outputComp.filePath = Utils::FilePaths::defaultProjectPath;
			}
			if (outputComp.fileName.empty()) {
				outputComp.fileName = "AniStudio.png";
			}
			std::filesystem::create_directories(outputComp.filePath);
		}

		// Send the CLONED entity to the system
		auto taskData = std::make_pair(newEntity, ECS::SDCPPSystem::TaskType::Img2Img);
		ANI::Events::Ref().QueueEventWithData("QueueDiffusionTask", taskData);
	}

	void DiffusionView::HandleEditEvent() {
		std::cout << "Adding new Edit entity..." << std::endl;

		// SERIALIZE AND DESERIALIZE TO COPY THE TEMPLATE ENTITY
		nlohmann::json entityData = mgr.SerializeEntity(editEntity);
		EntityID newEntity = mgr.DeserializeEntity(entityData);

		if (newEntity == 0) {
			std::cerr << "Failed to create new entity via serialization!" << std::endl;
			return;
		}

		std::cout << "Successfully cloned entity " << editEntity << " to " << newEntity << " via serialization" << std::endl;

		// Configure the cloned entity
		if (mgr.HasComponent<OutputImageComponent>(newEntity)) {
			auto& outputComp = mgr.GetComponent<OutputImageComponent>(newEntity);
			if (outputComp.filePath.empty()) {
				outputComp.filePath = Utils::FilePaths::defaultProjectPath;
			}
			if (outputComp.fileName.empty()) {
				outputComp.fileName = "AniStudio_edit.png";
			}
			std::filesystem::create_directories(outputComp.filePath);
		}

		// Send the CLONED entity to the system
		auto taskData = std::make_pair(newEntity, ECS::SDCPPSystem::TaskType::Edit);
		ANI::Events::Ref().QueueEventWithData("QueueDiffusionTask", taskData);
	}

	ECS::EntityID DiffusionView::GetCurrentEntity() const {
		switch (currentMode) {
		case 0: return txt2imgEntity;
		case 1: return img2imgEntity;
		case 2: return editEntity;
		default: return 0;
		}
	}

	void DiffusionView::RenderQueueList() {
		ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Queue")) {

			// Get progress data from DiffusionCallbackUtils
			const ProgressData& progressData = DiffusionCallbackUtils::GetProgressData();
			int currentStep = progressData.currentStep;
			int totalSteps = progressData.totalSteps;
			float time = progressData.currentTime;
			bool isProcessing = progressData.isProcessing;

			if (isProcessing && totalSteps > 0) {
				float progress = static_cast<float>(currentStep) / totalSteps;
				std::ostringstream ss;
				ss << "Processing: " << currentStep << "/" << totalSteps << " steps (" << std::fixed << std::setprecision(1)
					<< time << "s)";
				ImGui::Text("%s", ss.str().c_str());
				ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 0));
			}
			else {
				ImGui::Text("Waiting...");
				ImGui::ProgressBar(0.0f, ImVec2(-FLT_MIN, 0));
			}
			ImGui::Separator();

			if (ImGui::InputInt("Queue #", &numQueues, 1, 4)) {
				if (numQueues < 1) {
					numQueues = 1;
				}
			}

			if (ImGui::Button("Queue", ImVec2(-FLT_MIN, 0))) {
				EntityID targetEntity = GetCurrentEntity();

				if (mgr.HasComponent<LoraComponent>(targetEntity)) {
					auto& loraComp = mgr.GetComponent<LoraComponent>(targetEntity);
					if (loraComp.modelPath.empty())
						loraComp.modelPath = Utils::FilePaths::loraDir;
				}

				for (int i = 0; i < numQueues; i++) {
					switch (currentMode) {
					case 0: HandleT2IEvent(); break;
					case 1: HandleI2IEvent(); break;
					case 2: HandleEditEvent(); break;
					}
				}
			}

			ImGui::Separator();

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
			if (ImGui::BeginTable("Queue", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 42.0f);
				ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 44.0f);
				ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableHeadersRow();

				auto sdSystem = mgr.GetSystem<ECS::SDCPPSystem>();
				if (sdSystem) {
					auto queueItems = sdSystem->GetQueueSnapshot();
					for (size_t i = 0; i < queueItems.size(); i++) {
						const auto& item = queueItems[i];

						ImGui::TableNextRow();

						// ID column with context menu
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
				}
				ImGui::EndTable();
			}
		}
		ImGui::End();
	}

	void DiffusionView::Render() {
		RenderQueueList();

		ImGui::SetNextWindowSize(ImVec2(300, 800), ImGuiCond_FirstUseEver);
		if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen)) {
			if (ImGui::CollapsingHeader("Metadata Controls")) {
				RenderMetadataControls();
			}

			if (contextMenuUtils->HasValidClipboardData()) {
				ImGui::Separator();
				ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Clipboard: %s",
					contextMenuUtils->GetClipboardPreview().c_str());
				ImGui::Separator();
			}

			if (ImGui::BeginTabBar("Diffusion")) {
				if (ImGui::BeginTabItem("Txt2Img")) {
					currentMode = 0;
					RenderEntityComponents(txt2imgEntity);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Img2Img")) {
					currentMode = 1;
					RenderEntityComponents(img2imgEntity);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Edit")) {
					currentMode = 2;
					RenderEntityComponents(editEntity);
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}

			RenderMainContextMenu();
		}
		ImGui::End();

		if (!windowOpen) {
			std::unordered_map<std::string, std::any> eventData;
			eventData["workspaceID"] = GetID();
			eventData["viewTypeName"] = viewName;
			ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
		}
	}

	nlohmann::json DiffusionView::Serialize() const {
		EntityID targetEntity = GetCurrentEntity();
		nlohmann::json j = mgr.SerializeEntity(targetEntity);
		j["componentVisibility"] = componentVisibility;
		j["currentMode"] = currentMode;
		return j;
	}

	void DiffusionView::Deserialize(const nlohmann::json& j) {
		EntityID targetEntity = GetCurrentEntity();
		if (targetEntity == 0) {
			std::cerr << "Error: Invalid target entity for deserialization" << std::endl;
			return;
		}

		try {
			mgr.DeserializeEntity(j, targetEntity);
			if (j.contains("componentVisibility")) {
				componentVisibility = j["componentVisibility"];
			}
			if (j.contains("currentMode")) {
				currentMode = j["currentMode"];
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Error saving metadata: " << e.what() << std::endl;
		}
	}

	void DiffusionView::LoadMetadataFromPNG(const std::string& imagePath) {
		FILE* fp = fopen(imagePath.c_str(), "rb");
		if (!fp) return;

		png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (!png) {
			fclose(fp);
			return;
		}

		png_infop info = png_create_info_struct(png);
		if (!info) {
			png_destroy_read_struct(&png, nullptr, nullptr);
			fclose(fp);
			return;
		}

		if (setjmp(png_jmpbuf(png))) {
			png_destroy_read_struct(&png, &info, nullptr);
			fclose(fp);
			return;
		}

		png_init_io(png, fp);
		png_read_info(png, info);

		png_textp text_ptr;
		int num_text;
		if (png_get_text(png, info, &text_ptr, &num_text) > 0) {
			for (int i = 0; i < num_text; i++) {
				if (strcmp(text_ptr[i].key, "parameters") == 0) {
					try {
						nlohmann::json metadata = nlohmann::json::parse(text_ptr[i].text);
						nlohmann::json convertedJson;
						convertedJson["ID"] = metadata.value("ID", 0);

						if (metadata.contains("software")) convertedJson["software"] = metadata["software"];
						if (metadata.contains("timestamp")) convertedJson["timestamp"] = metadata["timestamp"];
						if (metadata.contains("version")) convertedJson["version"] = metadata["version"];

						convertedJson["components"] = nlohmann::json::array();

						if (metadata.contains("components") && metadata["components"].is_object()) {
							auto& componentsObj = metadata["components"];
							for (auto it = componentsObj.begin(); it != componentsObj.end(); ++it) {
								std::string componentName = it.key();
								nlohmann::json componentData = it.value();

								if (componentName == "Base_Component") {
									nlohmann::json baseComp;
									baseComp["compName"] = "Base_Component";
									convertedJson["components"].push_back(baseComp);
									continue;
								}

								if (componentData.is_object()) {
									if (componentData.contains(componentName) && componentData[componentName].is_object()) {
										nlohmann::json arrayElement;
										arrayElement[componentName] = componentData[componentName];
										convertedJson["components"].push_back(arrayElement);
									}
									else {
										nlohmann::json arrayElement;
										arrayElement[componentName] = componentData;
										convertedJson["components"].push_back(arrayElement);
									}
								}
							}
						}
						else if (metadata.contains("components") && metadata["components"].is_array()) {
							convertedJson["components"] = metadata["components"];
						}

						Deserialize(convertedJson);
					}
					catch (const std::exception& e) {
						std::cerr << "Error loading metadata: " << e.what() << std::endl;
					}
					break;
				}
			}
		}

		png_destroy_read_struct(&png, &info, nullptr);
		fclose(fp);
	}

	void DiffusionView::RenderMetadataControls() {
		if (ImGui::Button("Save Metadata", ImVec2(-FLT_MIN, 0))) {
			IGFD::FileDialogConfig config;
			config.path = Utils::FilePaths::defaultProjectPath;
			ImGuiFileDialog::Instance()->OpenDialog("SaveMetadataDialog", "Save Metadata", ".json", config);
		}

		if (ImGui::Button("Load Metadata", ImVec2(-FLT_MIN, 0))) {
			IGFD::FileDialogConfig config;
			config.path = Utils::FilePaths::defaultProjectPath;
			ImGuiFileDialog::Instance()->OpenDialog("LoadMetadataDialog", "Load Metadata", ".json,.png", config);
		}

		// Handle Save Dialog
		if (ImGuiFileDialog::Instance()->Display("SaveMetadataDialog", 32, ImVec2(700, 400))) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				std::string filepath = ImGuiFileDialog::Instance()->GetFilePathName();
				SaveMetadataToJson(filepath);
			}
			ImGuiFileDialog::Instance()->Close();
		}

		// Handle Load Dialog
		if (ImGuiFileDialog::Instance()->Display("LoadMetadataDialog", 32, ImVec2(700, 400))) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				std::string filepath = ImGuiFileDialog::Instance()->GetFilePathName();
				std::string extension = std::filesystem::path(filepath).extension().string();

				if (extension == ".json") {
					LoadMetadataFromJson(filepath);
				}
				else if (extension == ".png" || extension == ".jpg" || extension == ".jpeg") {
					LoadMetadataFromPNG(filepath);
				}
			}
			ImGuiFileDialog::Instance()->Close();
		}
	}

	void DiffusionView::LoadMetadataFromJson(const std::string& filepath) {
		try {
			std::ifstream file(filepath);
			if (file.is_open()) {
				nlohmann::json metadata;
				file >> metadata;
				Deserialize(metadata);
				file.close();
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Error loading metadata: " << e.what() << std::endl;
		}
	}

	void DiffusionView::SaveMetadataToJson(const std::string& filepath) {
		try {
			nlohmann::json metadata = Serialize();
			std::ofstream file(filepath);
			if (file.is_open()) {
				file << metadata.dump(4);
				file.close();
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Error saving metadata: " << e.what() << std::endl;
		}
	}

} // namespace GUI