#include "DiffusionView.hpp"
#include "../Events/Events.hpp"
#include "Constants.hpp"
#include "UISchema.hpp"
#include "PngMetadataUtils.hpp"
#include "utils.h"
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

static void LogCallback(sd_log_level_t level, const char* text, void* data) {
	switch (level) {
	case SD_LOG_DEBUG:
		std::cout << "[DEBUG]: " << text;
		break;
	case SD_LOG_INFO:
		std::cout << "[INFO]: " << text;
		break;
	case SD_LOG_WARN:
		std::cout << "[WARNING]: " << text;
		break;
	case SD_LOG_ERROR:
		std::cerr << "[ERROR]: " << text;
		break;
	default:
		std::cerr << "[UNKNOWN LOG LEVEL]: " << text;
		break;
	}
}

static GUI::ProgressData progressData;
static void ProgressCallback(int step, int steps, float time, void* data) {
	progressData.currentStep = step;
	progressData.totalSteps = steps;
	progressData.currentTime = time;
	progressData.isProcessing = (steps > 0);
	std::cout << "Progress: Step " << step << " of " << steps << " | Time: " << time << "s" << std::endl;
}

namespace GUI {

	DiffusionView::DiffusionView(EntityManager& entityMgr) : BaseView(entityMgr) {
		viewName = "DiffusionView";
		windowOpen = true;

		// Initialize component visibility states
		InitializeComponentVisibility();
	}

	DiffusionView::~DiffusionView() {
		if (txt2imgEntity != 0)
			mgr.DestroyEntity(txt2imgEntity);
		if (img2imgEntity != 0)
			mgr.DestroyEntity(img2imgEntity);
	}

	void DiffusionView::InitializeComponentVisibility() {
		// Initialize all components as visible by default
		componentVisibility["ModelComponent"] = true;
		componentVisibility["ClipLComponent"] = true;
		componentVisibility["ClipGComponent"] = true;
		componentVisibility["T5XXLComponent"] = true;
		componentVisibility["DiffusionModelComponent"] = true;
		componentVisibility["VaeComponent"] = true;
		componentVisibility["LoraComponent"] = true;
		componentVisibility["TaesdComponent"] = true;
		componentVisibility["LatentComponent"] = true;
		componentVisibility["SamplerComponent"] = true;
		componentVisibility["GuidanceComponent"] = true;
		componentVisibility["ClipSkipComponent"] = true;
		componentVisibility["PromptComponent"] = true;
		componentVisibility["LayerSkipComponent"] = true;
		componentVisibility["OutputImageComponent"] = true;
		componentVisibility["InputImageComponent"] = true;
		componentVisibility["ControlnetComponent"] = true;
		componentVisibility["EmbeddingComponent"] = true;
		componentVisibility["EsrganComponent"] = true;
	}

	void DiffusionView::Init() {
		sd_set_log_callback(LogCallback, nullptr);
		sd_set_progress_callback(ProgressCallback, nullptr);
		ResetEntities();
	}

