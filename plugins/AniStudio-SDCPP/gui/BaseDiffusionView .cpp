#include "BaseDiffusionView.hpp"
#include "Events.hpp"
#include "UISchema.hpp"
#include "SDcppSystem.hpp"
#include "FilePathSystem.hpp"
#include "FileDialogUtil.hpp"
#include "FileDialogFilters.hpp"
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <fstream>

using namespace ECS;
using namespace ANI;

namespace GUI {

    BaseDiffusionView::BaseDiffusionView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseView(mgr, vm) {
        contextMenuUtils = std::make_unique<Utils::ContextMenuUtils>(m_entityManager);
        RegisterAllComponentAdders();
    }

    BaseDiffusionView::~BaseDiffusionView() {
        QuickSave();
        if (stateEntity != 0) m_entityManager.DestroyEntity(stateEntity);
        if (activeEntity != 0) m_entityManager.DestroyEntity(activeEntity);
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
        m_componentAdders["Taesd"] = [this](EntityID e) { this->m_entityManager.AddComponent<TaesdComponent>(e); };
        m_componentAdders["Latent"] = [this](EntityID e) { this->m_entityManager.AddComponent<LatentComponent>(e); };
        m_componentAdders["Sampler"] = [this](EntityID e) { this->m_entityManager.AddComponent<SamplerComponent>(e); };
        m_componentAdders["Guidance"] = [this](EntityID e) { this->m_entityManager.AddComponent<GuidanceComponent>(e); };
        m_componentAdders["Prompt"] = [this](EntityID e) { this->m_entityManager.AddComponent<PromptComponent>(e); };
        m_componentAdders["OutputImage"] = [this](EntityID e) { this->m_entityManager.AddComponent<OutputImageComponent>(e); };
        m_componentAdders["InputImage"] = [this](EntityID e) { this->m_entityManager.AddComponent<InputImageComponent>(e); };
        m_componentAdders["Lora"] = [this](EntityID e) { this->m_entityManager.AddComponent<LoraComponent>(e); };
        m_componentAdders["ControlNet"] = [this](EntityID e) { this->m_entityManager.AddComponent<ControlNetComponent>(e); };
        m_componentAdders["Embedding"] = [this](EntityID e) { this->m_entityManager.AddComponent<EmbeddingComponent>(e); };
        m_componentAdders["PhotoMaker"] = [this](EntityID e) { this->m_entityManager.AddComponent<PhotoMakerComponent>(e); };
        m_componentAdders["StackedIdEmbed"] = [this](EntityID e) { this->m_entityManager.AddComponent<StackedIdEmbedComponent>(e); };
        m_componentAdders["Conversion"] = [this](EntityID e) { this->m_entityManager.AddComponent<ConversionComponent>(e); };
        m_componentAdders["Esrgan"] = [this](EntityID e) { this->m_entityManager.AddComponent<EsrganComponent>(e); };
        m_componentAdders["HighNoiseDiffusionModel"] = [this](EntityID e) { this->m_entityManager.AddComponent<HighNoiseDiffusionModelComponent>(e); };
        m_componentAdders["HighNoiseSampler"] = [this](EntityID e) { this->m_entityManager.AddComponent<HighNoiseSamplerComponent>(e); };
        m_componentAdders["VideoParams"] = [this](EntityID e) { this->m_entityManager.AddComponent<VideoParamsComponent>(e); };
    }

    void BaseDiffusionView::InitializeBase() {
        if (stateEntity != 0) m_entityManager.DestroyEntity(stateEntity);
        if (activeEntity != 0) m_entityManager.DestroyEntity(activeEntity);

        stateEntity = m_entityManager.AddNewEntity();
        activeEntity = m_entityManager.AddNewEntity();

        for (const auto& [name, adder] : m_componentAdders) {
            adder(stateEntity);
        }

        auto visible = GetDefaultVisibleComponents();
        for (const auto& name : visible) {
            componentVisibility[name] = true;
        }
        auto compIds = m_entityManager.GetEntityComponents(stateEntity);
        for (auto cid : compIds) {
            std::string name = m_entityManager.GetComponentNameById(cid);
            if (componentVisibility.find(name) == componentVisibility.end()) {
                componentVisibility[name] = false;
            }
        }

        for (const auto& [name, visibleFlag] : componentVisibility) {
            if (visibleFlag) {
                CopyComponentToActive(name);
            }
        }

        std::cout << "[BaseDiffusionView] State entity " << stateEntity
            << " has " << m_entityManager.GetEntityComponents(stateEntity).size() << " components.\n";
        std::cout << "[BaseDiffusionView] Active entity " << activeEntity
            << " has " << m_entityManager.GetEntityComponents(activeEntity).size() << " components.\n";
    }

