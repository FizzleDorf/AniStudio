/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license information, please contact legal@kframe.ai.
 */

#include "VideoDiffusionView.hpp"
#include "../Events/Events.hpp"
#include "Constants.hpp"
#include "UISchema.hpp"
#include "PngMetadataUtils.hpp"
#include "utils.h"
#include <ImGuiFileDialog.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>

using namespace ECS;
using namespace ANI;

// Define ProgressData struct locally since we can't rely on DiffusionView
namespace GUI {
	struct ProgressData {
		int currentStep = 0;
		int totalSteps = 0;
		float currentTime = 0.0f;
		bool isProcessing = false;
	};
}

// External progress data for shared progress tracking
static GUI::ProgressData progressData;

namespace GUI {

	VideoDiffusionView::VideoDiffusionView(EntityManager& entityMgr) : BaseView(entityMgr) {
		viewName = "VideoDiffusionView";
		windowOpen = true;

		// Initialize component visibility states
		InitializeComponentVisibility();
	}

	VideoDiffusionView::~VideoDiffusionView() {
		if (img2vidEntity != 0)
			mgr.DestroyEntity(img2vidEntity);
		if (editEntity != 0)
			mgr.DestroyEntity(editEntity);
	}

	void VideoDiffusionView::InitializeComponentVisibility() {
		// Initialize all video-relevant components as visible by default
		componentVisibility["ModelComponent"] = true;
		componentVisibility["ClipLComponent"] = true;
		componentVisibility["ClipGComponent"] = true;
		componentVisibility["ClipVisionComponent"] = true;  // NEW: For I2V models
		componentVisibility["T5XXLComponent"] = true;
		componentVisibility["DiffusionModelComponent"] = true;
		componentVisibility["HighNoiseDiffusionModelComponent"] = true;  // NEW: For Wan 2.2
		componentVisibility["VaeComponent"] = true;
		componentVisibility["LoraComponent"] = true;
		componentVisibility["TaesdComponent"] = true;
		componentVisibility["LatentComponent"] = true;
		componentVisibility["SamplerComponent"] = true;
		componentVisibility["HighNoiseSamplerComponent"] = true;  // NEW: For Wan 2.2
		componentVisibility["VideoParamsComponent"] = true;  // NEW: Video settings
		componentVisibility["GuidanceComponent"] = true;
		componentVisibility["ClipSkipComponent"] = true;
		componentVisibility["PromptComponent"] = true;
		componentVisibility["LayerSkipComponent"] = true;
		componentVisibility["OutputImageComponent"] = true;
		componentVisibility["InputImageComponent"] = true;
		componentVisibility["EndImageComponent"] = true;  // NEW: For FLF2V
		componentVisibility["ControlnetComponent"] = true;
		componentVisibility["EmbeddingComponent"] = true;
		componentVisibility["EsrganComponent"] = true;
	}

	void VideoDiffusionView::Init() {
		ResetEntities();
	}

	void VideoDiffusionView::Update(float deltaT) {
		// Video diffusion view doesn't need to track videos - that's VideoView's job
		// This is just for generating videos, not viewing them
	}

