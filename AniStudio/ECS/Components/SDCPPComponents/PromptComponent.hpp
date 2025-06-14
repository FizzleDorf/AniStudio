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

#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include "UISchema.hpp"
#include <string>

namespace ECS {

	struct PromptComponent : public BaseComponent {
		PromptComponent() {
			compName = "Prompt";

			// Schema that creates SEPARATE IMGUI WINDOWS via UISchema
			schema = {
				{"title", "Prompt Settings"},
				{"type", "object"},
				{"propertyOrder", {"posPrompt", "negPrompt"}},
				{"ui:separate_windows", true},  // Enable separate window mode
				{"properties", {
					{"posPrompt", {
						{"type", "string"},
						{"title", "Positive"},
						{"ui:widget", "zep_editor"},  // Use Zep editor with full functionality
						{"ui:window", true},  // Create separate window for this property
						{"ui:window_name", "Positive Prompt"},  // Window name
						{"ui:options", {
							{"maxLength", 8192},
							{"showMenuBar", true}
						}}
					}},
					{"negPrompt", {
						{"type", "string"},
						{"title", "Negative"},
						{"ui:widget", "zep_editor"},  // Use Zep editor with full functionality
						{"ui:window", true},  // Create separate window for this property
						{"ui:window_name", "Negative Prompt"},  // Window name
						{"ui:options", {
							{"maxLength", 8192},
							{"showMenuBar", true}
						}}
					}}
				}}
			};
		}

		std::string posPrompt = "";
		std::string negPrompt = "";

		// Override the GetPropertyMap method to map component properties to UI schema
		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			std::unordered_map<std::string, UISchema::PropertyVariant> properties;
			properties["posPrompt"] = &posPrompt;
			properties["negPrompt"] = &negPrompt;
			return properties;
		}

		PromptComponent& operator=(const PromptComponent& other) {
			if (this != &other) {
				posPrompt = other.posPrompt;
				negPrompt = other.negPrompt;
			}
			return *this;
		}

		nlohmann::json Serialize() const override {
			nlohmann::json j;
			j["compName"] = compName;
			j[compName] = {
				{"posPrompt", posPrompt},
				{"negPrompt", negPrompt}
			};
			return j;
		}

		void Deserialize(const nlohmann::json& j) override {
			BaseComponent::Deserialize(j);

			nlohmann::json componentData;

			if (j.contains(compName)) {
				componentData = j.at(compName);
			}
			else {
				for (auto it = j.begin(); it != j.end(); ++it) {
					if (it.key() == compName) {
						componentData = it.value();
						break;
					}
				}
				if (componentData.empty()) {
					componentData = j;
				}
			}

			if (componentData.contains("posPrompt")) {
				posPrompt = componentData["posPrompt"].get<std::string>();
				UISchema::StringWidgets::UpdateEditorContent(&posPrompt);
			}

			if (componentData.contains("negPrompt")) {
				negPrompt = componentData["negPrompt"].get<std::string>();
				UISchema::StringWidgets::UpdateEditorContent(&negPrompt);
			}
		}

		// Cleanup method to be called when component is destroyed
		~PromptComponent() {
			// Clean up the Zep editor instances for this component
			UISchema::StringWidgets::CleanupEditor(&posPrompt);
			UISchema::StringWidgets::CleanupEditor(&negPrompt);
		}
	};

} // namespace ECS