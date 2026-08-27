// BaseDiffusionView.cpp
#include "BaseDiffusionView.hpp"
#include "Events.hpp"
#include "UISchema.hpp"
#include "SDcppSystem.hpp"
#include "FilePathSystem.hpp"
#include "FileDialogUtil.hpp"
#include "FileDialogFilters.hpp"
#include "VectorWidgets.hpp"
#include "SDCPPComponents.h"
#include "ProjectSystem.hpp"
#include "ImageUtils.hpp"
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <unordered_map>

using namespace ECS;
using namespace ANI;

namespace GUI {

    BaseDiffusionView::BaseDiffusionView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseView(mgr, vm) {
        contextMenuUtils = std::make_unique<Utils::ContextMenuUtils>(m_entityManager);
        RegisterAllComponentAdders();
        m_quickLoaded = false;
    }

    BaseDiffusionView::~BaseDiffusionView() {
        auto projSys = m_entityManager.GetSystem<ProjectSystem>();
        if (projSys && !projSys->IsProjectOpen()) {
            auto sdcpp = m_entityManager.GetSystem<ECS::SDCPPSystem>();
            if (sdcpp) {
                sdcpp->Shutdown();
            }
        }
    }

    void BaseDiffusionView::Init() {
        InitializeBase();
    }

    void BaseDiffusionView::RegisterAllComponentAdders() {
        using namespace ECS;
        m_componentAdders["Checkpoint"] = [this](EntityID e) { this->m_entityManager.AddComponent<CheckpointComponent>(e); };
        m_componentAdders["DiffusionModel"] = [this](EntityID e) { this->m_entityManager.AddComponent<DiffusionModelComponent>(e); };
        m_componentAdders["ClipL"] = [this](EntityID e) { this->m_entityManager.AddComponent<ClipLComponent>(e); };
        m_componentAdders["ClipG"] = [this](EntityID e) { this->m_entityManager.AddComponent<ClipGComponent>(e); };
        m_componentAdders["T5XXL"] = [this](EntityID e) { this->m_entityManager.AddComponent<T5XXLComponent>(e); };
        m_componentAdders["ClipVision"] = [this](EntityID e) { this->m_entityManager.AddComponent<ClipVisionComponent>(e); };
        m_componentAdders["LlmEncoder"] = [this](EntityID e) { this->m_entityManager.AddComponent<LlmEncoderComponent>(e); };
        m_componentAdders["LlmVision"] = [this](EntityID e) { this->m_entityManager.AddComponent<LlmVisionComponent>(e); };
        m_componentAdders["Vae"] = [this](EntityID e) { this->m_entityManager.AddComponent<VaeComponent>(e); };
        m_componentAdders["VaeTiling"] = [this](EntityID e) { this->m_entityManager.AddComponent<VaeTilingComponent>(e); };
        m_componentAdders["Taesd"] = [this](EntityID e) { this->m_entityManager.AddComponent<TaesdComponent>(e); };
        m_componentAdders["AudioVae"] = [this](EntityID e) { this->m_entityManager.AddComponent<AudioVaeComponent>(e); };
        m_componentAdders["Latent"] = [this](EntityID e) { this->m_entityManager.AddComponent<LatentComponent>(e); };
        m_componentAdders["Sampler"] = [this](EntityID e) { this->m_entityManager.AddComponent<SamplerComponent>(e); };
        m_componentAdders["Guidance"] = [this](EntityID e) { this->m_entityManager.AddComponent<GuidanceComponent>(e); };
        m_componentAdders["Prompt"] = [this](EntityID e) { this->m_entityManager.AddComponent<PromptComponent>(e); };
        m_componentAdders["OutputImage"] = [this](EntityID e) { this->m_entityManager.AddComponent<OutputImageComponent>(e); };
        m_componentAdders["InputImage"] = [this](EntityID e) { this->m_entityManager.AddComponent<InputImageComponent>(e); };
        m_componentAdders["InputVideo"] = [this](EntityID e) { this->m_entityManager.AddComponent<InputVideoComponent>(e); };
        m_componentAdders["OutputVideo"] = [this](EntityID e) { this->m_entityManager.AddComponent<OutputVideoComponent>(e); };
        m_componentAdders["Lora"] = [this](EntityID e) { this->m_entityManager.AddComponent<LoraComponent>(e); };
        m_componentAdders["ControlNet"] = [this](EntityID e) { this->m_entityManager.AddComponent<ControlNetComponent>(e); };
        m_componentAdders["Embeddings"] = [this](EntityID e) { this->m_entityManager.AddComponent<EmbeddingsComponent>(e); };
        m_componentAdders["RefImages"] = [this](EntityID e) { this->m_entityManager.AddComponent<RefImagesComponent>(e); };
        m_componentAdders["ControlFrames"] = [this](EntityID e) { this->m_entityManager.AddComponent<ControlFramesComponent>(e); };
        m_componentAdders["RefVideo"] = [this](EntityID e) { this->m_entityManager.AddComponent<RefVideoComponent>(e); };
        m_componentAdders["RefAudio"] = [this](EntityID e) { this->m_entityManager.AddComponent<RefAudioComponent>(e); };
        m_componentAdders["PhotoMaker"] = [this](EntityID e) { this->m_entityManager.AddComponent<PhotoMakerComponent>(e); };
        m_componentAdders["StackedIdEmbed"] = [this](EntityID e) { this->m_entityManager.AddComponent<StackedIdEmbedComponent>(e); };
        m_componentAdders["Conversion"] = [this](EntityID e) { this->m_entityManager.AddComponent<ConversionComponent>(e); };
        m_componentAdders["Esrgan"] = [this](EntityID e) { this->m_entityManager.AddComponent<EsrganComponent>(e); };
        m_componentAdders["HighNoiseDiffusionModel"] = [this](EntityID e) { this->m_entityManager.AddComponent<HighNoiseDiffusionModelComponent>(e); };
        m_componentAdders["HighNoiseSampler"] = [this](EntityID e) { this->m_entityManager.AddComponent<HighNoiseSamplerComponent>(e); };
        m_componentAdders["VideoParams"] = [this](EntityID e) { this->m_entityManager.AddComponent<VideoParamsComponent>(e); };
        m_componentAdders["EasyCache"] = [this](EntityID e) { this->m_entityManager.AddComponent<EasyCacheComponent>(e); };
    }

