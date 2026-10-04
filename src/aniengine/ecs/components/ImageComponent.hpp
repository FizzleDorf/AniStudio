#pragma once

#include "BaseComponent.hpp"
#include "OpenGLWrapper.hpp"
#include "FileFormats.hpp"
#include <string>
#include <stb_image.h>
#include <memory>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <algorithm>

namespace ECS {

    struct ImageComponent : public BaseComponent {
        std::string fileName = "AniStudio";
        std::string filePath = "";
        std::shared_ptr<unsigned char[]> imageDataPtr;
        unsigned char* imageData = nullptr;
        int width = 0;
        int height = 0;
        int channels = 0;
        bool hasExifData = false;
        bool hasLSBData = false;
        bool hasAniStudioMetadata = false;

        uint64_t fileSize = 0;
        std::string fileDate;
        std::string fileTime;

        ImageComponent() = default;

        const char* GetCompName() const override { return "Image"; }
        const char* GetCompCategory() const override { return "Image"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "Image"},
                {"type", "object"},
                {"properties", {
                    {"fileName", {{"type", "string"}, {"title", "File Name"}}},
                    {"filePath", {{"type", "string"}, {"title", "File Path"}}},
                    {"fileSize", {{"type", "integer"}, {"title", "File Size (bytes)"}}},
                    {"fileDate", {{"type", "string"}, {"title", "Date Modified"}}},
                    {"fileTime", {{"type", "string"}, {"title", "Time Modified"}}}
                }}
            };
            return j;
        }

        ImageComponent(const ImageComponent& other) : BaseComponent(other) {
            fileName = other.fileName;
            filePath = other.filePath;
            width = other.width;
            height = other.height;
            channels = other.channels;
            imageData = nullptr;
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

        virtual ~ImageComponent() {
            imageDataPtr.reset();
            imageData = nullptr;
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"fileName", &fileName},
                {"filePath", &filePath},
                {"fileSize", &fileSize},
                {"fileDate", &fileDate},
                {"fileTime", &fileTime}
            };
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"width", width}, {"height", height}, {"channels", channels},
                {"fileName", fileName}, {"filePath", filePath},
                {"fileSize", fileSize}, {"fileDate", fileDate}, {"fileTime", fileTime},
                {"hasExifData", hasExifData},
                {"hasLSBData", hasLSBData},
                {"hasAniStudioMetadata", hasAniStudioMetadata}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();
            nlohmann::json componentData = j.contains(key) ? j.at(key) : j;
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
                imageDataPtr = std::shared_ptr<unsigned char[]>(
                    data, [](unsigned char* ptr) { if (ptr) free(ptr); });
                imageData = imageDataPtr.get();
                width = w; height = h; channels = ch;
            }
            else {
                ClearImageData();
            }
        }

        void ClearImageData() {
            imageDataPtr.reset();
            imageData = nullptr;
            width = 0; height = 0; channels = 0;
        }
    };

    // One row of a bulk image export: "ext|enabled|writeExif|writeLsb|quality|compression"
    struct ImageExportTarget {
        std::string fileExtension = ".png";
        bool enabled = true;
        bool writeExif = true;
        bool writeLsb = false;
        int quality = 90;
        int compressionLevel = 6;

        nlohmann::json Serialize() const {
            return {
                {"fileExtension", fileExtension},
                {"enabled", enabled},
                {"writeExif", writeExif},
                {"writeLsb", writeLsb},
                {"quality", quality},
                {"compressionLevel", compressionLevel}
            };
        }

        void Deserialize(const nlohmann::json& j) {
            if (j.contains("fileExtension")) fileExtension = j["fileExtension"];
            if (j.contains("enabled")) enabled = j["enabled"];
            if (j.contains("writeExif")) writeExif = j["writeExif"];
            if (j.contains("writeLsb")) writeLsb = j["writeLsb"];
            if (j.contains("quality")) quality = j["quality"];
            if (j.contains("compressionLevel")) compressionLevel = j["compressionLevel"];
        }

        static std::string ToRow(const ImageExportTarget& t) {
            std::ostringstream ss;
            ss << t.fileExtension << "|" << (t.enabled ? 1 : 0)
                << "|" << (t.writeExif ? 1 : 0)
                << "|" << (t.writeLsb ? 1 : 0)
                << "|" << t.quality
                << "|" << t.compressionLevel;
            return ss.str();
        }

        static bool FromRow(const std::string& row, ImageExportTarget& out) {
            std::vector<std::string> parts;
            std::string cur;
            for (char c : row) {
                if (c == '|') { parts.push_back(cur); cur.clear(); }
                else cur.push_back(c);
            }
            parts.push_back(cur);
            if (parts.size() < 6) return false;
            try {
                out.fileExtension = parts[0];
                out.enabled = parts[1] == "1";
                out.writeExif = parts[2] == "1";
                out.writeLsb = parts[3] == "1";
                out.quality = std::stoi(parts[4]);
                out.compressionLevel = std::stoi(parts[5]);
            }
            catch (...) { return false; }
            return true;
        }
    };

    struct ImageExportComponent : public BaseComponent {
        std::string fileName = "AniStudio";
        std::string filePath = "OutputFolder";
        bool uniquePerTarget = true;

        // UI-facing rows: "ext|enabled|writeExif|writeLsb|quality|compression"
        std::vector<std::string> targetRows;

        ImageExportComponent() {
            ImageExportTarget t;
            t.fileExtension = ".png";
            targetRows.push_back(ImageExportTarget::ToRow(t));
        }

        const char* GetCompName() const override { return "ImageExport"; }
        const char* GetCompCategory() const override { return "Image"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json items =
                FileFormats::GetComboItemsJson(FileFormats::GetImageExtensions());
            static const nlohmann::json j = {
                {"title", "Image Export"},
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
                            {"browseTooltip", "Directory where every format is saved"}
                        }}
                    }},
                    {"fileName", {
                        {"type", "string"},
                        {"title", "Base Name"},
                        {"ui:widget", "input_text"},
                        {"ui:options", {
                            {"dialogDefaultPath", "OutputFolder"},
                            {"defaultPath", "OutputFolder"},
                            {"resetButtonText", "Reset to Default"}
                        }}
                    }},
                    {"uniquePerTarget", {
                        {"type", "boolean"},
                        {"title", "Unique Filename Per Format"}
                    }},
                    {"targetRows", {
                        {"type", "array"},
                        {"title", "Formats"},
                        {"description", "One row per format: ext|enabled|writeExif|writeLsb|quality|compression"},
                        {"ui:widget", "list"},
                        {"ui:options", {
                            {"formatHint", "ext|on|exif|lsb|quality|compression"},
                            {"availableFormats", items}
                        }}
                    }}
                }},
                {"propertyOrder", {"filePath", "fileName", "uniquePerTarget", "targetRows"}}
            };
            return j;
        }

        ImageExportComponent(const ImageExportComponent& other)
            : BaseComponent(other)
            , fileName(other.fileName)
            , filePath(other.filePath)
            , uniquePerTarget(other.uniquePerTarget)
            , targetRows(other.targetRows) {
        }

        ImageExportComponent& operator=(const ImageExportComponent& other) {
            if (this != &other) {
                fileName = other.fileName;
                filePath = other.filePath;
                uniquePerTarget = other.uniquePerTarget;
                targetRows = other.targetRows;
            }
            return *this;
        }

        virtual ~ImageExportComponent() {}

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"fileName", &fileName},
                {"filePath", &filePath},
                {"uniquePerTarget", &uniquePerTarget},
                {"targetRows", &targetRows}
            };
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"fileName", fileName},
                {"filePath", filePath},
                {"uniquePerTarget", uniquePerTarget},
                {"targets", nlohmann::json::array()}
            };
            for (const auto& row : targetRows) {
                ImageExportTarget t;
                if (ImageExportTarget::FromRow(row, t))
                    j[GetCompName()]["targets"].push_back(t.Serialize());
            }
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();
            nlohmann::json d = j.contains(key) ? j.at(key) : j;
            if (d.contains("fileName")) fileName = d["fileName"];
            if (d.contains("filePath")) filePath = d["filePath"];
            if (d.contains("uniquePerTarget")) uniquePerTarget = d["uniquePerTarget"];
            targetRows.clear();
            if (d.contains("targets") && d["targets"].is_array()) {
                for (const auto& t : d["targets"]) {
                    ImageExportTarget target;
                    target.Deserialize(t);
                    targetRows.push_back(ImageExportTarget::ToRow(target));
                }
            }
            if (targetRows.empty()) {
                ImageExportTarget def;
                targetRows.push_back(ImageExportTarget::ToRow(def));
            }
        }

        std::vector<ImageExportTarget> GetEnabledTargets() const {
            std::vector<ImageExportTarget> out;
            for (const auto& row : targetRows) {
                ImageExportTarget t;
                if (ImageExportTarget::FromRow(row, t) && t.enabled)
                    out.push_back(t);
            }
            return out;
        }
    };

    struct PreviewImageComponent : public ImageComponent {
        std::string sourceName = "preview";

        PreviewImageComponent() { fileName = ""; filePath = ""; }

        const char* GetCompName() const override { return "PreviewImage"; }
        const char* GetCompCategory() const override { return "Image"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = nlohmann::json::object();
            return j;
        }

        PreviewImageComponent(const PreviewImageComponent& other)
            : ImageComponent(other), sourceName(other.sourceName) {
        }

        PreviewImageComponent& operator=(const PreviewImageComponent& other) {
            if (this != &other) {
                ImageComponent::operator=(other);
                sourceName = other.sourceName;
            }
            return *this;
        }

        virtual ~PreviewImageComponent() {}

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override { return {}; }
        nlohmann::json Serialize() const override { return nlohmann::json::object(); }
        void Deserialize(const nlohmann::json&) override {}
    };

    struct InputImageComponent : public ImageComponent {
        InputImageComponent() {
            fileName = ""; filePath = "";
            width = 0; height = 0; channels = 0;
        }

        const char* GetCompName() const override { return "InputImage"; }
        const char* GetCompCategory() const override { return "Image"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "Input Image"},
                {"type", "object"},
                {"properties", {
                    {"filePath", {
                        {"type", "string"},
                        {"title", "Input Image File"},
                        {"ui:widget", "file_selector"},
                        {"ui:options", {
                            {"mode", "file"},
                            {"filters", ".png,.jpg,.jpeg,.bmp,.tga,.webp,.tiff"},
                            {"filterName", "Image Files"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Clear"},
                            {"browseTooltip", "Browse for image files"}
                        }}
                    }}
                }},
                {"propertyOrder", {"filePath", "fileName", "width", "height", "channels"}}
            };
            return j;
        }

        InputImageComponent(const InputImageComponent& other) : ImageComponent(other) {}

        InputImageComponent& operator=(const InputImageComponent& other) {
            if (this != &other) ImageComponent::operator=(other);
            return *this;
        }

        virtual ~InputImageComponent() {}

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"fileName", &fileName}, {"filePath", &filePath},
                {"width", &width}, {"height", &height}, {"channels", &channels}
            };
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"fileName", fileName}, {"filePath", filePath},
                {"width", width}, {"height", height}, {"channels", channels}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();
            nlohmann::json d = j.contains(key) ? j.at(key) : j;
            if (d.contains("fileName")) fileName = d["fileName"];
            if (d.contains("filePath")) filePath = d["filePath"];
            if (d.contains("width")) width = d["width"];
            if (d.contains("height")) height = d["height"];
            if (d.contains("channels")) channels = d["channels"];
        }
    };

    struct OutputImageComponent : public ImageComponent {
        std::string fileExtension = ".png";
        bool writeExif = true;
        bool writeLsb = false;
        int quality = 90;
        int compressionLevel = 6;

        OutputImageComponent() = default;

        const char* GetCompName() const override { return "OutputImage"; }
        const char* GetCompCategory() const override { return "Image"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json items =
                FileFormats::GetComboItemsJson(FileFormats::GetImageExtensions());
            static const nlohmann::json j = {
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
                            {"browseTooltip", "Directory for saved images"}
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
                        {"ui:options", {{"resetButtonText", "Reset to PNG"}}}
                    }},
                    {"writeExif", {
                        {"type", "boolean"},
                        {"title", "Write EXIF"}
                    }},
                    {"writeLsb", {
                        {"type", "boolean"},
                        {"title", "Write LSB"}
                    }},
                    {"quality", {
                        {"type", "integer"},
                        {"title", "Quality"},
                        {"description", "JPEG/WebP quality (1-100)"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", 1}, {"max", 100}, {"step", 1}}}
                    }},
                    {"compressionLevel", {
                        {"type", "integer"},
                        {"title", "PNG Compression"},
                        {"description", "PNG zlib level (0-9)"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", 0}, {"max", 9}, {"step", 1}}}
                    }}
                }},
                {"propertyOrder", {
                    "filePath", "fileName", "fileExtension",
                    "writeExif", "writeLsb", "quality", "compressionLevel"
                }}
            };
            return j;
        }

        OutputImageComponent(const OutputImageComponent& other)
            : ImageComponent(other)
            , fileExtension(other.fileExtension)
            , writeExif(other.writeExif)
            , writeLsb(other.writeLsb)
            , quality(other.quality)
            , compressionLevel(other.compressionLevel) {
        }

        OutputImageComponent& operator=(const OutputImageComponent& other) {
            if (this != &other) {
                ImageComponent::operator=(other);
                fileExtension = other.fileExtension;
                writeExif = other.writeExif;
                writeLsb = other.writeLsb;
                quality = other.quality;
                compressionLevel = other.compressionLevel;
            }
            return *this;
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"fileName", &fileName},
                {"filePath", &filePath},
                {"fileExtension", &fileExtension},
                {"writeExif", &writeExif},
                {"writeLsb", &writeLsb},
                {"quality", &quality},
                {"compressionLevel", &compressionLevel}
            };
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"width", width}, {"height", height}, {"channels", channels},
                {"fileName", fileName}, {"filePath", filePath},
                {"fileSize", fileSize}, {"fileDate", fileDate}, {"fileTime", fileTime},
                {"hasExifData", hasExifData},
                {"hasLSBData", hasLSBData},
                {"hasAniStudioMetadata", hasAniStudioMetadata},
                {"fileExtension", fileExtension},
                {"writeExif", writeExif},
                {"writeLsb", writeLsb},
                {"quality", quality},
                {"compressionLevel", compressionLevel}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();
            nlohmann::json d = j.contains(key) ? j.at(key) : j;
            if (d.contains("width")) width = d["width"];
            if (d.contains("height")) height = d["height"];
            if (d.contains("channels")) channels = d["channels"];
            if (d.contains("fileName")) fileName = d["fileName"];
            if (d.contains("filePath")) filePath = d["filePath"];
            if (d.contains("fileSize")) fileSize = d["fileSize"];
            if (d.contains("fileDate")) fileDate = d["fileDate"];
            if (d.contains("fileTime")) fileTime = d["fileTime"];
            if (d.contains("hasExifData")) hasExifData = d["hasExifData"];
            if (d.contains("hasLSBData")) hasLSBData = d["hasLSBData"];
            if (d.contains("hasAniStudioMetadata")) hasAniStudioMetadata = d["hasAniStudioMetadata"];
            if (d.contains("fileExtension")) fileExtension = d["fileExtension"];
            if (d.contains("writeExif")) writeExif = d["writeExif"];
            if (d.contains("writeLsb")) writeLsb = d["writeLsb"];
            if (d.contains("quality")) quality = d["quality"];
            if (d.contains("compressionLevel")) compressionLevel = d["compressionLevel"];
        }
    };

    struct MaskImageComponent : public ImageComponent {
        float value = 0.75f;
        std::string maskFilePath = "";

        MaskImageComponent() { fileName = ""; filePath = ""; }

        const char* GetCompName() const override { return "MaskImageComponent"; }
        const char* GetCompCategory() const override { return "Image"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
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
                            {"browseTooltip", "Browse for grayscale mask images"}
                        }}
                    }},
                    {"value", {
                        {"type", "number"},
                        {"title", "Mask Strength"},
                        {"description", "Strength of the mask effect (0.0 to 1.0)"},
                        {"ui:widget", "slider_float"},
                        {"minimum", 0.0},
                        {"maximum", 1.0},
                        {"ui:options", {{"step", 0.01}, {"format", "%.2f"}}}
                    }}
                }},
                {"propertyOrder", {"maskFilePath", "value"}}
            };
            return j;
        }

        MaskImageComponent(const MaskImageComponent& other)
            : ImageComponent(other)
            , value(other.value)
            , maskFilePath(other.maskFilePath) {
        }

        MaskImageComponent& operator=(const MaskImageComponent& other) {
            if (this != &other) {
                ImageComponent::operator=(other);
                value = other.value;
                maskFilePath = other.maskFilePath;
            }
            return *this;
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return { {"maskFilePath", &maskFilePath}, {"value", &value} };
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"width", width}, {"height", height}, {"channels", channels},
                {"fileName", fileName}, {"filePath", filePath},
                {"fileSize", fileSize}, {"fileDate", fileDate}, {"fileTime", fileTime},
                {"hasExifData", hasExifData},
                {"hasLSBData", hasLSBData},
                {"hasAniStudioMetadata", hasAniStudioMetadata},
                {"value", value},
                {"maskFilePath", maskFilePath}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();
            nlohmann::json d = j.contains(key) ? j.at(key) : j;
            if (d.contains("width")) width = d["width"];
            if (d.contains("height")) height = d["height"];
            if (d.contains("channels")) channels = d["channels"];
            if (d.contains("fileName")) fileName = d["fileName"];
            if (d.contains("filePath")) filePath = d["filePath"];
            if (d.contains("fileSize")) fileSize = d["fileSize"];
            if (d.contains("fileDate")) fileDate = d["fileDate"];
            if (d.contains("fileTime")) fileTime = d["fileTime"];
            if (d.contains("hasExifData")) hasExifData = d["hasExifData"];
            if (d.contains("hasLSBData")) hasLSBData = d["hasLSBData"];
            if (d.contains("hasAniStudioMetadata")) hasAniStudioMetadata = d["hasAniStudioMetadata"];
            if (d.contains("value")) value = d["value"];
            if (d.contains("maskFilePath")) maskFilePath = d["maskFilePath"];
        }
    };

    struct EndImageComponent : public ImageComponent {
        EndImageComponent() {
            fileName = ""; filePath = "";
            width = 0; height = 0; channels = 0;
        }

        const char* GetCompName() const override { return "EndImage"; }
        const char* GetCompCategory() const override { return "Image"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "End Image"},
                {"type", "object"},
                {"properties", {
                    {"filePath", {
                        {"type", "string"},
                        {"title", "End Image File"},
                        {"ui:widget", "file_selector"},
                        {"ui:options", {
                            {"mode", "file"},
                            {"filters", ".png,.jpg,.jpeg,.bmp,.tga"},
                            {"filterName", "Image Files"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Clear"},
                            {"browseTooltip", "Browse for the end-frame image"}
                        }}
                    }}
                }},
                {"propertyOrder", {"filePath", "fileName", "width", "height", "channels"}}
            };
            return j;
        }

        EndImageComponent(const EndImageComponent& other) : ImageComponent(other) {}

        EndImageComponent& operator=(const EndImageComponent& other) {
            if (this != &other) ImageComponent::operator=(other);
            return *this;
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"fileName", &fileName}, {"filePath", &filePath},
                {"width", &width}, {"height", &height}, {"channels", &channels}
            };
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"fileName", fileName}, {"filePath", filePath},
                {"width", width}, {"height", height}, {"channels", channels}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();
            nlohmann::json d = j.contains(key) ? j.at(key) : j;
            if (d.contains("fileName")) fileName = d["fileName"];
            if (d.contains("filePath")) filePath = d["filePath"];
            if (d.contains("width")) width = d["width"];
            if (d.contains("height")) height = d["height"];
            if (d.contains("channels")) channels = d["channels"];
        }
    };

} // namespace ECS