	void VideoDiffusionView::ResetEntities() {
		if (img2vidEntity != 0) {
			mgr.DestroyEntity(img2vidEntity);
			img2vidEntity = 0;
		}

		if (editEntity != 0) {
			mgr.DestroyEntity(editEntity);
			editEntity = 0;
		}

		// Create entity for Img2Vid mode (requires input image)
		img2vidEntity = mgr.AddNewEntity();

		// Add components for Img2Vid - Include NEW video components
		mgr.AddComponent<ModelComponent>(img2vidEntity);
		mgr.AddComponent<ClipLComponent>(img2vidEntity);
		mgr.AddComponent<ClipGComponent>(img2vidEntity);
		mgr.AddComponent<ClipVisionComponent>(img2vidEntity);  // NEW: For I2V models
		mgr.AddComponent<T5XXLComponent>(img2vidEntity);
		mgr.AddComponent<DiffusionModelComponent>(img2vidEntity);
		mgr.AddComponent<HighNoiseDiffusionModelComponent>(img2vidEntity);  // NEW: For Wan 2.2
		mgr.AddComponent<VaeComponent>(img2vidEntity);
		mgr.AddComponent<LoraComponent>(img2vidEntity);
		mgr.AddComponent<TaesdComponent>(img2vidEntity);
		mgr.AddComponent<LatentComponent>(img2vidEntity);
		mgr.AddComponent<SamplerComponent>(img2vidEntity);
		mgr.AddComponent<HighNoiseSamplerComponent>(img2vidEntity);  // NEW: For Wan 2.2
		mgr.AddComponent<VideoParamsComponent>(img2vidEntity);  // NEW: Video parameters
		mgr.AddComponent<GuidanceComponent>(img2vidEntity);
		mgr.AddComponent<ClipSkipComponent>(img2vidEntity);
		mgr.AddComponent<PromptComponent>(img2vidEntity);
		mgr.AddComponent<LayerSkipComponent>(img2vidEntity);
		mgr.AddComponent<OutputImageComponent>(img2vidEntity);
		mgr.AddComponent<InputImageComponent>(img2vidEntity);

		// Create entity for Edit mode (also requires input image)
		editEntity = mgr.AddNewEntity();

		// Add components for Edit - Include NEW video components
		mgr.AddComponent<ModelComponent>(editEntity);
		mgr.AddComponent<ClipLComponent>(editEntity);
		mgr.AddComponent<ClipGComponent>(editEntity);
		mgr.AddComponent<ClipVisionComponent>(editEntity);  // NEW: For I2V models
		mgr.AddComponent<T5XXLComponent>(editEntity);
		mgr.AddComponent<DiffusionModelComponent>(editEntity);
		mgr.AddComponent<HighNoiseDiffusionModelComponent>(editEntity);  // NEW: For Wan 2.2
		mgr.AddComponent<VaeComponent>(editEntity);
		mgr.AddComponent<LoraComponent>(editEntity);
		mgr.AddComponent<TaesdComponent>(editEntity);
		mgr.AddComponent<LatentComponent>(editEntity);
		mgr.AddComponent<SamplerComponent>(editEntity);
		mgr.AddComponent<HighNoiseSamplerComponent>(editEntity);  // NEW: For Wan 2.2
		mgr.AddComponent<VideoParamsComponent>(editEntity);  // NEW: Video parameters
		mgr.AddComponent<GuidanceComponent>(editEntity);
		mgr.AddComponent<ClipSkipComponent>(editEntity);
		mgr.AddComponent<PromptComponent>(editEntity);
		mgr.AddComponent<LayerSkipComponent>(editEntity);
		mgr.AddComponent<OutputImageComponent>(editEntity);
		mgr.AddComponent<InputImageComponent>(editEntity);

		// Set default denoise values
		mgr.GetComponent<SamplerComponent>(img2vidEntity).denoise = 0.6f;
		mgr.GetComponent<SamplerComponent>(editEntity).denoise = 0.6f;
	}

	bool VideoDiffusionView::IsEntitySafeToUse(EntityID entity) const {
		return mgr.IsEntityValid(entity);
	}