    void BaseDiffusionView::CopyComponentToActive(const std::string& name) {
        auto it = m_componentAdders.find(name);
        if (it == m_componentAdders.end()) {
            std::cerr << "[CopyComponentToActive] No adder for " << name << std::endl;
            return;
        }
        auto compId = m_entityManager.GetComponentTypeIdByName(name);
        if (compId == 0) {
            std::cerr << "[CopyComponentToActive] No component ID for " << name << std::endl;
            return;
        }
        auto* stateComp = m_entityManager.GetComponentById(stateEntity, compId);
        if (!stateComp) {
            std::cerr << "[CopyComponentToActive] State component " << name << " not found\n";
            return;
        }
        nlohmann::json compData = stateComp->Serialize();

        // Remove from active if exists, then add and deserialize
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

    void BaseDiffusionView::ToggleComponent(const std::string& name) {
        bool current = IsComponentActive(name);
        SetComponentActive(name, !current);
    }

    bool BaseDiffusionView::IsComponentActive(const std::string& name) const {
        auto it = componentVisibility.find(name);
        if (it == componentVisibility.end()) return false;
        return it->second;
    }

    void BaseDiffusionView::SetComponentActive(const std::string& name, bool active) {
        if (active) {
            if (m_componentAdders.find(name) == m_componentAdders.end()) return;
            componentVisibility[name] = true;
            CopyComponentToActive(name);
        }
        else {
            componentVisibility[name] = false;
            RemoveComponentFromActive(name);
        }
    }

    void BaseDiffusionView::RenderComponent(ECS::ComponentTypeID compId, const std::string& name) {
        auto* comp = m_entityManager.GetComponentById(activeEntity, compId);
        if (!comp) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Component %s not on active entity", name.c_str());
            return;
        }
        if (comp->schema.empty()) {
            ImGui::TextColored(ImVec4(0.5, 0.5, 0.5, 1), "No schema for %s", name.c_str());
            return;
        }
        try {
            auto props = comp->GetPropertyMap();
            UISchema::RenderSchema(comp->schema, props);
            if (UseStateActiveSeparation()) {
                SyncComponentToState(compId);
            }
        }
        catch (const std::exception& e) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error rendering %s: %s", name.c_str(), e.what());
        }
        catch (...) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Unknown error rendering %s", name.c_str());
        }
    }

    void BaseDiffusionView::RenderComponentsUI() {
        auto compIds = m_entityManager.GetEntityComponents(activeEntity);
        if (compIds.empty()) {
            ImGui::TextColored(ImVec4(1, 0.5, 0, 1), "No active components. Try toggling some on.");
            return;
        }
        std::vector<std::pair<ECS::ComponentTypeID, std::string>> comps;
        for (auto cid : compIds) {
            std::string name = m_entityManager.GetComponentNameById(cid);
            comps.emplace_back(cid, name);
        }
        std::sort(comps.begin(), comps.end(), [](auto& a, auto& b) { return a.second < b.second; });

        for (auto [cid, name] : comps) {
            if (ImGui::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Indent();
                RenderComponent(cid, name);
                ImGui::Unindent();
            }
        }
    }

    void BaseDiffusionView::RenderMenuBar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Save Metadata...")) {
                    std::string defaultPath = Utils::g_FilePathSystem ? Utils::g_FilePathSystem->GetPath("DefaultProject") : "";
                    std::string defaultFilename = viewName + "_" + std::to_string(GetID()) + ".json";
                    std::string selectedFile;
                    if (FileDialog::SaveFile("Save Metadata", FileDialog::FilterType::METADATA_FILE, defaultFilename, selectedFile, defaultPath)) {
                        SaveMetadataToJson(selectedFile);
                    }
                }
                if (ImGui::MenuItem("Load Metadata...")) {
                    std::string defaultPath = Utils::g_FilePathSystem ? Utils::g_FilePathSystem->GetPath("DefaultProject") : "";
                    std::string selectedFile;
                    if (FileDialog::OpenFile("Load Metadata", FileDialog::FilterType::METADATA_FILE, selectedFile, defaultPath)) {
                        std::string ext = std::filesystem::path(selectedFile).extension().string();
                        if (ext == ".json")
                            LoadMetadataFromJson(selectedFile);
                        else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
                            LoadMetadataFromPNG(selectedFile);
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

            if (ImGui::BeginMenu("Components")) {
                if (ImGui::MenuItem("Reset to Defaults"))
                    Init();
                ImGui::Separator();
                auto compIds = m_entityManager.GetEntityComponents(stateEntity);
                for (auto cid : compIds) {
                    std::string name = m_entityManager.GetComponentNameById(cid);
                    bool present = IsComponentActive(name);
                    if (ImGui::MenuItem(name.c_str(), nullptr, present))
                        ToggleComponent(name);
                }
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
            if (ImGui::MenuItem("Copy Entity")) {
                contextMenuUtils->CopyEntity(activeEntity);
            }
            ImGui::Separator();
            if (contextMenuUtils->HasClipboardEntity()) {
                ImGui::TextDisabled("Clipboard: %s", contextMenuUtils->GetClipboardPreview().c_str());
                ImGui::Separator();
                contextMenuUtils->RenderPasteMenu(activeEntity);
            }
            else {
                ImGui::TextDisabled("Nothing to paste");
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
        j["stateEntity"] = m_entityManager.SerializeEntity(stateEntity);
        j["activeEntity"] = m_entityManager.SerializeEntity(activeEntity);
        j["componentVisibility"] = componentVisibility;
        return j;
    }

    void BaseDiffusionView::Deserialize(const nlohmann::json& j) {
        try {
            if (stateEntity != 0) m_entityManager.DestroyEntity(stateEntity);
            if (activeEntity != 0) m_entityManager.DestroyEntity(activeEntity);
            stateEntity = m_entityManager.DeserializeEntity(j["stateEntity"]);
            activeEntity = m_entityManager.DeserializeEntity(j["activeEntity"]);
            if (j.contains("componentVisibility")) {
                componentVisibility = j["componentVisibility"];
            }
        }
        catch (...) {}
    }

    void BaseDiffusionView::QuickSave() {
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
            }
        }
        catch (...) {}
    }

    void BaseDiffusionView::LoadMetadataFromPNG(const std::string& pngPath) {
        std::cout << "[BaseDiffusionView] LoadMetadataFromPNG: " << pngPath << std::endl;
    }

    std::vector<std::string> BaseDiffusionView::GetDefaultVisibleComponents() const {
        return GetDefaultComponents();
    }

} // namespace GUI