    void BaseDiffusionView::InitializeBase() {
        if (stateEntity != 0) m_entityManager.DestroyEntity(stateEntity);
        if (activeEntity != 0) m_entityManager.DestroyEntity(activeEntity);

        stateEntity = m_entityManager.AddNewEntity();
        activeEntity = m_entityManager.AddNewEntity();

        auto filteredComponents = GetFilteredComponents();

        for (size_t i = 0; i < ALL_COMPONENTS_COUNT; ++i) {
            std::string name(ALL_COMPONENTS[i]);

            if (std::find(filteredComponents.begin(), filteredComponents.end(), name) != filteredComponents.end()) {
                continue;
            }

            auto it = m_componentAdders.find(name);
            if (it != m_componentAdders.end()) {
                it->second(stateEntity);
            }
        }

        auto defaultComponents = GetDefaultComponents();
        for (const auto& name : defaultComponents) {
            CopyComponentToActive(name);
        }

        EnsureMutualExclusivity();
    }

    void BaseDiffusionView::CopyComponentToActive(const std::string& name) {
        auto it = m_componentAdders.find(name);
        if (it == m_componentAdders.end()) return;
        auto compId = m_entityManager.GetComponentTypeIdByName(name);
        if (compId == 0) return;
        auto* stateComp = m_entityManager.GetComponentById(stateEntity, compId);
        if (!stateComp) return;
        nlohmann::json compData = stateComp->Serialize();

        if (m_entityManager.HasComponentById(activeEntity, compId)) {
            m_entityManager.RemoveComponentById(activeEntity, compId);
        }
        it->second(activeEntity);
        auto* activeComp = m_entityManager.GetComponentById(activeEntity, compId);
        if (activeComp) {
            activeComp->Deserialize(compData);
            activeComp->RefreshSchema();
        }
    }