	void VideoDiffusionView::RenderComponentWithCheckbox(const EntityID entity, const std::string& componentName, const std::string& displayName, const std::function<void()>& renderFunc) {
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

	void VideoDiffusionView::RenderEntityComponents(const EntityID entity) {
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
					// UNet/Diffusion Model
					RenderComponentWithCheckbox(entity, "DiffusionModelComponent", "Diffusion Model", [&]() {
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

					// NEW: High Noise Diffusion Model for Wan 2.2
					RenderComponentWithCheckbox(entity, "HighNoiseDiffusionModelComponent", "High Noise Model", [&]() {
						if (mgr.HasComponent<HighNoiseDiffusionModelComponent>(entity)) {
							auto& comp = mgr.GetComponent<HighNoiseDiffusionModelComponent>(entity);
							if (!comp.schema.empty()) {
								try {
									auto properties = comp.GetPropertyMap();
									UISchema::RenderSchema(comp.schema, properties);
								}
								catch (const std::exception& e) {
									ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering HighNoiseDiffusionModelComponent: %s", e.what());
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

						// NEW: CLIP Vision for I2V models
						RenderComponentWithCheckbox(entity, "ClipVisionComponent", "CLIP Vision", [&]() {
							if (mgr.HasComponent<ClipVisionComponent>(entity)) {
								auto& comp = mgr.GetComponent<ClipVisionComponent>(entity);
								if (!comp.schema.empty()) {
									try {
										auto properties = comp.GetPropertyMap();
										UISchema::RenderSchema(comp.schema, properties);
									}
									catch (const std::exception& e) {
										ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering ClipVisionComponent: %s", e.what());
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

		// Video Generation Options
		if (ImGui::CollapsingHeader("Video Generation Options", ImGuiTreeNodeFlags_DefaultOpen)) {
			// Video Dimensions (use LatentComponent for dimensions)
			RenderComponentWithCheckbox(entity, "LatentComponent", "Video Dimensions", [&]() {
				if (mgr.HasComponent<LatentComponent>(entity)) {
					auto& comp = mgr.GetComponent<LatentComponent>(entity);
					if (!comp.schema.empty()) {
						try {
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

			// NEW: Video Parameters for Wan models
			RenderComponentWithCheckbox(entity, "VideoParamsComponent", "Video Parameters", [&]() {
				if (mgr.HasComponent<VideoParamsComponent>(entity)) {
					auto& comp = mgr.GetComponent<VideoParamsComponent>(entity);
					if (!comp.schema.empty()) {
						try {
							auto properties = comp.GetPropertyMap();
							UISchema::RenderSchema(comp.schema, properties);

							// Show calculated duration
							ImGui::Separator();
							float duration = static_cast<float>(comp.video_frames) / comp.fps;
							ImGui::Text("Video Duration: %.2f seconds", duration);
						}
						catch (const std::exception& e) {
							ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering VideoParamsComponent: %s", e.what());
						}
					}
				}
			});

			// Sampler Settings
			RenderComponentWithCheckbox(entity, "SamplerComponent", "Main Sampler", [&]() {
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

			// NEW: High Noise Sampler for Wan 2.2
			RenderComponentWithCheckbox(entity, "HighNoiseSamplerComponent", "High Noise Sampler", [&]() {
				if (mgr.HasComponent<HighNoiseSamplerComponent>(entity)) {
					auto& comp = mgr.GetComponent<HighNoiseSamplerComponent>(entity);
					if (!comp.schema.empty()) {
						try {
							auto properties = comp.GetPropertyMap();
							UISchema::RenderSchema(comp.schema, properties);
						}
						catch (const std::exception& e) {
							ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering HighNoiseSamplerComponent: %s", e.what());
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

		// Input/Output Settings
		if (ImGui::CollapsingHeader("Input/Output Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
			// Input Image (both modes require this)
			RenderComponentWithCheckbox(entity, "InputImageComponent", "Input Image", [&]() {
				if (mgr.HasComponent<InputImageComponent>(entity)) {
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
				}
			});

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

		// Advanced Options (collapsed by default for video generation)
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

	void VideoDiffusionView::RenderComponentSchema(const EntityID entity, const std::string& componentName, BaseComponent* component) {
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

	void VideoDiffusionView::UpdateModelPath(const EntityID entity, const std::string& componentName) {
		// Handle special model path updates if needed for video models
	}

	void VideoDiffusionView::HandleImg2VidEvent() {
		std::cout << "Adding new Img2Vid entity..." << std::endl;

		EntityID newEntity = mgr.CloneEntity(img2vidEntity);
		if (newEntity == 0) {
			std::cerr << "Failed to create new entity!" << std::endl;
			return;
		}

		// Ensure output path is valid - use .mp4 extension for video
		if (mgr.HasComponent<OutputImageComponent>(newEntity)) {
			auto& outputComp = mgr.GetComponent<OutputImageComponent>(newEntity);
			if (outputComp.filePath.empty()) {
				outputComp.filePath = Utils::FilePaths::defaultProjectPath;
			}
			if (outputComp.fileName.empty()) {
				outputComp.fileName = "AniStudio_video.mp4";
			}
			// Force .mp4 extension for video
			std::string filename = outputComp.fileName;
			size_t lastDot = filename.find_last_of('.');
			if (lastDot != std::string::npos) {
				filename = filename.substr(0, lastDot);
			}
			outputComp.fileName = filename + ".mp4";

			std::filesystem::create_directories(outputComp.filePath);
		}

		// Queue event
		Event event;
		event.entityID = newEntity;
		event.type = EventType::Img2VidRequest;
		ANI::Events::Ref().QueueEvent(event);
	}

	void VideoDiffusionView::HandleEditEvent() {
		std::cout << "Adding new Edit entity..." << std::endl;

		EntityID newEntity = mgr.CloneEntity(editEntity);
		if (newEntity == 0) {
			std::cerr << "Failed to create new entity!" << std::endl;
			return;
		}

		// Ensure output path is valid - use .mp4 extension for video
		if (mgr.HasComponent<OutputImageComponent>(newEntity)) {
			auto& outputComp = mgr.GetComponent<OutputImageComponent>(newEntity);
			if (outputComp.filePath.empty()) {
				outputComp.filePath = Utils::FilePaths::defaultProjectPath;
			}
			if (outputComp.fileName.empty()) {
				outputComp.fileName = "AniStudio_edit.mp4";
			}
			// Force .mp4 extension for video
			std::string filename = outputComp.fileName;
			size_t lastDot = filename.find_last_of('.');
			if (lastDot != std::string::npos) {
				filename = filename.substr(0, lastDot);
			}
			outputComp.fileName = filename + ".mp4";

			std::filesystem::create_directories(outputComp.filePath);
		}

		// Queue event
		Event event;
		event.entityID = newEntity;
		event.type = EventType::EditRequest;
		ANI::Events::Ref().QueueEvent(event);
	}

	void VideoDiffusionView::RenderQueueList() {
		ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Video Queue")) {

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
				EntityID targetEntity = isImg2VidMode ? img2vidEntity : editEntity;
				if (mgr.HasComponent<LoraComponent>(targetEntity)) {
					auto& loraComp = mgr.GetComponent<LoraComponent>(targetEntity);
					loraComp.modelPath = Utils::FilePaths::loraDir;
				}

				for (int i = 0; i < numQueues; i++) {
					if (isImg2VidMode) {
						HandleImg2VidEvent();
					}
					else {
						HandleEditEvent();
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

	void VideoDiffusionView::Render() {
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

			// Tab bar for switching between Img2Vid and Edit
			if (ImGui::BeginTabBar("VideoDiffusion")) {
				// Image-to-Video tab
				if (ImGui::BeginTabItem("Img2Vid")) {
					isImg2VidMode = true;
					RenderEntityComponents(img2vidEntity);
					ImGui::EndTabItem();
				}

				// Edit tab
				if (ImGui::BeginTabItem("Edit")) {
					isImg2VidMode = false;
					RenderEntityComponents(editEntity);
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
		}
		ImGui::End();
	}

	nlohmann::json VideoDiffusionView::Serialize() const {
		EntityID targetEntity = isImg2VidMode ? img2vidEntity : editEntity;
		nlohmann::json j = mgr.SerializeEntity(targetEntity);

		// Also serialize component visibility states
		j["componentVisibility"] = componentVisibility;
		j["isImg2VidMode"] = isImg2VidMode;

		return j;
	}

	void VideoDiffusionView::Deserialize(const nlohmann::json& j) {
		EntityID targetEntity = isImg2VidMode ? img2vidEntity : editEntity;

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

			if (j.contains("isImg2VidMode")) {
				isImg2VidMode = j["isImg2VidMode"];
			}

			std::cout << "Successfully deserialized data to entity " << targetEntity << std::endl;
		}
		catch (const std::exception& e) {
			std::cerr << "Exception during deserialization: " << e.what() << std::endl;
		}
	}

	void VideoDiffusionView::SaveMetadataToJson(const std::string& filepath) {
		try {
			nlohmann::json metadata = Serialize();
			std::ofstream file(filepath);
			if (file.is_open()) {
				file << metadata.dump(4);
				file.close();
				std::cout << "Video metadata saved to: " << filepath << std::endl;
			}
			else {
				std::cerr << "Failed to open file for writing: " << filepath << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Error saving video metadata: " << e.what() << std::endl;
		}
	}

	void VideoDiffusionView::LoadMetadataFromJson(const std::string& filepath) {
		try {
			std::ifstream file(filepath);
			if (file.is_open()) {
				nlohmann::json metadata;
				file >> metadata;
				Deserialize(metadata);
				file.close();
				std::cout << "Video metadata loaded from: " << filepath << std::endl;
			}
			else {
				std::cerr << "Failed to open file for reading: " << filepath << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Error loading video metadata: " << e.what() << std::endl;
		}
	}

	void VideoDiffusionView::LoadMetadataFromVideo(const std::string& videoPath) {
		std::cout << "Attempting to load metadata from video: " << videoPath << std::endl;
		// TODO: Implement video metadata extraction (similar to PNG metadata extraction)
		// This would require reading video file metadata/comments
	}

	void VideoDiffusionView::RenderMetadataControls() {
		if (ImGui::Button("Save Video Metadata", ImVec2(-FLT_MIN, 0))) {
			IGFD::FileDialogConfig config;
			config.path = Utils::FilePaths::defaultProjectPath;
			ImGuiFileDialog::Instance()->OpenDialog("SaveVideoMetadataDialog", "Save Video Metadata", ".json", config);
		}

		if (ImGui::Button("Load Video Metadata", ImVec2(-FLT_MIN, 0))) {
			IGFD::FileDialogConfig config;
			config.path = Utils::FilePaths::defaultProjectPath;
			ImGuiFileDialog::Instance()->OpenDialog("LoadVideoMetadataDialog", "Load Video Metadata", ".json,.mp4,.avi,.mkv", config);
		}

		// Handle Save Dialog
		if (ImGuiFileDialog::Instance()->Display("SaveVideoMetadataDialog", 32, ImVec2(700, 400))) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				std::string filepath = ImGuiFileDialog::Instance()->GetFilePathName();
				SaveMetadataToJson(filepath);
			}
			ImGuiFileDialog::Instance()->Close();
		}

		// Handle Load Dialog
		if (ImGuiFileDialog::Instance()->Display("LoadVideoMetadataDialog", 32, ImVec2(700, 400))) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				std::string filepath = ImGuiFileDialog::Instance()->GetFilePathName();
				std::string extension = std::filesystem::path(filepath).extension().string();

				if (extension == ".json") {
					LoadMetadataFromJson(filepath);
				}
				else if (extension == ".mp4" || extension == ".avi" || extension == ".mkv") {
					LoadMetadataFromVideo(filepath);
				}
			}
			ImGuiFileDialog::Instance()->Close();
		}
	}

} // namespace GUI