// UpscaleView.cpp
#include "UpscaleView.hpp"
#include "Events.hpp"
#include "DiffusionOptions.hpp"
#include "UISchema.hpp"
#include "ContextMenuUtils.hpp"
#include "SDcppSystem.hpp"
#include "FileDialogUtil.hpp"
#include "FileDialogFilters.hpp"
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace ECS;
using namespace ANI;
using namespace Utils;

namespace GUI {

    UpscaleView::UpscaleView(EntityManager& entityMgr, ImGuiContext* mainContext) : BaseView(entityMgr),
        isFilenameChanged(false) {
        viewName = "UpscaleView";
        windowOpen = true;
        contextMenuUtils = std::make_unique<Utils::ContextMenuUtils>(entityMgr);
    }

    UpscaleView::~UpscaleView() {
        if (upscaleEntity != 0) {
            mgr.DestroyEntity(upscaleEntity);
        }
    }

    void UpscaleView::Init() {
        GUI::DiffusionCallbackUtils::InitializeCallbacks();
        ResetEntity();
    }

    void UpscaleView::ResetEntity() {
        if (upscaleEntity != 0) {
            mgr.DestroyEntity(upscaleEntity);
            upscaleEntity = 0;
        }

        upscaleEntity = mgr.AddNewEntity();

        mgr.AddComponent<InputImageComponent>(upscaleEntity);
        mgr.AddComponent<OutputImageComponent>(upscaleEntity);
        mgr.AddComponent<EsrganComponent>(upscaleEntity);

        auto& outputComp = mgr.GetComponent<OutputImageComponent>(upscaleEntity);

        std::string outputFolder = Utils::g_FilePathSystem ? Utils::g_FilePathSystem->GetPath("OutputFolder") : "";
        std::string defaultProject = Utils::g_FilePathSystem ? Utils::g_FilePathSystem->GetPath("DefaultProject") : "";

        outputComp.fileName = "upscaled_output";

        if (!outputFolder.empty() && outputFolder[0] != '\0') {
            outputComp.filePath = outputFolder;
        }
        else if (!defaultProject.empty() && defaultProject[0] != '\0') {
            outputComp.filePath = defaultProject;
        }

        auto& esrganComp = mgr.GetComponent<EsrganComponent>(upscaleEntity);
        esrganComp.upscaleFactor = 2;
        esrganComp.preserveAspectRatio = true;
    }

    bool UpscaleView::IsEntitySafeToUse(ECS::EntityID entity) const {
        return mgr.IsEntityValid(entity);
    }

    std::vector<std::string> UpscaleView::GetCategoryRenderOrder() const {
        return {
            "Image",
            "Models",
            "Processing",
            "Output"
        };
    }

