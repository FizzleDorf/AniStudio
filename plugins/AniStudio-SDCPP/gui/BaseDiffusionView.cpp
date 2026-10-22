// BaseDiffusionView.cpp
#include "BaseDiffusionView.hpp"
#include "Events.hpp"
#include "UISchema.hpp"
#include "SDCPPSystem.hpp"
#include "SettingsSystem.hpp"
#include "FilePathComponent.hpp"
#include "FileDialogUtil.hpp"
#include "FileDialogFilters.hpp"
#include "VectorWidgets.hpp"
#include "SDCPPComponents.h"
#include "ProjectSystem.hpp"
#include "ImageUtils.hpp"
#include "Log.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <map>
#include <typeinfo>

using namespace ECS;
using namespace ANI;

namespace GUI {

    // ---------------------------------------------------------------
    // Component table
    // ---------------------------------------------------------------
    void BaseDiffusionView::BuildComponentTable() {
        m_components.clear();

        auto addTyped = [&](auto typedTag, const char* name) {
            using T = typename decltype(typedTag)::Type;

            ComponentEntry e{};
            e.name = name;

            T probe{};
            std::string cat = probe.GetCompCategory();
            if (cat.empty()) cat = "Other";
            e.category = std::move(cat);

            e.add = [this](ECS::EntityID id) { m_entityManager.AddComponent<T>(id); };
            e.present = [this](ECS::EntityID id) { return m_entityManager.HasComponent<T>(id); };
            e.remove = [this](ECS::EntityID id) { m_entityManager.RemoveComponent<T>(id); };
            e.render = [this, name]() { RenderComponentByType<T>(name); };

            m_components.push_back(std::move(e));
            };

        struct T_Checkpoint { using Type = CheckpointComponent; };
        struct T_DiffusionModel { using Type = DiffusionModelComponent; };
        struct T_ClipL { using Type = ClipLComponent; };
        struct T_ClipG { using Type = ClipGComponent; };
        struct T_T5XXL { using Type = T5XXLComponent; };
        struct T_ClipVision { using Type = ClipVisionComponent; };
        struct T_LlmEncoder { using Type = LlmEncoderComponent; };
        struct T_LlmVision { using Type = LlmVisionComponent; };
        struct T_Vae { using Type = VaeComponent; };
        struct T_VaeTiling { using Type = VaeTilingComponent; };
        struct T_Taesd { using Type = TaesdComponent; };
        struct T_AudioVae { using Type = AudioVaeComponent; };
        struct T_Latent { using Type = LatentComponent; };
        struct T_Sampler { using Type = SamplerComponent; };
        struct T_Guidance { using Type = GuidanceComponent; };
        struct T_Prompt { using Type = PromptComponent; };
        struct T_OutputImage { using Type = OutputImageComponent; };
        struct T_InputImage { using Type = InputImageComponent; };
        struct T_InputVideo { using Type = InputVideoComponent; };
        struct T_OutputVideo { using Type = OutputVideoComponent; };
        struct T_Lora { using Type = LoraComponent; };
        struct T_ControlNet { using Type = ControlNetComponent; };
        struct T_Embeddings { using Type = EmbeddingsComponent; };
        struct T_RefImages { using Type = RefImagesComponent; };
        struct T_ControlFrames { using Type = ControlFramesComponent; };
        struct T_RefVideo { using Type = RefVideoComponent; };
        struct T_RefAudio { using Type = RefAudioComponent; };
        struct T_PhotoMaker { using Type = PhotoMakerComponent; };
        struct T_StackedIdEmbed { using Type = StackedIdEmbedComponent; };
        struct T_Conversion { using Type = ConversionComponent; };
        struct T_Esrgan { using Type = EsrganComponent; };
        struct T_HighNoiseModel { using Type = HighNoiseDiffusionModelComponent; };
        struct T_HighNoiseSampler { using Type = HighNoiseSamplerComponent; };
        struct T_EasyCache { using Type = EasyCacheComponent; };

        addTyped(T_Checkpoint{}, "Checkpoint");
        addTyped(T_DiffusionModel{}, "DiffusionModel");
        addTyped(T_ClipL{}, "ClipL");
        addTyped(T_ClipG{}, "ClipG");
        addTyped(T_T5XXL{}, "T5XXL");
        addTyped(T_ClipVision{}, "ClipVision");
        addTyped(T_LlmEncoder{}, "LlmEncoder");
        addTyped(T_LlmVision{}, "LlmVision");
        addTyped(T_Vae{}, "Vae");
        addTyped(T_VaeTiling{}, "VaeTiling");
        addTyped(T_Taesd{}, "Taesd");
        addTyped(T_AudioVae{}, "AudioVae");
        addTyped(T_Latent{}, "Latent");
        addTyped(T_Sampler{}, "Sampler");
        addTyped(T_Guidance{}, "Guidance");
        addTyped(T_Prompt{}, "Prompt");
        addTyped(T_OutputImage{}, "OutputImage");
        addTyped(T_InputImage{}, "InputImage");
        addTyped(T_InputVideo{}, "InputVideo");
        addTyped(T_OutputVideo{}, "OutputVideo");
        addTyped(T_Lora{}, "Lora");
        addTyped(T_ControlNet{}, "ControlNet");
        addTyped(T_Embeddings{}, "Embeddings");
        addTyped(T_RefImages{}, "RefImages");
        addTyped(T_ControlFrames{}, "ControlFrames");
        addTyped(T_RefVideo{}, "RefVideo");
        addTyped(T_RefAudio{}, "RefAudio");
        addTyped(T_PhotoMaker{}, "PhotoMaker");
        addTyped(T_StackedIdEmbed{}, "StackedIdEmbed");
        addTyped(T_Conversion{}, "Conversion");
        addTyped(T_Esrgan{}, "Esrgan");
        addTyped(T_HighNoiseModel{}, "HighNoiseDiffusionModel");
        addTyped(T_HighNoiseSampler{}, "HighNoiseSampler");
        addTyped(T_EasyCache{}, "EasyCache");

        std::sort(m_components.begin(), m_components.end(),
            [](const ComponentEntry& a, const ComponentEntry& b) {
                return std::string(a.name) < std::string(b.name);
            });
    }

