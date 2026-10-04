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
#include "SteganographyUtils.hpp"
#include "Log.hpp"

#include <imgui.h>
#include <stb_image.h>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace GUI {

    MetadataView::MetadataView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseView(mgr, vm) {
        viewName = "MetadataView";
        metadata = nlohmann::json::object();
        fileMetadata = nlohmann::json::object();
        lsbMetadata = nlohmann::json::object();
        currentFile = "";
        filterText = "";
        ANI_LOG_DEBUG("Constructed");
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

            bool hasFileMeta = !(fileMetadata.is_null() || fileMetadata.empty());
            bool hasLsbMeta = !(lsbMetadata.is_null() || lsbMetadata.empty());

            if (ImGui::BeginMenu("Metadata Source")) {
                if (ImGui::MenuItem("File Metadata", nullptr,
                    metadataSource == MetadataSource::File, hasFileMeta)) {
                    metadataSource = MetadataSource::File;
                    ApplyMetadataSource();
                }
                if (ImGui::MenuItem("LSB Metadata", nullptr,
                    metadataSource == MetadataSource::LSB, hasLsbMeta)) {
                    metadataSource = MetadataSource::LSB;
                    ApplyMetadataSource();
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
            bool hasFileMeta = !(fileMetadata.is_null() || fileMetadata.empty());
            bool hasLsbMeta = !(lsbMetadata.is_null() || lsbMetadata.empty());
            if (ImGui::MenuItem("Show File Metadata", nullptr,
                metadataSource == MetadataSource::File, hasFileMeta)) {
                metadataSource = MetadataSource::File;
                ApplyMetadataSource();
            }
            if (ImGui::MenuItem("Show LSB Metadata", nullptr,
                metadataSource == MetadataSource::LSB, hasLsbMeta)) {
                metadataSource = MetadataSource::LSB;
                ApplyMetadataSource();
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
        ImGui::SameLine();

        const char* sourceLabel =
            (metadataSource == MetadataSource::LSB) ? "[LSB]" : "[File]";
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", sourceLabel);

        ImGui::Separator();

        if (metadata.is_null() || metadata.empty()) {
            if (metadataSource == MetadataSource::LSB) {
                ImGui::Text("No LSB metadata found in this file.");
            }
            else {
                ImGui::Text("No file metadata found in this file.");
            }
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

        if (displayMode == DisplayMode::Tree) {
            ImGui::SameLine();
            if (ImGui::Button("Expand All")) {
                ++treeGeneration;
                treeForceOpen = 1;
                treeForceOpenPending = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Collapse All")) {
                ++treeGeneration;
                treeForceOpen = 0;
                treeForceOpenPending = true;
            }
        }

        ImGui::Separator();

        if (ImGui::BeginChild("MetadataViewerChild", ImVec2(0, 0), true)) {
            nodeCounter = 0;
            if (displayMode == DisplayMode::Tree) {
                RenderJsonTree(metadata);
                treeForceOpenPending = false;
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

    void MetadataView::RenderJsonTree(const nlohmann::json& j, int depth, const std::string& path) {
        float indent = 16.0f;
        bool hasFilter = !filterText.empty();
        std::string lowerFilter = filterText;
        std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);

        if (j.is_object()) {
            for (auto it = j.begin(); it != j.end(); ++it) {
                std::string key = it.key();

                std::string lowerKey = key;
                std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
                bool keyMatches = !hasFilter || lowerKey.find(lowerFilter) != std::string::npos;

                const auto& value = it.value();

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

                std::string childPath = path + "/" + key;
                std::string nodeId = childPath + "##g" + std::to_string(treeGeneration);

                if (value.is_object() || value.is_array()) {
                    if (treeForceOpenPending) {
                        ImGui::SetNextItemOpen(treeForceOpen != 0, ImGuiCond_Always);
                    }

                    bool nodeOpen = ImGui::TreeNodeEx(nodeId.c_str(),
                        ImGuiTreeNodeFlags_OpenOnArrow |
                        ImGuiTreeNodeFlags_OpenOnDoubleClick |
                        ImGuiTreeNodeFlags_SpanAvailWidth,
                        "\"%s\": %s", key.c_str(),
                        value.is_object() ? "{" : "[");

                    ImGui::SameLine();
                    RenderCopyButton(value);

                    if (nodeOpen) {
                        RenderJsonTree(value, depth + 1, childPath);

                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + depth * indent);
                        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                            value.is_object() ? "}" : "]");
                        ImGui::SameLine();
                        RenderCopyButton(value);
                        ImGui::TreePop();
                    }
                }
                else {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "\"%s\"", key.c_str());
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), ":");
                    ImGui::SameLine();

                    float avail = ImGui::GetContentRegionAvail().x;
                    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + avail);
                    RenderJsonValue(value);
                    ImGui::PopTextWrapPos();

                    ImGui::SameLine();
                    RenderCopyButton(value);
                }
            }
        }
        else if (j.is_array()) {
            for (size_t i = 0; i < j.size(); ++i) {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + depth * indent);

                std::string key = "[" + std::to_string(i) + "]";
                std::string childPath = path + "/" + key;
                std::string nodeId = childPath + "##g" + std::to_string(treeGeneration);

                const auto& value = j[i];

                std::string lowerKey = key;
                std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
                bool keyMatches = !hasFilter || lowerKey.find(lowerFilter) != std::string::npos;

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

                if (value.is_object() || value.is_array()) {
                    if (treeForceOpenPending) {
                        ImGui::SetNextItemOpen(treeForceOpen != 0, ImGuiCond_Always);
                    }

                    bool nodeOpen = ImGui::TreeNodeEx(nodeId.c_str(),
                        ImGuiTreeNodeFlags_OpenOnArrow |
                        ImGuiTreeNodeFlags_OpenOnDoubleClick |
                        ImGuiTreeNodeFlags_SpanAvailWidth,
                        "%s: %s", key.c_str(),
                        value.is_object() ? "{" : "[");

                    ImGui::SameLine();
                    RenderCopyButton(value);

                    if (nodeOpen) {
                        RenderJsonTree(value, depth + 1, childPath);

                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + depth * indent);
                        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                            value.is_object() ? "}" : "]");
                        ImGui::SameLine();
                        RenderCopyButton(value);
                        ImGui::TreePop();
                    }
                }
                else {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", key.c_str());
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), ":");
                    ImGui::SameLine();

                    float avail = ImGui::GetContentRegionAvail().x;
                    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + avail);
                    RenderJsonValue(value);
                    ImGui::PopTextWrapPos();

                    ImGui::SameLine();
                    RenderCopyButton(value);
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

    void MetadataView::RenderJsonValue(const nlohmann::json& value) {
        ImVec4 color = GetColorForType(value);

        if (value.is_string()) {
            std::string str = value.get<std::string>();
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

    void MetadataView::RenderCopyButton(const nlohmann::json& value) {
        ImGui::PushID(nodeCounter++);
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
        ANI_LOG_TRACE("Copied value to clipboard");
    }

    void MetadataView::CopyEntireEntityToClipboard() {
        if (metadata.empty()) {
            ANI_LOG_TRACE("CopyEntireEntityToClipboard: no metadata");
            return;
        }

        nlohmann::json clipboardData;
        clipboardData["dataType"] = "entity";
        clipboardData["source"] = "metadata";
        clipboardData["data"] = metadata;

        std::string jsonStr = clipboardData.dump(2);
        ImGui::SetClipboardText(jsonStr.c_str());
        ANI_LOG_DEBUG("Copied entire entity to clipboard (%zu bytes)", jsonStr.size());
    }

    void MetadataView::ClearMetadata() {
        metadata = nlohmann::json::object();
        fileMetadata = nlohmann::json::object();
        lsbMetadata = nlohmann::json::object();
        currentFile = "";
        metadataSource = MetadataSource::File;
        ANI_LOG_DEBUG("Cleared metadata");
    }

    void MetadataView::PasteFromClipboard() {
        const char* clipboardText = ImGui::GetClipboardText();
        if (!clipboardText) {
            ANI_LOG_TRACE("PasteFromClipboard: no clipboard text");
            return;
        }

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

            ExpandEmbeddedJson(metadata);
            fileMetadata = metadata;
            lsbMetadata = nlohmann::json::object();
            metadataSource = MetadataSource::File;
            ++treeGeneration;
            treeForceOpen = 1;
            treeForceOpenPending = true;

            ANI_LOG_DEBUG("Pasted from clipboard");
        }
        catch (const std::exception& e) {
            ANI_LOG_WARN("Failed to parse clipboard JSON: %s", e.what());
        }
    }

    void MetadataView::ExpandEmbeddedJson(nlohmann::json& j) {
        if (j.is_object()) {
            for (auto it = j.begin(); it != j.end(); ++it) {
                ExpandEmbeddedJson(it.value());
            }
        }
        else if (j.is_array()) {
            for (auto& element : j) {
                ExpandEmbeddedJson(element);
            }
        }
        else if (j.is_string()) {
            const std::string& s = j.get_ref<const std::string&>();
            if (s.empty()) return;

            size_t start = 0;
            while (start < s.size() &&
                (s[start] == ' ' || s[start] == '\t' ||
                    s[start] == '\r' || s[start] == '\n'))
            {
                ++start;
            }
            if (start >= s.size()) return;
            if (s[start] != '{' && s[start] != '[') return;

            try {
                nlohmann::json parsed = nlohmann::json::parse(s);
                if (parsed.is_object() || parsed.is_array()) {
                    j = parsed;
                    ExpandEmbeddedJson(j);
                }
            }
            catch (...) {
            }
        }
    }

    void MetadataView::ReadAllMetadataFromFile(const std::string& filePath) {
        fileMetadata = nlohmann::json::object();
        lsbMetadata = nlohmann::json::object();

        std::string ext = std::filesystem::path(filePath).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        try {
            if (ext == ".json") {
                std::ifstream file(filePath);
                if (file.is_open()) {
                    file >> fileMetadata;
                    file.close();
                }
                else {
                    ANI_LOG_WARN("Failed to open JSON file: %s", filePath.c_str());
                }
            }
            else if (ext == ".png") {
                fileMetadata = Utils::PngMetadata::ReadMetadataFromPNG(filePath);
            }
            else if (ext == ".jpg" || ext == ".jpeg") {
                fileMetadata = Utils::JpegMetadata::ReadMetadataFromJPEG(filePath);
            }
            else if (ext == ".webp") {
                fileMetadata = Utils::WebPMetadata::ReadMetadataFromWebP(filePath);
            }
            else if (ext == ".tiff") {
#ifdef USE_EXIV2
                fileMetadata = Utils::MetadataUtils::ReadMetadataFromTIFF(filePath);
#else
                ANI_LOG_WARN("TIFF support disabled (USE_EXIV2 not defined): %s", filePath.c_str());
#endif
            }
            else if (ext == ".mp4" || ext == ".webm" || ext == ".mkv" || ext == ".avi" ||
                ext == ".mov" || ext == ".flv" || ext == ".m4v" || ext == ".3gp" ||
                ext == ".ogv" || ext == ".ts" || ext == ".wmv" || ext == ".mpg" ||
                ext == ".mpeg" || ext == ".ogg") {
                fileMetadata = Utils::VideoMetadataUtils::ReadMetadataFromVideo(filePath);
            }
            else {
                std::string jsonPath = filePath + ".json";
                if (std::filesystem::exists(jsonPath)) {
                    fileMetadata = Utils::MetadataUtils::LoadMetadataFromJson(jsonPath);
                }
                else {
                    std::ifstream file(filePath);
                    if (file.is_open()) {
                        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                        file.close();
                        try {
                            fileMetadata = nlohmann::json::parse(content);
                        }
                        catch (...) {
                        }
                    }
                    else {
                        ANI_LOG_WARN("Could not open file for metadata read: %s", filePath.c_str());
                    }
                }
            }
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("Error reading file metadata from %s: %s", filePath.c_str(), e.what());
        }

        ExpandEmbeddedJson(fileMetadata);

        const bool isImageExt = (ext == ".png" || ext == ".jpg" || ext == ".jpeg"
            || ext == ".webp" || ext == ".tiff");
        if (isImageExt) {
            try {
                int w = 0, h = 0, ch = 0;
                unsigned char* pixels = stbi_load(filePath.c_str(), &w, &h, &ch, 4);
                if (pixels) {
                    nlohmann::json parsed =
                        Utils::SteganographyUtils::ExtractJsonFromAlpha(pixels, w, h);
                    stbi_image_free(pixels);

                    if (!parsed.is_null() && !parsed.empty()) {
                        ExpandEmbeddedJson(parsed);
                        lsbMetadata = parsed;
                        if (Utils::SteganographyUtils::LooksLikeAniStudioMetadata(parsed)) {
                            lsbMetadata["_aniStudioCompatible"] = true;
                        }
                        ANI_LOG_DEBUG("Read LSB metadata from: %s", filePath.c_str());
                    }
                }
            }
            catch (const std::exception& e) {
                ANI_LOG_WARN("LSB metadata read failed for %s: %s", filePath.c_str(), e.what());
            }
        }

        if (fileMetadata.is_null() || fileMetadata.empty()) {
            std::string jsonPath = filePath + ".json";
            if (std::filesystem::exists(jsonPath)) {
                ANI_LOG_TRACE("Trying sidecar JSON: %s", jsonPath.c_str());
                fileMetadata = Utils::MetadataUtils::LoadMetadataFromJson(jsonPath);
                ExpandEmbeddedJson(fileMetadata);
            }
        }
    }

    void MetadataView::ApplyMetadataSource() {
        if (metadataSource == MetadataSource::LSB) {
            metadata = lsbMetadata;
        }
        else {
            metadata = fileMetadata;
        }
        ++treeGeneration;
        treeForceOpen = 1;
        treeForceOpenPending = true;
    }

    void MetadataView::LoadFromFile(const std::string& filePath) {
        currentFile = filePath;
        ReadAllMetadataFromFile(filePath);

        const bool hasFileMeta = !(fileMetadata.is_null() || fileMetadata.empty());
        const bool hasLsbMeta = !(lsbMetadata.is_null() || lsbMetadata.empty());

        if (hasFileMeta) {
            metadataSource = MetadataSource::File;
        }
        else if (hasLsbMeta) {
            metadataSource = MetadataSource::LSB;
        }
        else {
            metadataSource = MetadataSource::File;
        }

        ApplyMetadataSource();

        if (metadata.is_null() || metadata.empty()) {
            metadata = nlohmann::json::object();
            ANI_LOG_WARN("Loaded file but no metadata found: %s", filePath.c_str());
        }
        else {
            ANI_LOG_INFO("Loaded metadata from: %s (source=%s)",
                filePath.c_str(),
                metadataSource == MetadataSource::LSB ? "lsb" : "file");
        }
    }

    void MetadataView::LoadFromClipboard() {
        const char* clipboardText = ImGui::GetClipboardText();
        if (!clipboardText) {
            ANI_LOG_TRACE("LoadFromClipboard: no clipboard text");
            return;
        }

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

            ExpandEmbeddedJson(metadata);
            fileMetadata = metadata;
            lsbMetadata = nlohmann::json::object();
            metadataSource = MetadataSource::File;
            ++treeGeneration;
            treeForceOpen = 1;
            treeForceOpenPending = true;

            ANI_LOG_DEBUG("Loaded metadata from clipboard");
        }
        catch (const std::exception& e) {
            ANI_LOG_WARN("Failed to parse clipboard JSON: %s", e.what());
        }
    }

    void MetadataView::SaveMetadataToFile() {
        if (metadata.empty()) {
            ANI_LOG_TRACE("SaveMetadataToFile: no metadata");
            return;
        }

        std::string filePath;
        if (FileDialog::SaveFile("Save Metadata As", FileDialog::FilterType::METADATA_FILE, "metadata.json", filePath)) {
            std::ofstream file(filePath);
            if (file.is_open()) {
                file << metadata.dump(2);
                file.close();
                ANI_LOG_INFO("Saved metadata to: %s", filePath.c_str());
            }
            else {
                ANI_LOG_ERROR("Failed to open file for writing: %s", filePath.c_str());
            }
        }
    }

    void MetadataView::SetMetadata(const nlohmann::json& metadata, const std::string& source) {
        this->metadata = metadata;
        ExpandEmbeddedJson(this->metadata);
        this->fileMetadata = this->metadata;
        this->lsbMetadata = nlohmann::json::object();
        this->metadataSource = MetadataSource::File;
        currentFile = source;
        ++treeGeneration;
        treeForceOpen = 1;
        treeForceOpenPending = true;
        ANI_LOG_DEBUG("Metadata set from: %s", source.c_str());
    }

} // namespace GUI