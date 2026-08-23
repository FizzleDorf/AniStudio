#include "ContextMenuUtils.hpp"
#include "PngMetadataUtils.hpp"
#include "ImageComponent.hpp"
#include "VideoComponent.hpp"
#include "PropertyTypes.hpp"
#include <png.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <windows.h>

namespace Utils {

    ContextMenuUtils::ContextMenuUtils(ECS::EntityManager& entityMgr)
        : entityManager(entityMgr) {
    }

    bool ContextMenuUtils::SetClipboardDIB(unsigned char* data, int width, int height, int channels) {
        if (!data || width <= 0 || height <= 0 || channels < 1 || channels > 4) return false;

        int bpp = 0;
        switch (channels) {
        case 1: bpp = 8; break;
        case 2: bpp = 16; break;
        case 3: bpp = 24; break;
        case 4: bpp = 32; break;
        default: return false;
        }

        int stride = ((width * bpp + 31) / 32) * 4;
        int imageSize = stride * height;

        BITMAPINFOHEADER bmi = { 0 };
        bmi.biSize = sizeof(BITMAPINFOHEADER);
        bmi.biWidth = width;
        bmi.biHeight = -height;
        bmi.biPlanes = 1;
        bmi.biBitCount = bpp;
        bmi.biCompression = BI_RGB;
        bmi.biSizeImage = imageSize;
        bmi.biXPelsPerMeter = 0;
        bmi.biYPelsPerMeter = 0;
        bmi.biClrUsed = 0;
        bmi.biClrImportant = 0;

        HGLOBAL hMem = GlobalAlloc(GHND, sizeof(BITMAPINFOHEADER) + imageSize);
        if (!hMem) return false;

        char* pMem = (char*)GlobalLock(hMem);
        memcpy(pMem, &bmi, sizeof(BITMAPINFOHEADER));
        char* pBits = pMem + sizeof(BITMAPINFOHEADER);

        if (channels == 3) {
            for (int y = 0; y < height; ++y) {
                unsigned char* src = data + (y * width * channels);
                unsigned char* dst = (unsigned char*)pBits + (y * stride);
                for (int x = 0; x < width; ++x) {
                    dst[0] = src[2];
                    dst[1] = src[1];
                    dst[2] = src[0];
                    src += 3;
                    dst += 3;
                }
            }
        }
        else if (channels == 4) {
            for (int y = 0; y < height; ++y) {
                unsigned char* src = data + (y * width * channels);
                unsigned char* dst = (unsigned char*)pBits + (y * stride);
                for (int x = 0; x < width; ++x) {
                    dst[0] = src[2];
                    dst[1] = src[1];
                    dst[2] = src[0];
                    dst[3] = src[3];
                    src += 4;
                    dst += 4;
                }
            }
        }
        else {
            GlobalUnlock(hMem);
            GlobalFree(hMem);
            return false;
        }

        GlobalUnlock(hMem);

        if (!OpenClipboard(nullptr)) {
            GlobalFree(hMem);
            return false;
        }
        EmptyClipboard();

        HANDLE hClip = ::SetClipboardData(CF_DIB, hMem);
        CloseClipboard();

        if (hClip == NULL) {
            GlobalFree(hMem);
            return false;
        }
        return true;
    }

    void ContextMenuUtils::RenderEntityContextMenu(ECS::EntityID entityId) {
        if (!entityManager.IsEntityValid(entityId)) return;

        std::string popupId = "EntityContextMenu##" + std::to_string(entityId);
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup(popupId.c_str());
        }