    BaseDiffusionView::BaseDiffusionView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseView(mgr, vm) {
        contextMenuUtils = std::make_unique<Utils::ContextMenuUtils>(m_entityManager);
        BuildComponentTable();
        m_quickLoaded = false;
        m_hasRestoredState = false;
        ANI_LOG_DEBUG("Constructed (view='%s')", viewName.c_str());
    }

    BaseDiffusionView::~BaseDiffusionView() {
        auto projSys = m_entityManager.GetSystem<ProjectSystem>();
        if (projSys && !projSys->IsProjectOpen()) {
            auto sdcpp = m_entityManager.GetSystem<ECS::SDCPPSystem>();
            if (sdcpp) {
                ANI_LOG_DEBUG("Destructor: project not open, shutting down SDCPPSystem");
                sdcpp->Shutdown();
            }
        }
    }

    void BaseDiffusionView::Init() {
        ANI_LOG_INFO("Init: view='%s' hasRestoredState=%d",
            viewName.c_str(), m_hasRestoredState ? 1 : 0);

        if (activeEntity == 0) {
            activeEntity = m_entityManager.AddNewEntity();
            ANI_LOG_DEBUG("Init: created placeholder activeEntity=%u", (unsigned)activeEntity);
        }
    }

    void BaseDiffusionView::ResetToDefaults() {
        ANI_LOG_INFO("ResetToDefaults: view='%s' old activeEntity=%u",
            viewName.c_str(), (unsigned)activeEntity);

        if (activeEntity != 0) {
            m_entityManager.DestroyEntity(activeEntity);
            activeEntity = 0;
        }

        activeEntity = m_entityManager.AddNewEntity();

        ANI_LOG_DEBUG("ResetToDefaults: new activeEntity=%u", (unsigned)activeEntity);

        auto filtered = GetFilteredComponents();
        std::string defaultModel = GetDefaultModelComponent();

        for (auto& e : m_components) {
            std::string name(e.name);

            if (std::find(filtered.begin(), filtered.end(), name) != filtered.end())
                continue;

            if (name == "DiffusionModel" || name == "Checkpoint") {
                if (name != defaultModel) continue;
            }

            ANI_LOG_TRACE("ResetToDefaults: adding '%s' to activeEntity %u",
                name.c_str(), (unsigned)activeEntity);
            e.add(activeEntity);
        }

        m_hasRestoredState = true;

        ANI_LOG_INFO("ResetToDefaults: done. activeEntity=%u", (unsigned)activeEntity);
    }