    void UpscaleView::RenderEntityComponents(const EntityID entity) {
        if (entity == 0 || !IsEntitySafeToUse(entity)) return;

        auto componentIds = mgr.GetEntityComponents(entity);
        std::map<std::string, std::vector<std::pair<ComponentTypeID, std::string>>> categorizedComponents;

        for (ComponentTypeID compId : componentIds) {
            std::string componentName = mgr.GetComponentNameById(compId);
            auto* component = mgr.GetComponentByIdConst(entity, compId);
            if (!component) continue;

            std::string category = std::string(component->compCategory).empty() ? "Uncategorized" : component->compCategory;
            categorizedComponents[category].emplace_back(compId, componentName);
        }

        std::vector<std::string> renderOrder = GetCategoryRenderOrder();

        for (const auto& category : renderOrder) {
            auto it = categorizedComponents.find(category);
            if (it != categorizedComponents.end()) {
                if (ImGui::CollapsingHeader(category.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    for (const auto& [compId, componentName] : it->second) {
                        RenderComponent(entity, compId, componentName);
                    }
                }
                categorizedComponents.erase(it);
            }
        }

        for (const auto& [category, components] : categorizedComponents) {
            if (ImGui::CollapsingHeader(category.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                for (const auto& [compId, componentName] : components) {
                    RenderComponent(entity, compId, componentName);
                }
            }
        }
    }

    void UpscaleView::RenderComponent(EntityID entity, ComponentTypeID compId, const std::string& componentName) {
        if (componentVisibility.find(componentName) == componentVisibility.end()) {
            componentVisibility[componentName] = true;
        }

        bool isVisible = componentVisibility[componentName];
        if (ImGui::Checkbox(componentName.c_str(), &isVisible)) {
            componentVisibility[componentName] = isVisible;
        }

        if (!isVisible) return;

        ImGui::Indent();

        ImVec2 contentStart = ImGui::GetCursorScreenPos();

        auto* component = mgr.GetComponentById(entity, compId);
        if (component && !component->schema.empty()) {
            try {
                auto properties = component->GetPropertyMap();

                if (componentName == "InputImage") {
                    UISchema::RenderSchema(component->schema, properties);

                    if (ImGui::Button("Show Output Estimate", ImVec2(-1.0f, 0))) {
                        if (mgr.HasComponent<InputImageComponent>(entity) && mgr.HasComponent<EsrganComponent>(entity)) {
                            auto& inputComp = mgr.GetComponent<InputImageComponent>(entity);
                            auto& esrganComp = mgr.GetComponent<EsrganComponent>(entity);

                            if (inputComp.width > 0 && inputComp.height > 0) {
                                int outWidth = inputComp.width * esrganComp.upscaleFactor;
                                int outHeight = inputComp.height * esrganComp.upscaleFactor;
                                float memoryMB = (outWidth * outHeight * 4) / (1024.0f * 1024.0f);

                                ImGui::Text("Estimated output: %dx%d (%.1f MB)", outWidth, outHeight, memoryMB);
                            }
                        }
                    }
                }
                else if (componentName == "Esrgan") {
                    UISchema::RenderSchema(component->schema, properties);

                    if (mgr.HasComponent<EsrganComponent>(entity)) {
                        auto& esrganComp = mgr.GetComponent<EsrganComponent>(entity);
                        ImGui::Text("Current factor: %dx", esrganComp.upscaleFactor);

                        if (mgr.HasComponent<InputImageComponent>(entity)) {
                            auto& inputComp = mgr.GetComponent<InputImageComponent>(entity);
                            if (inputComp.width > 0 && inputComp.height > 0) {
                                int outWidth = inputComp.width * esrganComp.upscaleFactor;
                                int outHeight = inputComp.height * esrganComp.upscaleFactor;
                                ImGui::Text("Output will be: %dx%d", outWidth, outHeight);
                            }
                        }
                    }
                }
                else {
                    UISchema::RenderSchema(component->schema, properties);
                }
            }
            catch (const std::exception& e) {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error rendering %s: %s",
                    componentName.c_str(), e.what());
            }
        }

        ImVec2 contentEnd = ImGui::GetCursorScreenPos();
        ImVec2 contentSize = ImVec2(ImGui::GetContentRegionAvail().x, contentEnd.y - contentStart.y);

        ImGui::SetCursorScreenPos(contentStart);
        ImGui::InvisibleButton(("##component_context_" + componentName).c_str(), contentSize);

        std::string popupId = "ComponentContextMenu##" + componentName + "_" + std::to_string(entity);
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup(popupId.c_str());
        }

        if (ImGui::BeginPopup(popupId.c_str())) {
            ImGui::Text("Component: %s", componentName.c_str());
            ImGui::Text("Entity: %zu", entity);
            ImGui::Separator();

            if (ImGui::MenuItem("Copy Component")) {
                contextMenuUtils->CopyComponent(entity, compId);
            }

            if (ImGui::MenuItem("Copy Entity")) {
                contextMenuUtils->CopyEntity(entity);
            }

            ImGui::Separator();

            bool hasClipboardEntity = contextMenuUtils->HasClipboardEntity();
            if (hasClipboardEntity) {
                std::vector<std::string> clipboardComponents = contextMenuUtils->GetClipboardComponents();
                if (!clipboardComponents.empty()) {
                    if (ImGui::BeginMenu("Paste Component")) {
                        for (const auto& compName : clipboardComponents) {
                            if (ImGui::MenuItem(compName.c_str())) {
                                contextMenuUtils->PasteComponent(entity, compName);
                            }
                        }
                        ImGui::EndMenu();
                    }
                }

                if (ImGui::MenuItem("Paste Entity")) {
                    nlohmann::json clipboardData = contextMenuUtils->GetClipboardData();
                    if (clipboardData.contains("data")) {
                        mgr.DeserializeEntity(clipboardData["data"], entity);
                    }
                }
            }
            else {
                ImGui::TextDisabled("Nothing to paste");
            }

            ImGui::EndPopup();
        }

        ImGui::SetCursorScreenPos(contentEnd);
        ImGui::Unindent();
    }

