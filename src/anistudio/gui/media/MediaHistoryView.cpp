#include "MediaHistoryView.hpp"
#include "Events.hpp"
#include "DragDropUtils.hpp"
#include "ContextMenuUtils.hpp"
#include "ViewManager.hpp"
#include "ImageSystem.hpp"
#include "VideoSystem.hpp"
#include <imgui.h>
#include <algorithm>
#include <chrono>

namespace GUI {

    MediaHistoryView::MediaHistoryView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseView(mgr, vm) {
        viewName = "MediaHistoryView";
        contextMenuUtils = std::make_unique<Utils::ContextMenuUtils>(m_entityManager);
    }

    void MediaHistoryView::Init() {
        imageSystem = m_entityManager.GetSystem<ECS::ImageSystem>();
        if (!imageSystem) {
            m_entityManager.RegisterSystem<ECS::ImageSystem>();
            imageSystem = m_entityManager.GetSystem<ECS::ImageSystem>();
        }
        if (imageSystem) {
            imageSystem->RegisterImageAddedCallback([this](ECS::EntityID entity) { OnMediaAdded(entity); });
            imageSystem->RegisterImageRemovedCallback([this](ECS::EntityID entity) { OnMediaRemoved(entity); });
        }

        videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
        if (!videoSystem) {
            m_entityManager.RegisterSystem<ECS::VideoSystem>();
            videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
        }
        if (videoSystem) {
            videoSystem->RegisterVideoAddedCallback([this](ECS::EntityID entity) { OnMediaAdded(entity); });
            videoSystem->RegisterVideoRemovedCallback([this](ECS::EntityID entity) { OnMediaRemoved(entity); });
        }

        RefreshEntities();

        ANI::Events::Ref().RegisterEventWithData("SelectMediaEntity", [this](const std::any& data) {
            try {
                auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
                auto it = eventData.find("workspaceID");
                if (it != eventData.end()) {
                    WorkspaceID wsID = std::any_cast<WorkspaceID>(it->second);
                    if (wsID == GetID()) {
                        auto entityIt = eventData.find("entityID");
                        if (entityIt != eventData.end()) {
                            ECS::EntityID entity = std::any_cast<ECS::EntityID>(entityIt->second);
                            if (m_entityManager.IsEntityValid(entity)) {
                                selectedEntityID = entity;
                            }
                        }
                    }
                }
            }
            catch (const std::exception& e) {
                std::cerr << "[MediaHistoryView] SelectMediaEntity event error: " << e.what() << std::endl;
            }
            });
    }

    void MediaHistoryView::Update(float deltaT) {}