    void BaseDiffusionView::AddComponentByName(ECS::EntityID entity, const std::string& name) {
        for (auto& e : m_components) {
            if (name == e.name) { e.add(entity); return; }
        }
        ANI_LOG_ERROR("No component registered with name: %s", name.c_str());
    }

    bool BaseDiffusionView::IsComponentFiltered(const std::string& name) const {
        auto filtered = GetFilteredComponents();
        return std::find(filtered.begin(), filtered.end(), name) != filtered.end();
    }

    template<typename T>
    void BaseDiffusionView::RenderComponentByType(const char* name) {
        if (!m_entityManager.HasComponent<T>(activeEntity)) {
            return;
        }

        auto& comp = m_entityManager.GetComponent<T>(activeEntity);

        ImGui::PushID(name);

        if (m_expandAllRequested)   ImGui::SetNextItemOpen(true, ImGuiCond_Always);
        if (m_collapseAllRequested) ImGui::SetNextItemOpen(false, ImGuiCond_Always);

        if (ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();

            if (ImGui::BeginPopupContextItem(("ComponentContext_" + std::string(name)).c_str())) {
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

                auto props = comp.GetPropertyMap();
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
                    m_entityManager.RemoveComponent<T>(activeEntity);
                }
                if (ImGui::MenuItem("Reset Component")) {
                    m_entityManager.RemoveComponent<T>(activeEntity);
                    m_entityManager.AddComponent<T>(activeEntity);
                }
                ImGui::EndPopup();
            }

            const std::unordered_map<std::string, std::string>* pathMap = nullptr;
            auto ss = m_entityManager.GetSystem<ECS::SettingsSystem>();
            if (ss && m_entityManager.HasComponent<ECS::FilePathComponent>(ss->GetSettingsEntity())) {
                auto& fpComp = m_entityManager.GetComponent<ECS::FilePathComponent>(ss->GetSettingsEntity());
                pathMap = &fpComp.GetCurrentMap();
            }

            if constexpr (std::is_same_v<T, LoraComponent>) {
                VectorWidgets::RenderLoraList(comp.loras);
            }
            else if constexpr (std::is_same_v<T, EmbeddingsComponent>) {
                VectorWidgets::RenderEmbeddingsPairList(comp.embeddings);
            }
            else if constexpr (std::is_same_v<T, RefImagesComponent>) {
                VectorWidgets::RenderFilePathList(comp.ref_image_paths);
            }
            else if constexpr (std::is_same_v<T, ControlFramesComponent>) {
                VectorWidgets::RenderFilePathList(comp.filePaths);
            }
            else if constexpr (std::is_same_v<T, RefVideoComponent>) {
                VectorWidgets::RenderStringList(comp.videoPaths);
            }
            else if constexpr (std::is_same_v<T, RefAudioComponent>) {
                VectorWidgets::RenderStringList(comp.audioPaths);
            }
            else if constexpr (std::is_same_v<T, CustomSigmasComponent>) {
                VectorWidgets::RenderFloatList(comp.custom_sigmas);
            }
            else {
                const auto& schema = comp.GetSchema();

                if (!schema.is_object() || schema.empty()) {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No schema for %s", name);
                }
                else {
                    std::string nameStr = name;
                    auto onPropRightClick = [this, nameStr](const std::string& propName, const nlohmann::json&) {
                        ImGui::OpenPopup(("PropContext_" + nameStr + "_" + propName).c_str());
                        if (ImGui::BeginPopup(("PropContext_" + nameStr + "_" + propName).c_str())) {
                            if (ImGui::MenuItem("Copy Value")) {
                                Clipboard::CopyProperty(m_entityManager, activeEntity, nameStr, propName);
                            }
                            if (Clipboard::HasProperty() && ImGui::MenuItem("Paste Value")) {
                                Clipboard::PasteProperty(m_entityManager, activeEntity, nameStr, propName);
                            }
                            if (Clipboard::HasEntity() && Clipboard::EntityClipboardHasProperty(m_entityManager, nameStr, propName)) {
                                if (ImGui::MenuItem("Paste Value from Copied Entity")) {
                                    Clipboard::PastePropertyFromEntity(m_entityManager, activeEntity, nameStr, propName);
                                }
                            }
                            ImGui::EndPopup();
                        }
                        };
                    Engine::PropertyMap props = comp.GetPropertyMap();
                    UISchema::RenderSchema(schema, props, onPropRightClick, nameStr, activeEntity, pathMap);
                }
            }

            ImGui::Unindent();
        }

