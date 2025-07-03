/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y888b. 888    888  888 d88" 888 888 d88""88b
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

#pragma once

#include "BaseComponent.hpp"
#include "FilePaths.hpp"
#include "imgui.h"

namespace ECS {

	struct ChromaComponent : public BaseComponent {
		bool use_dit_mask = false;          // chroma_use_dit_mask parameter
		bool use_t5_mask = false;           // chroma_use_t5_mask parameter
		int t5_mask_pad = 0;                // chroma_t5_mask_pad parameter

		// Additional Chroma settings
		bool enable_chroma_mode = false;    // Whether to enable Chroma-specific features
		std::string chroma_model_type = "standard";  // Type of Chroma model being used

		ChromaComponent() {
			compName = "Chroma";
			compCategory = "Advanced";

			// Define the JSON schema for this component
			schema = {
				{"title", "Chroma Settings"},
				{"type", "object"},
				{"properties", {
					{"use_dit_mask", {
						{"type", "boolean"},
						{"title", "Use DiT Mask"},
						{"description", "Enable DiT (Diffusion in Transformers) masking for Chroma models"},
						{"default", false}
					}},
					{"use_t5_mask", {
						{"type", "boolean"},
						{"title", "Use T5 Mask"},
						{"description", "Enable T5 text encoder masking for improved prompt attention"},
						{"default", false}
					}},
					{"t5_mask_pad", {
						{"type", "integer"},
						{"title", "T5 Mask Padding"},
						{"description", "Number of padding tokens to unmask in T5 encoder (0 = auto)"},
						{"minimum", 0},
						{"maximum", 100},
						{"default", 0}
					}},
					{"enable_chroma_mode", {
						{"type", "boolean"},
						{"title", "Enable Chroma Mode"},
						{"description", "Enable Chroma-specific optimizations and features"},
						{"default", false}
					}},
					{"chroma_model_type", {
						{"type", "string"},
						{"title", "Chroma Model Type"},
						{"description", "Type of Chroma model being used"},
						{"enum", {"standard", "flux_schnell", "experimental"}},
						{"default", "standard"}
					}}
				}}
			};
		}

		// UI property mapping for ImGui rendering
		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			return {
				{"use_dit_mask", &use_dit_mask},
				{"use_t5_mask", &use_t5_mask},
				{"t5_mask_pad", &t5_mask_pad},
				{"enable_chroma_mode", &enable_chroma_mode},
				{"chroma_model_type", &chroma_model_type}
			};
		}

		// Serialization
		nlohmann::json Serialize() const override {
			nlohmann::json j = BaseComponent::Serialize();
			j[compName] = {
				{"use_dit_mask", use_dit_mask},
				{"use_t5_mask", use_t5_mask},
				{"t5_mask_pad", t5_mask_pad},
				{"enable_chroma_mode", enable_chroma_mode},
				{"chroma_model_type", chroma_model_type}
			};
			return j;
		}

		// Deserialization
		void Deserialize(const nlohmann::json& j) override {
			BaseComponent::Deserialize(j);
			if (j.contains(compName)) {
				auto comp = j[compName];
				if (comp.contains("use_dit_mask")) use_dit_mask = comp["use_dit_mask"];
				if (comp.contains("use_t5_mask")) use_t5_mask = comp["use_t5_mask"];
				if (comp.contains("t5_mask_pad")) t5_mask_pad = comp["t5_mask_pad"];
				if (comp.contains("enable_chroma_mode")) enable_chroma_mode = comp["enable_chroma_mode"];
				if (comp.contains("chroma_model_type")) chroma_model_type = comp["chroma_model_type"];
			}
		}
	};

	// Component for Stacked ID Embeddings (PhotoMaker support)
	// TODO: move this somewhere else
	struct StackedIdEmbedComponent : public BaseComponent {
		std::string modelName = "";
		std::string modelPath = "";
		bool enabled = false;
		float strength = 1.0f;

		StackedIdEmbedComponent() {
			compName = "StackedIdEmbed";
			compCategory = "Model";

			schema = {
				{"title", "Stacked ID Embedding"},
				{"type", "object"},
				{"properties", {
					{"modelName", {
						{"type", "string"},
						{"title", "Model Name"},
						{"description", "Name of the stacked ID embedding model"},
						{"default", ""}
					}},
					{"modelPath", {
						{"type", "string"},
						{"title", "Model Path"},
						{"description", "Full path to the stacked ID embedding model file"},
						{"default", ""}
					}},
					{"enabled", {
						{"type", "boolean"},
						{"title", "Enabled"},
						{"description", "Enable stacked ID embedding processing"},
						{"default", false}
					}},
					{"strength", {
						{"type", "number"},
						{"title", "Strength"},
						{"description", "Strength of the ID embedding effect"},
						{"minimum", 0.0},
						{"maximum", 2.0},
						{"default", 1.0}
					}}
				}}
			};
		}

		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			return {
				{"modelName", &modelName},
				{"modelPath", &modelPath},
				{"enabled", &enabled},
				{"strength", &strength}
			};
		}

		nlohmann::json Serialize() const override {
			nlohmann::json j = BaseComponent::Serialize();
			j[compName] = {
				{"modelName", modelName},
				{"modelPath", modelPath},
				{"enabled", enabled},
				{"strength", strength}
			};
			return j;
		}

		void Deserialize(const nlohmann::json& j) override {
			BaseComponent::Deserialize(j);
			if (j.contains(compName)) {
				auto comp = j[compName];
				if (comp.contains("modelName")) modelName = comp["modelName"];
				if (comp.contains("modelPath")) modelPath = comp["modelPath"];
				if (comp.contains("enabled")) enabled = comp["enabled"];
				if (comp.contains("strength")) strength = comp["strength"];
			}
		}
	};

} // namespace ECS