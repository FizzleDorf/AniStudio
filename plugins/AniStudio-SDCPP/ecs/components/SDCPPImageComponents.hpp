#pragma once

#include "ImageComponent.hpp"
#include "stable-diffusion.h"
#include <string>
#include <vector>

namespace ECS {

    struct ControlNetImageComponent : public InputImageComponent {
        std::string controlType = "canny";
        float strength = 1.0f;
        float startStep = 0.0f;
        float endStep = 1.0f;

        ControlNetImageComponent() : InputImageComponent() {
            compName = "ControlNetImage";
            compCategory = "Image";
            setupControlNetSchema();
        }

        virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            auto properties = InputImageComponent::GetPropertyMap();
            properties["controlType"] = &controlType;
            properties["strength"] = &strength;
            properties["startStep"] = &startStep;
            properties["endStep"] = &endStep;
            return properties;
        }

        virtual nlohmann::json Serialize() const override {
            auto j = InputImageComponent::Serialize();
            j[compName]["controlType"] = controlType;
            j[compName]["strength"] = strength;
            j[compName]["startStep"] = startStep;
            j[compName]["endStep"] = endStep;
            return j;
        }

        virtual void Deserialize(const nlohmann::json& j) override {
            InputImageComponent::Deserialize(j);
            nlohmann::json componentData;
            if (j.contains(compName)) {
                componentData = j.at(compName);
            }
            if (componentData.contains("controlType"))
                controlType = componentData["controlType"];
            if (componentData.contains("strength"))
                strength = componentData["strength"];
            if (componentData.contains("startStep"))
                startStep = componentData["startStep"];
            if (componentData.contains("endStep"))
                endStep = componentData["endStep"];
        }

    private:
        void setupControlNetSchema() {
            schema = {
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
                    }},
                    {"startStep", {
                        {"type", "number"},
                        {"title", "Start Step"},
                        {"ui:widget", "slider_float"},
                        {"minimum", 0.0},
                        {"maximum", 1.0},
                        {"ui:options", {{"step", 0.05}, {"format", "%.2f"}}}
                    }},
                    {"endStep", {
                        {"type", "number"},
                        {"title", "End Step"},
                        {"ui:widget", "slider_float"},
                        {"minimum", 0.0},
                        {"maximum", 1.0},
                        {"ui:options", {{"step", 0.05}, {"format", "%.2f"}}}
                    }}
                }},
                {"propertyOrder", {"filePath", "controlType", "strength", "startStep", "endStep"}}
            };
        }
    };

    struct PhotoMakerImageComponent : public InputImageComponent {
        float styleStrength = 1.0f;
        bool isPrimaryID = true;

        PhotoMakerImageComponent() : InputImageComponent() {
            compName = "PhotoMakerImage";
            compCategory = "Image";
            setupPhotoMakerSchema();
        }

        virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            auto properties = InputImageComponent::GetPropertyMap();
            properties["styleStrength"] = &styleStrength;
            properties["isPrimaryID"] = &isPrimaryID;
            return properties;
        }

        virtual nlohmann::json Serialize() const override {
            auto j = InputImageComponent::Serialize();
            j[compName]["styleStrength"] = styleStrength;
            j[compName]["isPrimaryID"] = isPrimaryID;
            return j;
        }

        virtual void Deserialize(const nlohmann::json& j) override {
            InputImageComponent::Deserialize(j);
            nlohmann::json componentData;
            if (j.contains(compName)) {
                componentData = j.at(compName);
            }
            if (componentData.contains("styleStrength"))
                styleStrength = componentData["styleStrength"];
            if (componentData.contains("isPrimaryID"))
                isPrimaryID = componentData["isPrimaryID"];
        }

    private:
        void setupPhotoMakerSchema() {
            schema = {
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
        }
    };

    struct RefImagesComponent : public InputImageComponent {
        std::vector<std::string> ref_image_paths;
        std::string ref_image_args;

        RefImagesComponent() : InputImageComponent() {
            compName = "RefImages";
            compCategory = "Image";
            setupRefImagesSchema();
        }

        virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            auto properties = InputImageComponent::GetPropertyMap();
            properties["ref_image_paths"] = &ref_image_paths;
            properties["ref_image_args"] = &ref_image_args;
            return properties;
        }

        virtual nlohmann::json Serialize() const override {
            auto j = InputImageComponent::Serialize();
            j[compName]["ref_image_paths"] = ref_image_paths;
            j[compName]["ref_image_args"] = ref_image_args;
            return j;
        }

        virtual void Deserialize(const nlohmann::json& j) override {
            InputImageComponent::Deserialize(j);
            nlohmann::json componentData;
            if (j.contains(compName)) {
                componentData = j.at(compName);
            }
            if (componentData.contains("ref_image_paths"))
                ref_image_paths = componentData["ref_image_paths"].get<std::vector<std::string>>();
            if (componentData.contains("ref_image_args"))
                ref_image_args = componentData["ref_image_args"].get<std::string>();
        }

    private:
        void setupRefImagesSchema() {
            schema = {
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
        }
    };

    struct ControlFramesComponent : public InputImageComponent {
        std::vector<std::string> filePaths;

        ControlFramesComponent() : InputImageComponent() {
            compName = "ControlFrames";
            compCategory = "Video";
            setupControlFramesSchema();
        }

        virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            auto properties = InputImageComponent::GetPropertyMap();
            properties["filePaths"] = &filePaths;
            return properties;
        }

        virtual nlohmann::json Serialize() const override {
            auto j = InputImageComponent::Serialize();
            j[compName]["filePaths"] = filePaths;
            return j;
        }

        virtual void Deserialize(const nlohmann::json& j) override {
            InputImageComponent::Deserialize(j);
            if (j.contains(compName)) {
                auto compData = j.at(compName);
                if (compData.contains("filePaths"))
                    filePaths = compData["filePaths"].get<std::vector<std::string>>();
            }
        }

    private:
        void setupControlFramesSchema() {
            schema = {
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
        }
    };
} // namespace ECS