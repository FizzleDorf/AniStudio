#pragma once

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>
#include <functional>
#include <filesystem>
#include <GL/glew.h>
#include "FileDialogUtil.hpp"
#include "FileDialogFilters.hpp"
#include "ui_schema/UISchemaUtils.hpp"
#include "ui_schema/UISchemaContext.hpp"
#include <stb_image.h>
#include "FilePathSystem.hpp"
#include <iostream>

namespace ECS { class FilePathSystem; }
namespace Utils { extern ECS::FilePathSystem* g_FilePathSystem; }

namespace UISchema {

    struct FileDialogResult {
        bool wasOkPressed = false;
        std::string selectedPath = "";
        std::string selectedFileName = "";
        std::string fullPath = "";
    };

    using FileDialogCallback = std::function<void(const FileDialogResult&)>;

    class FileDialogWidgets {
    private:
        static inline std::unordered_map<std::string, GLuint> imagePreviewCache;
        static inline std::unordered_map<std::string, std::pair<int, int>> imageDimensionsCache;
        static inline bool pendingModification = false;

        static std::string ResolveDialogPath(const std::string& pathOrKey, const std::string& fallbackPath = "") {
            if (Utils::g_FilePathSystem) {
                if (!pathOrKey.empty() && Utils::g_FilePathSystem->HasPath(pathOrKey)) {
                    std::string resolvedPath = Utils::g_FilePathSystem->GetPath(pathOrKey);
                    if (!resolvedPath.empty() && std::filesystem::exists(resolvedPath)) {
                        return resolvedPath;
                    }
                }
            }

            if (!pathOrKey.empty() && std::filesystem::exists(pathOrKey)) {
                return pathOrKey;
            }

            std::string result = fallbackPath.empty() ? "." : fallbackPath;
            return result;
        }

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

        static GLuint CreateImagePreview(const std::string& imagePath) {
            if (imagePreviewCache.count(imagePath)) {
                return imagePreviewCache[imagePath];
            }

            if (!std::filesystem::exists(imagePath)) {
                return 0;
            }

            int width, height, channels;
            unsigned char* data = stbi_load(imagePath.c_str(), &width, &height, &channels, 0);
            if (!data) {
                return 0;
            }

            GLuint textureID;
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            GLenum format = (channels == 4) ? GL_RGBA : (channels == 3) ? GL_RGB : GL_RED;
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

            imagePreviewCache[imagePath] = textureID;
            imageDimensionsCache[imagePath] = { width, height };

            stbi_image_free(data);

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

        static void OpenFileDialog(
            const std::string& dialogKey,
            const std::string& title,
            const std::string& filters,
            const std::string& pathOrKey,
            bool isDirectoryMode = false,
            FileDialogCallback callback = nullptr
        ) {
            std::string actualPath = ResolveDialogPath(pathOrKey);

            FileDialog::FilterType filterType = FileDialog::FilterType::ALL_FILES;
            if (!filters.empty()) {
                if (filters.find("png") != std::string::npos || filters.find("jpg") != std::string::npos) {
                    filterType = FileDialog::FilterType::IMAGE_FILE;
                }
                else if (filters.find("mp4") != std::string::npos || filters.find("webm") != std::string::npos) {
                    filterType = FileDialog::FilterType::VIDEO_FILE;
                }
                else if (filters.find("safetensors") != std::string::npos || filters.find("ckpt") != std::string::npos) {
                    filterType = FileDialog::FilterType::DIFFUSION_MODEL;
                }
                else if (filters.find("json") != std::string::npos) {
                    filterType = FileDialog::FilterType::METADATA_FILE;
                }
            }

            FileDialogResult result;
            result.wasOkPressed = false;

            if (isDirectoryMode) {
                std::string selectedFolder;
                if (FileDialog::SelectFolder(title, selectedFolder, actualPath)) {
                    result.wasOkPressed = true;
                    result.selectedPath = selectedFolder;
                    result.fullPath = selectedFolder;
                }
            }
            else {
                std::string selectedFile;
                if (FileDialog::OpenFile(title, filterType, selectedFile, actualPath)) {
                    result.wasOkPressed = true;
                    result.fullPath = selectedFile;
                    std::filesystem::path filePath(selectedFile);
                    result.selectedPath = filePath.parent_path().string();
                    result.selectedFileName = filePath.filename().string();
                }
            }

            if (callback && result.wasOkPressed) {
                callback(result);
                pendingModification = true;
            }
        }

        static bool RenderFileSelector(
            const std::string& label,
            std::string* value,
            const nlohmann::json& options,
            const UIRenderContext& context
        ) {
            bool modified = false;

            std::string mode = GetSchemaValue<std::string>(options, "mode", "file");
            std::string filters = GetSchemaValue<std::string>(options, "filters", "");
            std::string filterName = GetSchemaValue<std::string>(options, "filterName", "Files");
            std::string defaultPath = GetSchemaValue<std::string>(options, "defaultPath", "");
            std::string dialogDefaultPath = GetSchemaValue<std::string>(options, "dialogDefaultPath", defaultPath);
            std::string buttonText = GetSchemaValue<std::string>(options, "buttonText", "Browse...");
            std::string resetButtonText = GetSchemaValue<std::string>(options, "resetButtonText", "Clear");
            std::string browseTooltip = GetSchemaValue<std::string>(options, "browseTooltip", "");

            std::string propertyName = label;
            size_t hashPos = label.find("##");
            if (hashPos != std::string::npos) {
                propertyName = label.substr(0, hashPos);
                if (hashPos == 0) {
                    std::string uniquePart = label.substr(2);
                    size_t underscorePos = uniquePart.find('_');
                    if (underscorePos != std::string::npos) {
                        size_t secondUnderscorePos = uniquePart.find('_', underscorePos + 1);
                        if (secondUnderscorePos != std::string::npos) {
                            propertyName = uniquePart.substr(underscorePos + 1, secondUnderscorePos - underscorePos - 1);
                        }
                    }
                }
            }

            if (!value->empty() && std::filesystem::exists(*value)) {
                std::filesystem::path filePath(*value);
                std::string extension = filePath.extension().string();
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

                if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
                    extension == ".bmp" || extension == ".tga") {
                    RenderImagePreview(*value, 200.0f);

                    std::string clearImageButtonId = context.GenerateWidgetId(propertyName, "clear_image");
                    std::string clearImageButtonLabel = "Clear Image##" + clearImageButtonId;
                    if (ImGui::Button(clearImageButtonLabel.c_str())) {
                        *value = "";
                        modified = true;
                    }
                    ImGui::Spacing();
                }
            }

            if (!value->empty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));

