#pragma once

#include "ImageComponent.hpp"
#include "stable-diffusion.h"
#include <string>
#include <vector>

namespace ECS {

    struct ControlNetImageComponent : public InputImageComponent {
        std::string controlType = "canny";
        float strength = 1.0f;

        ControlNetImageComponent() : InputImageComponent() {}

        const char* GetCompName() const override { return "ControlNetImage"; }
        const char* GetCompCategory() const override { return "Image"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "ControlNet Image"},
                {"type", "object"},
                {"properties", {
                    {"filePath", {
                        {"type", "string"},
                        {"title", "Control Image"},
                        {"ui:widget", "file_selector"},
                        {"ui:options", {
                            {"mode", "file"},
                            {"filters", ".png,.jpg,.jpeg,.bmp,.tga"},
                            {"filterName", "ControlNet Images"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Clear"},
                            {"browseTooltip", "Browse for control images (edge maps, depth maps, etc.)"}
                        }}
                    }},
                    {"controlType", {
                        {"type", "string"},
                        {"title", "Control Type"},
                        {"ui:widget", "combo"},
                        {"items", {
                            {{"label", "Canny Edge"}},
                            {{"label", "Depth Map"}},
                            {{"label", "Normal Map"}},
                            {{"label", "Scribble"}},
                            {{"label", "Segmentation"}},
                            {{"label", "OpenPose"}}
                        }}
                    }},
                    {"strength", {
                        {"type", "number"},
                        {"title", "Strength"},
                        {"ui:widget", "slider_float"},
                        {"minimum", 0.0},
                        {"maximum", 2.0},
                        {"ui:options", {{"step", 0.05}, {"format", "%.2f"}}}
                    }}
                }},
                {"propertyOrder", {"filePath", "controlType", "strength"}}
            };
            return j;
        }

        ControlNetImageComponent(const ControlNetImageComponent& other)
            : InputImageComponent(other)
            , controlType(other.controlType)
            , strength(other.strength) {
        }

        ControlNetImageComponent& operator=(const ControlNetImageComponent& other) {
            if (this != &other) {
                InputImageComponent::operator=(other);
                controlType = other.controlType;
                strength = other.strength;
            }
            return *this;
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            auto properties = InputImageComponent::GetPropertyMap();
            properties["controlType"] = &controlType;
            properties["strength"] = &strength;
            return properties;
        }

