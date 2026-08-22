#include "MetadataView.hpp"
#include "FileDialogUtil.hpp"
#include "FileDialogFilters.hpp"
#include "Events.hpp"
#include "ImageUtils.hpp"
#include "VideoMetadataUtils.hpp"
#include "MetadataUtils.hpp"
#include "ClipboardUtilities.hpp"
#include "DragDropUtils.hpp"
#include "ContextMenuUtils.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <png.h>
#include <regex>
#include <cstring>
#include <set>

namespace GUI {

    MetadataView::MetadataView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseView(mgr, vm), contextMenuUtils(std::make_unique<Utils::ContextMenuUtils>(mgr)) {
        viewName = "MetadataView";
        std::memset(filterBuffer, 0, sizeof(filterBuffer));
    }

    void MetadataView::Init() {}

    void MetadataView::Update(float deltaT) {}

    void MetadataView::Render() {
        ImGui::Begin(GetWindowTitle().c_str(), &windowOpen, ImGuiWindowFlags_MenuBar);

        if (!metadata.is_null() && !metadata.empty()) {
            ImGui::Text("Source: %s", sourceDisplayName.c_str());
            ImGui::Separator();
        }

        RenderMenuBar();
        RenderFilterBar();

        if (metadata.is_null() || metadata.empty()) {
            ImGui::Text("No metadata loaded. Use File menu to load or drag & drop a file/entity.");
            HandleDropTarget();
            ImGui::End();
            return;
        }

        if (currentMode == DisplayMode::Simplified) {
            RenderSimplifiedView();
        }
        else {
            RenderRawJSONView();
        }

        HandleDropTarget();
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
            RenderFileMenu();
            RenderViewMenu();
            RenderEditMenu();
            ImGui::EndMenuBar();
        }
    }

    void MetadataView::RenderFileMenu() {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Load from File...")) {
                std::string outPath;
                if (FileDialog::OpenFile("Load Metadata", FileDialog::FilterType::ALL_FILES, outPath)) {
                    LoadFromFile(outPath);
                }
            }
            if (ImGui::MenuItem("Load from Entity...")) {
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save As JSON...", nullptr, false, !metadata.is_null())) {
                std::string outPath;
                std::string defaultName = "metadata.json";
                if (FileDialog::SaveFile("Save Metadata as JSON", FileDialog::FilterType::ALL_FILES, defaultName, outPath)) {
                    std::ofstream file(outPath);
                    if (file.is_open()) {
                        file << metadata.dump(4);
                        file.close();
                        std::cout << "[MetadataView] Saved metadata to " << outPath << std::endl;
                    }
                }
            }
            if (ImGui::MenuItem("Export to Sidecar", nullptr, false, !metadata.is_null() && !sourcePath.empty())) {
                std::string sidecarPath = sourcePath + ".json";
                std::ofstream file(sidecarPath);
                if (file.is_open()) {
                    file << metadata.dump(4);
                    file.close();
                    std::cout << "[MetadataView] Exported sidecar to " << sidecarPath << std::endl;
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Clear")) {
                ClearMetadata();
            }
            ImGui::EndMenu();
        }
    }

    void MetadataView::RenderViewMenu() {
        if (ImGui::BeginMenu("View")) {
            bool simplified = (currentMode == DisplayMode::Simplified);
            if (ImGui::MenuItem("Simplified", nullptr, &simplified)) {
                currentMode = DisplayMode::Simplified;
            }
            bool raw = (currentMode == DisplayMode::RawJSON);
            if (ImGui::MenuItem("Raw JSON", nullptr, &raw)) {
                currentMode = DisplayMode::RawJSON;
            }
            ImGui::EndMenu();
        }
    }

    void MetadataView::RenderEditMenu() {
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Copy Metadata as JSON", nullptr, false, !metadata.is_null())) {
                CopyMetadataAsJSON();
            }
            if (ImGui::MenuItem("Paste from Clipboard", nullptr, false)) {
                PasteFromClipboard();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Clear")) {
                ClearMetadata();
            }
            ImGui::EndMenu();
        }
    }

    void MetadataView::RenderFilterBar() {
        if (metadata.is_null() || metadata.empty()) return;
        ImGui::Text("Filter:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        if (ImGui::InputText("##filter", filterBuffer, sizeof(filterBuffer))) {
            filterText = filterBuffer;
            UpdateDisplayMetadata();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            filterText.clear();
            std::memset(filterBuffer, 0, sizeof(filterBuffer));
            UpdateDisplayMetadata();
        }
        ImGui::Separator();
    }

    void MetadataView::RenderSimplifiedView() {
        ImGui::BeginChild("SimplifiedView", ImVec2(0, 0), true);

        if (displayMetadata.is_object()) {
            for (auto it = displayMetadata.begin(); it != displayMetadata.end(); ++it) {
                RenderJSONNode(it.value(), it.key(), it.key());
            }
        }
        else {
            RenderJSONNode(displayMetadata, "root", "root");
        }

        if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup("SimplifiedViewBackgroundContext");
        }
        if (ImGui::BeginPopup("SimplifiedViewBackgroundContext")) {
            if (ImGui::MenuItem("Paste from Clipboard")) {
                PasteFromClipboard();
            }
            if (ImGui::MenuItem("Clear")) {
                ClearMetadata();
            }
            ImGui::EndPopup();
        }

        ImGui::EndChild();
    }

    void MetadataView::RenderJSONNode(const nlohmann::json& value, const std::string& key, const std::string& path) {
        ImGui::PushID(path.c_str());

        if (value.is_object()) {
            if (ImGui::TreeNodeEx(key.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                for (auto it = value.begin(); it != value.end(); ++it) {
                    std::string childPath = path + "." + it.key();
                    RenderJSONNode(it.value(), it.key(), childPath);
                }
                ImGui::TreePop();
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                ImGui::OpenPopup(("NodeContext##" + path).c_str());
            }
            RenderNodeContextMenu(path);
        }
        else if (value.is_array()) {
            bool allObjects = true;
            bool allHaveSingleKey = true;
            std::set<std::string> keys;
            for (const auto& elem : value) {
                if (!elem.is_object()) { allObjects = false; break; }
                if (elem.size() != 1) { allHaveSingleKey = false; break; }
                for (auto eit = elem.begin(); eit != elem.end(); ++eit) keys.insert(eit.key());
            }
            if (allObjects && allHaveSingleKey && keys.size() == value.size()) {
                nlohmann::json merged;
                for (const auto& elem : value) {
                    for (auto eit = elem.begin(); eit != elem.end(); ++eit) {
                        merged[eit.key()] = eit.value();
                    }
                }
                if (ImGui::TreeNodeEx(key.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    for (auto it = merged.begin(); it != merged.end(); ++it) {
                        std::string childPath = path + "." + it.key();
                        RenderJSONNode(it.value(), it.key(), childPath);
                    }
                    ImGui::TreePop();
                }
            }
            else {
                if (ImGui::TreeNodeEx(key.c_str(), ImGuiTreeNodeFlags_DefaultOpen, "[%zu]", value.size())) {
                    for (size_t i = 0; i < value.size(); ++i) {
                        std::string childPath = path + "[" + std::to_string(i) + "]";
                        RenderJSONNode(value[i], "[" + std::to_string(i) + "]", childPath);
                    }
                    ImGui::TreePop();
                }
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                ImGui::OpenPopup(("NodeContext##" + path).c_str());
            }
            RenderNodeContextMenu(path);
        }
        else {
            RenderValue(value, key, path);
        }

        ImGui::PopID();
    }

    void MetadataView::RenderValue(const nlohmann::json& value, const std::string& key, const std::string& path) {
        std::string displayValue;
        if (value.is_string()) {
            displayValue = value.get<std::string>();
        }
        else if (value.is_number()) {
            displayValue = std::to_string(value.get<double>());
        }
        else if (value.is_boolean()) {
            displayValue = value.get<bool>() ? "true" : "false";
        }
        else if (value.is_null()) {
            displayValue = "null";
        }
        else {
            displayValue = value.dump();
        }

        const size_t maxLen = 80;
        std::string truncated = displayValue;
        if (truncated.length() > maxLen) {
            truncated = truncated.substr(0, maxLen) + "...";
        }

        ImGui::Text("%s: ", key.c_str());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "%s", truncated.c_str());

        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup(("ValueContext##" + path).c_str());
        }
        if (ImGui::BeginPopup(("ValueContext##" + path).c_str())) {
            ImGui::Text("Key: %s", key.c_str());
            ImGui::Separator();
            if (ImGui::MenuItem("Copy Value")) {
                CopyValue(path);
            }
            if (ImGui::MenuItem("Copy Key")) {
                CopyPath(path);
            }
            if (ImGui::MenuItem("Copy Entire Metadata")) {
                CopyMetadataAsJSON();
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();
        if (ImGui::SmallButton(("Copy##" + path).c_str())) {
            CopyValue(path);
        }
    }

    void MetadataView::RenderNodeContextMenu(const std::string& path) {
        if (ImGui::BeginPopup(("NodeContext##" + path).c_str())) {
            ImGui::Text("Node: %s", path.c_str());
            ImGui::Separator();
            if (ImGui::MenuItem("Copy Node as JSON")) {
                CopyValue(path);
            }
            if (ImGui::MenuItem("Copy Path")) {
                CopyPath(path);
            }
            if (ImGui::MenuItem("Copy Entire Metadata")) {
                CopyMetadataAsJSON();
            }
            ImGui::EndPopup();
        }
    }

    void MetadataView::RenderRawJSONView() {
        ImGui::BeginChild("RawView", ImVec2(0, 0), true);

        std::string jsonStr = displayMetadata.dump(4);
        ImGui::InputTextMultiline("##rawjson", const_cast<char*>(jsonStr.c_str()), jsonStr.size() + 1,
            ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly);

        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup("RawViewContext");
        }
        if (ImGui::BeginPopup("RawViewContext")) {
            if (ImGui::MenuItem("Copy All")) {
                CopyMetadataAsJSON();
            }
            if (ImGui::MenuItem("Paste from Clipboard")) {
                PasteFromClipboard();
            }
            if (ImGui::MenuItem("Clear")) {
                ClearMetadata();
            }
            ImGui::EndPopup();
        }

        ImGui::EndChild();
    }

    void MetadataView::CopyValue(const std::string& path) {
        nlohmann::json* current = &displayMetadata;
        std::stringstream ss(path);
        std::string token;
        while (std::getline(ss, token, '.')) {
            if (token.find('[') != std::string::npos) {
                size_t bracket = token.find('[');
                std::string key = token.substr(0, bracket);
                std::string indexStr = token.substr(bracket + 1);
                indexStr.pop_back();
                int index = std::stoi(indexStr);
                if (current->is_object() && current->contains(key)) {
                    current = &(*current)[key];
                }
                if (current->is_array() && index >= 0 && index < current->size()) {
                    current = &(*current)[index];
                }
                else {
                    return;
                }
            }
            else {
                if (current->is_object() && current->contains(token)) {
                    current = &(*current)[token];
                }
                else {
                    return;
                }
            }
        }
        std::string valueStr = current->dump(4);
        ImGui::SetClipboardText(valueStr.c_str());
        std::cout << "[MetadataView] Copied value at path: " << path << std::endl;
    }

    void MetadataView::CopyPath(const std::string& path) {
        ImGui::SetClipboardText(path.c_str());
        std::cout << "[MetadataView] Copied path: " << path << std::endl;
    }

    void MetadataView::CopyMetadataAsJSON() {
        if (metadata.is_null() || metadata.empty()) return;
        std::string jsonStr = metadata.dump(4);
        ImGui::SetClipboardText(jsonStr.c_str());
        std::cout << "[MetadataView] Copied metadata as JSON." << std::endl;
    }

    std::string MetadataView::ReadRawTextChunkFromPNG(const std::string& filePath, const std::string& key) {
        std::string result;
        FILE* fp = fopen(filePath.c_str(), "rb");
        if (!fp) return result;

        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (!png) { fclose(fp); return result; }

        png_infop info = png_create_info_struct(png);
        if (!info) { png_destroy_read_struct(&png, nullptr, nullptr); fclose(fp); return result; }

        if (setjmp(png_jmpbuf(png))) {
            png_destroy_read_struct(&png, &info, nullptr);
            fclose(fp);
            return result;
        }

        png_init_io(png, fp);
        png_read_info(png, info);

        png_textp text_ptr;
        int num_text;
        if (png_get_text(png, info, &text_ptr, &num_text) > 0) {
            for (int i = 0; i < num_text; i++) {
                if (strcmp(text_ptr[i].key, key.c_str()) == 0) {
                    result = text_ptr[i].text;
                    break;
                }
            }
        }

        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        return result;
    }

    std::vector<std::pair<std::string, std::string>> MetadataView::ReadAllTextChunksFromPNG(const std::string& filePath) {
        std::vector<std::pair<std::string, std::string>> chunks;
        FILE* fp = fopen(filePath.c_str(), "rb");
        if (!fp) return chunks;

        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (!png) { fclose(fp); return chunks; }

        png_infop info = png_create_info_struct(png);
        if (!info) { png_destroy_read_struct(&png, nullptr, nullptr); fclose(fp); return chunks; }

        if (setjmp(png_jmpbuf(png))) {
            png_destroy_read_struct(&png, &info, nullptr);
            fclose(fp);
            return chunks;
        }

        png_init_io(png, fp);
        png_read_info(png, info);

        png_textp text_ptr;
        int num_text;
        if (png_get_text(png, info, &text_ptr, &num_text) > 0) {
            for (int i = 0; i < num_text; i++) {
                chunks.push_back({ text_ptr[i].key, text_ptr[i].text });
            }
        }

        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        return chunks;
    }

    nlohmann::json MetadataView::ParseRawMetadata(const std::string& rawText) {
        nlohmann::json result;
        if (rawText.empty()) return result;

        std::vector<std::string> lines;
        std::istringstream stream(rawText);
        std::string line;
        while (std::getline(stream, line)) {
            if (!line.empty()) lines.push_back(line);
        }

        if (lines.empty()) return result;

        std::string positivePrompt;
        std::string negativePrompt;
        std::vector<std::string> paramLines;

        size_t i = 0;
        bool inNegative = false;
        bool inParams = false;

        while (i < lines.size()) {
            const std::string& l = lines[i];

            if (!inNegative && l.find("Negative prompt:") == 0) {
                std::string neg = l.substr(std::string("Negative prompt:").length());
                if (!neg.empty() && neg[0] == ' ') neg.erase(0, 1);
                negativePrompt = neg;
                inNegative = true;
                ++i;
                continue;
            }

            if (inNegative && !inParams) {
                if (l.find(':') != std::string::npos && (l.find("Steps:") == 0 || l.find("CFG scale:") == 0 || l.find("Seed:") == 0 ||
                    l.find("Size:") == 0 || l.find("Model hash:") == 0 || l.find("Denoising strength:") == 0 ||
                    l.find("Clip skip:") == 0 || l.find("Mask blur:") == 0 || l.find("SD upscale overlap:") == 0 ||
                    l.find("SD upscale upscaler:") == 0)) {
                    inParams = true;
                }
            }

            if (inNegative && !inParams) {
                if (!negativePrompt.empty()) negativePrompt += "\n";
                negativePrompt += l;
                ++i;
                continue;
            }

            if (inParams) {
                paramLines.push_back(l);
                ++i;
                continue;
            }

            if (!inNegative && !inParams) {
                if (!positivePrompt.empty()) positivePrompt += "\n";
                positivePrompt += l;
                ++i;
            }
        }

        if (!positivePrompt.empty()) {
            result["Positive prompt"] = positivePrompt;
        }
        if (!negativePrompt.empty()) {
            result["Negative prompt"] = negativePrompt;
        }

        for (const auto& paramLine : paramLines) {
            std::vector<std::string> pairs;
            std::stringstream ss(paramLine);
            std::string pair;
            while (std::getline(ss, pair, ',')) {
                if (!pair.empty()) pairs.push_back(pair);
            }
            for (const auto& p : pairs) {
                size_t colon = p.find(':');
                if (colon != std::string::npos) {
                    std::string key = p.substr(0, colon);
                    std::string value = p.substr(colon + 1);
                    key.erase(0, key.find_first_not_of(" \t"));
                    key.erase(key.find_last_not_of(" \t") + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t") + 1);
                    if (!key.empty() && !value.empty()) {
                        result[key] = value;
                    }
                }
            }
        }

        return result;
    }

    nlohmann::json MetadataView::FlattenEntityMetadata(const nlohmann::json& input) {
        if (!input.is_object()) return input;

        nlohmann::json output;
        for (auto it = input.begin(); it != input.end(); ++it) {
            const std::string& key = it.key();
            const nlohmann::json& value = it.value();

            if (value.is_array()) {
                bool allObjects = true;
                bool allHaveSingleKey = true;
                std::set<std::string> keys;
                for (const auto& elem : value) {
                    if (!elem.is_object()) { allObjects = false; break; }
                    if (elem.size() != 1) { allHaveSingleKey = false; break; }
                    for (auto eit = elem.begin(); eit != elem.end(); ++eit) keys.insert(eit.key());
                }
                if (allObjects && allHaveSingleKey && keys.size() == value.size()) {
                    nlohmann::json merged;
                    for (const auto& elem : value) {
                        for (auto eit = elem.begin(); eit != elem.end(); ++eit) {
                            merged[eit.key()] = eit.value();
                        }
                    }
                    output[key] = merged;
                }
                else {
                    nlohmann::json newArray = nlohmann::json::array();
                    for (const auto& elem : value) {
                        if (elem.is_object()) {
                            newArray.push_back(FlattenEntityMetadata(elem));
                        }
                        else {
                            newArray.push_back(elem);
                        }
                    }
                    output[key] = newArray;
                }
            }
            else if (value.is_object()) {
                output[key] = FlattenEntityMetadata(value);
            }
            else {
                output[key] = value;
            }
        }

        return output;
    }

    nlohmann::json MetadataView::FilterJSONNode(const nlohmann::json& node, const std::string& filterLower) {
        if (filterLower.empty()) return node;

        if (node.is_object()) {
            nlohmann::json result;
            for (auto it = node.begin(); it != node.end(); ++it) {
                std::string keyLower = it.key();
                std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(), ::tolower);
                bool keyMatches = (keyLower.find(filterLower) != std::string::npos);

                nlohmann::json filteredChild;
                bool childHasMatch = false;

                if (!keyMatches) {
                    filteredChild = FilterJSONNode(it.value(), filterLower);
                    if (!filteredChild.is_null() && !filteredChild.empty()) {
                        childHasMatch = true;
                    }
                }

                if (keyMatches || childHasMatch) {
                    if (keyMatches) {
                        result[it.key()] = it.value(); // include whole subtree
                    }
                    else {
                        result[it.key()] = filteredChild;
                    }
                }
            }
            return result;
        }
        else if (node.is_array()) {
            nlohmann::json result = nlohmann::json::array();
            for (const auto& elem : node) {
                nlohmann::json filteredElem = FilterJSONNode(elem, filterLower);
                if (!filteredElem.is_null() && !filteredElem.empty()) {
                    result.push_back(filteredElem);
                }
            }
            return result;
        }
        else if (node.is_string()) {
            std::string valueLower = node.get<std::string>();
            std::transform(valueLower.begin(), valueLower.end(), valueLower.begin(), ::tolower);
            if (valueLower.find(filterLower) != std::string::npos) {
                return node;
            }
            return nlohmann::json();
        }
        else {
            return node; // numbers, booleans, null are returned as is
        }
    }

    void MetadataView::UpdateDisplayMetadata() {
        if (metadata.is_null() || metadata.empty()) {
            displayMetadata = metadata;
            return;
        }

        nlohmann::json base = FlattenEntityMetadata(metadata);

        if (filterText.empty()) {
            displayMetadata = base;
            return;
        }

        std::string filterLower = filterText;
        std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);

        displayMetadata = FilterJSONNode(base, filterLower);
    }

    bool MetadataView::LoadFromFile(const std::string& filePath) {
        if (filePath.empty() || !std::filesystem::exists(filePath)) {
            std::cerr << "[MetadataView] File does not exist: " << filePath << std::endl;
            return false;
        }

        nlohmann::json loaded;
        std::string ext = std::filesystem::path(filePath).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".webp") {
            loaded = Utils::ImageUtils::ReadMetadataFromImage(filePath);
            if (loaded.is_null() || loaded.empty()) {
                if (ext == ".png") {
                    auto allChunks = ReadAllTextChunksFromPNG(filePath);
                    for (const auto& chunk : allChunks) {
                        const std::string& key = chunk.first;
                        const std::string& value = chunk.second;
                        try {
                            nlohmann::json parsed = nlohmann::json::parse(value);
                            if (!parsed.is_null() && !parsed.empty()) {
                                loaded = parsed;
                                break;
                            }
                        }
                        catch (...) {
                            if (value.find(':') != std::string::npos) {
                                nlohmann::json parsed = ParseRawMetadata(value);
                                if (!parsed.empty()) {
                                    loaded = parsed;
                                    break;
                                }
                            }
                        }
                    }
                    if (loaded.is_null() || loaded.empty()) {
                        for (const auto& chunk : allChunks) {
                            if (!chunk.second.empty()) {
                                loaded["chunk_" + chunk.first] = chunk.second;
                            }
                        }
                    }
                }
            }
        }
        else if (ext == ".mp4" || ext == ".webm" || ext == ".mkv" || ext == ".avi" || ext == ".mov") {
            loaded = Utils::VideoMetadataUtils::ReadMetadataFromVideo(filePath);
        }
        else if (ext == ".json") {
            std::ifstream file(filePath);
            if (file.is_open()) {
                try {
                    file >> loaded;
                }
                catch (const std::exception& e) {
                    std::cerr << "[MetadataView] Failed to parse JSON: " << e.what() << std::endl;
                    return false;
                }
            }
            else {
                std::cerr << "[MetadataView] Could not open JSON file: " << filePath << std::endl;
                return false;
            }
        }
        else {
            std::string jsonPath = filePath + ".json";
            if (std::filesystem::exists(jsonPath)) {
                loaded = Utils::MetadataUtils::LoadMetadataFromJson(jsonPath);
            }
            else {
                std::ifstream file(filePath);
                if (file.is_open()) {
                    try {
                        file >> loaded;
                    }
                    catch (...) {
                        std::cerr << "[MetadataView] Unsupported file format: " << filePath << std::endl;
                        return false;
                    }
                }
                else {
                    std::cerr << "[MetadataView] Could not open file: " << filePath << std::endl;
                    return false;
                }
            }
        }

        if (loaded.is_null() || loaded.empty()) {
            std::cerr << "[MetadataView] No metadata found in file: " << filePath << std::endl;
            return false;
        }

        metadata = loaded;
        sourcePath = filePath;
        sourceDisplayName = std::filesystem::path(filePath).filename().string();
        filterText.clear();
        std::memset(filterBuffer, 0, sizeof(filterBuffer));
        UpdateDisplayMetadata();
        std::cout << "[MetadataView] Loaded metadata from: " << filePath << std::endl;
        return true;
    }

    bool MetadataView::LoadFromEntity(ECS::EntityID entityID) {
        if (!m_entityManager.IsEntityValid(entityID)) {
            std::cerr << "[MetadataView] Invalid entity ID: " << entityID << std::endl;
            return false;
        }

        nlohmann::json entityData = m_entityManager.SerializeEntity(entityID);
        if (entityData.is_null() || entityData.empty()) {
            std::cerr << "[MetadataView] Entity serialization returned empty." << std::endl;
            return false;
        }

        metadata = entityData;
        sourcePath = "";
        sourceDisplayName = "Entity " + std::to_string(entityID);
        filterText.clear();
        std::memset(filterBuffer, 0, sizeof(filterBuffer));
        UpdateDisplayMetadata();
        std::cout << "[MetadataView] Loaded metadata from entity: " << entityID << std::endl;
        return true;
    }

    bool MetadataView::LoadFromJSON(const nlohmann::json& jsonData, const std::string& sourceName) {
        if (jsonData.is_null() || jsonData.empty()) {
            std::cerr << "[MetadataView] Provided JSON is empty." << std::endl;
            return false;
        }
        metadata = jsonData;
        sourcePath = "";
        sourceDisplayName = sourceName.empty() ? "Custom JSON" : sourceName;
        filterText.clear();
        std::memset(filterBuffer, 0, sizeof(filterBuffer));
        UpdateDisplayMetadata();
        std::cout << "[MetadataView] Loaded custom JSON metadata." << std::endl;
        return true;
    }

    void MetadataView::ClearMetadata() {
        metadata = nlohmann::json();
        displayMetadata = nlohmann::json();
        sourcePath.clear();
        sourceDisplayName.clear();
        filterText.clear();
        std::memset(filterBuffer, 0, sizeof(filterBuffer));
        std::cout << "[MetadataView] Metadata cleared." << std::endl;
    }

    bool MetadataView::PasteFromClipboard() {
        const char* clipboardText = ImGui::GetClipboardText();
        if (!clipboardText) return false;
        std::string text = clipboardText;
        if (text.empty()) return false;

        try {
            nlohmann::json parsed = nlohmann::json::parse(text);
            if (!parsed.is_null() && !parsed.empty()) {
                LoadFromJSON(parsed, "Clipboard");
                return true;
            }
        }
        catch (...) {
            std::cerr << "[MetadataView] Clipboard content is not valid JSON." << std::endl;
        }
        return false;
    }

    void MetadataView::HandleDropTarget() {
        std::vector<std::string> files;
        if (GUI::DragDrop::AcceptFileDrop(files)) {
            if (!files.empty()) {
                LoadFromFile(files[0]);
                return;
            }
        }

        ECS::EntityID droppedEntity;
        if (GUI::DragDrop::AcceptEntityDrop(droppedEntity)) {
            if (m_entityManager.IsEntityValid(droppedEntity)) {
                LoadFromEntity(droppedEntity);
                return;
            }
        }
    }

    nlohmann::json MetadataView::Serialize() const {
        nlohmann::json data;
        data["metadata"] = metadata;
        data["sourcePath"] = sourcePath;
        data["sourceDisplayName"] = sourceDisplayName;
        data["mode"] = static_cast<int>(currentMode);
        data["filterText"] = filterText;
        return data;
    }

    void MetadataView::Deserialize(const nlohmann::json& data) {
        if (data.contains("metadata") && data["metadata"].is_object()) {
            metadata = data["metadata"];
            UpdateDisplayMetadata();
        }
        if (data.contains("sourcePath")) {
            sourcePath = data["sourcePath"].get<std::string>();
        }
        if (data.contains("sourceDisplayName")) {
            sourceDisplayName = data["sourceDisplayName"].get<std::string>();
        }
        if (data.contains("mode")) {
            currentMode = static_cast<DisplayMode>(data["mode"].get<int>());
        }
        if (data.contains("filterText")) {
            filterText = data["filterText"].get<std::string>();
            std::strncpy(filterBuffer, filterText.c_str(), sizeof(filterBuffer) - 1);
            filterBuffer[sizeof(filterBuffer) - 1] = '\0';
            UpdateDisplayMetadata();
        }
        std::cout << "[MetadataView] Deserialized metadata view." << std::endl;
    }

}