                std::filesystem::path filePath(*value);
                std::string displayName = filePath.filename().string();
                if (displayName.empty()) {
                    displayName = *value;
                }
                ImGui::TextWrapped("%s", displayName.c_str());

                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 50.0f);
                    ImGui::Text("Full path: %s", value->c_str());
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }

                ImGui::PopStyleColor();
            }
            else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                ImGui::TextWrapped("No %s selected", mode.c_str());
                ImGui::PopStyleColor();
            }
            ImGui::Spacing();

            std::string actualDialogPath;

            if (!value->empty()) {
                std::filesystem::path currentPath(*value);
                if (std::filesystem::exists(currentPath.parent_path())) {
                    actualDialogPath = currentPath.parent_path().string();
                }
            }

            if (actualDialogPath.empty() && !dialogDefaultPath.empty()) {
                actualDialogPath = ResolveDialogPath(dialogDefaultPath);
            }

            if (actualDialogPath.empty() && mode == "directory") {
                if (propertyName == "modelPath") {
                    if (label.find("LoRA") != std::string::npos) {
                        if (Utils::g_FilePathSystem)
                            actualDialogPath = Utils::g_FilePathSystem->GetPath("Lora");
                    }
                    else if (label.find("Embedding") != std::string::npos) {
                        if (Utils::g_FilePathSystem)
                            actualDialogPath = Utils::g_FilePathSystem->GetPath("Embed");
                    }
                }
            }

            if (actualDialogPath.empty() || !std::filesystem::exists(actualDialogPath)) {
                actualDialogPath = ".";
            }

            std::string browseButtonId = context.GenerateWidgetId(propertyName, "browse");
            std::string browseButtonLabel = buttonText + "##" + browseButtonId;

            if (ImGui::Button(browseButtonLabel.c_str())) {
                std::string dialogKey = context.GenerateWidgetId(propertyName, "file_dialog");
                std::string dialogTitle = mode == "directory" ? "Select Directory" : "Select File";
                bool isDirectoryMode = (mode == "directory");

                std::string formattedFilters = filters;

                OpenFileDialog(
                    dialogKey,
                    dialogTitle,
                    formattedFilters,
                    actualDialogPath,
                    isDirectoryMode,
                    [value, isDirectoryMode](const FileDialogResult& result) {
                        if (result.wasOkPressed) {
                            if (isDirectoryMode) {
                                *value = result.selectedPath;
                            }
                            else {
                                *value = result.fullPath;
                            }
                        }
                    }
                );
            }

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                if (!browseTooltip.empty()) {
                    ImGui::Text("%s", browseTooltip.c_str());
                }
                else if (mode == "directory") {
                    ImGui::Text("Browse to select a directory path");
                }
                else {
                    ImGui::Text("Browse to select a file");
                }
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }

            ImGui::SameLine();
            std::string clearButtonId = context.GenerateWidgetId(propertyName, "clear");
            std::string clearButtonLabel = "Clear##" + clearButtonId;

            if (ImGui::Button(clearButtonLabel.c_str())) {
                if (!defaultPath.empty()) {
                    std::string resolvedDefault = ResolveDialogPath(defaultPath);
                    if (!resolvedDefault.empty() && std::filesystem::exists(resolvedDefault)) {
                        *value = resolvedDefault;
                    }
                    else {
                        *value = "";
                    }
                }
                else {
                    *value = "";
                }
                modified = true;
            }

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                if (!defaultPath.empty()) {
                    ImGui::Text("Reset to default path: %s", defaultPath.c_str());
                }
                else {
                    ImGui::Text("Clear the selected file/directory");
                }
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }

            return modified;
        }

        static bool Render(const std::string& label, std::string* value, const std::string& widgetType, const nlohmann::json& schema, const UIRenderContext& context) {
            nlohmann::json options = {};
            if (schema.contains("ui:options") && schema["ui:options"].is_object()) {
                options = schema["ui:options"];
            }

            if (widgetType == "file_selector") {
                return RenderFileSelector(label, value, options, context);
            }
            else {
                std::cerr << "Unknown file dialog widget type '" << widgetType << "'" << std::endl;
                return false;
            }
        }
    };

}