        ImGui::PopID();
    }

    void BaseDiffusionView::RenderComponentsUI() {
        bool any = false;
        for (auto& e : m_components) {
            if (e.present(activeEntity)) { any = true; break; }
        }

        if (!any) {
            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "No active components.");
            return;
        }

        for (auto& e : m_components) {
            if (e.present(activeEntity)) {
                e.render();
            }
        }

        m_expandAllRequested = false;
        m_collapseAllRequested = false;
    }

    void BaseDiffusionView::RenderAddRemoveComponentsMenu() {
        std::map<std::string, std::vector<const ComponentEntry*>> byCategory;
        for (auto& e : m_components) {
            std::string cat = e.category.empty() ? "Other" : e.category;
            byCategory[cat].push_back(&e);
        }

        for (auto& [category, entries] : byCategory) {
            if (!ImGui::BeginMenu(category.c_str())) continue;

            for (auto* e : entries) {
                bool present = e->present(activeEntity);
                if (ImGui::MenuItem(e->name, nullptr, present)) {
                    if (present) e->remove(activeEntity);
                    else         e->add(activeEntity);
                }
            }

            ImGui::EndMenu();
        }
    }

    void BaseDiffusionView::RenderMenuBar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                std::string defaultProjectPath;
                auto ss = m_entityManager.GetSystem<ECS::SettingsSystem>();
                if (ss && m_entityManager.HasComponent<ECS::FilePathComponent>(ss->GetSettingsEntity())) {
                    auto& fpComp = m_entityManager.GetComponent<ECS::FilePathComponent>(ss->GetSettingsEntity());
                    const auto& pathMap = fpComp.GetCurrentMap();
                    auto it = pathMap.find("DefaultProject");
                    if (it != pathMap.end()) defaultProjectPath = it->second;
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
                        if (ext == ".json") LoadMetadataFromJson(selectedFile);
                        else                LoadMetadataFromMedia(selectedFile);
                    }
                }
                if (ImGui::MenuItem("Quick Save")) QuickSave();
                if (ImGui::MenuItem("Quick Load")) QuickLoad();
                ImGui::Separator();
                if (ImGui::MenuItem("Reset View")) ResetToDefaults();
                ImGui::EndMenu();
            }

            if (ImGui::Button("Expand All"))   m_expandAllRequested = true;
            ImGui::SameLine();
            if (ImGui::Button("Collapse All")) m_collapseAllRequested = true;

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
                    for (auto& e : m_components) {
                        if (!e.present(activeEntity)) continue;
                        if (ImGui::MenuItem(e.name)) {
                            Clipboard::CopyComponent(m_entityManager, activeEntity, e.name);
                        }
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Copy Value")) {
                    for (auto& e : m_components) {
                        if (!e.present(activeEntity)) continue;
                        auto cid = m_entityManager.GetComponentTypeIdByName(e.name);
                        if (cid == ECS::MAX_COMPONENT_COUNT) continue;
                        auto* comp = m_entityManager.GetComponentById(activeEntity, cid);
                        if (!comp) continue;
                        auto props = comp->GetPropertyMap();
                        if (props.empty()) continue;
                        if (ImGui::BeginMenu(e.name)) {
                            for (const auto& [propName, _] : props) {
                                if (ImGui::MenuItem(propName.c_str())) {
                                    Clipboard::CopyProperty(m_entityManager, activeEntity, e.name, propName);
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

            if (ImGui::BeginMenu("Add / Remove Components")) {
                RenderAddRemoveComponentsMenu();
                ImGui::EndMenu();
            }

            ImGui::EndPopup();
        }
    }

    void BaseDiffusionView::Render() {
        if (!m_hasRestoredState && m_entityManager.GetEntityComponents(activeEntity).empty()) {
            ANI_LOG_INFO("Render: view='%s' no restored state, resetting to defaults",
                viewName.c_str());
            ResetToDefaults();
        }

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
            eventData["serializedState"] = Serialize();
            ANI_LOG_DEBUG("Render: view='%s' queuing RemoveView with serializedState",
                viewName.c_str());
            ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
        }
    }

    nlohmann::json BaseDiffusionView::Serialize() const {
        nlohmann::json j;
        j["activeEntity"] = m_entityManager.SerializeEntity(activeEntity);
        return j;
    }

    void BaseDiffusionView::Deserialize(const nlohmann::json& j) {
        ANI_LOG_INFO("Deserialize: view='%s'", viewName.c_str());

        try {
            if (activeEntity != 0) {
                m_entityManager.DestroyEntity(activeEntity);
                activeEntity = 0;
            }

            if (j.contains("activeEntity") && !j["activeEntity"].is_null()) {
                activeEntity = m_entityManager.DeserializeEntity(j["activeEntity"]);
                m_hasRestoredState = true;
                m_quickLoaded = true;
                ANI_LOG_INFO("Deserialize: view='%s' restored activeEntity=%u",
                    viewName.c_str(), (unsigned)activeEntity);
            }
            else {
                activeEntity = m_entityManager.AddNewEntity();
                ANI_LOG_DEBUG("Deserialize: view='%s' created fresh activeEntity=%u (no data)",
                    viewName.c_str(), (unsigned)activeEntity);
            }
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("Deserialize: view='%s' exception: %s", viewName.c_str(), e.what());
            ResetToDefaults();
        }
        catch (...) {
            ANI_LOG_ERROR("Deserialize: view='%s' unknown exception", viewName.c_str());
            ResetToDefaults();
        }
    }

    void BaseDiffusionView::QuickSave() {
        if (!m_quickLoaded) {
            ANI_LOG_TRACE("QuickSave: skipped, not yet quick-loaded");
            return;
        }
        try {
            auto ss = m_entityManager.GetSystem<ECS::SettingsSystem>();
            if (!ss) {
                ANI_LOG_WARN("QuickSave: SettingsSystem unavailable");
                return;
            }
            std::string dataPath = ss->GetPath("ProjectData");
            if (dataPath.empty()) dataPath = ss->GetPath("DefaultProject");
            if (dataPath.empty()) {
                ANI_LOG_WARN("QuickSave: no data path available");
                return;
            }
            std::filesystem::create_directories(dataPath);
            std::string filename = viewName + ".json";
            std::string filepath = (std::filesystem::path(dataPath) / filename).string();
            SaveMetadataToJson(filepath);
            ANI_LOG_DEBUG("QuickSave: saved to %s", filepath.c_str());
        }
        catch (const std::exception& e) {
            ANI_LOG_WARN("QuickSave: exception: %s", e.what());
        }
    }

    void BaseDiffusionView::QuickLoad() {
        if (m_quickLoaded) {
            ANI_LOG_TRACE("QuickLoad: skipped, already loaded");
            return;
        }
        try {
            auto ss = m_entityManager.GetSystem<ECS::SettingsSystem>();
            if (!ss) {
                ANI_LOG_WARN("QuickLoad: SettingsSystem unavailable");
                return;
            }
            std::string dataPath = ss->GetPath("ProjectData");
            if (dataPath.empty()) dataPath = ss->GetPath("DefaultProject");
            if (dataPath.empty()) {
                ANI_LOG_WARN("QuickLoad: no data path available");
                return;
            }
            std::string filename = viewName + ".json";
            std::string filepath = (std::filesystem::path(dataPath) / filename).string();
            if (!std::filesystem::exists(filepath)) {
                ANI_LOG_TRACE("QuickLoad: no saved file at %s", filepath.c_str());
                return;
            }
            LoadMetadataFromJson(filepath);
            m_quickLoaded = true;
            ANI_LOG_DEBUG("QuickLoad: loaded from %s", filepath.c_str());
        }
        catch (const std::exception& e) {
            ANI_LOG_WARN("QuickLoad: exception: %s", e.what());
        }
    }

    void BaseDiffusionView::SaveMetadataToJson(const std::string& filepath) {
        try {
            nlohmann::json meta = Serialize();
            std::ofstream file(filepath);
            if (file.is_open()) {
                file << meta.dump(4);
                ANI_LOG_DEBUG("SaveMetadataToJson: wrote %s", filepath.c_str());
            }
            else {
                ANI_LOG_WARN("SaveMetadataToJson: failed to open %s for writing", filepath.c_str());
            }
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("SaveMetadataToJson: exception writing %s: %s", filepath.c_str(), e.what());
        }
    }

    void BaseDiffusionView::LoadMetadataFromJson(const std::string& filepath) {
        try {
            std::ifstream file(filepath);
            if (file.is_open()) {
                nlohmann::json meta;
                file >> meta;
                Deserialize(meta);
                m_quickLoaded = true;
                ANI_LOG_DEBUG("LoadMetadataFromJson: loaded %s", filepath.c_str());
            }
            else {
                ANI_LOG_WARN("LoadMetadataFromJson: failed to open %s for reading", filepath.c_str());
            }
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("LoadMetadataFromJson: exception reading %s: %s", filepath.c_str(), e.what());
        }
    }

    void BaseDiffusionView::LoadMetadataFromMedia(const std::string& filePath) {
        ANI_LOG_INFO("LoadMetadataFromMedia: %s", filePath.c_str());
        try {
            nlohmann::json metadata = Utils::ImageUtils::ReadMetadataFromImage(filePath);
            if (metadata.is_null() || metadata.empty()) {
                ANI_LOG_DEBUG("LoadMetadataFromMedia: no metadata in %s", filePath.c_str());
                return;
            }

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
            }

            if (!entityData.contains("components") || !entityData["components"].is_array()) {
                ANI_LOG_WARN("LoadMetadataFromMedia: missing 'components' array after wrapping");
                return;
            }

            if (activeEntity != 0) {
                m_entityManager.DestroyEntity(activeEntity);
            }
            activeEntity = m_entityManager.DeserializeEntity(entityData);
            m_hasRestoredState = true;
            m_quickLoaded = true;

            ANI_LOG_INFO("LoadMetadataFromMedia: success. activeEntity=%u", (unsigned)activeEntity);
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("LoadMetadataFromMedia: exception: %s", e.what());
        }
        catch (...) {
            ANI_LOG_ERROR("LoadMetadataFromMedia: unknown exception");
        }
    }

} // namespace GUI