    void MediaHistoryView::Render() {
        static auto lastRefresh = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRefresh).count() > 1000) {
            RefreshEntities();
            lastRefresh = now;
        }

        ImGui::Begin("Media History", &windowOpen, ImGuiWindowFlags_MenuBar);

        RenderMenuBar();

        if (mediaEntities.empty()) {
            ImGui::Text("No media loaded.");
            ImGui::End();
            return;
        }

        float availableWidth = ImGui::GetContentRegionAvail().x;
        const float thumbnailSize = 150.0f;
        float spacing = 8.0f;
        float itemWidth = (currentDisplayMode == Thumbnail::DisplayMode::Detailed) ? thumbnailSize + 170 : thumbnailSize + spacing;
        int columns = std::max(1, static_cast<int>(availableWidth / itemWidth));

        ImGui::Columns(columns, nullptr, false);

        for (size_t i = 0; i < mediaEntities.size(); ++i) {
            ECS::EntityID entityID = mediaEntities[i];
            if (!m_entityManager.IsEntityValid(entityID)) {
                ImGui::NextColumn();
                continue;
            }
            bool isImage = m_entityManager.HasComponent<ECS::ImageComponent>(entityID);
            bool isVideo = m_entityManager.HasComponent<ECS::VideoComponent>(entityID);
            if (!isImage && !isVideo) {
                ImGui::NextColumn();
                continue;
            }

            if (currentFilter == MediaFilter::Images && !isImage) {
                ImGui::NextColumn();
                continue;
            }
            if (currentFilter == MediaFilter::Videos && !isVideo) {
                ImGui::NextColumn();
                continue;
            }

            Thumbnail::ThumbnailData data = BuildThumbnailData(entityID);
            data.activeEntityID = selectedEntityID;
            Thumbnail::RenderThumbnail(
                data,
                i,
                thumbnailSize,
                currentDisplayMode,
                [this](ECS::EntityID id) { SelectMedia(id); },
                contextMenuUtils.get(),
                true
            );
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

    void MediaHistoryView::RenderMenuBar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("View")) {
                const char* modeItems[] = { "Compact", "Detailed" };
                int modeIdx = static_cast<int>(currentDisplayMode);
                if (ImGui::Combo("Display Mode", &modeIdx, modeItems, IM_ARRAYSIZE(modeItems))) {
                    currentDisplayMode = static_cast<Thumbnail::DisplayMode>(modeIdx);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Filters")) {
                const char* filterItems[] = { "All", "Images", "Videos" };
                int filterIdx = static_cast<int>(currentFilter);
                if (ImGui::Combo("Media Type", &filterIdx, filterItems, IM_ARRAYSIZE(filterItems))) {
                    currentFilter = static_cast<MediaFilter>(filterIdx);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Actions")) {
                if (ImGui::MenuItem("Refresh")) {
                    RefreshEntities();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
    }

    Thumbnail::ThumbnailData MediaHistoryView::BuildThumbnailData(ECS::EntityID entityID) {
        Thumbnail::ThumbnailData data;
        data.entityID = entityID;

        if (m_entityManager.HasComponent<ECS::VideoComponent>(entityID)) {
            const auto& comp = m_entityManager.GetComponent<ECS::VideoComponent>(entityID);
            data.filePath = comp.filePath;
            data.fileName = comp.fileName;
            data.textureID = comp.currentTexture;
            data.width = comp.width;
            data.height = comp.height;
            data.fps = static_cast<float>(comp.fps);
            data.isVideo = true;
            data.channels = 4;
        }
        else if (m_entityManager.HasComponent<ECS::ImageComponent>(entityID)) {
            const auto& comp = m_entityManager.GetComponent<ECS::ImageComponent>(entityID);
            data.filePath = comp.filePath;
            data.fileName = comp.fileName;
            data.textureID = comp.textureID;
            data.width = comp.width;
            data.height = comp.height;
            data.channels = comp.channels;
            data.hasExif = comp.hasExifData;
            data.hasLSB = comp.hasLSBData;
            data.isVideo = false;
        }

        try {
            data.fileSize = std::filesystem::file_size(data.filePath);
        }
        catch (...) { data.fileSize = 0; }

        try {
            auto ftime = std::filesystem::last_write_time(data.filePath);
            auto now = std::chrono::system_clock::now();
            auto diff = ftime - std::filesystem::file_time_type::clock::now();
            auto sys_time = now + std::chrono::duration_cast<std::chrono::system_clock::duration>(diff);
            std::time_t tt = std::chrono::system_clock::to_time_t(sys_time);
            std::tm tm = *std::localtime(&tt);
            char buffer[32];
            strftime(buffer, sizeof(buffer), "%Y-%m-%d", &tm);
            data.fileDate = buffer;
            strftime(buffer, sizeof(buffer), "%H:%M:%S", &tm);
            data.fileTime = buffer;
        }
        catch (...) {}

        return data;
    }

    void MediaHistoryView::RefreshEntities() {
        mediaEntities.clear();

        if (imageSystem) {
            for (auto id : imageSystem->GetAllImageEntities()) {
                if (m_entityManager.IsEntityValid(id)) {
                    if (std::find(mediaEntities.begin(), mediaEntities.end(), id) == mediaEntities.end()) {
                        mediaEntities.push_back(id);
                    }
                }
            }
        }

        if (videoSystem) {
            for (auto id : videoSystem->GetAllVideoEntities()) {
                if (m_entityManager.IsEntityValid(id)) {
                    if (std::find(mediaEntities.begin(), mediaEntities.end(), id) == mediaEntities.end()) {
                        mediaEntities.push_back(id);
                    }
                }
            }
        }

        if (selectedEntityID != 0 && !m_entityManager.IsEntityValid(selectedEntityID)) {
            selectedEntityID = mediaEntities.empty() ? 0 : mediaEntities[0];
        }
    }

    void MediaHistoryView::OnMediaAdded(ECS::EntityID entity) {
        RefreshEntities();
    }

    void MediaHistoryView::OnMediaRemoved(ECS::EntityID entity) {
        RefreshEntities();
        UpdateSelectedAfterRemoval(entity);
    }

    void MediaHistoryView::UpdateSelectedAfterRemoval(ECS::EntityID removedEntity) {
        if (selectedEntityID == removedEntity) {
            selectedEntityID = mediaEntities.empty() ? 0 : mediaEntities[0];
        }
    }

    void MediaHistoryView::SelectMedia(ECS::EntityID entityID) {
        if (entityID == 0 || !m_entityManager.IsEntityValid(entityID)) return;
        selectedEntityID = entityID;

        std::unordered_map<std::string, std::any> eventData;
        eventData["workspaceID"] = GetID();
        eventData["entityID"] = entityID;
        ANI::Events::Ref().QueueEventWithData("SelectMediaEntity", eventData);
    }

}