    void BaseDiffusionView::RemoveComponentFromActive(const std::string& name) {
        auto compId = m_entityManager.GetComponentTypeIdByName(name);
        if (compId != 0 && m_entityManager.HasComponentById(activeEntity, compId)) {
            m_entityManager.RemoveComponentById(activeEntity, compId);
        }
    }

    void BaseDiffusionView::SyncComponentToState(ECS::ComponentTypeID compId) {
        if (!UseStateActiveSeparation()) return;
        if (!m_entityManager.HasComponentById(activeEntity, compId)) return;
        auto* activeComp = m_entityManager.GetComponentById(activeEntity, compId);
        if (!activeComp) return;
        nlohmann::json data = activeComp->Serialize();
        if (m_entityManager.HasComponentById(stateEntity, compId)) {
            auto* stateComp = m_entityManager.GetComponentById(stateEntity, compId);
            if (stateComp) {
                stateComp->Deserialize(data);
                stateComp->RefreshSchema();
            }
        }
    }

    std::vector<std::string> BaseDiffusionView::GetAllComponentNames() const {
        std::vector<std::string> names;
        for (const auto& [name, _] : m_componentAdders) {
            names.push_back(name);
        }
        return names;
    }

    void BaseDiffusionView::AddComponentByName(ECS::EntityID entity, const std::string& name) {
        auto it = m_componentAdders.find(name);
        if (it != m_componentAdders.end()) {
            it->second(entity);
            return;
        }
        std::string withoutSuffix = name;
        if (withoutSuffix.size() > 9 && withoutSuffix.substr(withoutSuffix.size() - 9) == "Component") {
            withoutSuffix = withoutSuffix.substr(0, withoutSuffix.size() - 9);
            auto itShort = m_componentAdders.find(withoutSuffix);
            if (itShort != m_componentAdders.end()) {
                itShort->second(entity);
                return;
            }
        }
        std::string withSuffix = name + "Component";
        auto itWith = m_componentAdders.find(withSuffix);
        if (itWith != m_componentAdders.end()) {
            itWith->second(entity);
            return;
        }
        std::cerr << "[BaseDiffusionView] No adder found for component: " << name << std::endl;
    }

    void BaseDiffusionView::RenderComponent(ECS::ComponentTypeID compId, const std::string& name) {
        auto* comp = m_entityManager.GetComponentById(activeEntity, compId);
        if (!comp) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Component %s not on active entity", name.c_str());
            return;
        }

        ImGui::PushID(name.c_str());

