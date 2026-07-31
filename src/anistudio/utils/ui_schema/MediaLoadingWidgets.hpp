#pragma once

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>
#include <functional>
#include <filesystem>
#include <GL/glew.h>
#include "FileDialogUtil.hpp"
#include "FileDialogFilters.hpp"
#include "UISchemaUtils.hpp"
#include "ImageUtils.hpp"
#include "PropertyTypes.hpp"
#include "UISchemaContext.hpp"
#include <stb_image.h>
#include <iostream>

namespace UISchema {

    class MediaLoadingWidgets {
    private:
        static inline bool pendingModification = false;

        static inline std::unordered_map<std::string, GLuint> imagePreviewCache;
        static inline std::unordered_map<std::string, std::pair<int, int>> imageDimensionsCache;

    public:
        static void CleanupPreviews() {
            for (auto& [path, textureID] : imagePreviewCache) {
                if (textureID != 0) {
                    glDeleteTextures(1, &textureID);
                }
            }
            imagePreviewCache.clear();
            imageDimensionsCache.clear();
        }

        static bool WasModified() {
            bool result = pendingModification;
            pendingModification = false;
            return result;
        }

        static bool ProcessDialog() {
            return false;
        }

        static GLuint CreateTextureFromImageData(int width, int height, int channels, unsigned char* data) {
            if (!data || width <= 0 || height <= 0) {
                return 0;
            }

            GLuint textureID;
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            GLenum format = GL_RGB;
            if (channels == 4) format = GL_RGBA;
            else if (channels == 1) format = GL_RED;

            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

            glBindTexture(GL_TEXTURE_2D, 0);
            return textureID;
        }

        static GLuint CreateImagePreview(const std::string& imagePath) {
            if (imagePreviewCache.count(imagePath)) {
                return imagePreviewCache[imagePath];
            }

            if (!std::filesystem::exists(imagePath)) {
                return 0;
            }

            int width, height, channels;
            unsigned char* data = Utils::ImageUtils::LoadImageData(imagePath, width, height, channels);
            if (!data) {
                std::cerr << "Failed to load image for preview: " << imagePath << std::endl;
                return 0;
            }

            GLuint textureID = CreateTextureFromImageData(width, height, channels, data);

            imagePreviewCache[imagePath] = textureID;
            imageDimensionsCache[imagePath] = { width, height };

            Utils::ImageUtils::FreeImageData(data);

            std::cout << "Created preview texture for: " << imagePath << " (" << width << "x" << height << ")" << std::endl;
            return textureID;
        }

        static void RenderImagePreview(const std::string& imagePath, float maxSize = 256.0f) {
            if (imagePath.empty() || !std::filesystem::exists(imagePath)) {
                return;
            }

            GLuint textureID = CreateImagePreview(imagePath);
            if (textureID == 0) {
                return;
            }

            auto dimIt = imageDimensionsCache.find(imagePath);
            if (dimIt == imageDimensionsCache.end()) {
                return;
            }

            int width = dimIt->second.first;
            int height = dimIt->second.second;

            float aspectRatio = static_cast<float>(width) / height;
            ImVec2 previewSize;
            if (aspectRatio > 1.0f) {
                previewSize = ImVec2(maxSize, maxSize / aspectRatio);
            }
            else {
                previewSize = ImVec2(maxSize * aspectRatio, maxSize);
            }

            ImGui::Text("Preview (%dx%d):", width, height);
            ImGui::Image((ImTextureID)(intptr_t)textureID, previewSize);
        }

        static bool LoadImageIntoProperties(
            const std::string& filePath,
            const PropertyMap& properties
        ) {
            if (filePath.empty() || !std::filesystem::exists(filePath)) {
                return false;
            }

            int width, height, channels;
            unsigned char* imageData = Utils::ImageUtils::LoadImageData(filePath, width, height, channels);

            if (!imageData) {
                std::cerr << "ERROR: Failed to load image data from: " << filePath << std::endl;
                return false;
            }

            std::cout << "=========================================" << std::endl;
            std::cout << "MediaLoadingWidgets::LoadImageIntoProperties()" << std::endl;
            std::cout << "File selected: " << filePath << std::endl;
            std::cout << "Image loaded: " << width << "x" << height << "x" << channels << std::endl;

            std::filesystem::path fsPath(filePath);

            if (properties.find("fileName") != properties.end()) {
                if (auto fileNamePtr = std::get_if<std::string*>(&properties.at("fileName"))) {
                    std::cout << "Setting fileName to: " << fsPath.filename().string() << std::endl;
                    **fileNamePtr = fsPath.filename().string();
                }
            }

            if (properties.find("filePath") != properties.end()) {
                if (auto filePathPtr = std::get_if<std::string*>(&properties.at("filePath"))) {
                    std::cout << "Setting filePath to: " << filePath << std::endl;
                    **filePathPtr = filePath;
                }
            }

            if (properties.find("width") != properties.end()) {
                if (auto widthPtr = std::get_if<int*>(&properties.at("width"))) {
                    std::cout << "Setting width to: " << width << std::endl;
                    **widthPtr = width;
                }
            }

            if (properties.find("height") != properties.end()) {
                if (auto heightPtr = std::get_if<int*>(&properties.at("height"))) {
                    std::cout << "Setting height to: " << height << std::endl;
                    **heightPtr = height;
                }
            }

            if (properties.find("channels") != properties.end()) {
                if (auto channelsPtr = std::get_if<int*>(&properties.at("channels"))) {
                    std::cout << "Setting channels to: " << channels << std::endl;
                    **channelsPtr = channels;
                }
            }

            std::cout << "=========================================" << std::endl;

            Utils::ImageUtils::FreeImageData(imageData);
            return true;
        }