	void DiffusionView::ResetEntities() {
		if (txt2imgEntity != 0) {
			mgr.DestroyEntity(txt2imgEntity);
			txt2imgEntity = 0;
		}

		if (img2imgEntity != 0) {
			mgr.DestroyEntity(img2imgEntity);
			img2imgEntity = 0;
		}

		// Create entity for Txt2Img mode
		txt2imgEntity = mgr.AddNewEntity();

		// Add components for Txt2Img (NO InputImageComponent)
		mgr.AddComponent<ModelComponent>(txt2imgEntity);
		mgr.AddComponent<ClipLComponent>(txt2imgEntity);
		mgr.AddComponent<ClipGComponent>(txt2imgEntity);
		mgr.AddComponent<T5XXLComponent>(txt2imgEntity);
		mgr.AddComponent<DiffusionModelComponent>(txt2imgEntity);
		mgr.AddComponent<VaeComponent>(txt2imgEntity);
		mgr.AddComponent<LoraComponent>(txt2imgEntity);
		mgr.AddComponent<TaesdComponent>(txt2imgEntity);
		mgr.AddComponent<LatentComponent>(txt2imgEntity);
		mgr.AddComponent<SamplerComponent>(txt2imgEntity);
		mgr.AddComponent<GuidanceComponent>(txt2imgEntity);
		mgr.AddComponent<ClipSkipComponent>(txt2imgEntity);
		mgr.AddComponent<PromptComponent>(txt2imgEntity);
		mgr.AddComponent<LayerSkipComponent>(txt2imgEntity);
		mgr.AddComponent<OutputImageComponent>(txt2imgEntity);

		// Create entity for Img2Img mode 
		img2imgEntity = mgr.AddNewEntity();

		// Add components for Img2Img (WITH InputImageComponent)
		mgr.AddComponent<ModelComponent>(img2imgEntity);
		mgr.AddComponent<ClipLComponent>(img2imgEntity);
		mgr.AddComponent<ClipGComponent>(img2imgEntity);
		mgr.AddComponent<T5XXLComponent>(img2imgEntity);
		mgr.AddComponent<DiffusionModelComponent>(img2imgEntity);
		mgr.AddComponent<VaeComponent>(img2imgEntity);
		mgr.AddComponent<LoraComponent>(img2imgEntity);
		mgr.AddComponent<TaesdComponent>(img2imgEntity);
		mgr.AddComponent<LatentComponent>(img2imgEntity);
		mgr.AddComponent<SamplerComponent>(img2imgEntity);
		mgr.AddComponent<GuidanceComponent>(img2imgEntity);
		mgr.AddComponent<ClipSkipComponent>(img2imgEntity);
		mgr.AddComponent<PromptComponent>(img2imgEntity);
		mgr.AddComponent<LayerSkipComponent>(img2imgEntity);
		mgr.AddComponent<OutputImageComponent>(img2imgEntity);
		mgr.AddComponent<InputImageComponent>(img2imgEntity);

		// Default denoise value for Img2Img
		mgr.GetComponent<SamplerComponent>(img2imgEntity).denoise = 0.6f;
	}

	bool DiffusionView::IsEntitySafeToUse(ECS::EntityID entity) const {
		return mgr.IsEntityValid(entity);
	}

	void DiffusionView::RenderComponentWithCheckbox(const EntityID entity, const std::string& componentName, const std::string& displayName, const std::function<void()>& renderFunc) {
		if (!componentVisibility.count(componentName)) {
			componentVisibility[componentName] = true; // Default to visible
		}

		bool isVisible = componentVisibility[componentName];
		if (ImGui::Checkbox(displayName.c_str(), &isVisible)) {
			componentVisibility[componentName] = isVisible;
		}

		if (isVisible) {
			ImGui::Indent();
			renderFunc();
			ImGui::Unindent();
		}
	}