    void UpscaleView::RenderMainContextMenu() {
        if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup("UpscaleMainContext");
        }

        if (ImGui::BeginPopup("UpscaleMainContext")) {
            ImGui::Text("Upscale View");
            ImGui::Separator();

            if (ImGui::MenuItem("Copy Entity")) {
                contextMenuUtils->CopyEntity(upscaleEntity);
            }

            ImGui::Separator();

            bool hasClipboardEntity = contextMenuUtils->HasClipboardEntity();
            if (hasClipboardEntity) {
                std::vector<std::string> clipboardComponents = contextMenuUtils->GetClipboardComponents();
                if (!clipboardComponents.empty()) {
                    if (ImGui::BeginMenu("Paste Component")) {
                        for (const auto& compName : clipboardComponents) {
                            if (ImGui::MenuItem(compName.c_str())) {
                                contextMenuUtils->PasteComponent(upscaleEntity, compName);
                            }
                        }
                        ImGui::EndMenu();
                    }
                }

                if (ImGui::MenuItem("Paste Entity")) {
                    nlohmann::json clipboardData = contextMenuUtils->GetClipboardData();
                    if (clipboardData.contains("data")) {
                        mgr.DeserializeEntity(clipboardData["data"], upscaleEntity);
                    }
                }

                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Clipboard: %s",
                    contextMenuUtils->GetClipboardPreview().c_str());
            }
            else {
                ImGui::TextDisabled("Nothing to paste");
            }

