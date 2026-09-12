#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include <string>

namespace ECS {

	struct PromptComponent : public BaseComponent {
		std::string posPrompt = "";
		std::string negPrompt = "";
		bool normalize_input = false;

		PromptComponent() = default;

		const char* GetCompName() const override { return "Prompt"; }
		const char* GetCompCategory() const override { return "Sampling"; }

		const nlohmann::json& GetSchema() const override {
			static const nlohmann::json j = {
				{"title", "Prompt Settings"},
				{"type", "object"},
				{"propertyOrder", {"normalize_input", "posPrompt", "negPrompt"}},
				{"properties", {
					 {"normalize_input", {
						{"type", "boolean"},
						{"title", "Normalize Tokens"},
						{"description", "Normalization of token attention across chunks"},
						{"ui:widget", "checkbox"}
					}},
					{"posPrompt", {
						{"type", "string"},
						{"title", "Positive"},
						{"ui:widget", "text_editor"},
						{"ui:window_name", "Positive Prompt"},
						{"ui:options", {
							{"maxLength", 8192},
							{"showMenuBar", true}
						}}
					}},
					{"negPrompt", {
						{"type", "string"},
						{"title", "Negative"},
						{"ui:widget", "text_editor"},
						{"ui:window_name", "Negative Prompt"},
						{"ui:options", {
							{"maxLength", 8192},
							{"showMenuBar", true}
						}}
					}}
				}}
			};
			return j;
		}

		PromptComponent(const PromptComponent& other) : BaseComponent(other) {
			posPrompt = other.posPrompt;
			negPrompt = other.negPrompt;
			normalize_input = other.normalize_input;
		}

		PromptComponent& operator=(const PromptComponent& other) {
			if (this != &other) {
				posPrompt = other.posPrompt;
				negPrompt = other.negPrompt;
				normalize_input = other.normalize_input;
			}
			return *this;
		}

		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			return {
				{"posPrompt", &posPrompt},
				{"negPrompt", &negPrompt},
				{"normalize_input", &normalize_input}
			};
		}

		nlohmann::json Serialize() const override {
			nlohmann::json j;
			j[GetCompName()] = {
				{"posPrompt", posPrompt},
				{"negPrompt", negPrompt},
				{"normalize_input", normalize_input}
			};
			return j;
		}

		void Deserialize(const nlohmann::json& j) override {
			const char* key = GetCompName();

			nlohmann::json componentData;
			if (j.contains(key)) {
				componentData = j.at(key);
			}
			else {
				for (auto it = j.begin(); it != j.end(); ++it) {
					if (it.key() == key) {
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
			}
			if (componentData.contains("negPrompt")) {
				negPrompt = componentData["negPrompt"].get<std::string>();
			}
			if (componentData.contains("normalize_input")) {
				normalize_input = componentData["normalize_input"].get<bool>();
			}
		}
	};

} // namespace ECS