	void DiffusionView::RenderEntityComponents(const EntityID entity) {
		if (entity == 0 || !IsEntitySafeToUse(entity)) return;

		// Model Selection
		if (ImGui::CollapsingHeader("Model Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::BeginTabBar("ModelTabs")) {
				if (ImGui::BeginTabItem("Full")) {
					// Checkpoint Model
					RenderComponentWithCheckbox(entity, "ModelComponent", "Checkpoint", [&]() {
						if (mgr.HasComponent<ModelComponent>(entity)) {
							auto& comp = mgr.GetComponent<ModelComponent>(entity);
							if (!comp.schema.empty()) {
								try {
									auto properties = comp.GetPropertyMap();
									UISchema::RenderSchema(comp.schema, properties);
								}
								catch (const std::exception& e) {
									ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering ModelComponent: %s", e.what());
								}
							}
						}
					});

					// VAE
					RenderComponentWithCheckbox(entity, "VaeComponent", "VAE", [&]() {
						if (mgr.HasComponent<VaeComponent>(entity)) {
							auto& comp = mgr.GetComponent<VaeComponent>(entity);
							if (!comp.schema.empty()) {
								try {
									auto properties = comp.GetPropertyMap();
									UISchema::RenderSchema(comp.schema, properties);
								}
								catch (const std::exception& e) {
									ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering VaeComponent: %s", e.what());
								}
							}
						}
					});

					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Split")) {
					// UNet
					RenderComponentWithCheckbox(entity, "DiffusionModelComponent", "UNet", [&]() {
						if (mgr.HasComponent<DiffusionModelComponent>(entity)) {
							auto& comp = mgr.GetComponent<DiffusionModelComponent>(entity);
							if (!comp.schema.empty()) {
								try {
									auto properties = comp.GetPropertyMap();
									UISchema::RenderSchema(comp.schema, properties);
								}
								catch (const std::exception& e) {
									ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering DiffusionModelComponent: %s", e.what());
								}
							}
						}
					});

					// Text Encoders Category
					if (ImGui::CollapsingHeader("Text Encoders", ImGuiTreeNodeFlags_DefaultOpen)) {
						// CLIP-L
						RenderComponentWithCheckbox(entity, "ClipLComponent", "CLIP-L", [&]() {
							if (mgr.HasComponent<ClipLComponent>(entity)) {
								auto& comp = mgr.GetComponent<ClipLComponent>(entity);
								if (!comp.schema.empty()) {
									try {
										auto properties = comp.GetPropertyMap();
										UISchema::RenderSchema(comp.schema, properties);
									}
									catch (const std::exception& e) {
										ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering ClipLComponent: %s", e.what());
									}
								}
							}
						});

						// CLIP-G
						RenderComponentWithCheckbox(entity, "ClipGComponent", "CLIP-G", [&]() {
							if (mgr.HasComponent<ClipGComponent>(entity)) {
								auto& comp = mgr.GetComponent<ClipGComponent>(entity);
								if (!comp.schema.empty()) {
									try {
										auto properties = comp.GetPropertyMap();
										UISchema::RenderSchema(comp.schema, properties);
									}
									catch (const std::exception& e) {
										ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering ClipGComponent: %s", e.what());
									}
								}
							}
						});

						// T5-XXL
						RenderComponentWithCheckbox(entity, "T5XXLComponent", "T5-XXL", [&]() {
							if (mgr.HasComponent<T5XXLComponent>(entity)) {
								auto& comp = mgr.GetComponent<T5XXLComponent>(entity);
								if (!comp.schema.empty()) {
									try {
										auto properties = comp.GetPropertyMap();
										UISchema::RenderSchema(comp.schema, properties);
									}
									catch (const std::exception& e) {
										ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering T5XXLComponent: %s", e.what());
									}
								}
							}
						});
					}

					// VAE
					RenderComponentWithCheckbox(entity, "VaeComponent", "VAE", [&]() {
						if (mgr.HasComponent<VaeComponent>(entity)) {
							auto& comp = mgr.GetComponent<VaeComponent>(entity);
							if (!comp.schema.empty()) {
								try {
									auto properties = comp.GetPropertyMap();
									UISchema::RenderSchema(comp.schema, properties);
								}
								catch (const std::exception& e) {
									ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering VaeComponent: %s", e.what());
								}
							}
						}
					});

					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
		}

		// Sampling Options
		if (ImGui::CollapsingHeader("Sampling Options", ImGuiTreeNodeFlags_DefaultOpen)) {
			// Image Dimensions
			RenderComponentWithCheckbox(entity, "LatentComponent", "Image Dimensions", [&]() {
				if (mgr.HasComponent<LatentComponent>(entity)) {
					auto& comp = mgr.GetComponent<LatentComponent>(entity);
					if (!comp.schema.empty()) {
						try {
							// Render schema
							auto properties = comp.GetPropertyMap();
							UISchema::RenderSchema(comp.schema, properties);

							// Render swap button separately
							if (ImGui::Button("Swap Width/Height", ImVec2(-1.0f, 0))) {
								int temp = comp.latentWidth;
								comp.latentWidth = comp.latentHeight;
								comp.latentHeight = temp;
							}
						}
						catch (const std::exception& e) {
							ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering LatentComponent: %s", e.what());
						}
					}
				}
			});

			// Sampler Settings
			RenderComponentWithCheckbox(entity, "SamplerComponent", "Sampler Settings", [&]() {
				if (mgr.HasComponent<SamplerComponent>(entity)) {
					auto& comp = mgr.GetComponent<SamplerComponent>(entity);
					if (!comp.schema.empty()) {
						try {
							auto properties = comp.GetPropertyMap();
							UISchema::RenderSchema(comp.schema, properties);
						}
						catch (const std::exception& e) {
							ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering SamplerComponent: %s", e.what());
						}
					}
				}
			});

			// Guidance
			RenderComponentWithCheckbox(entity, "GuidanceComponent", "Guidance", [&]() {
				if (mgr.HasComponent<GuidanceComponent>(entity)) {
					auto& comp = mgr.GetComponent<GuidanceComponent>(entity);
					if (!comp.schema.empty()) {
						try {
							auto properties = comp.GetPropertyMap();
							UISchema::RenderSchema(comp.schema, properties);
						}
						catch (const std::exception& e) {
							ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering GuidanceComponent: %s", e.what());
						}
					}
				}
			});

			// Prompts
			RenderComponentWithCheckbox(entity, "PromptComponent", "Prompts", [&]() {
				if (mgr.HasComponent<PromptComponent>(entity)) {
					auto& comp = mgr.GetComponent<PromptComponent>(entity);
					if (!comp.schema.empty()) {
						try {
							auto properties = comp.GetPropertyMap();
							UISchema::RenderSchema(comp.schema, properties);
						}
						catch (const std::exception& e) {
							ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering PromptComponent: %s", e.what());
						}
					}
				}
			});
		}

		// Image Settings
		if (ImGui::CollapsingHeader("Image Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			// Input Image (only for img2img)
			if (mgr.HasComponent<InputImageComponent>(entity)) {
				RenderComponentWithCheckbox(entity, "InputImageComponent", "Input Image", [&]() {
					auto& comp = mgr.GetComponent<InputImageComponent>(entity);
					if (!comp.schema.empty()) {
						try {
							auto properties = comp.GetPropertyMap();
							UISchema::RenderSchema(comp.schema, properties);
						}
						catch (const std::exception& e) {
							ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering InputImageComponent: %s", e.what());
						}
					}
				});
			}

			// Output Settings
			RenderComponentWithCheckbox(entity, "OutputImageComponent", "Output Settings", [&]() {
				if (mgr.HasComponent<OutputImageComponent>(entity)) {
					auto& comp = mgr.GetComponent<OutputImageComponent>(entity);
					if (!comp.schema.empty()) {
						try {
							auto properties = comp.GetPropertyMap();
							UISchema::RenderSchema(comp.schema, properties);
						}
						catch (const std::exception& e) {
							ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering OutputImageComponent: %s", e.what());
						}
					}
				}
			});
		}

		// Advanced Options
		if (ImGui::CollapsingHeader("Advanced Options")) {
			// CLIP Skip
			RenderComponentWithCheckbox(entity, "ClipSkipComponent", "CLIP Skip", [&]() {
				if (mgr.HasComponent<ClipSkipComponent>(entity)) {
					auto& comp = mgr.GetComponent<ClipSkipComponent>(entity);
					if (!comp.schema.empty()) {
						try {
							auto properties = comp.GetPropertyMap();
							UISchema::RenderSchema(comp.schema, properties);
						}
						catch (const std::exception& e) {
							ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering ClipSkipComponent: %s", e.what());
						}
					}
				}
			});

			// Layer Skip
			RenderComponentWithCheckbox(entity, "LayerSkipComponent", "Layer Skip", [&]() {
				if (mgr.HasComponent<LayerSkipComponent>(entity)) {
					auto& comp = mgr.GetComponent<LayerSkipComponent>(entity);
					if (!comp.schema.empty()) {
						try {
							auto properties = comp.GetPropertyMap();
							UISchema::RenderSchema(comp.schema, properties);
						}
						catch (const std::exception& e) {
							ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering LayerSkipComponent: %s", e.what());
						}
					}
				}
			});

			// LoRA
			RenderComponentWithCheckbox(entity, "LoraComponent", "LoRA", [&]() {
				if (mgr.HasComponent<LoraComponent>(entity)) {
					auto& comp = mgr.GetComponent<LoraComponent>(entity);
					if (!comp.schema.empty()) {
						try {
							auto properties = comp.GetPropertyMap();
							UISchema::RenderSchema(comp.schema, properties);
						}
						catch (const std::exception& e) {
							ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering LoraComponent: %s", e.what());
						}
					}
				}
			});

			// TAESD
			RenderComponentWithCheckbox(entity, "TaesdComponent", "TAESD", [&]() {
				if (mgr.HasComponent<TaesdComponent>(entity)) {
					auto& comp = mgr.GetComponent<TaesdComponent>(entity);
					if (!comp.schema.empty()) {
						try {
							auto properties = comp.GetPropertyMap();
							UISchema::RenderSchema(comp.schema, properties);
						}
						catch (const std::exception& e) {
							ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering TaesdComponent: %s", e.what());
						}
					}
				}
			});

			// ControlNet
			RenderComponentWithCheckbox(entity, "ControlnetComponent", "ControlNet", [&]() {
				if (mgr.HasComponent<ControlnetComponent>(entity)) {
					auto& comp = mgr.GetComponent<ControlnetComponent>(entity);
					if (!comp.schema.empty()) {
						try {
							auto properties = comp.GetPropertyMap();
							UISchema::RenderSchema(comp.schema, properties);
						}
						catch (const std::exception& e) {
							ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering ControlnetComponent: %s", e.what());
						}
					}
				}
			});

			// Embeddings
			RenderComponentWithCheckbox(entity, "EmbeddingComponent", "Embeddings", [&]() {
				if (mgr.HasComponent<EmbeddingComponent>(entity)) {
					auto& comp = mgr.GetComponent<EmbeddingComponent>(entity);
					if (!comp.schema.empty()) {
						try {
							auto properties = comp.GetPropertyMap();
							UISchema::RenderSchema(comp.schema, properties);
						}
						catch (const std::exception& e) {
							ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering EmbeddingComponent: %s", e.what());
						}
					}
				}
			});

			// ESRGAN Upscaler
			RenderComponentWithCheckbox(entity, "EsrganComponent", "ESRGAN Upscaler", [&]() {
				if (mgr.HasComponent<EsrganComponent>(entity)) {
					auto& comp = mgr.GetComponent<EsrganComponent>(entity);
					if (!comp.schema.empty()) {
						try {
							auto properties = comp.GetPropertyMap();
							UISchema::RenderSchema(comp.schema, properties);
						}
						catch (const std::exception& e) {
							ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering EsrganComponent: %s", e.what());
						}
					}
				}
			});
		}
	}

	void DiffusionView::RenderComponentSchema(const EntityID entity, const std::string& componentName, ECS::BaseComponent* component) {
		// This method is now simplified since everything goes through RenderEntityComponents
		if (!component || component->schema.empty()) {
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "No schema available for %s", componentName.c_str());
			return;
		}

		try {
			auto properties = component->GetPropertyMap();
			UISchema::RenderSchema(component->schema, properties);
		}
		catch (const std::exception& e) {
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
				"Error rendering %s: %s", componentName.c_str(), e.what());
		}
	}

	void DiffusionView::UpdateModelPath(const EntityID entity, const std::string& componentName) {
		// Handle special model path updates if needed
		// This can be expanded for specific component behaviors
	}

	void DiffusionView::HandleT2IEvent() {
		std::cout << "Adding new T2I entity..." << std::endl;

		EntityID newEntity = mgr.CloneEntity(txt2imgEntity);
		if (newEntity == 0) {
			std::cerr << "Failed to create new entity!" << std::endl;
			return;
		}

		// Ensure output path is valid
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

		// Queue event
		Event event;
		event.entityID = newEntity;
		event.type = EventType::InferenceRequest;
		ANI::Events::Ref().QueueEvent(event);
	}

	void DiffusionView::HandleI2IEvent() {
		std::cout << "Adding new I2I entity..." << std::endl;

		EntityID newEntity = mgr.CloneEntity(img2imgEntity);
		if (newEntity == 0) {
			std::cerr << "Failed to create new entity!" << std::endl;
			return;
		}

		// Ensure output path is valid
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

		// Queue event
		Event event;
		event.entityID = newEntity;
		event.type = EventType::Img2ImgRequest;
		ANI::Events::Ref().QueueEvent(event);
	}

	void DiffusionView::HandleUpscaleEvent() {
		std::cout << "Upscale event not yet implemented" << std::endl;
	}

	void DiffusionView::RenderQueueList() {
		ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Queue")) {

			// Get current progress values from the global progressData
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
				EntityID targetEntity = isTxt2ImgMode ? txt2imgEntity : img2imgEntity;
				if (mgr.HasComponent<LoraComponent>(targetEntity)) {
					auto& loraComp = mgr.GetComponent<LoraComponent>(targetEntity);
					loraComp.modelPath = Utils::FilePaths::loraDir;
				}

				for (int i = 0; i < numQueues; i++) {
					if (isTxt2ImgMode) {
						HandleT2IEvent();
					}
					else {
						HandleI2IEvent();
					}
				}
			}

			ImGui::Separator();

			if (isPaused) {
				if (ImGui::Button("Resume", ImVec2(-FLT_MIN, 0))) {
					Event event;
					event.type = EventType::ResumeInference;
					ANI::Events::Ref().QueueEvent(event);
					isPaused = false;
				}
			}
			else {
				if (ImGui::Button("Pause", ImVec2(-FLT_MIN, 0))) {
					Event event;
					event.type = EventType::PauseInference;
					ANI::Events::Ref().QueueEvent(event);
					isPaused = true;
				}
			}

			if (ImGui::Button("Stop", ImVec2(-FLT_MIN, 0))) {
				Event event;
				event.type = EventType::StopCurrentTask;
				ANI::Events::Ref().QueueEvent(event);
			}

			if (ImGui::Button("Clear Queue", ImVec2(-FLT_MIN, 0))) {
				Event event;
				event.type = EventType::ClearInferenceQueue;
				ANI::Events::Ref().QueueEvent(event);
			}

			ImGui::Separator();

			if (ImGui::BeginTable("Queue", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 42.0f);
				ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 44.0f);
				ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableHeadersRow();

				auto sdSystem = mgr.GetSystem<SDCPPSystem>();
				if (sdSystem) {
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
									sdSystem->MoveInQueue(i, i - 1);
								}
								ImGui::SameLine();
							}

