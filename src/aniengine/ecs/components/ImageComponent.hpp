#pragma once

#include "BaseComponent.hpp"
#include "OpenGLWrapper.hpp"
#include "FileFormats.hpp"
#include <string>
#include <stb_image.h>
#include <memory>
#include <chrono>
#include <filesystem>

namespace ECS {
    struct ImageComponent : public BaseComponent {
        std::string fileName = "AniStudio";
        std::string filePath = "";
        std::shared_ptr<unsigned char[]> imageDataPtr;
        unsigned char* imageData = nullptr;
        int width = 0;
        int height = 0;
        int channels = 0;
        GLuint textureID = 0;
        bool hasExifData = false;
        bool hasLSBData = false;
        bool hasAniStudioMetadata = false;

        uint64_t fileSize = 0;
        std::string fileDate;
        std::string fileTime;

        ImageComponent() {
            compName = "Image";
            compCategory = "Image";
            setupBaseSchema();
        }

        virtual ~ImageComponent() {
            if (textureID != 0) {
                glDeleteTextures(1, &textureID);
                textureID = 0;
            }
            imageDataPtr.reset();
            imageData = nullptr;
        }

        virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            std::unordered_map<std::string, UISchema::PropertyVariant> properties;
            properties["fileName"] = &fileName;
            properties["filePath"] = &filePath;
            properties["fileSize"] = &fileSize;
            properties["fileDate"] = &fileDate;
            properties["fileTime"] = &fileTime;
            return properties;
        }

        virtual nlohmann::json Serialize() const override {
            nlohmann::json j;
            j["compName"] = compName;
            j[compName] = {
                {"width", width},
                {"height", height},
                {"channels", channels},
                {"fileName", fileName},
                {"filePath", filePath},
                {"fileSize", fileSize},
                {"fileDate", fileDate},
                {"fileTime", fileTime},
                {"hasExifData", hasExifData},
                {"hasLSBData", hasLSBData},
                {"hasAniStudioMetadata", hasAniStudioMetadata}
            };
            return j;
        }

        virtual void Deserialize(const nlohmann::json& j) override {
            BaseComponent::Deserialize(j);

            nlohmann::json componentData;
            if (j.contains(compName))
                componentData = j.at(compName);
            else
                componentData = j;

            if (componentData.contains("width")) width = componentData["width"];
            if (componentData.contains("height")) height = componentData["height"];
            if (componentData.contains("channels")) channels = componentData["channels"];
            if (componentData.contains("fileName")) fileName = componentData["fileName"];
            if (componentData.contains("filePath")) filePath = componentData["filePath"];
            if (componentData.contains("fileSize")) fileSize = componentData["fileSize"];
            if (componentData.contains("fileDate")) fileDate = componentData["fileDate"];
            if (componentData.contains("fileTime")) fileTime = componentData["fileTime"];
            if (componentData.contains("hasExifData")) hasExifData = componentData["hasExifData"];
            if (componentData.contains("hasLSBData")) hasLSBData = componentData["hasLSBData"];
            if (componentData.contains("hasAniStudioMetadata")) hasAniStudioMetadata = componentData["hasAniStudioMetadata"];
        }

        void SetImageData(unsigned char* data, int w, int h, int ch) {
            if (data && w > 0 && h > 0 && ch > 0) {
                size_t dataSize = w * h * ch;
                imageDataPtr = std::shared_ptr<unsigned char[]>(
                    data,
                    [](unsigned char* ptr) {
                        if (ptr) stbi_image_free(ptr);
                    }
                );
                imageData = imageDataPtr.get();
                width = w;
                height = h;
                channels = ch;
            }
            else {
                ClearImageData();
            }
        }

        void ClearImageData() {
            imageDataPtr.reset();
            imageData = nullptr;
            width = 0;
            height = 0;
            channels = 0;
        }

        ImageComponent& operator=(const ImageComponent& other) {
            if (this != &other) {
                fileName = other.fileName;
                filePath = other.filePath;
                width = other.width;
                height = other.height;
                channels = other.channels;
                fileSize = other.fileSize;
                fileDate = other.fileDate;
                fileTime = other.fileTime;
                hasExifData = other.hasExifData;
                hasLSBData = other.hasLSBData;
                hasAniStudioMetadata = other.hasAniStudioMetadata;

                if (other.imageData && other.width > 0 && other.height > 0 && other.channels > 0) {
                    size_t dataSize = other.width * other.height * other.channels;
                    unsigned char* newData = static_cast<unsigned char*>(malloc(dataSize));
                    if (newData) {
                        memcpy(newData, other.imageData, dataSize);
                        SetImageData(newData, other.width, other.height, other.channels);
                    }
                }
                else {
                    ClearImageData();
                }
            }
            return *this;
        }

