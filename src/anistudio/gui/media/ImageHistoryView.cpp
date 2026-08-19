#include "ImageHistoryView.hpp"
#include "Events.hpp"
#include "DragDropUtils.hpp"
#include "ContextMenuUtils.hpp"
#include "ViewManager.hpp"
#include <imgui.h>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <iomanip>

namespace GUI {

    ImageHistoryView::ImageHistoryView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseView(mgr, vm), imageSystem(nullptr), parentWorkspaceID(0), selectedEntityID(0) {
        viewName = "ImageHistoryView";
    }

    void ImageHistoryView::Init() {
        imageSystem = m_entityManager.GetSystem<ECS::ImageSystem>();
        if (!imageSystem) {
            m_entityManager.RegisterSystem<ECS::ImageSystem>();
            imageSystem = m_entityManager.GetSystem<ECS::ImageSystem>();
        }
        if (imageSystem) {
            imageSystem->RegisterImageAddedCallback([this](ECS::EntityID entity) { OnImageAdded(entity); });
            imageSystem->RegisterImageRemovedCallback([this](ECS::EntityID entity) { OnImageRemoved(entity); });
        }
        RefreshEntities();
    }

    void ImageHistoryView::Update(float deltaT) {}

    void ImageHistoryView::Render() {
        ImGui::Begin("Image History", &windowOpen);
        if (imageEntities.empty()) {
            ImGui::Text("No images loaded.");
            ImGui::End();
            return;
        }

        float availableWidth = ImGui::GetContentRegionAvail().x;
        float thumbnailSize = 150.0f;
        float spacing = 8.0f;
        float itemWidth = thumbnailSize + spacing;
        int columns = std::max(1, static_cast<int>(availableWidth / itemWidth));

        ImGui::Columns(columns, nullptr, false);

        for (size_t i = 0; i < imageEntities.size(); ++i) {
            RenderImageThumbnail(imageEntities[i], i);
            ImGui::NextColumn();
        }

        ImGui::Columns(1);
        ImGui::End();

        if (!windowOpen) {
            std::unordered_map<std::string, std::any> eventData;
            eventData["workspaceID"] = GetID();
            eventData["viewTypeName"] = viewName;
            ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
        }
    }

    void ImageHistoryView::RenderImageThumbnail(ECS::EntityID entityID, size_t index) {
        if (!m_entityManager.IsEntityValid(entityID) || !m_entityManager.HasComponent<ECS::ImageComponent>(entityID))
            return;

        const auto& imageComp = m_entityManager.GetComponent<ECS::ImageComponent>(entityID);

        ImGui::BeginGroup();

        ImVec2 thumbSize(128.0f, 128.0f);
        if (imageComp.textureID != 0) {
            float aspect = (imageComp.height > 0) ? (float)imageComp.width / imageComp.height : 1.0f;
            ImVec2 size = thumbSize;
            if (aspect > 1.0f) size.y = thumbSize.x / aspect;
            else size.x = thumbSize.y * aspect;
            if (ImGui::ImageButton(("##img" + std::to_string(index)).c_str(),
                (ImTextureID)(intptr_t)imageComp.textureID, size)) {
                SelectImage(entityID);
            }
        }
        else {
            ImGui::Button(("Select##" + std::to_string(index)).c_str(), thumbSize);
            if (ImGui::IsItemClicked()) SelectImage(entityID);
        }

        // Internal drag-drop (within the app)
        if (ImGui::BeginDragDropSource()) {
            nlohmann::json payload;
            payload["entityID"] = entityID;
            ImGui::SetDragDropPayload(GUI::DragDrop::PAYLOAD_ENTITY,
                payload.dump().c_str(), payload.dump().size() + 1);
            if (imageComp.textureID != 0) {
                ImGui::Image((ImTextureID)(intptr_t)imageComp.textureID, ImVec2(64, 64));
            }
            ImGui::Text("%s", imageComp.fileName.c_str());
            ImGui::EndDragDropSource();
        }

        // Full context menu matching the base view
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup(("ImageContextMenu##" + std::to_string(entityID)).c_str());
        }

        if (ImGui::BeginPopup(("ImageContextMenu##" + std::to_string(entityID)).c_str())) {
            Utils::ContextMenuUtils menuUtils(m_entityManager);
            menuUtils.RenderEntityContextMenu(entityID);
            ImGui::EndPopup();
        }

        std::string filename = imageComp.fileName;
        if (filename.length() > 20) {
            filename = filename.substr(0, 18) + "...";
        }
        ImGui::Text("%s", filename.c_str());
        ImGui::Text("%dx%d", imageComp.width, imageComp.height);

        std::string date = GetFileDate(imageComp.filePath);
        if (!date.empty()) ImGui::Text("%s", date.c_str());

        bool hasMeta = HasMetadata(imageComp.filePath);
        ImGui::TextColored(hasMeta ? ImVec4(0, 1, 0, 1) : ImVec4(0.6f, 0.6f, 0.6f, 1),
            hasMeta ? "Metadata: Yes" : "Metadata: No");

        if (entityID == selectedEntityID) {
            ImVec2 min = ImGui::GetItemRectMin();
            ImVec2 max = ImGui::GetItemRectMax();
            ImGui::GetWindowDrawList()->AddRect(min, max, IM_COL32(255, 255, 0, 128), 2.0f);
        }

        ImGui::EndGroup();
    }

    bool ImageHistoryView::HasMetadata(const std::string& filePath) {
        if (filePath.empty() || !std::filesystem::exists(filePath)) return false;
        return false;
    }

    std::string ImageHistoryView::GetFileDate(const std::string& filePath) {
        if (filePath.empty() || !std::filesystem::exists(filePath)) return "";
        try {
            auto ftime = std::filesystem::last_write_time(filePath);
            return FormatDate(ftime);
        }
        catch (...) {
            return "";
        }
    }

    std::string ImageHistoryView::FormatDate(const std::filesystem::file_time_type& ftime) {
        auto now = std::chrono::system_clock::now();
        auto diff = ftime - std::filesystem::file_time_type::clock::now();
        auto sys_time = now + std::chrono::duration_cast<std::chrono::system_clock::duration>(diff);
        std::time_t tt = std::chrono::system_clock::to_time_t(sys_time);
        std::tm tm = *std::localtime(&tt);
        char buffer[32];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &tm);
        return std::string(buffer);
    }

    void ImageHistoryView::RefreshEntities() {
        imageEntities.clear();
        if (!imageSystem) return;
        for (auto entityID : imageSystem->GetAllImageEntities()) {
            imageEntities.push_back(entityID);
        }
    }

    void ImageHistoryView::OnImageAdded(ECS::EntityID entity) {
        RefreshEntities();
    }

    void ImageHistoryView::OnImageRemoved(ECS::EntityID entity) {
        RefreshEntities();
        if (selectedEntityID == entity) {
            selectedEntityID = imageEntities.empty() ? 0 : imageEntities[0];
        }
    }

    void ImageHistoryView::SelectImage(ECS::EntityID entity) {
        selectedEntityID = entity;
        if (parentWorkspaceID != 0) {
            std::unordered_map<std::string, std::any> eventData;
            eventData["workspaceID"] = parentWorkspaceID;
            eventData["entityID"] = entity;
            ANI::Events::Ref().QueueEventWithData("SelectMediaEntity", eventData);
        }
    }

    void ImageHistoryView::SetParentViewWorkspace(WorkspaceID parentWorkspace) {
        parentWorkspaceID = parentWorkspace;
    }

} // namespace GUI