							if (i < queueItems.size() - 1) {
								if (ImGui::ArrowButton(("down##" + std::to_string(i)).c_str(), ImGuiDir_Down)) {
									sdSystem->MoveInQueue(i, i + 1);
								}
								ImGui::SameLine();
							}

							if (i > 0) {
								if (ImGui::Button(("Top##Top" + std::to_string(i)).c_str())) {
									if (queueItems[0].processing) {
										sdSystem->MoveInQueue(i, 1);
									}
									else {
										sdSystem->MoveInQueue(i, 0);
									}
								}
								ImGui::SameLine();
							}

							if (i < queueItems.size() - 1) {
								if (ImGui::Button(("Bottom##Bottom" + std::to_string(i)).c_str())) {
									sdSystem->MoveInQueue(i, queueItems.size() - 1);
								}
								ImGui::SameLine();
							}

							if (ImGui::Button(("X##Remove" + std::to_string(i)).c_str())) {
								sdSystem->RemoveFromQueue(i);
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
		// Render queue controls
		RenderQueueList();

		// Main window
		ImGui::SetNextWindowSize(ImVec2(300, 800), ImGuiCond_FirstUseEver);
		if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen)) {
			
			if (!windowOpen) {
				ANI::Events::Ref().RequestRemoveView(GetID(), viewName);
			}

			// Metadata controls
			if (ImGui::CollapsingHeader("Metadata Controls")) {
				RenderMetadataControls();
			}

			// Tab bar for switching between Txt2Img and Img2Img
			if (ImGui::BeginTabBar("Diffusion")) {
				// Text-to-Image tab
				if (ImGui::BeginTabItem("Txt2Img")) {
					isTxt2ImgMode = true;
					RenderEntityComponents(txt2imgEntity);
					ImGui::EndTabItem();
				}

				// Image-to-Image tab
				if (ImGui::BeginTabItem("Img2Img")) {
					isTxt2ImgMode = false;
					RenderEntityComponents(img2imgEntity);
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
		}
		ImGui::End();
	}

	nlohmann::json DiffusionView::Serialize() const {
		EntityID targetEntity = isTxt2ImgMode ? txt2imgEntity : img2imgEntity;
		nlohmann::json j = mgr.SerializeEntity(targetEntity);

		// Also serialize component visibility states
		j["componentVisibility"] = componentVisibility;

		return j;
	}

	void DiffusionView::Deserialize(const nlohmann::json& j) {
		EntityID targetEntity = isTxt2ImgMode ? txt2imgEntity : img2imgEntity;

		if (targetEntity == 0) {
			std::cerr << "Error: Invalid target entity for deserialization" << std::endl;
			return;
		}

		try {
			mgr.DeserializeEntity(j, targetEntity);

			// Also deserialize component visibility states
			if (j.contains("componentVisibility")) {
				componentVisibility = j["componentVisibility"];
			}

			std::cout << "Successfully deserialized data to entity " << targetEntity << std::endl;
		}
		catch (const std::exception& e) {
			std::cerr << "Exception during deserialization: " << e.what() << std::endl;
		}
	}

	void DiffusionView::SaveMetadataToJson(const std::string& filepath) {
		try {
			nlohmann::json metadata = Serialize();
			std::ofstream file(filepath);
			if (file.is_open()) {
				file << metadata.dump(4);
				file.close();
				std::cout << "Metadata saved to: " << filepath << std::endl;
			}
			else {
				std::cerr << "Failed to open file for writing: " << filepath << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Error saving metadata: " << e.what() << std::endl;
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
				std::cout << "Metadata loaded from: " << filepath << std::endl;
			}
			else {
				std::cerr << "Failed to open file for reading: " << filepath << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Error loading metadata: " << e.what() << std::endl;
		}
	}

	void DiffusionView::LoadMetadataFromPNG(const std::string& imagePath) {
		std::cout << "Attempting to load metadata from: " << imagePath << std::endl;

		FILE* fp = fopen(imagePath.c_str(), "rb");
		if (!fp) {
			std::cerr << "Failed to open PNG file: " << imagePath << std::endl;
			return;
		}

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

		// Get text chunks
		png_textp text_ptr;
		int num_text;
		if (png_get_text(png, info, &text_ptr, &num_text) > 0) {
			for (int i = 0; i < num_text; i++) {
				if (strcmp(text_ptr[i].key, "parameters") == 0) {
					try {
						// Parse metadata from PNG
						nlohmann::json metadata = nlohmann::json::parse(text_ptr[i].text);
						std::cout << "Loading metadata: " << metadata.dump(2) << std::endl;

						// Convert nested object format to array format
						nlohmann::json convertedJson;

						// Keep the app info
						convertedJson["ID"] = metadata.value("ID", 0);
						if (metadata.contains("software"))
							convertedJson["software"] = metadata["software"];
						if (metadata.contains("timestamp"))
							convertedJson["timestamp"] = metadata["timestamp"];
						if (metadata.contains("version"))
							convertedJson["version"] = metadata["version"];

						convertedJson["components"] = nlohmann::json::array();

						// If metadata has the nested components object format, convert it
						if (metadata.contains("components") && metadata["components"].is_object()) {
							auto& componentsObj = metadata["components"];

							// Process each component type
							for (auto it = componentsObj.begin(); it != componentsObj.end(); ++it) {
								std::string componentName = it.key();
								nlohmann::json componentData = it.value();

								// Handle base component as a special case
								if (componentName == "Base_Component") {
									nlohmann::json baseComp;
									baseComp["compName"] = "Base_Component";
									convertedJson["components"].push_back(baseComp);
									continue;
								}

								// For nested objects, create an array element for each component
								if (componentData.is_object()) {
									// Remove the double nesting if present
									if (componentData.contains(componentName) && componentData[componentName].is_object()) {
										nlohmann::json arrayElement;
										arrayElement[componentName] = componentData[componentName];
										convertedJson["components"].push_back(arrayElement);
									}
									else {
										// Just add it as is
										nlohmann::json arrayElement;
										arrayElement[componentName] = componentData;
										convertedJson["components"].push_back(arrayElement);
									}
								}
							}
						}
						else if (metadata.contains("components") && metadata["components"].is_array()) {
							// If it's already in the array format, use it directly
							convertedJson["components"] = metadata["components"];
						}

						std::cout << "Converted JSON: " << convertedJson.dump(2) << std::endl;

						// Now deserialize the converted JSON using your existing method
						Deserialize(convertedJson);
						std::cout << "Successfully loaded metadata" << std::endl;
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

} // namespace GUI