        nlohmann::json Serialize() const override {
            auto j = InputImageComponent::Serialize();
            j[GetCompName()]["controlType"] = controlType;
            j[GetCompName()]["strength"] = strength;
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            InputImageComponent::Deserialize(j);
            const char* key = GetCompName();
            nlohmann::json componentData;
            if (j.contains(key)) {
                componentData = j.at(key);
            }
            else {
                componentData = j;
            }
            if (componentData.contains("controlType"))
                controlType = componentData["controlType"];
            if (componentData.contains("strength"))
                strength = componentData["strength"];
        }
    };

    struct PhotoMakerImageComponent : public InputImageComponent {
        float styleStrength = 1.0f;
        bool isPrimaryID = true;

        PhotoMakerImageComponent() : InputImageComponent() {}

        const char* GetCompName() const override { return "PhotoMakerImage"; }
        const char* GetCompCategory() const override { return "Image"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "PhotoMaker ID Image"},
                {"type", "object"},
                {"properties", {
                    {"filePath", {
                        {"type", "string"},
                        {"title", "ID Image"},
                        {"ui:widget", "file_selector"},
                        {"ui:options", {
                            {"mode", "file"},
                            {"filters", ".png,.jpg,.jpeg,.bmp,.tga"},
                            {"filterName", "ID Images"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Clear"},
                            {"browseTooltip", "Browse for identity reference images for PhotoMaker"}
                        }}
                    }},
                    {"styleStrength", {
                        {"type", "number"},
                        {"title", "Style Strength"},
                        {"ui:widget", "slider_float"},
                        {"minimum", 0.0},
                        {"maximum", 2.0},
                        {"ui:options", {{"step", 0.05}, {"format", "%.2f"}}}
                    }},
                    {"isPrimaryID", {
                        {"type", "boolean"},
                        {"title", "Primary ID"},
                        {"ui:widget", "checkbox"}
                    }}
                }},
                {"propertyOrder", {"filePath", "styleStrength", "isPrimaryID"}}
            };
            return j;
        }

        PhotoMakerImageComponent(const PhotoMakerImageComponent& other)
            : InputImageComponent(other)
            , styleStrength(other.styleStrength)
            , isPrimaryID(other.isPrimaryID) {
        }

        PhotoMakerImageComponent& operator=(const PhotoMakerImageComponent& other) {
            if (this != &other) {
                InputImageComponent::operator=(other);
                styleStrength = other.styleStrength;
                isPrimaryID = other.isPrimaryID;
            }
            return *this;
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            auto properties = InputImageComponent::GetPropertyMap();
            properties["styleStrength"] = &styleStrength;
            properties["isPrimaryID"] = &isPrimaryID;
            return properties;
        }

        nlohmann::json Serialize() const override {
            auto j = InputImageComponent::Serialize();
            j[GetCompName()]["styleStrength"] = styleStrength;
            j[GetCompName()]["isPrimaryID"] = isPrimaryID;
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            InputImageComponent::Deserialize(j);
            const char* key = GetCompName();
            nlohmann::json componentData;
            if (j.contains(key)) {
                componentData = j.at(key);
            }
            else {
                componentData = j;
            }
            if (componentData.contains("styleStrength"))
                styleStrength = componentData["styleStrength"];
            if (componentData.contains("isPrimaryID"))
                isPrimaryID = componentData["isPrimaryID"];
        }
    };

    struct RefImagesComponent : public InputImageComponent {
        std::vector<std::string> ref_image_paths;
        std::string ref_image_args;

        RefImagesComponent() : InputImageComponent() {}

        const char* GetCompName() const override { return "RefImages"; }
        const char* GetCompCategory() const override { return "Image"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "Reference Images"},
                {"type", "object"},
                {"properties", {
                    {"ref_image_paths", {
                        {"type", "array"},
                        {"title", "Reference Image Paths"},
                        {"items", {
                            {"type", "string"},
                            {"ui:widget", "file_selector"},
                            {"ui:options", {
                                {"mode", "file"},
                                {"filters", ".png,.jpg,.jpeg,.bmp,.tga"},
                                {"filterName", "Reference Images"}
                            }}
                        }}
                    }},
                    {"ref_image_args", {
                        {"type", "string"},
                        {"title", "Ref Image Args"},
                        {"ui:widget", "textarea"}
                    }}
                }},
                {"propertyOrder", {"ref_image_paths", "ref_image_args"}}
            };
            return j;
        }

        RefImagesComponent(const RefImagesComponent& other)
            : InputImageComponent(other)
            , ref_image_paths(other.ref_image_paths)
            , ref_image_args(other.ref_image_args) {
        }

        RefImagesComponent& operator=(const RefImagesComponent& other) {
            if (this != &other) {
                InputImageComponent::operator=(other);
                ref_image_paths = other.ref_image_paths;
                ref_image_args = other.ref_image_args;
            }
            return *this;
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            auto properties = InputImageComponent::GetPropertyMap();
            properties["ref_image_paths"] = &ref_image_paths;
            properties["ref_image_args"] = &ref_image_args;
            return properties;
        }

        nlohmann::json Serialize() const override {
            auto j = InputImageComponent::Serialize();
            j[GetCompName()]["ref_image_paths"] = ref_image_paths;
            j[GetCompName()]["ref_image_args"] = ref_image_args;
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            InputImageComponent::Deserialize(j);
            const char* key = GetCompName();
            nlohmann::json componentData;
            if (j.contains(key)) {
                componentData = j.at(key);
            }
            else {
                componentData = j;
            }
            if (componentData.contains("ref_image_paths"))
                ref_image_paths = componentData["ref_image_paths"].get<std::vector<std::string>>();
            if (componentData.contains("ref_image_args"))
                ref_image_args = componentData["ref_image_args"].get<std::string>();
        }
    };

    struct ControlFramesComponent : public InputImageComponent {
        std::vector<std::string> filePaths;

        ControlFramesComponent() : InputImageComponent() {}

        const char* GetCompName() const override { return "ControlFrames"; }
        const char* GetCompCategory() const override { return "Video"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "Control Frames"},
                {"type", "object"},
                {"properties", {
                    {"filePaths", {
                        {"type", "array"},
                        {"title", "Frame Paths"},
                        {"items", {
                            {"type", "string"},
                            {"ui:widget", "file_selector"},
                            {"ui:options", {
                                {"mode", "file"},
                                {"filters", ".png,.jpg,.jpeg,.bmp,.tga"},
                                {"filterName", "Control Frames"}
                            }}
                        }}
                    }}
                }},
                {"propertyOrder", {"filePaths"}}
            };
            return j;
        }

        ControlFramesComponent(const ControlFramesComponent& other)
            : InputImageComponent(other)
            , filePaths(other.filePaths) {
        }

        ControlFramesComponent& operator=(const ControlFramesComponent& other) {
            if (this != &other) {
                InputImageComponent::operator=(other);
                filePaths = other.filePaths;
            }
            return *this;
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            auto properties = InputImageComponent::GetPropertyMap();
            properties["filePaths"] = &filePaths;
            return properties;
        }

        nlohmann::json Serialize() const override {
            auto j = InputImageComponent::Serialize();
            j[GetCompName()]["filePaths"] = filePaths;
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            InputImageComponent::Deserialize(j);
            const char* key = GetCompName();
            nlohmann::json componentData;
            if (j.contains(key)) {
                componentData = j.at(key);
            }
            else {
                componentData = j;
            }
            if (componentData.contains("filePaths"))
                filePaths = componentData["filePaths"].get<std::vector<std::string>>();
        }
    };
} // namespace ECS