            ImGui::EndPopup();
        }
    }

    void UpscaleView::HandleUpscaleEvent() {
        std::cout << "Creating entity for upscaling..." << std::endl;

        EntityID newEntity = mgr.CloneEntity(upscaleEntity);
        if (newEntity == 0) {
            std::cerr << "Failed to create new entity!" << std::endl;
            return;
        }

        if (mgr.HasComponent<OutputImageComponent>(newEntity)) {
            auto& outputComp = mgr.GetComponent<OutputImageComponent>(newEntity);

            std::string defaultProject = Utils::g_FilePathSystem ? Utils::g_FilePathSystem->GetPath("DefaultProject") : "";

            if (outputComp.filePath.empty()) {
                if (!defaultProject.empty() && defaultProject[0] != '\0') {
                    outputComp.filePath = defaultProject;
                }
            }

            if (outputComp.fileName.empty()) {
                outputComp.fileName = "AniStudio_upscaled";
            }

            std::filesystem::create_directories(outputComp.filePath);
        }

        auto taskData = std::make_pair(newEntity, ECS::SDCPPSystem::TaskType::Upscaling);
        ANI::Events::Ref().QueueEventWithData("QueueDiffusionTask", taskData);

        std::cout << "Upscaling request queued for entity: " << newEntity << std::endl;
    }

    void UpscaleView::RenderQueueList() {
        ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Upscale Queue")) {
            const auto& progressData = DiffusionCallbackUtils::GetProgressData();
            int currentStep = progressData.currentStep;
            int totalSteps = progressData.totalSteps;
            float time = progressData.currentTime;
            bool isProcessing = progressData.isProcessing;

            if (isProcessing && totalSteps > 0) {
                float progress = static_cast<float>(currentStep) / totalSteps;
                std::ostringstream ss;
                ss << "Processing: " << currentStep << "/" << totalSteps << " steps ("
                    << std::fixed << std::setprecision(1) << time << "s)";
                ImGui::Text("%s", ss.str().c_str());
                ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 0));
            }
            else {
                ImGui::Text("Waiting...");
                ImGui::ProgressBar(0.0f, ImVec2(-FLT_MIN, 0));
            }

            ImGui::Separator();

            if (ImGui::InputInt("Queue #", &numQueues, 1, 4)) {
                if (numQueues < 1) {
                    numQueues = 1;
                }
            }

            if (ImGui::Button("Queue", ImVec2(-FLT_MIN, 0))) {
                for (int i = 0; i < numQueues; i++) {
                    HandleUpscaleEvent();
                }
            }

            ImGui::Separator();

            if (isPaused) {
                if (ImGui::Button("Resume", ImVec2(-FLT_MIN, 0))) {
                    ANI::Events::Ref().QueueEvent("ResumeDiffusionWorker");
                    isPaused = false;
                }
            }
            else {
                if (ImGui::Button("Pause", ImVec2(-FLT_MIN, 0))) {
                    ANI::Events::Ref().QueueEvent("PauseDiffusionWorker");
                    isPaused = true;
                }
            }

            if (ImGui::Button("Stop", ImVec2(-FLT_MIN, 0))) {
                ANI::Events::Ref().QueueEvent("StopCurrentDiffusionTask");
            }

            if (ImGui::Button("Clear Queue", ImVec2(-FLT_MIN, 0))) {
                ANI::Events::Ref().QueueEvent("ClearDiffusionQueue");
            }

            ImGui::Separator();

            if (ImGui::BeginTable("Queue", 3,
                ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {

                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 42.0f);
                ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 44.0f);
                ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                auto sdSystem = mgr.GetSystem<ECS::SDCPPSystem>();
                if (sdSystem) {
                    auto queueItems = sdSystem->GetQueueSnapshot();
                    for (size_t i = 0; i < queueItems.size(); i++) {
                        const auto& item = queueItems[i];

                        ImGui::TableNextRow();

                        ImGui::TableNextColumn();
                        ImGui::Text("%d", static_cast<int>(item.entityID));

                        ImGui::TableNextColumn();
                        if (item.processing) {
                            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Active");
                        }
                        else {
                            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "Queued");
                        }

                        ImGui::TableNextColumn();
                        if (!item.processing) {
                            if (i > 0) {
                                if (ImGui::ArrowButton(("up##" + std::to_string(i)).c_str(), ImGuiDir_Up)) {
                                    auto moveData = std::make_pair(i, i - 1);
                                    ANI::Events::Ref().QueueEventWithData("MoveInDiffusionQueue", moveData);
                                }
                                ImGui::SameLine();
                            }

                            if (i < queueItems.size() - 1) {
                                if (ImGui::ArrowButton(("down##" + std::to_string(i)).c_str(), ImGuiDir_Down)) {
                                    auto moveData = std::make_pair(i, i + 1);
                                    ANI::Events::Ref().QueueEventWithData("MoveInDiffusionQueue", moveData);
                                }
                                ImGui::SameLine();
                            }

                            if (i > 0) {
                                if (ImGui::Button(("Top##Top" + std::to_string(i)).c_str())) {
                                    size_t targetIndex = queueItems[0].processing ? 1 : 0;
                                    auto moveData = std::make_pair(i, targetIndex);
                                    ANI::Events::Ref().QueueEventWithData("MoveInDiffusionQueue", moveData);
                                }
                                ImGui::SameLine();
                            }

                            if (i < queueItems.size() - 1) {
                                if (ImGui::Button(("Bottom##Bottom" + std::to_string(i)).c_str())) {
                                    auto moveData = std::make_pair(i, queueItems.size() - 1);
                                    ANI::Events::Ref().QueueEventWithData("MoveInDiffusionQueue", moveData);
                                }
                                ImGui::SameLine();
                            }

                            if (ImGui::Button(("X##Remove" + std::to_string(i)).c_str())) {
                                ANI::Events::Ref().QueueEventWithData("RemoveFromDiffusionQueue", i);
                            }
                        }
                    }
                }
                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    void UpscaleView::Render() {
        RenderQueueList();

        ImGui::SetNextWindowSize(ImVec2(400, 800), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen)) {

            if (ImGui::CollapsingHeader("Metadata Controls")) {
                RenderMetadataControls();
            }

            if (contextMenuUtils->HasClipboardEntity()) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Clipboard: %s",
                    contextMenuUtils->GetClipboardPreview().c_str());
                ImGui::Separator();
            }

            RenderEntityComponents(upscaleEntity);

            ImGui::Separator();
            if (ImGui::Button("Run Upscale", ImVec2(-FLT_MIN, 0))) {
                HandleUpscaleEvent();
            }

            ImGui::SameLine();
            if (ImGui::Button("Queue 4", ImVec2(-FLT_MIN, 0))) {
                numQueues = 4;
                for (int i = 0; i < numQueues; i++) {
                    HandleUpscaleEvent();
                }
            }

            RenderMainContextMenu();
        }
        ImGui::End();

        if (!windowOpen) {
            std::unordered_map<std::string, std::any> eventData;
            eventData["workspaceID"] = GetID();
            eventData["viewTypeName"] = viewName;
            ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
        }
    }

    nlohmann::json UpscaleView::Serialize() const {
        if (upscaleEntity == 0) {
            return nlohmann::json();
        }

        nlohmann::json j = mgr.SerializeEntity(upscaleEntity);
        j["componentVisibility"] = componentVisibility;
        return j;
    }

    void UpscaleView::Deserialize(const nlohmann::json& j) {
        if (upscaleEntity == 0) {
            std::cerr << "Error: Invalid upscale entity for deserialization" << std::endl;
            return;
        }

        try {
            mgr.DeserializeEntity(j, upscaleEntity);
            if (j.contains("componentVisibility")) {
                componentVisibility = j["componentVisibility"];
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Exception during deserialization: " << e.what() << std::endl;
        }
    }

    void UpscaleView::RenderMetadataControls() {
        std::string defaultPath = Utils::g_FilePathSystem ? Utils::g_FilePathSystem->GetPath("DefaultProject") : "";

        if (ImGui::Button("Save Settings", ImVec2(-FLT_MIN, 0))) {
            std::string selectedFile;
            if (FileDialog::SaveFile("Save Settings", FileDialog::FilterType::METADATA_FILE, "upscale_settings.json", selectedFile, defaultPath)) {
                SaveMetadataToJson(selectedFile);
            }
        }

        if (ImGui::Button("Load Settings", ImVec2(-FLT_MIN, 0))) {
            std::string selectedFile;
            if (FileDialog::OpenFile("Load Settings", FileDialog::FilterType::METADATA_FILE, selectedFile, defaultPath)) {
                LoadMetadataFromJson(selectedFile);
            }
        }
    }

    void UpscaleView::SaveMetadataToJson(const std::string& filepath) {
        try {
            nlohmann::json metadata = Serialize();
            std::ofstream file(filepath);
            if (file.is_open()) {
                file << metadata.dump(4);
                file.close();
                std::cout << "Settings saved to: " << filepath << std::endl;
            }
            else {
                std::cerr << "Failed to open file for writing: " << filepath << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error saving settings: " << e.what() << std::endl;
        }
    }

    void UpscaleView::LoadMetadataFromJson(const std::string& filepath) {
        try {
            std::ifstream file(filepath);
            if (file.is_open()) {
                nlohmann::json metadata;
                file >> metadata;
                Deserialize(metadata);
                file.close();
                std::cout << "Settings loaded from: " << filepath << std::endl;
            }
            else {
                std::cerr << "Failed to open file for reading: " << filepath << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error loading settings: " << e.what() << std::endl;
        }
    }

}