        if (ImGui::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();

            if (ImGui::BeginPopupContextItem(("ComponentContext_" + name).c_str())) {
                if (ImGui::MenuItem("Copy Entity")) {
                    Clipboard::CopyEntity(m_entityManager, activeEntity);
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Copy Component")) {
                    Clipboard::CopyComponent(m_entityManager, activeEntity, name);
                }
                if (Clipboard::HasComponent() && ImGui::MenuItem("Paste Component")) {
                    Clipboard::PasteComponent(m_entityManager, activeEntity, name);
                }

                if (Clipboard::HasEntity()) {
                    auto compNames = Clipboard::GetEntityComponentNames(m_entityManager);
                    if (!compNames.empty()) {
                        if (ImGui::BeginMenu("Paste Component from Copied Entity")) {
                            for (const auto& srcCompName : compNames) {
                                if (ImGui::MenuItem(srcCompName.c_str())) {
                                    Clipboard::PasteComponentFromEntity(m_entityManager, activeEntity, srcCompName,
                                        [this](ECS::EntityID e, const std::string& n) { this->AddComponentByName(e, n); });
                                }
                            }
                            ImGui::EndMenu();
                        }
                    }
                }

                ImGui::Separator();

                auto props = comp->GetPropertyMap();
                if (!props.empty()) {
                    if (ImGui::BeginMenu("Copy Value")) {
                        for (const auto& [propName, propVariant] : props) {
                            if (ImGui::MenuItem(propName.c_str())) {
                                Clipboard::CopyProperty(m_entityManager, activeEntity, name, propName);
                            }
                        }
                        ImGui::EndMenu();
                    }
                }

                if (Clipboard::HasEntity()) {
                    auto propNames = Clipboard::GetComponentPropertyNames(m_entityManager, name);
                    if (!propNames.empty()) {
                        if (ImGui::BeginMenu("Paste Value from Copied Entity")) {
                            for (const auto& propName : propNames) {
                                if (Clipboard::EntityClipboardHasProperty(m_entityManager, name, propName)) {
                                    if (ImGui::MenuItem(propName.c_str())) {
                                        Clipboard::PastePropertyFromEntity(m_entityManager, activeEntity, name, propName);
                                    }
                                }
                            }
                            ImGui::EndMenu();
                        }
                    }
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Remove Component")) {
                    Clipboard::RemoveComponent(m_entityManager, activeEntity, name);
                }
                if (ImGui::MenuItem("Reset Component")) {
                    Clipboard::ResetComponent(m_entityManager, activeEntity, name,
                        [this](ECS::EntityID e, const std::string& n) { this->AddComponentByName(e, n); });
                }
                ImGui::EndPopup();
            }

            const std::unordered_map<std::string, std::string>* pathMap = nullptr;
            auto filePathSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
            if (filePathSys) {
                auto fileCompId = m_entityManager.GetComponentTypeIdByName("FilePathComponent");
                if (fileCompId != 0) {
                    auto* base = m_entityManager.GetComponentById(filePathSys->GetEntityID(), fileCompId);
                    if (auto* fpComp = dynamic_cast<ECS::FilePathComponent*>(base)) {
                        pathMap = &fpComp->GetPathMap();
                    }
                }
            }

            if (name == "Lora") {
                auto* loraComp = dynamic_cast<LoraComponent*>(comp);
                if (loraComp) {
                    VectorWidgets::RenderLoraList(loraComp->loras);
                }
            }
            else if (name == "Embeddings") {
                auto* embsComp = dynamic_cast<EmbeddingsComponent*>(comp);
                if (embsComp) {
                    VectorWidgets::RenderEmbeddingsPairList(embsComp->embeddings);
                }
            }
            else if (name == "RefImages") {
                auto* refComp = dynamic_cast<RefImagesComponent*>(comp);
                if (refComp) {
                    VectorWidgets::RenderFilePathList(refComp->ref_image_paths);
                }
            }
            else if (name == "ControlFrames") {
                auto* cfComp = dynamic_cast<ControlFramesComponent*>(comp);
                if (cfComp) {
                    VectorWidgets::RenderFilePathList(cfComp->filePaths);
                }
            }
            else if (name == "RefVideo") {
                auto* rvComp = dynamic_cast<RefVideoComponent*>(comp);
                if (rvComp) {
                    VectorWidgets::RenderStringList(rvComp->videoPaths);
                }
            }
            else if (name == "RefAudio") {
                auto* raComp = dynamic_cast<RefAudioComponent*>(comp);
                if (raComp) {
                    VectorWidgets::RenderStringList(raComp->audioPaths);
                }
            }
            else if (name == "RefVideoAudio") {
                auto* rvaComp = dynamic_cast<RefVideoAudioComponent*>(comp);
                if (rvaComp) {
                    VectorWidgets::RenderStringList(rvaComp->audioPaths);
                }
            }
            else if (name == "CustomSigmas") {
                auto* csComp = dynamic_cast<CustomSigmasComponent*>(comp);
                if (csComp) {
                    VectorWidgets::RenderFloatList(csComp->custom_sigmas);
                }
            }
            else {
                if (comp->schema.empty()) {
                    ImGui::TextColored(ImVec4(0.5, 0.5, 0.5, 1), "No schema for %s", name.c_str());
                }
                else {
                    auto onPropRightClick = [this, name](const std::string& propName, const nlohmann::json& value) {
                        ImGui::OpenPopup(("PropContext_" + name + "_" + propName).c_str());
                        if (ImGui::BeginPopup(("PropContext_" + name + "_" + propName).c_str())) {
                            if (ImGui::MenuItem("Copy Value")) {
                                Clipboard::CopyProperty(m_entityManager, activeEntity, name, propName);
                            }
                            if (Clipboard::HasProperty() && ImGui::MenuItem("Paste Value")) {
                                Clipboard::PasteProperty(m_entityManager, activeEntity, name, propName);
                            }
                            if (Clipboard::HasEntity() && Clipboard::EntityClipboardHasProperty(m_entityManager, name, propName)) {
                                if (ImGui::MenuItem("Paste Value from Copied Entity")) {
                                    Clipboard::PastePropertyFromEntity(m_entityManager, activeEntity, name, propName);
                                }
                            }
                            ImGui::EndPopup();
                        }
                        };
                    UISchema::RenderSchema(comp->schema, comp->GetPropertyMap(), onPropRightClick, name, activeEntity, pathMap);
                }
            }

            if (UseStateActiveSeparation()) {
                SyncComponentToState(compId);
            }

            ImGui::Unindent();
        }

