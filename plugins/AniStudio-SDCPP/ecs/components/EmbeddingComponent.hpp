#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include <string>
#include <vector>
#include <utility>

namespace ECS {

    struct EmbeddingsComponent : public BaseComponent {
        std::vector<std::pair<std::string, std::string>> embeddings; // name, path

        EmbeddingsComponent() {
            compName = "Embeddings";
            compCategory = "Model";
            schema = {
                {"title", "Textual Inversion Embeddings"},
                {"type", "object"},
                {"properties", {
                    {"embeddings", {
                        {"type", "array"},
                        {"title", "Embeddings"},
                        {"description", "List of textual inversion embeddings (name and file path)"},
                        {"items", {
                            {"type", "object"},
                            {"properties", {
                                {"name", {"type", "string", "title", "Name", "description", "Embedding name used in prompts"}},
                                {"path", {
                                    {"type", "string"},
                                    {"title", "Path"},
                                    {"description", "File path to the embedding file"},
                                    {"ui:widget", "file_selector"},
                                    {"ui:options", {
                                        {"mode", "file"},
                                        {"filters", ".pt,.safetensors,.bin"},
                                        {"filterName", "Embedding Files"},
                                        {"dialogDefaultPath", "embeddings"},
                                        {"buttonText", "Browse..."},
                                        {"resetButtonText", "Clear"},
                                        {"browseTooltip", "Select embedding file"}
                                    }}
                                }}
                            }}
                        }}
                    }}
                }}
            };
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"embeddings", &embeddings}
            };
        }

        EmbeddingsComponent& operator=(const EmbeddingsComponent& other) {
            if (this != &other) {
                embeddings = other.embeddings;
            }
            return *this;
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            nlohmann::json arr = nlohmann::json::array();
            for (const auto& e : embeddings) {
                arr.push_back({ {"name", e.first}, {"path", e.second} });
            }
            j[compName] = arr;
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            BaseComponent::Deserialize(j);
            nlohmann::json arr;
            if (j.contains(compName)) {
                arr = j.at(compName);
            }
            else {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (it.key() == compName) {
                        arr = it.value();
                        break;
                    }
                }
            }
            embeddings.clear();
            if (arr.is_array()) {
                for (const auto& item : arr) {
                    std::string name, path;
                    if (item.contains("name")) name = item["name"].get<std::string>();
                    if (item.contains("path")) path = item["path"].get<std::string>();
                    if (!name.empty() || !path.empty()) {
                        embeddings.emplace_back(name, path);
                    }
                }
            }
        }

        std::vector<sd_embedding_t> to_sd_embeddings() const {
            std::vector<sd_embedding_t> result;
            result.reserve(embeddings.size());
            for (const auto& e : embeddings) {
                sd_embedding_t emb;
                emb.name = e.first.c_str();
                emb.path = e.second.c_str();
                result.push_back(emb);
            }
            return result;
        }
    };

} // namespace ECS