        ImageComponent(const ImageComponent& other) : BaseComponent(other) {
            fileName = other.fileName;
            filePath = other.filePath;
            width = other.width;
            height = other.height;
            channels = other.channels;
            imageData = nullptr;
            textureID = 0;
            fileSize = other.fileSize;
            fileDate = other.fileDate;
            fileTime = other.fileTime;
            hasExifData = other.hasExifData;
            hasLSBData = other.hasLSBData;
            hasAniStudioMetadata = other.hasAniStudioMetadata;

            if (other.imageData && other.width > 0 && other.height > 0 && other.channels > 0) {
                size_t dataSize = other.width * other.height * other.channels;
                unsigned char* newData = static_cast<unsigned char*>(malloc(dataSize));
                if (newData) {
                    memcpy(newData, other.imageData, dataSize);
                    SetImageData(newData, other.width, other.height, other.channels);
                }
            }

            setupBaseSchema();
        }

    protected:
        void setupBaseSchema() {
            schema = {
                {"title", "Image"},
                {"type", "object"},
                {"properties", {
                    {"fileName", {
                        {"type", "string"},
                        {"title", "File Name"}
                    }},
                    {"filePath", {
                        {"type", "string"},
                        {"title", "File Path"}
                    }},
                    {"fileSize", {
                        {"type", "integer"},
                        {"title", "File Size (bytes)"}
                    }},
                    {"fileDate", {
                        {"type", "string"},
                        {"title", "Date Modified"}
                    }},
                    {"fileTime", {
                        {"type", "string"},
                        {"title", "Time Modified"}
                    }}
                }}
            };
        }
    };

    struct InputImageComponent : public ImageComponent {
        InputImageComponent() {
            compName = "InputImage";
            compCategory = "Image";
            fileName = "";
            filePath = "";
            width = 0;
            height = 0;
            channels = 0;
            setupInputSchema();
        }

        virtual ~InputImageComponent() {
            // ImageComponent destructor handles cleanup
        }

        virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            std::unordered_map<std::string, UISchema::PropertyVariant> properties;
            properties["fileName"] = &fileName;
            properties["filePath"] = &filePath;
            properties["width"] = &width;
            properties["height"] = &height;
            properties["channels"] = &channels;
            return properties;
        }

        virtual nlohmann::json Serialize() const override {
            nlohmann::json j;
            j["compName"] = compName;
            j[compName] = {
                {"fileName", fileName},
                {"filePath", filePath},
                {"width", width},
                {"height", height},
                {"channels", channels}
            };
            return j;
        }

        virtual void Deserialize(const nlohmann::json& j) override {
            BaseComponent::Deserialize(j);

            nlohmann::json componentData;
            if (j.contains(compName))
                componentData = j.at(compName);
            else
                componentData = j;

            if (componentData.contains("fileName")) fileName = componentData["fileName"];
            if (componentData.contains("filePath")) filePath = componentData["filePath"];
            if (componentData.contains("width")) width = componentData["width"];
            if (componentData.contains("height")) height = componentData["height"];
            if (componentData.contains("channels")) channels = componentData["channels"];
        }

        InputImageComponent(const InputImageComponent& other) : ImageComponent(other) {
            compName = "InputImage";
            setupInputSchema();
        }

        InputImageComponent& operator=(const InputImageComponent& other) {
            if (this != &other) {
                ImageComponent::operator=(other);
                compName = "InputImage";
                setupInputSchema();
            }
            return *this;
        }

    private:
        void setupInputSchema() {
            schema = {
                {"title", "Input Image"},
                {"type", "object"},
                {"properties", {
                    {"filePath", {
                        {"type", "string"},
                        {"title", "Input Image File"},
                        {"ui:widget", "file_selector"},
                        {"ui:options", {
                            {"mode", "file"},
                            {"filters", ".png,.jpg,.jpeg,.bmp,.tga"},
                            {"filterName", "Image Files"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Clear"},
                            {"browseTooltip", "Browse for image files (.png, .jpg, .jpeg, .bmp, .tga)"}
                        }}
                    }}
                }},
                {"propertyOrder", {"filePath", "fileName", "width", "height", "channels"}}
            };
        }
    };

    struct OutputImageComponent : public ImageComponent {
        std::string fileExtension = ".png";

        OutputImageComponent() {
            compName = "OutputImage";
            compCategory = "Image";
            setupOutputSchema();
        }

        virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            std::unordered_map<std::string, UISchema::PropertyVariant> properties;
            properties["fileName"] = &fileName;
            properties["filePath"] = &filePath;
            properties["fileExtension"] = &fileExtension;
            return properties;
        }

        virtual nlohmann::json Serialize() const override {
            auto j = ImageComponent::Serialize();
            j[compName]["fileExtension"] = fileExtension;
            return j;
        }

        virtual void Deserialize(const nlohmann::json& j) override {
            ImageComponent::Deserialize(j);
            nlohmann::json componentData;
            if (j.contains(compName))
                componentData = j.at(compName);
            if (componentData.contains("fileExtension"))
                fileExtension = componentData["fileExtension"];
        }

        OutputImageComponent& operator=(const OutputImageComponent& other) {
            if (this != &other) {
                ImageComponent::operator=(other);
                compName = "OutputImage";
                fileExtension = other.fileExtension;
                setupOutputSchema();
            }
            return *this;
        }

        OutputImageComponent(const OutputImageComponent& other) : ImageComponent(other) {
            compName = "OutputImage";
            fileExtension = other.fileExtension;
            setupOutputSchema();
        }

    private:
        void setupOutputSchema() {
            auto items = FileFormats::GetComboItemsJson(FileFormats::GetImageExtensions());
            schema = {
                {"title", "Output Image"},
                {"type", "object"},
                {"properties", {
                    {"filePath", {
                        {"type", "string"},
                        {"title", "Output Directory"},
                        {"ui:widget", "file_selector"},
                        {"ui:options", {
                            {"mode", "directory"},
                            {"defaultPath", "OutputFolder"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Reset"},
                            {"browseTooltip", "Browse to select output directory for saving images"}
                        }}
                    }},
                    {"fileName", {
                        {"type", "string"},
                        {"title", "File Name"},
                        {"ui:widget", "input_text"},
                        {"ui:options", {
                            {"dialogDefaultPath", "OutputFolder"},
                            {"defaultPath", "OutputFolder"},
                            {"resetButtonText", "Reset to Default"}
                        }}
                    }},
                    {"fileExtension", {
                        {"type", "string"},
                        {"title", "File Format"},
                        {"ui:widget", "combo"},
                        {"items", items},
                        {"ui:options", {
                            {"resetButtonText", "Reset to PNG"}
                        }}
                    }}
                }},
                {"propertyOrder", {"filePath", "fileName", "fileExtension"}}
            };
        }
    };

    struct MaskImageComponent : public ImageComponent {
        float value = 0.75f;
        std::string maskFilePath = "";

        MaskImageComponent() {
            compName = "MaskImageComponent";
            fileName = "";
            filePath = "";
            setupMaskSchema();
        }

        virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            std::unordered_map<std::string, UISchema::PropertyVariant> properties;
            properties["maskFilePath"] = &maskFilePath;
            properties["value"] = &value;
            return properties;
        }

        virtual nlohmann::json Serialize() const override {
            nlohmann::json j = ImageComponent::Serialize();
            j[compName]["value"] = value;
            j[compName]["maskFilePath"] = maskFilePath;
            return j;
        }

        virtual void Deserialize(const nlohmann::json& j) override {
            ImageComponent::Deserialize(j);
            nlohmann::json componentData;
            if (j.contains(compName))
                componentData = j.at(compName);
            if (componentData.contains("value"))
                value = componentData["value"];
            if (componentData.contains("maskFilePath"))
                maskFilePath = componentData["maskFilePath"];
        }

        MaskImageComponent& operator=(const MaskImageComponent& other) {
            if (this != &other) {
                ImageComponent::operator=(other);
                compName = "MaskImageComponent";
                value = other.value;
                maskFilePath = other.maskFilePath;
                setupMaskSchema();
            }
            return *this;
        }

        MaskImageComponent(const MaskImageComponent& other) : ImageComponent(other) {
            compName = "MaskImageComponent";
            value = other.value;
            maskFilePath = other.maskFilePath;
            setupMaskSchema();
        }

    private:
        void setupMaskSchema() {
            schema = {
                {"title", "Mask Image"},
                {"type", "object"},
                {"properties", {
                    {"maskFilePath", {
                        {"type", "string"},
                        {"title", "Mask Image File"},
                        {"ui:widget", "file_selector"},
                        {"ui:options", {
                            {"mode", "file"},
                            {"filters", ".png,.jpg,.jpeg,.bmp,.tga"},
                            {"filterName", "Image Files"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Clear"},
                            {"browseTooltip", "Browse for mask image files (grayscale images)"}
                        }}
                    }},
                    {"value", {
                        {"type", "number"},
                        {"title", "Mask Strength"},
                        {"description", "Strength of the mask effect (0.0 to 1.0)"},
                        {"ui:widget", "slider_float"},
                        {"minimum", 0.0},
                        {"maximum", 1.0},
                        {"ui:options", {
                            {"step", 0.01},
                            {"format", "%.2f"}
                        }}
                    }}
                }},
                {"propertyOrder", {"maskFilePath", "value"}}
            };
        }
    };
} // namespace ECS