        static bool RenderMediaLoader(
            const std::string& label,
            const nlohmann::json& options,
            const PropertyMap& properties,
            const UIRenderContext& context
        ) {
            bool modified = false;

            std::string filters = GetSchemaValue<std::string>(options, "filters", ".png,.jpg,.jpeg,.bmp,.tga");
            std::string filterName = GetSchemaValue<std::string>(options, "filterName", "Image Files");
            std::string defaultPath = GetSchemaValue<std::string>(options, "dialogDefaultPath", "");
            std::string buttonText = GetSchemaValue<std::string>(options, "buttonText", "Load Image...");
            std::string browseTooltip = GetSchemaValue<std::string>(options, "browseTooltip", "");

            std::string uniqueId = label + "##MediaLoader";

            ImGui::Text("Image Information:");
            ImGui::Separator();

            std::string currentFileName = "";
            std::string currentFilePath = "";
            int currentWidth = 0, currentHeight = 0, currentChannels = 0;

            if (properties.find("fileName") != properties.end()) {
                if (auto fileNamePtr = std::get_if<std::string*>(&properties.at("fileName"))) {
                    currentFileName = **fileNamePtr;
                    ImGui::Text("File Name: %s", currentFileName.empty() ? "(none)" : currentFileName.c_str());
                }
            }

            if (properties.find("filePath") != properties.end()) {
                if (auto filePathPtr = std::get_if<std::string*>(&properties.at("filePath"))) {
                    currentFilePath = **filePathPtr;
                    ImGui::Text("File Path: %s", currentFilePath.empty() ? "(none)" : currentFilePath.c_str());
                }
            }

            bool hasValidDimensions = false;
            if (properties.find("width") != properties.end()) {
                if (auto widthPtr = std::get_if<int*>(&properties.at("width"))) {
                    currentWidth = **widthPtr;
                    hasValidDimensions = currentWidth > 0;
                }
            }
            if (properties.find("height") != properties.end()) {
                if (auto heightPtr = std::get_if<int*>(&properties.at("height"))) {
                    currentHeight = **heightPtr;
                }
            }
            if (properties.find("channels") != properties.end()) {
                if (auto channelsPtr = std::get_if<int*>(&properties.at("channels"))) {
                    currentChannels = **channelsPtr;
                }
            }

            if (hasValidDimensions) {
                ImGui::Text("Dimensions: %dx%dx%d", currentWidth, currentHeight, currentChannels);

                std::string previewPath = currentFilePath;
                if (!previewPath.empty() && std::filesystem::exists(previewPath)) {
                    RenderImagePreview(previewPath, 128.0f);
                }
            }
            else {
                ImGui::Text("Dimensions: (no image loaded)");
            }

            ImGui::Separator();

            std::string actualDialogPath = defaultPath;
            if (!currentFilePath.empty()) {
                actualDialogPath = std::filesystem::path(currentFilePath).parent_path().string();
            }
            if (actualDialogPath.empty() && context.pathMap) {
                auto it = context.pathMap->find("DefaultProject");
                if (it != context.pathMap->end() && !it->second.empty()) {
                    actualDialogPath = it->second;
                }
            }
            if (actualDialogPath.empty()) {
                actualDialogPath = ".";
            }

            if (ImGui::Button((buttonText + "##" + uniqueId).c_str())) {
                std::string selectedFile;
                FileDialog::FilterType filterType = FileDialog::FilterType::IMAGE_FILE;

                if (FileDialog::OpenFile("Load Image", filterType, selectedFile, actualDialogPath)) {
                    if (LoadImageIntoProperties(selectedFile, properties)) {
                        pendingModification = true;
                        modified = true;
                        std::cout << "Media loader: Properties were modified via dialog" << std::endl;
                    }
                }
            }

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                if (!browseTooltip.empty()) {
                    ImGui::Text("%s", browseTooltip.c_str());
                }
                else {
                    ImGui::Text("Browse to load an image file");
                }
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }

            if (WasModified()) {
                modified = true;
            }

            return modified;
        }

        static bool Render(const std::string& label, const std::string& widgetType, const nlohmann::json& schema, const PropertyMap& properties, const UIRenderContext& context) {
            nlohmann::json options = {};
            if (schema.contains("ui:options") && schema["ui:options"].is_object()) {
                options = schema["ui:options"];
            }

            if (widgetType == "media_loader") {
                return RenderMediaLoader(label, options, properties, context);
            }
            else {
                std::cerr << "Unknown media loading widget type '" << widgetType << "'" << std::endl;
                return false;
            }
        }
    };

}