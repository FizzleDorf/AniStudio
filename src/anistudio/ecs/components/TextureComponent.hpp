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

        TextureComponent() {
            compName = "Texture";
            compCategory = "Rendering";
        }

        virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            std::unordered_map<std::string, UISchema::PropertyVariant> props;
            props["textureID"] = &textureID;
            props["width"] = &width;
            props["height"] = &height;
            props["channels"] = &channels;
            props["needsUpdate"] = &needsUpdate;
            return props;
        }

        virtual nlohmann::json Serialize() const override {
            nlohmann::json j;
            j["compName"] = compName;
            j[compName] = {
                {"textureID", textureID},
                {"width", width},
                {"height", height},
                {"channels", channels},
                {"needsUpdate", needsUpdate}
            };
            return j;
        }

        virtual void Deserialize(const nlohmann::json& j) override {
            BaseComponent::Deserialize(j);
            auto& data = j.contains(compName) ? j[compName] : j;
            if (data.contains("textureID")) textureID = data["textureID"];
            if (data.contains("width")) width = data["width"];
            if (data.contains("height")) height = data["height"];
            if (data.contains("channels")) channels = data["channels"];
            if (data.contains("needsUpdate")) needsUpdate = data["needsUpdate"];
        }
    };

}