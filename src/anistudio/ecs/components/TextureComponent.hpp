#pragma once
#include "BaseComponent.hpp"
#include <GL/glew.h>

namespace ECS {

    struct TextureComponent : public BaseComponent {
        GLuint textureID = 0;
        int width = 0;
        int height = 0;
        int channels = 0;
        bool needsUpdate = false;

        TextureComponent() = default;

        const char* GetCompName() const override { return "Texture"; }
        const char* GetCompCategory() const override { return "Rendering"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = nlohmann::json::object();
            return j;
        }

        TextureComponent(const TextureComponent& other) : BaseComponent(other) {
            textureID = 0;
            width = other.width;
            height = other.height;
            channels = other.channels;
            needsUpdate = other.needsUpdate;
        }

        TextureComponent& operator=(const TextureComponent& other) {
            if (this != &other) {
                width = other.width;
                height = other.height;
                channels = other.channels;
                needsUpdate = other.needsUpdate;
            }
            return *this;
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"textureID", &textureID},
                {"width", &width},
                {"height", &height},
                {"channels", &channels},
                {"needsUpdate", &needsUpdate}
            };
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"textureID", textureID},
                {"width", width},
                {"height", height},
                {"channels", channels},
                {"needsUpdate", needsUpdate}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();
            nlohmann::json componentData;
            if (j.contains(key))
                componentData = j.at(key);
            else
                componentData = j;

            if (componentData.contains("textureID")) textureID = componentData["textureID"];
            if (componentData.contains("width")) width = componentData["width"];
            if (componentData.contains("height")) height = componentData["height"];
            if (componentData.contains("channels")) channels = componentData["channels"];
            if (componentData.contains("needsUpdate")) needsUpdate = componentData["needsUpdate"];
        }
    };

}