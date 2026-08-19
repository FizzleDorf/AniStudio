#include "VideoHistoryView.hpp"
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

    VideoHistoryView::VideoHistoryView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseView(mgr, vm), videoSystem(nullptr), parentWorkspaceID(0), selectedEntityID(0) {
        viewName = "VideoHistoryView";
    }

    void VideoHistoryView::Init() {
        videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
        if (!videoSystem) {
            m_entityManager.RegisterSystem<ECS::VideoSystem>();
            videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
        }
        if (videoSystem) {
            videoSystem->RegisterVideoAddedCallback([this](ECS::EntityID entity) { OnVideoAdded(entity); });
            videoSystem->RegisterVideoRemovedCallback([this](ECS::EntityID entity) { OnVideoRemoved(entity); });
        }
        RefreshEntities();
    }

    void VideoHistoryView::Update(float deltaT) {}

    void VideoHistoryView::Render() {
        ImGui::Begin("Video History", &windowOpen);
        if (videoEntities.empty()) {
            ImGui::Text("No videos loaded.");
            ImGui::End();
            return;
        }

        float availableWidth = ImGui::GetContentRegionAvail().x;
        float thumbnailSize = 150.0f;
        float spacing = 8.0f;
        float itemWidth = thumbnailSize + spacing;
        int columns = std::max(1, static_cast<int>(availableWidth / itemWidth));

        ImGui::Columns(columns, nullptr, false);

        for (size_t i = 0; i < videoEntities.size(); ++i) {
            RenderVideoThumbnail(videoEntities[i], i);
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

    void VideoHistoryView::RenderVideoThumbnail(ECS::EntityID entityID, size_t index) {
        if (!m_entityManager.IsEntityValid(entityID) || !m_entityManager.HasComponent<ECS::VideoComponent>(entityID))
            return;

        const auto& videoComp = m_entityManager.GetComponent<ECS::VideoComponent>(entityID);

        ImGui::BeginGroup();

        ImVec2 thumbSize(128.0f, 128.0f);
        if (videoComp.currentTexture != 0) {
            float aspect = (videoComp.height > 0) ? (float)videoComp.width / videoComp.height : 1.0f;
            ImVec2 size = thumbSize;
            if (aspect > 1.0f) size.y = thumbSize.x / aspect;
            else size.x = thumbSize.y * aspect;
            if (ImGui::ImageButton(("##vid" + std::to_string(index)).c_str(),
                (ImTextureID)(intptr_t)videoComp.currentTexture, size)) {
                SelectVideo(entityID);
            }
        }
        else {
            ImGui::Button(("Select##" + std::to_string(index)).c_str(), thumbSize);
            if (ImGui::IsItemClicked()) SelectVideo(entityID);
        }

        if (ImGui::BeginDragDropSource()) {
            nlohmann::json payload;
            payload["entityID"] = entityID;
            ImGui::SetDragDropPayload(GUI::DragDrop::PAYLOAD_ENTITY,
                payload.dump().c_str(), payload.dump().size() + 1);
            if (videoComp.currentTexture != 0) {
                ImGui::Image((ImTextureID)(intptr_t)videoComp.currentTexture, ImVec2(64, 64));
            }
            ImGui::Text("%s", videoComp.fileName.c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup(("VideoContextMenu##" + std::to_string(entityID)).c_str());
        }

        if (ImGui::BeginPopup(("VideoContextMenu##" + std::to_string(entityID)).c_str())) {
            Utils::ContextMenuUtils menuUtils(m_entityManager);
            menuUtils.RenderEntityContextMenu(entityID);
            ImGui::EndPopup();
        }

        std::string filename = videoComp.fileName;
        if (filename.length() > 20) {
            filename = filename.substr(0, 18) + "...";
        }
        ImGui::Text("%s", filename.c_str());
        ImGui::Text("%dx%d", videoComp.width, videoComp.height);
        ImGui::Text("%.1f fps", videoComp.fps);

        std::string date = GetFileDate(videoComp.filePath);
        if (!date.empty()) ImGui::Text("%s", date.c_str());

        if (entityID == selectedEntityID) {
            ImVec2 min = ImGui::GetItemRectMin();
            ImVec2 max = ImGui::GetItemRectMax();
            ImGui::GetWindowDrawList()->AddRect(min, max, IM_COL32(255, 255, 0, 128), 2.0f);
        }

        ImGui::EndGroup();
    }

    std::string VideoHistoryView::GetFileDate(const std::string& filePath) {
        if (filePath.empty() || !std::filesystem::exists(filePath)) return "";
        try {
            auto ftime = std::filesystem::last_write_time(filePath);
            return FormatDate(ftime);
        }
        catch (...) {
            return "";
        }
    }

    std::string VideoHistoryView::FormatDate(const std::filesystem::file_time_type& ftime) {
        auto now = std::chrono::system_clock::now();
        auto diff = ftime - std::filesystem::file_time_type::clock::now();
        auto sys_time = now + std::chrono::duration_cast<std::chrono::system_clock::duration>(diff);
        std::time_t tt = std::chrono::system_clock::to_time_t(sys_time);
        std::tm tm = *std::localtime(&tt);
        char buffer[32];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &tm);
        return std::string(buffer);
    }

    void VideoHistoryView::RefreshEntities() {
        videoEntities.clear();
        if (!videoSystem) return;
        for (auto entityID : videoSystem->GetAllVideoEntities()) {
            videoEntities.push_back(entityID);
        }
    }

    void VideoHistoryView::OnVideoAdded(ECS::EntityID entity) {
        RefreshEntities();
    }

    void VideoHistoryView::OnVideoRemoved(ECS::EntityID entity) {
        RefreshEntities();
        if (selectedEntityID == entity) {
            selectedEntityID = videoEntities.empty() ? 0 : videoEntities[0];
        }
    }

    void VideoHistoryView::SelectVideo(ECS::EntityID entity) {
        selectedEntityID = entity;
        if (parentWorkspaceID != 0) {
            std::unordered_map<std::string, std::any> eventData;
            eventData["workspaceID"] = parentWorkspaceID;
            eventData["entityID"] = entity;
            ANI::Events::Ref().QueueEventWithData("SelectMediaEntity", eventData);
        }
    }

    void VideoHistoryView::SetParentViewWorkspace(WorkspaceID parentWorkspace) {
        parentWorkspaceID = parentWorkspace;
    }

} // namespace GUI