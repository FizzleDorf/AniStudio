#include "MetadataView.hpp"
#include "ImageUtils.hpp"
#include "VideoUtils.hpp"
#include "FileFormats.hpp"
#include "FileDialogUtil.hpp"
#include "FileDialogFilters.hpp"
#include "Events.hpp"
#include "PngMetadataUtils.hpp"
#include "JpegMetadataUtils.hpp"
#include "WebPMetadataUtils.hpp"
#include "VideoMetadataUtils.hpp"
#include "MetadataUtils.hpp"
#include <imgui.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace GUI {

    MetadataView::MetadataView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseView(mgr, vm) {
        viewName = "MetadataView";
        metadata = nlohmann::json::object();
        currentFile = "";
        filterText = "";
    }

    void MetadataView::Init() {
    }

    void MetadataView::Update(float deltaT) {
    }

    void MetadataView::Render() {
        ImGui::Begin("Metadata Viewer", &windowOpen, ImGuiWindowFlags_MenuBar);

        RenderMenuBar();

        if (currentFile.empty()) {
            ImGui::Text("No file loaded. Use File > Open Metadata to load a file.");
            ImGui::Text("Or use 'Send to Metadata Viewer' from a media view.");
        }
        else {
            RenderMetadataDisplay();
        }

        RenderContextMenu();

        ImGui::End();

        if (!windowOpen) {
            std::unordered_map<std::string, std::any> eventData;
            eventData["workspaceID"] = GetID();
            eventData["viewTypeName"] = viewName;
            ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
        }
    }

    void MetadataView::RenderMenuBar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open Metadata...")) {
                    std::string filePath;
                    if (FileDialog::OpenFile("Open Metadata File", FileDialog::FilterType::METADATA_FILE, filePath)) {
                        LoadFromFile(filePath);
                    }
                }
                if (ImGui::MenuItem("Load from Clipboard")) {
                    LoadFromClipboard();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Refresh", nullptr, false, !currentFile.empty())) {
                    if (!currentFile.empty()) {
                        LoadFromFile(currentFile);
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Copy Entire Entity to Clipboard", nullptr, false, !metadata.empty())) {
                    CopyEntireEntityToClipboard();
                }
                if (ImGui::MenuItem("Paste from Clipboard", nullptr, false)) {
                    PasteFromClipboard();
                }
                if (ImGui::MenuItem("Clear Metadata", nullptr, false, !metadata.empty())) {
                    ClearMetadata();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Save As JSON...", nullptr, false, !metadata.empty())) {
                    SaveMetadataToFile();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Tree View", nullptr, displayMode == DisplayMode::Tree)) {
                    displayMode = DisplayMode::Tree;
                }
                if (ImGui::MenuItem("Text View", nullptr, displayMode == DisplayMode::Text)) {
                    displayMode = DisplayMode::Text;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
    }

    void MetadataView::RenderContextMenu() {
        if (ImGui::BeginPopupContextWindow("MetadataContextMenu")) {
            if (ImGui::MenuItem("Copy Entire Entity", nullptr, false, !metadata.empty())) {
                CopyEntireEntityToClipboard();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Paste from Clipboard", nullptr, false)) {
                PasteFromClipboard();
            }
            if (ImGui::MenuItem("Clear Metadata", nullptr, false, !metadata.empty())) {
                ClearMetadata();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Refresh", nullptr, false, !currentFile.empty())) {
                if (!currentFile.empty()) {
                    LoadFromFile(currentFile);
                }
            }
            ImGui::EndPopup();
        }
    }

    void MetadataView::RenderMetadataDisplay() {
        ImGui::Text("File: %s", currentFile.c_str());
        ImGui::Separator();

        if (metadata.is_null() || metadata.empty()) {
            ImGui::Text("No metadata found in this file.");
            return;
        }

        ImGui::Text("Filter:");
        ImGui::SameLine();

        char filterBuffer[256];
        strncpy(filterBuffer, filterText.c_str(), sizeof(filterBuffer) - 1);
        filterBuffer[sizeof(filterBuffer) - 1] = '\0';

        if (ImGui::InputText("##filter", filterBuffer, sizeof(filterBuffer))) {
            filterText = filterBuffer;
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear Filter")) {
            filterText.clear();
        }

        ImGui::SameLine();
        if (ImGui::Button("Copy All")) {
            CopyEntireEntityToClipboard();
        }

        ImGui::Separator();

        if (ImGui::BeginChild("MetadataViewerChild", ImVec2(0, 0), true)) {
            if (displayMode == DisplayMode::Tree) {
                RenderJsonTree(metadata);
            }
            else {
                std::string jsonStr = metadata.dump(2);
                static char textBuffer[65536];
                strncpy(textBuffer, jsonStr.c_str(), sizeof(textBuffer) - 1);
                textBuffer[sizeof(textBuffer) - 1] = '\0';

                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
                ImGui::InputTextMultiline("##metadata", textBuffer, sizeof(textBuffer),
                    ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_ReadOnly);
                ImGui::PopFont();
            }
        }
        ImGui::EndChild();
    }

    void MetadataView::RenderJsonTree(const nlohmann::json& j, int depth) {
        float indent = 16.0f;
        bool hasFilter = !filterText.empty();
        std::string lowerFilter = filterText;
        std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);

        if (j.is_object()) {
            for (auto it = j.begin(); it != j.end(); ++it) {
                std::string key = it.key();

                // Check if key matches filter
                std::string lowerKey = key;
                std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
                bool keyMatches = !hasFilter || lowerKey.find(lowerFilter) != std::string::npos;

                const auto& value = it.value();

                // For objects and arrays, check if any child matches
                bool childMatches = false;
                if (keyMatches || !hasFilter) {
                    childMatches = true;
                }
                else if (value.is_object() || value.is_array()) {
                    childMatches = HasMatchingChild(value, lowerFilter);
                }

                if (hasFilter && !keyMatches && !childMatches) {
                    continue;
                }

                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + depth * indent);

                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "\"%s\"", key.c_str());
                ImGui::SameLine();

                if (value.is_object()) {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), ": {");
                    ImGui::SameLine();
                    RenderCopyButton(value, "copy_obj_" + key);
                    RenderJsonTree(value, depth + 1);
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + depth * indent);
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "}");
                    if (std::next(it) != j.end()) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), ",");
                    }
                }
                else if (value.is_array()) {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), ": [");
                    ImGui::SameLine();
                    RenderCopyButton(value, "copy_arr_" + key);
                    RenderJsonTree(value, depth + 1);
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + depth * indent);
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "]");
                    if (std::next(it) != j.end()) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), ",");
                    }
                }
                else {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), ": ");
                    ImGui::SameLine();
                    RenderJsonValue(value, key, depth);
                    ImGui::SameLine();
                    RenderCopyButton(value, "copy_val_" + key);
                    if (std::next(it) != j.end()) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), ",");
                    }
                }
            }
        }
        else if (j.is_array()) {
            for (size_t i = 0; i < j.size(); ++i) {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + depth * indent);

                std::string key = "[" + std::to_string(i) + "]";

                std::string lowerKey = key;
                std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
                bool keyMatches = !hasFilter || lowerKey.find(lowerFilter) != std::string::npos;

                const auto& value = j[i];

                bool childMatches = false;
                if (keyMatches || !hasFilter) {
                    childMatches = true;
                }
                else if (value.is_object() || value.is_array()) {
                    childMatches = HasMatchingChild(value, lowerFilter);
                }

                if (hasFilter && !keyMatches && !childMatches) {
                    continue;
                }

                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", key.c_str());
                ImGui::SameLine();

                if (value.is_object()) {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), ": {");
                    ImGui::SameLine();
                    RenderCopyButton(value, "copy_obj_" + key);
                    RenderJsonTree(value, depth + 1);
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + depth * indent);
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "}");
                    if (i < j.size() - 1) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), ",");
                    }
                }
                else if (value.is_array()) {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), ": [");
                    ImGui::SameLine();
                    RenderCopyButton(value, "copy_arr_" + key);
                    RenderJsonTree(value, depth + 1);
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + depth * indent);
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "]");
                    if (i < j.size() - 1) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), ",");
                    }
                }
                else {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), ": ");
                    ImGui::SameLine();
                    RenderJsonValue(value, key, depth);
                    ImGui::SameLine();
                    RenderCopyButton(value, "copy_val_" + key);
                    if (i < j.size() - 1) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), ",");
                    }
                }
            }
        }
    }

    bool MetadataView::HasMatchingChild(const nlohmann::json& j, const std::string& lowerFilter) {
        if (j.is_object()) {
            for (auto it = j.begin(); it != j.end(); ++it) {
                std::string lowerKey = it.key();
                std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
                if (lowerKey.find(lowerFilter) != std::string::npos) {
                    return true;
                }
                const auto& value = it.value();
                if (value.is_object() || value.is_array()) {
                    if (HasMatchingChild(value, lowerFilter)) {
                        return true;
                    }
                }
            }
        }
        else if (j.is_array()) {
            for (size_t i = 0; i < j.size(); ++i) {
                const auto& value = j[i];
                if (value.is_object() || value.is_array()) {
                    if (HasMatchingChild(value, lowerFilter)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    void MetadataView::RenderJsonValue(const nlohmann::json& value, const std::string& key, int depth) {
        ImVec4 color = GetColorForType(value);

        if (value.is_string()) {
            std::string str = value.get<std::string>();
            if (str.length() > 80) {
                str = str.substr(0, 77) + "...";
            }
            ImGui::TextColored(color, "\"%s\"", str.c_str());
        }
        else if (value.is_number_integer()) {
            ImGui::TextColored(color, "%d", value.get<int>());
        }
        else if (value.is_number_float()) {
            ImGui::TextColored(color, "%f", value.get<double>());
        }
        else if (value.is_boolean()) {
            ImGui::TextColored(color, "%s", value.get<bool>() ? "true" : "false");
        }
        else if (value.is_null()) {
            ImGui::TextColored(color, "null");
        }
        else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "?");
        }
    }

    void MetadataView::RenderCopyButton(const nlohmann::json& value, const std::string& label) {
        ImGui::SameLine();
        ImGui::PushID(label.c_str());
        if (ImGui::SmallButton("Copy")) {
            CopyValueToClipboard(value);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Copy this value to clipboard");
        }
        ImGui::PopID();
    }

    ImVec4 MetadataView::GetColorForType(const nlohmann::json& value) {
        if (value.is_string()) {
            return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
        }
        else if (value.is_number()) {
            return ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
        }
        else if (value.is_boolean()) {
            return ImVec4(1.0f, 0.0f, 1.0f, 1.0f);
        }
        else if (value.is_null()) {
            return ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
        }
        return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    bool MetadataView::MatchesFilter(const std::string& text) {
        if (filterText.empty()) return true;
        std::string lowerText = text;
        std::string lowerFilter = filterText;
        std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
        std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);
        return lowerText.find(lowerFilter) != std::string::npos;
    }

    void MetadataView::CopyValueToClipboard(const nlohmann::json& value) {
        std::string str;
        if (value.is_string()) {
            str = value.get<std::string>();
        }
        else {
            str = value.dump();
        }
        ImGui::SetClipboardText(str.c_str());
        std::cout << "[MetadataView] Copied value to clipboard" << std::endl;
    }

    void MetadataView::CopyEntireEntityToClipboard() {
        if (metadata.empty()) return;

        nlohmann::json clipboardData;
        clipboardData["dataType"] = "entity";
        clipboardData["source"] = "metadata";
        clipboardData["data"] = metadata;

        std::string jsonStr = clipboardData.dump(2);
        ImGui::SetClipboardText(jsonStr.c_str());
        std::cout << "[MetadataView] Copied entire entity to clipboard" << std::endl;
    }

    void MetadataView::ClearMetadata() {
        metadata = nlohmann::json::object();
        currentFile = "";
        std::cout << "[MetadataView] Cleared metadata" << std::endl;
    }

    void MetadataView::PasteFromClipboard() {
        const char* clipboardText = ImGui::GetClipboardText();
        if (clipboardText) {
            try {
                nlohmann::json jsonData = nlohmann::json::parse(clipboardText);

                if (jsonData.contains("dataType") && jsonData.contains("data")) {
                    metadata = jsonData["data"];
                    currentFile = "Clipboard (Entity)";
                }
                else {
                    metadata = jsonData;
                    currentFile = "Clipboard";
                }

                std::cout << "[MetadataView] Pasted from clipboard" << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "[MetadataView] Failed to parse clipboard JSON: " << e.what() << std::endl;
            }
        }
    }

    nlohmann::json MetadataView::ReadMetadataFromFile(const std::string& filePath) {
        std::string ext = std::filesystem::path(filePath).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        nlohmann::json rawMetadata;

        try {
            if (ext == ".json") {
                std::ifstream file(filePath);
                if (file.is_open()) {
                    file >> rawMetadata;
                    file.close();
                }
            }
            else if (ext == ".png") {
                rawMetadata = Utils::PngMetadata::ReadMetadataFromPNG(filePath);
            }
            else if (ext == ".jpg" || ext == ".jpeg") {
                rawMetadata = Utils::JpegMetadata::ReadMetadataFromJPEG(filePath);
            }
            else if (ext == ".webp") {
                rawMetadata = Utils::WebPMetadata::ReadMetadataFromWebP(filePath);
            }
            else if (ext == ".tiff") {
#ifdef USE_EXIV2
                rawMetadata = Utils::MetadataUtils::ReadMetadataFromTIFF(filePath);
#endif
            }
            else if (ext == ".mp4" || ext == ".webm" || ext == ".mkv" || ext == ".avi" ||
                ext == ".mov" || ext == ".flv" || ext == ".m4v" || ext == ".3gp" ||
                ext == ".ogv" || ext == ".ts" || ext == ".wmv" || ext == ".mpg" ||
                ext == ".mpeg" || ext == ".ogg") {
                rawMetadata = Utils::VideoMetadataUtils::ReadMetadataFromVideo(filePath);
            }
            else {
                // For unknown formats, try to read as JSON or sidecar
                std::string jsonPath = filePath + ".json";
                if (std::filesystem::exists(jsonPath)) {
                    rawMetadata = Utils::MetadataUtils::LoadMetadataFromJson(jsonPath);
                }
                else {
                    // Try to read as text
                    std::ifstream file(filePath);
                    if (file.is_open()) {
                        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                        file.close();
                        try {
                            rawMetadata = nlohmann::json::parse(content);
                        }
                        catch (...) {
                            // Not valid JSON
                        }
                    }
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[MetadataView] Error reading metadata: " << e.what() << std::endl;
            return nlohmann::json::object();
        }

        if (rawMetadata.is_null() || rawMetadata.empty()) {
            std::string jsonPath = filePath + ".json";
            if (std::filesystem::exists(jsonPath)) {
                rawMetadata = Utils::MetadataUtils::LoadMetadataFromJson(jsonPath);
            }
        }

        return rawMetadata;
    }

    void MetadataView::LoadFromFile(const std::string& filePath) {
        currentFile = filePath;
        metadata = ReadMetadataFromFile(filePath);

        if (metadata.is_null() || metadata.empty()) {
            metadata = nlohmann::json::object();
        }

        std::cout << "[MetadataView] Loaded metadata from: " << filePath << std::endl;
    }

    void MetadataView::LoadFromClipboard() {
        const char* clipboardText = ImGui::GetClipboardText();
        if (clipboardText) {
            try {
                nlohmann::json jsonData = nlohmann::json::parse(clipboardText);

                if (jsonData.contains("dataType") && jsonData.contains("data")) {
                    metadata = jsonData["data"];
                    currentFile = "Clipboard (Entity)";
                }
                else {
                    metadata = jsonData;
                    currentFile = "Clipboard";
                }

                std::cout << "[MetadataView] Loaded metadata from clipboard" << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "[MetadataView] Failed to parse clipboard JSON: " << e.what() << std::endl;
            }
        }
    }

    void MetadataView::SaveMetadataToFile() {
        if (metadata.empty()) return;

        std::string filePath;
        if (FileDialog::SaveFile("Save Metadata As", FileDialog::FilterType::METADATA_FILE, "metadata.json", filePath)) {
            std::ofstream file(filePath);
            if (file.is_open()) {
                file << metadata.dump(2);
                file.close();
                std::cout << "[MetadataView] Saved metadata to: " << filePath << std::endl;
            }
        }
    }

    void MetadataView::SetMetadata(const nlohmann::json& metadata, const std::string& source) {
        this->metadata = metadata;
        currentFile = source;
        std::cout << "[MetadataView] Metadata set from: " << source << std::endl;
    }

}