        ImGui::PopID();
    }

    void BaseDiffusionView::RenderComponentsUI() {
        auto compIds = m_entityManager.GetEntityComponents(activeEntity);
        if (compIds.empty()) {
            ImGui::TextColored(ImVec4(1, 0.5, 0, 1), "No active components.");
            return;
        }
        std::vector<std::pair<ECS::ComponentTypeID, std::string>> comps;
        for (auto cid : compIds) {
            std::string name = m_entityManager.GetComponentNameById(cid);
            comps.emplace_back(cid, name);
        }
        std::sort(comps.begin(), comps.end(), [](auto& a, auto& b) { return a.second < b.second; });

        for (auto [cid, name] : comps) {
            RenderComponent(cid, name);
        }
    }

    void BaseDiffusionView::RenderMenuBar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                std::string defaultProjectPath;
                auto filePathSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
                if (filePathSys) {
                    auto compId = m_entityManager.GetComponentTypeIdByName("FilePathComponent");
                    if (compId != 0) {
                        auto* base = m_entityManager.GetComponentById(filePathSys->GetEntityID(), compId);
                        if (auto* fpComp = dynamic_cast<ECS::FilePathComponent*>(base)) {
                            const auto& pathMap = fpComp->GetPathMap();
                            auto it = pathMap.find("DefaultProject");
                            if (it != pathMap.end()) {
                                defaultProjectPath = it->second;
                            }
                        }
                    }
                }

                if (ImGui::MenuItem("Save Metadata...")) {
                    std::string defaultPath = defaultProjectPath;
                    std::string defaultFilename = viewName + "_" + std::to_string(static_cast<int>(GetID())) + ".json";
                    std::string selectedFile;
                    if (FileDialog::SaveFile("Save Metadata", FileDialog::FilterType::METADATA_FILE, defaultFilename, selectedFile, defaultPath)) {
                        SaveMetadataToJson(selectedFile);
                    }
                }
                if (ImGui::MenuItem("Load Metadata...")) {
                    std::string defaultPath = defaultProjectPath;
                    std::string selectedFile;
                    if (FileDialog::OpenFile("Load Metadata", FileDialog::FilterType::METADATA_FILE, selectedFile, defaultPath)) {
                        std::string ext = std::filesystem::path(selectedFile).extension().string();
                        if (ext == ".json")
                            LoadMetadataFromJson(selectedFile);
                        else
                            LoadMetadataFromMedia(selectedFile);
                    }
                }
                if (ImGui::MenuItem("Quick Save"))
                    QuickSave();
                if (ImGui::MenuItem("Quick Load"))
                    QuickLoad();
                ImGui::Separator();
                if (ImGui::MenuItem("Reset View"))
                    Init();
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
    }

    void BaseDiffusionView::RenderMainContextMenu() {
        if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup("DiffusionMainContext");
        }
        if (ImGui::BeginPopup("DiffusionMainContext")) {
            ImGui::Text("Diffusion View");
            ImGui::Separator();

            if (ImGui::BeginMenu("Copy")) {
                if (ImGui::MenuItem("Copy Entity")) {
                    Clipboard::CopyEntity(m_entityManager, activeEntity);
                }

                ImGui::Separator();

                if (ImGui::BeginMenu("Copy Component")) {
                    auto compIds = m_entityManager.GetEntityComponents(activeEntity);
                    for (auto cid : compIds) {
                        std::string name = m_entityManager.GetComponentNameById(cid);
                        if (ImGui::MenuItem(name.c_str())) {
                            Clipboard::CopyComponent(m_entityManager, activeEntity, name);
                        }
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Copy Value")) {
                    auto compIds = m_entityManager.GetEntityComponents(activeEntity);
                    for (auto cid : compIds) {
                        std::string compName = m_entityManager.GetComponentNameById(cid);
                        auto* comp = m_entityManager.GetComponentById(activeEntity, cid);
                        if (!comp) continue;
                        auto props = comp->GetPropertyMap();
                        if (props.empty()) continue;
                        if (ImGui::BeginMenu(compName.c_str())) {
                            for (const auto& [propName, _] : props) {
                                if (ImGui::MenuItem(propName.c_str())) {
                                    Clipboard::CopyProperty(m_entityManager, activeEntity, compName, propName);
                                }
                            }
                            ImGui::EndMenu();
                        }
                    }
                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Paste")) {
                contextMenuUtils->RenderPasteMenu(activeEntity);
                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::BeginMenu("Add Component")) {
                auto activeComps = m_entityManager.GetEntityComponents(activeEntity);
                for (const auto& [name, _] : m_componentAdders) {
                    bool exists = false;
                    for (auto cid : activeComps) {
                        if (m_entityManager.GetComponentNameById(cid) == name) { exists = true; break; }
                    }
                    if (!exists && ImGui::MenuItem(name.c_str())) {
                        Clipboard::AddComponent(m_entityManager, activeEntity, name,
                            [this](ECS::EntityID e, const std::string& n) { this->AddComponentByName(e, n); });
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Reset All Components")) {
                auto comps = m_entityManager.GetEntityComponents(activeEntity);
                std::vector<std::string> names;
                for (auto cid : comps) names.push_back(m_entityManager.GetComponentNameById(cid));
                Clipboard::ResetAllComponents(m_entityManager, activeEntity, names,
                    [this](ECS::EntityID e, const std::string& n) { this->AddComponentByName(e, n); });
            }

            ImGui::EndPopup();
        }
    }

    void BaseDiffusionView::Render() {
        ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen, ImGuiWindowFlags_MenuBar)) {
            RenderMenuBar();
            if (contextMenuUtils->HasClipboardEntity()) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Clipboard: %s", contextMenuUtils->GetClipboardPreview().c_str());
                ImGui::Separator();
            }
            RenderComponentsUI();
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

    nlohmann::json BaseDiffusionView::Serialize() const {
        nlohmann::json j;
        j["activeEntity"] = m_entityManager.SerializeEntity(activeEntity);
        return j;
    }

    void BaseDiffusionView::Deserialize(const nlohmann::json& j) {
        try {
            activeEntity = m_entityManager.DeserializeEntity(j["activeEntity"]);
            m_quickLoaded = true;
        }
        catch (...) {}
    }

    void BaseDiffusionView::QuickSave() {
        if (!m_quickLoaded) return;
        try {
            auto filePathSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
            if (!filePathSys) return;
            std::string dataPath = filePathSys->GetPath("ProjectDataPath");
            if (dataPath.empty()) dataPath = filePathSys->GetPath("DefaultProject");
            if (dataPath.empty()) return;
            std::filesystem::create_directories(dataPath);
            std::string filename = viewName + ".json";
            std::string filepath = (std::filesystem::path(dataPath) / filename).string();
            SaveMetadataToJson(filepath);
        }
        catch (...) {}
    }

    void BaseDiffusionView::QuickLoad() {
        if (m_quickLoaded) return;
        try {
            auto filePathSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
            if (!filePathSys) return;
            std::string dataPath = filePathSys->GetPath("ProjectDataPath");
            if (dataPath.empty()) dataPath = filePathSys->GetPath("DefaultProject");
            if (dataPath.empty()) return;
            std::string filename = viewName + ".json";
            std::string filepath = (std::filesystem::path(dataPath) / filename).string();
            if (!std::filesystem::exists(filepath)) return;
            LoadMetadataFromJson(filepath);
            m_quickLoaded = true;
        }
        catch (...) {}
    }

    void BaseDiffusionView::SaveMetadataToJson(const std::string& filepath) {
        try {
            nlohmann::json meta = Serialize();
            std::ofstream file(filepath);
            if (file.is_open()) file << meta.dump(4);
        }
        catch (...) {}
    }

    void BaseDiffusionView::LoadMetadataFromJson(const std::string& filepath) {
        try {
            std::ifstream file(filepath);
            if (file.is_open()) {
                nlohmann::json meta;
                file >> meta;
                Deserialize(meta);
                m_quickLoaded = true;
            }
        }
        catch (...) {}
    }

    void BaseDiffusionView::LoadMetadataFromMedia(const std::string& filePath) {
        std::cout << "[BaseDiffusionView] LoadMetadataFromMedia: " << filePath << std::endl;
        try {
            nlohmann::json metadata = Utils::ImageUtils::ReadMetadataFromImage(filePath);
            if (!metadata.is_null() && !metadata.empty()) {
                std::cout << "[BaseDiffusionView] Found metadata in media file: " << filePath << std::endl;

                nlohmann::json entityData = metadata;

                if (!metadata.contains("components")) {
                    nlohmann::json wrapped;
                    wrapped["components"] = nlohmann::json::array();
                    for (auto it = metadata.begin(); it != metadata.end(); ++it) {
                        if (it.value().is_object()) {
                            nlohmann::json comp;
                            comp[it.key()] = it.value();
                            wrapped["components"].push_back(comp);
                        }
                    }
                    entityData = wrapped;
                    std::cout << "[BaseDiffusionView] Wrapped metadata into entity format" << std::endl;
                }

                if (entityData.contains("components") && entityData["components"].is_array()) {
                    bool validComponents = false;
                    for (const auto& comp : entityData["components"]) {
                        if (comp.is_object() && !comp.empty()) {
                            validComponents = true;
                            break;
                        }
                    }

                    if (!validComponents) {
                        std::cout << "[BaseDiffusionView] Media metadata contains no valid components, skipping" << std::endl;
                        return;
                    }

                    std::cout << "[BaseDiffusionView] Loading metadata from media file" << std::endl;

                    if (activeEntity != 0) {
                        m_entityManager.DestroyEntity(activeEntity);
                    }
                    activeEntity = m_entityManager.DeserializeEntity(entityData);
                    m_quickLoaded = true;

                    std::cout << "[BaseDiffusionView] Successfully loaded metadata from media file: " << filePath << std::endl;
                }
                else {
                    std::cout << "[BaseDiffusionView] Media metadata missing 'components' array after wrapping" << std::endl;
                }
            }
            else {
                std::cout << "[BaseDiffusionView] No metadata found in media file: " << filePath << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[BaseDiffusionView] Failed to load metadata from media file: " << e.what() << std::endl;
        }
        catch (...) {
            std::cerr << "[BaseDiffusionView] Unknown error loading metadata from media file" << std::endl;
        }
    }

    void BaseDiffusionView::EnsureMutualExclusivity() {
        auto compIds = m_entityManager.GetEntityComponents(stateEntity);
        bool hasDiffusionModel = false, hasCheckpoint = false;
        for (auto cid : compIds) {
            std::string name = m_entityManager.GetComponentNameById(cid);
            if (name == "DiffusionModel") hasDiffusionModel = true;
            if (name == "Checkpoint") hasCheckpoint = true;
        }
        if (hasDiffusionModel && hasCheckpoint) {
            auto compId = m_entityManager.GetComponentTypeIdByName("Checkpoint");
            if (compId != 0 && m_entityManager.HasComponentById(stateEntity, compId)) {
                m_entityManager.RemoveComponentById(stateEntity, compId);
            }
        }
    }

} // namespace GUI