        if (ImGui::BeginPopup(popupId.c_str())) {
            RenderEntityContextMenuItems(entityId);
            ImGui::EndPopup();
        }
    }

    void ContextMenuUtils::RenderEntityContextMenuItems(ECS::EntityID entityId) {
        if (!entityManager.IsEntityValid(entityId)) return;

        ImGui::Text("Entity: %zu", entityId);
        ImGui::Separator();

        if (entityManager.HasComponent<ECS::ImageComponent>(entityId)) {
            const auto& imageComp = entityManager.GetComponent<ECS::ImageComponent>(entityId);
            if (!imageComp.filePath.empty() && std::filesystem::exists(imageComp.filePath)) {
                if (ImGui::MenuItem("Copy File Path")) {
                    CopyFilePath(entityId);
                }
                if (ImGui::MenuItem("Copy Image Data")) {
                    CopyImageData(entityId);
                }
                nlohmann::json metadata = ParseImageMetadata(imageComp.filePath);
                if (!metadata.empty()) {
                    RenderMetadataComponentMenu(metadata);
                    RenderMetadataValueMenu(metadata);
                    ImGui::Separator();
                }
            }
        }
        else if (entityManager.HasComponent<ECS::VideoComponent>(entityId)) {
            const auto& videoComp = entityManager.GetComponent<ECS::VideoComponent>(entityId);
            if (!videoComp.filePath.empty() && std::filesystem::exists(videoComp.filePath)) {
                if (ImGui::MenuItem("Copy File Path")) {
                    CopyFilePath(entityId);
                }
                if (ImGui::MenuItem("Copy Current Frame")) {
                    CopyVideoFrame(entityId);
                }
                ImGui::Text("Video file: %s", videoComp.fileName.c_str());
                ImGui::Separator();
            }
        }

        RenderEntityComponentMenu(entityId);
        ImGui::Separator();
        RenderPasteMenu(entityId);
    }

    void ContextMenuUtils::CopyFilePath(ECS::EntityID entityId) {
        std::string filePath;
        if (entityManager.HasComponent<ECS::ImageComponent>(entityId)) {
            filePath = entityManager.GetComponent<ECS::ImageComponent>(entityId).filePath;
        }
        else if (entityManager.HasComponent<ECS::VideoComponent>(entityId)) {
            filePath = entityManager.GetComponent<ECS::VideoComponent>(entityId).filePath;
        }
        if (filePath.empty()) return;
        ImGui::SetClipboardText(filePath.c_str());
    }

    void ContextMenuUtils::CopyImageData(ECS::EntityID entityId) {
        if (!entityManager.HasComponent<ECS::ImageComponent>(entityId)) return;
        const auto& imageComp = entityManager.GetComponent<ECS::ImageComponent>(entityId);
        if (!imageComp.imageData || imageComp.width <= 0 || imageComp.height <= 0 || imageComp.channels < 1) return;
        SetClipboardDIB(imageComp.imageData, imageComp.width, imageComp.height, imageComp.channels);
    }

    void ContextMenuUtils::CopyVideoFrame(ECS::EntityID entityId) {
        if (!entityManager.HasComponent<ECS::VideoComponent>(entityId)) return;
        const auto& videoComp = entityManager.GetComponent<ECS::VideoComponent>(entityId);
        if (videoComp.frameDataRGBA.empty() || videoComp.width <= 0 || videoComp.height <= 0) return;
        SetClipboardDIB(const_cast<unsigned char*>(videoComp.frameDataRGBA.data()), videoComp.width, videoComp.height, 4);
    }

    void ContextMenuUtils::RenderImageContextMenu(ECS::EntityID entityId) {
        RenderEntityContextMenu(entityId);
    }

    void ContextMenuUtils::RenderImageContextMenuWithPath(const std::string& imagePath) {
        std::string popupId = "ImagePathContextMenu##" + std::filesystem::path(imagePath).filename().string();
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup(popupId.c_str());
        }

        if (ImGui::BeginPopup(popupId.c_str())) {
            ImGui::Text("Image: %s", std::filesystem::path(imagePath).filename().string().c_str());
            ImGui::Separator();

            if (std::filesystem::exists(imagePath)) {
                if (ImGui::MenuItem("Copy Image Data")) {
                    int w, h, c;
                    unsigned char* data = stbi_load(imagePath.c_str(), &w, &h, &c, 0);
                    if (data) {
                        SetClipboardDIB(data, w, h, c);
                        stbi_image_free(data);
                    }
                }
                nlohmann::json metadata = ParseImageMetadata(imagePath);
                if (!metadata.empty()) {
                    RenderMetadataComponentMenu(metadata);
                    RenderMetadataValueMenu(metadata);
                }
                else {
                    ImGui::TextDisabled("No metadata found");
                }
            }
            else {
                ImGui::TextDisabled("File not found");
            }
            ImGui::EndPopup();
        }
    }

    void ContextMenuUtils::RenderVideoContextMenu(ECS::EntityID entityId) {
        RenderEntityContextMenu(entityId);
    }

    void ContextMenuUtils::RenderVideoContextMenuWithPath(const std::string& videoPath) {
        std::string popupId = "VideoPathContextMenu##" + std::filesystem::path(videoPath).filename().string();
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup(popupId.c_str());
        }

        if (ImGui::BeginPopup(popupId.c_str())) {
            ImGui::Text("Video: %s", std::filesystem::path(videoPath).filename().string().c_str());
            ImGui::Separator();
            ImGui::TextDisabled("Video metadata not yet supported");
            ImGui::EndPopup();
        }
    }

    void ContextMenuUtils::RenderComponentContextMenu(ECS::EntityID entityId, ECS::ComponentTypeID componentId) {
        std::string componentName = entityManager.GetComponentNameById(componentId);
        std::string popupId = "ComponentContextMenu##" + std::to_string(entityId) + "_" + std::to_string(componentId);

        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup(popupId.c_str());
        }

        if (ImGui::BeginPopup(popupId.c_str())) {
            ImGui::Text("Component: %s", componentName.c_str());
            ImGui::Text("Entity: %zu", entityId);
            ImGui::Separator();

            if (ImGui::MenuItem("Copy Component")) {
                CopyComponent(entityId, componentId);
            }

            if (ImGui::MenuItem("Copy Entity")) {
                CopyEntity(entityId);
            }

            ImGui::Separator();
            RenderValueCopyMenu(entityId, componentId, componentName);
            ImGui::Separator();
            RenderPasteMenu(entityId);

            ImGui::EndPopup();
        }
    }

    void ContextMenuUtils::RenderComponentContextMenu(ECS::EntityID entityId, const std::string& componentName) {
        ECS::ComponentTypeID componentId = entityManager.GetComponentTypeIdByName(componentName);
        if (componentId != ECS::MAX_COMPONENT_COUNT) {
            RenderComponentContextMenu(entityId, componentId);
        }
    }

    void ContextMenuUtils::CopyEntity(ECS::EntityID entityId) {
        if (!entityManager.IsEntityValid(entityId)) return;

        nlohmann::json entityData = entityManager.SerializeEntity(entityId);
        nlohmann::json clipboardData;
        clipboardData["dataType"] = "entity";
        clipboardData["data"] = entityData;
        clipboardData["source"] = "entity";
        SetClipboardData(clipboardData);
    }

    void ContextMenuUtils::CopyComponent(ECS::EntityID entityId, ECS::ComponentTypeID componentId) {
        if (!entityManager.IsEntityValid(entityId) || !entityManager.HasComponentById(entityId, componentId)) return;

        auto* component = entityManager.GetComponentByIdConst(entityId, componentId);
        if (!component) return;

        std::string componentName = entityManager.GetComponentNameById(componentId);
        nlohmann::json componentData = component->Serialize();

        nlohmann::json clipboardData;
        clipboardData["dataType"] = "component";
        clipboardData["componentName"] = componentName;
        clipboardData["componentData"] = componentData;
        clipboardData["source"] = "entity";
        SetClipboardData(clipboardData);
    }

    void ContextMenuUtils::CopyComponent(ECS::EntityID entityId, const std::string& componentName) {
        ECS::ComponentTypeID componentId = entityManager.GetComponentTypeIdByName(componentName);
        if (componentId != ECS::MAX_COMPONENT_COUNT) {
            CopyComponent(entityId, componentId);
        }
    }

    bool ContextMenuUtils::PasteEntity(ECS::EntityID targetEntityId) {
        if (!HasClipboardEntity() || !entityManager.IsEntityValid(targetEntityId)) return false;

        nlohmann::json clipboardData = GetClipboardData();
        if (!clipboardData.contains("data")) return false;

        entityManager.DeserializeEntity(clipboardData["data"], targetEntityId);
        return true;
    }

    bool ContextMenuUtils::PasteComponent(ECS::EntityID targetEntityId, const std::string& componentName) {
        if (!HasClipboardEntity() || !entityManager.IsEntityValid(targetEntityId)) return false;

        nlohmann::json componentData = GetClipboardComponent(componentName);
        if (componentData.empty()) return false;

        if (!entityManager.IsComponentNameRegistered(componentName)) return false;

        ECS::ComponentTypeID componentId = entityManager.GetComponentTypeIdByName(componentName);
        bool hadComponent = entityManager.HasComponentById(targetEntityId, componentId);

        if (!hadComponent) {
            nlohmann::json tempEntityData;
            tempEntityData["components"] = nlohmann::json::array();
            nlohmann::json compJson;
            compJson[componentName] = nlohmann::json::object();
            tempEntityData["components"].push_back(compJson);
            entityManager.DeserializeEntity(tempEntityData, targetEntityId);
        }

        auto* component = entityManager.GetComponentById(targetEntityId, componentId);
        if (component) {
            nlohmann::json a;
            a[componentName] = componentData;
            component->Deserialize(a);
            return true;
        }

        return false;
    }

    bool ContextMenuUtils::PasteValue(ECS::EntityID targetEntityId, const std::string& componentName, const std::string& propertyName) {
        if (!HasClipboardEntity() || !entityManager.IsEntityValid(targetEntityId)) return false;

        std::string clipboardValue = GetClipboardValue(componentName, propertyName);
        if (clipboardValue.empty()) return false;

        ECS::ComponentTypeID componentId = entityManager.GetComponentTypeIdByName(componentName);
        if (componentId == ECS::MAX_COMPONENT_COUNT) return false;

        auto* component = entityManager.GetComponentById(targetEntityId, componentId);
        if (!component) return false;

        nlohmann::json componentData = component->Serialize();

        if (componentData.contains(propertyName)) {
            const auto& currentValue = componentData[propertyName];

            if (currentValue.is_string()) {
                componentData[propertyName] = clipboardValue;
            }
            else if (currentValue.is_number_integer()) {
                try { componentData[propertyName] = std::stoi(clipboardValue); }
                catch (...) { componentData[propertyName] = clipboardValue; }
            }
            else if (currentValue.is_number_float()) {
                try { componentData[propertyName] = std::stof(clipboardValue); }
                catch (...) { componentData[propertyName] = clipboardValue; }
            }
            else if (currentValue.is_boolean()) {
                std::string lowerValue = clipboardValue;
                std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);
                if (lowerValue == "true" || lowerValue == "1" || lowerValue == "yes") {
                    componentData[propertyName] = true;
                }
                else if (lowerValue == "false" || lowerValue == "0" || lowerValue == "no") {
                    componentData[propertyName] = false;
                }
                else {
                    componentData[propertyName] = clipboardValue;
                }
            }
            else {
                componentData[propertyName] = clipboardValue;
            }
        }
        else {
            componentData[propertyName] = clipboardValue;
        }

        nlohmann::json properJson;
        properJson[componentName] = componentData;
        component->Deserialize(properJson);
        return true;
    }

    bool ContextMenuUtils::HasClipboardEntity() const {
        nlohmann::json clipboardData = GetClipboardData();
        return clipboardData.contains("dataType") && clipboardData["dataType"] == "entity";
    }

    std::string ContextMenuUtils::GetClipboardPreview() const {
        nlohmann::json clipboardData = GetClipboardData();
        if (!clipboardData.contains("dataType")) return "No data";

        std::string dataType = clipboardData["dataType"];
        if (dataType == "entity") {
            std::string source = clipboardData.value("source", "unknown");
            int componentCount = 0;
            if (clipboardData.contains("data") && clipboardData["data"].contains("components")) {
                const auto& components = clipboardData["data"]["components"];
                if (components.is_array()) componentCount = static_cast<int>(components.size());
            }
            return "Entity (" + std::to_string(componentCount) + " components)";
        }
        else if (dataType == "component") {
            if (clipboardData.contains("componentName")) {
                return "Component: " + clipboardData["componentName"].get<std::string>();
            }
            return "Component";
        }

        return "Unknown";
    }

    void ContextMenuUtils::ClearClipboard() {
        ImGui::SetClipboardText("");
    }

    std::vector<std::string> ContextMenuUtils::GetClipboardComponents() const {
        std::vector<std::string> components;

        if (!HasClipboardEntity()) return components;

        nlohmann::json clipboardData = GetClipboardData();
        if (!clipboardData.contains("data") || !clipboardData["data"].contains("components")) return components;

        const auto& comps = clipboardData["data"]["components"];

        if (comps.is_array()) {
            for (const auto& component : comps) {
                if (component.is_object()) {
                    for (auto it = component.begin(); it != component.end(); ++it) {
                        std::string key = it.key();
                        if (key == "Base_Component" || key == "compName") continue;
                        components.push_back(key);
                    }
                }
            }
        }

        std::sort(components.begin(), components.end());
        components.erase(std::unique(components.begin(), components.end()), components.end());

        return components;
    }

    std::vector<std::string> ContextMenuUtils::GetCommonProperties(ECS::EntityID targetEntityId, const std::string& componentName) const {
        std::vector<std::string> commonProperties;

        if (!HasClipboardEntity() || !entityManager.IsEntityValid(targetEntityId)) return commonProperties;

        nlohmann::json clipboardComponent = GetClipboardComponent(componentName);
        if (clipboardComponent.empty()) return commonProperties;

        ECS::ComponentTypeID componentId = entityManager.GetComponentTypeIdByName(componentName);
        if (componentId == ECS::MAX_COMPONENT_COUNT) return commonProperties;

        auto* component = entityManager.GetComponentByIdConst(targetEntityId, componentId);
        if (!component) return commonProperties;

        nlohmann::json targetComponentData = component->Serialize();

        for (auto it = clipboardComponent.begin(); it != clipboardComponent.end(); ++it) {
            std::string propName = it.key();
            if (propName == "compName" || propName == "entityID" || propName == "compCategory" || propName == "schema") continue;
            if (targetComponentData.contains(propName)) commonProperties.push_back(propName);
        }

        return commonProperties;
    }

    nlohmann::json ContextMenuUtils::GetClipboardData() const {
        const char* clipboardText = ImGui::GetClipboardText();
        if (!clipboardText) return nlohmann::json();

        try {
            return nlohmann::json::parse(std::string(clipboardText));
        }
        catch (...) {
            return nlohmann::json();
        }
    }

    void ContextMenuUtils::RenderPasteMenu(ECS::EntityID entityId) {
        if (!HasClipboardEntity()) {
            ImGui::TextDisabled("Nothing to paste");
            return;
        }

        if (ImGui::BeginMenu("Paste")) {
            if (ImGui::MenuItem("Paste Entity (Replace All)")) {
                PasteEntity(entityId);
            }

            std::vector<std::string> clipboardComponents = GetClipboardComponents();
            if (!clipboardComponents.empty()) {
                if (ImGui::BeginMenu("Paste Component")) {
                    for (const std::string& componentName : clipboardComponents) {
                        if (ImGui::MenuItem(componentName.c_str())) {
                            PasteComponent(entityId, componentName);
                        }
                    }
                    ImGui::EndMenu();
                }
            }

            if (ImGui::BeginMenu("Paste Value")) {
                nlohmann::json clipboardData = GetClipboardData();

                if (clipboardData.contains("data") && clipboardData["data"].contains("components")) {
                    const auto& components = clipboardData["data"]["components"];

                    if (components.is_array()) {
                        std::set<std::string> shownComponents;

                        for (const auto& component : components) {
                            if (!component.is_object()) continue;

                            for (auto it = component.begin(); it != component.end(); ++it) {
                                std::string componentName = it.key();

                                if (componentName == "Base_Component" || componentName == "compName") continue;

                                if (shownComponents.find(componentName) != shownComponents.end()) continue;
                                shownComponents.insert(componentName);

                                ECS::ComponentTypeID compId = entityManager.GetComponentTypeIdByName(componentName);
                                bool hasComponent = (compId != ECS::MAX_COMPONENT_COUNT) &&
                                    entityManager.HasComponentById(entityId, compId);

                                if (hasComponent) {
                                    const auto& compData = it.value();

                                    if (compData.is_object()) {
                                        if (ImGui::BeginMenu(componentName.c_str())) {
                                            for (auto propIt = compData.begin(); propIt != compData.end(); ++propIt) {
                                                std::string propName = propIt.key();

                                                if (propName == "compName" || propName == "entityID" ||
                                                    propName == "compCategory" || propName == "schema") continue;

                                                std::string valueStr;
                                                if (propIt.value().is_string()) valueStr = propIt.value().get<std::string>();
                                                else if (propIt.value().is_number()) valueStr = std::to_string(propIt.value().get<double>());
                                                else if (propIt.value().is_boolean()) valueStr = propIt.value().get<bool>() ? "true" : "false";
                                                else valueStr = propIt.value().dump();

                                                std::string label = propName;
                                                if (!valueStr.empty()) {
                                                    if (valueStr.length() > 20) valueStr = valueStr.substr(0, 17) + "...";
                                                    label += " (\"" + valueStr + "\")";
                                                }

                                                if (ImGui::MenuItem(label.c_str())) {
                                                    PasteValue(entityId, componentName, propName);
                                                }
                                            }
                                            ImGui::EndMenu();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
    }

    nlohmann::json ContextMenuUtils::ParseImageMetadata(const std::string& imagePath) {
        nlohmann::json metadata;

        if (!std::filesystem::exists(imagePath)) return metadata;

        std::string extension = std::filesystem::path(imagePath).extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        if (extension == ".png") {
            FILE* fp = fopen(imagePath.c_str(), "rb");
            if (!fp) return metadata;

            png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!png) { fclose(fp); return metadata; }

            png_infop info = png_create_info_struct(png);
            if (!info) { png_destroy_read_struct(&png, nullptr, nullptr); fclose(fp); return metadata; }

            if (setjmp(png_jmpbuf(png))) {
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return metadata;
            }

            png_init_io(png, fp);
            png_read_info(png, info);

            png_textp text_ptr;
            int num_text;
            if (png_get_text(png, info, &text_ptr, &num_text) > 0) {
                for (int i = 0; i < num_text; i++) {
                    if (strcmp(text_ptr[i].key, "parameters") == 0) {
                        try { metadata = nlohmann::json::parse(text_ptr[i].text); break; }
                        catch (...) {}
                    }
                }
            }

            png_destroy_read_struct(&png, &info, nullptr);
            fclose(fp);
        }

        return metadata;
    }

    std::vector<std::string> ContextMenuUtils::ExtractComponentsFromMetadata(const nlohmann::json& metadata) {
        std::vector<std::string> components;

        if (metadata.contains("components")) {
            if (metadata["components"].is_array()) {
                for (const auto& component : metadata["components"]) {
                    if (component.is_object()) {
                        for (auto it = component.begin(); it != component.end(); ++it) {
                            if (it.key() != "compName") components.push_back(it.key());
                        }
                    }
                }
            }
            else if (metadata["components"].is_object()) {
                for (auto it = metadata["components"].begin(); it != metadata["components"].end(); ++it) {
                    if (it.key() != "Base_Component") components.push_back(it.key());
                }
            }
        }

        return components;
    }

    void ContextMenuUtils::RenderMetadataComponentMenu(const nlohmann::json& metadata) {
        if (ImGui::BeginMenu("Copy from Metadata")) {
            if (ImGui::MenuItem("Copy Entire Entity")) {
                CopyEntityFromMetadata(metadata);
            }

            ImGui::Separator();

            std::vector<std::string> components = ExtractComponentsFromMetadata(metadata);

            if (components.empty()) {
                ImGui::TextDisabled("No components found");
            }
            else {
                for (const std::string& componentName : components) {
                    if (ImGui::MenuItem(componentName.c_str())) {
                        CopyComponentFromMetadata(metadata, componentName);
                    }
                }
            }

            ImGui::EndMenu();
        }
    }

    void ContextMenuUtils::RenderEntityComponentMenu(ECS::EntityID entityId) {
        if (ImGui::BeginMenu("Copy from Entity")) {
            if (ImGui::MenuItem("Copy Entity")) {
                CopyEntity(entityId);
            }

            ImGui::Separator();

            auto componentIds = entityManager.GetEntityComponents(entityId);
            if (componentIds.empty()) {
                ImGui::TextDisabled("No components");
            }
            else {
                for (ECS::ComponentTypeID compId : componentIds) {
                    std::string componentName = entityManager.GetComponentNameById(compId);
                    if (ImGui::MenuItem(componentName.c_str())) {
                        CopyComponent(entityId, compId);
                    }
                }
            }

            ImGui::EndMenu();
        }
    }

    void ContextMenuUtils::RenderValueCopyMenu(ECS::EntityID entityId, ECS::ComponentTypeID componentId, const std::string& componentName) {
        if (ImGui::BeginMenu("Copy Values")) {
            auto* component = entityManager.GetComponentByIdConst(entityId, componentId);
            if (!component) {
                ImGui::TextDisabled("No component");
            }
            else {
                nlohmann::json componentData = component->Serialize();

                for (auto it = componentData.begin(); it != componentData.end(); ++it) {
                    std::string propName = it.key();

                    if (propName == "compName" || propName == "entityID" || propName == "compCategory" || propName == "schema") continue;

                    std::string label = propName + ": ";
                    if (it.value().is_string()) {
                        std::string value = it.value().get<std::string>();
                        if (value.length() > 30) value = value.substr(0, 27) + "...";
                        label += value;
                    }
                    else if (it.value().is_number()) {
                        std::stringstream ss; ss << it.value(); label += ss.str();
                    }

                    if (ImGui::MenuItem(label.c_str())) {
                        nlohmann::json entityData;
                        entityData["dataType"] = "entity";
                        entityData["data"] = nlohmann::json::object();
                        entityData["data"]["components"] = nlohmann::json::array();

                        nlohmann::json componentJson;
                        componentJson[componentName] = nlohmann::json::object();
                        componentJson[componentName][propName] = it.value();
                        entityData["data"]["components"].push_back(componentJson);

                        SetClipboardData(entityData);
                    }
                }
            }

            ImGui::EndMenu();
        }
    }

    void ContextMenuUtils::RenderMetadataValueMenu(const nlohmann::json& metadata) {
        if (ImGui::BeginMenu("Copy Values from Metadata")) {
            std::vector<std::string> components = ExtractComponentsFromMetadata(metadata);

            if (components.empty()) {
                ImGui::TextDisabled("No components found");
            }
            else {
                for (const std::string& componentName : components) {
                    if (ImGui::BeginMenu(componentName.c_str())) {

                        if (metadata.contains("components")) {
                            nlohmann::json componentData;

                            if (metadata["components"].is_array()) {
                                for (const auto& component : metadata["components"]) {
                                    if (component.contains(componentName)) {
                                        componentData = component[componentName];
                                        break;
                                    }
                                }
                            }
                            else if (metadata["components"].is_object()) {
                                if (metadata["components"].contains(componentName)) {
                                    componentData = metadata["components"][componentName];
                                }
                            }

                            if (!componentData.empty() && componentData.is_object()) {
                                for (auto it = componentData.begin(); it != componentData.end(); ++it) {
                                    std::string propName = it.key();

                                    if (propName == "compName" || propName == "entityID" || propName == "compCategory" || propName == "schema") continue;

                                    std::string label = propName + ": ";
                                    if (it.value().is_string()) {
                                        std::string value = it.value().get<std::string>();
                                        if (value.length() > 30) value = value.substr(0, 27) + "...";
                                        label += value;
                                    }

                                    if (ImGui::MenuItem(label.c_str())) {
                                        nlohmann::json entityData;
                                        entityData["dataType"] = "entity";
                                        entityData["data"] = nlohmann::json::object();
                                        entityData["data"]["components"] = nlohmann::json::array();

                                        nlohmann::json componentJson;
                                        componentJson[componentName] = nlohmann::json::object();
                                        componentJson[componentName][propName] = it.value();
                                        entityData["data"]["components"].push_back(componentJson);

                                        SetClipboardData(entityData);
                                    }
                                }
                            }
                        }

                        ImGui::EndMenu();
                    }
                }
            }

            ImGui::EndMenu();
        }
    }

    void ContextMenuUtils::CopyComponentFromMetadata(const nlohmann::json& metadata, const std::string& componentName) {
        nlohmann::json componentData;

        if (metadata.contains("components")) {
            if (metadata["components"].is_array()) {
                for (const auto& component : metadata["components"]) {
                    if (component.contains(componentName)) {
                        componentData = component[componentName];
                        break;
                    }
                }
            }
            else if (metadata["components"].is_object()) {
                if (metadata["components"].contains(componentName)) {
                    componentData = metadata["components"][componentName];
                }
            }
        }

        if (!componentData.empty()) {
            nlohmann::json clipboardData;
            clipboardData["dataType"] = "component";
            clipboardData["componentName"] = componentName;
            clipboardData["componentData"] = componentData;
            clipboardData["source"] = "metadata";
            SetClipboardData(clipboardData);
        }
    }

    void ContextMenuUtils::CopyEntityFromMetadata(const nlohmann::json& metadata) {
        nlohmann::json clipboardData;
        clipboardData["dataType"] = "entity";
        clipboardData["data"] = metadata;
        clipboardData["source"] = "metadata";
        SetClipboardData(clipboardData);
    }

    void ContextMenuUtils::SetClipboardData(const nlohmann::json& data) {
        std::string clipboardString = data.dump();
        ImGui::SetClipboardText(clipboardString.c_str());
    }

    std::string ContextMenuUtils::GetClipboardValue(const std::string& componentName, const std::string& propertyName) const {
        nlohmann::json componentData = GetClipboardComponent(componentName);
        if (componentData.empty() || !componentData.contains(propertyName)) return "";

        const auto& value = componentData[propertyName];
        if (value.is_string()) return value.get<std::string>();
        else if (value.is_number()) return value.dump();
        else if (value.is_boolean()) return value.get<bool>() ? "true" : "false";

        return "";
    }

    nlohmann::json ContextMenuUtils::GetClipboardComponent(const std::string& componentName) const {
        nlohmann::json result;

        if (!HasClipboardEntity()) return result;

        nlohmann::json clipboardData = GetClipboardData();
        if (!clipboardData.contains("data") || !clipboardData["data"].contains("components")) return result;

        const auto& components = clipboardData["data"]["components"];

        if (components.is_array()) {
            for (const auto& component : components) {
                if (component.contains(componentName)) {
                    result = component[componentName];
                    break;
                }
            }
        }

        return result;
    }

}