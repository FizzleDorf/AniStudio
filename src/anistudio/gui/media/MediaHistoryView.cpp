// MediaHistoryView.cpp
#include "MediaHistoryView.hpp"
#include "Events.hpp"
#include "DragDropUtils.hpp"
#include "ContextMenuUtils.hpp"
#include "ViewManager.hpp"
#include "ImageSystem.hpp"
#include "VideoSystem.hpp"
#include "ImageUtils.hpp"
#include "ThumbnailFilters.hpp"
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

    void MediaHistoryView::Update(float deltaT) {
        static auto lastRefresh = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRefresh).count() > 1000) {
            RefreshEntities();
            lastRefresh = now;
        }
    }

    void MediaHistoryView::Render() {
        ImGui::Begin("Media History", &windowOpen, ImGuiWindowFlags_MenuBar);

        if (needsSort) {
            ApplyFiltersAndSort();
            needsSort = false;
        }

        RenderMenuBar();

        if (mediaEntities.empty()) {
            ImGui::Text("No media loaded.");
            ImGui::End();
            return;
        }

        RenderMediaGrid();

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
            bool changed = false;
            changed |= ThumbnailFilters::RenderViewMenu(filterSettings);
            changed |= ThumbnailFilters::RenderSortMenu(filterSettings);
            changed |= ThumbnailFilters::RenderFiltersMenu(filterSettings, false, false);

            if (ImGui::BeginMenu("Actions")) {
                if (ImGui::MenuItem("Refresh")) {
                    RefreshEntities();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
            if (changed) {
                needsSort = true;
            }
        }
    }

    void MediaHistoryView::ApplyFiltersAndSort() {
        auto getInfo = [this](ECS::EntityID eid) -> ThumbnailFilters::MediaItemInfo {
            ThumbnailFilters::MediaItemInfo info;
            info.entityID = eid;

            if (!m_entityManager.IsEntityValid(eid)) {
                return info;
            }

            if (m_entityManager.HasComponent<ECS::ImageComponent>(eid)) {
                const auto& comp = m_entityManager.GetComponent<ECS::ImageComponent>(eid);
                if (comp.width <= 0 || comp.height <= 0 || comp.imageData == nullptr) {
                    return info;
                }
                info.fileName = comp.fileName;
                info.filePath = comp.filePath;
                info.fileSize = comp.fileSize;
                info.dateTime = comp.fileDate + " " + comp.fileTime;
                info.channels = comp.channels;
                info.hasMetadata = comp.hasAniStudioMetadata;
                info.width = comp.width;
                info.height = comp.height;
                info.isImage = true;
            }
            else if (m_entityManager.HasComponent<ECS::VideoComponent>(eid)) {
                const auto& comp = m_entityManager.GetComponent<ECS::VideoComponent>(eid);
                if (comp.width <= 0 || comp.height <= 0) {
                    return info;
                }
                info.fileName = comp.fileName;
                info.filePath = comp.filePath;
                info.fileSize = comp.fileSize;
                info.dateTime = comp.fileDate + " " + comp.fileTime;
                info.channels = 4;
                info.hasMetadata = comp.hasAniStudioMetadata;
                info.width = comp.width;
                info.height = comp.height;
                info.duration = (comp.frameCount > 0) ? comp.frameCount / comp.fps : 0.0;
                info.fps = static_cast<float>(comp.fps);
                info.isVideo = true;
            }
            return info;
            };
        ThumbnailFilters::ApplyFiltersAndSort(mediaEntities, filterSettings, getInfo);
    }

    void MediaHistoryView::RenderMediaGrid() {
        float availableWidth = ImGui::GetContentRegionAvail().x;
        float thumbnailSizePx = GUI::Thumbnail::GetThumbnailSize(filterSettings.thumbnailSize);
        const float spacing = 12.0f;

        float itemWidth = 0.0f;
        float itemHeight = 0.0f;

        if (filterSettings.displayMode == GUI::Thumbnail::DisplayMode::Compact) {
            itemWidth = thumbnailSizePx + spacing;
            itemHeight = thumbnailSizePx + ImGui::GetFontSize() + spacing;
        }
        else if (filterSettings.displayMode == GUI::Thumbnail::DisplayMode::List) {
            itemWidth = availableWidth;
            itemHeight = thumbnailSizePx + 8.0f + spacing;
        }
        else {
            itemWidth = thumbnailSizePx + 160 + spacing;
            itemHeight = thumbnailSizePx + 60 + spacing;
        }

        int columns = std::max(1, static_cast<int>((availableWidth + spacing) / (itemWidth + spacing)));
        if (filterSettings.displayMode == GUI::Thumbnail::DisplayMode::List) {
            columns = 1;
        }

        int itemIndex = 0;

        if (filterSettings.displayMode == GUI::Thumbnail::DisplayMode::List) {
            GUI::Thumbnail::BeginListMode(thumbnailSizePx);
        }

        for (size_t i = 0; i < mediaEntities.size(); ++i) {
            ECS::EntityID entityID = mediaEntities[i];
            if (!m_entityManager.IsEntityValid(entityID)) continue;

            bool isImage = m_entityManager.HasComponent<ECS::ImageComponent>(entityID);
            bool isVideo = m_entityManager.HasComponent<ECS::VideoComponent>(entityID);
            if (!isImage && !isVideo) continue;

            if (filterSettings.displayMode != GUI::Thumbnail::DisplayMode::List) {
                if (itemIndex > 0 && (itemIndex % columns) != 0) {
                    ImGui::SameLine(0, spacing);
                }
                else if (itemIndex > 0) {
                    ImGui::NewLine();
                }
            }

            std::variant<const ECS::ImageComponent*, const ECS::VideoComponent*> compVariant;
            if (isImage) {
                compVariant = &m_entityManager.GetComponent<ECS::ImageComponent>(entityID);
            }
            else {
                compVariant = &m_entityManager.GetComponent<ECS::VideoComponent>(entityID);
            }

            if (filterSettings.displayMode == GUI::Thumbnail::DisplayMode::List) {
                GUI::Thumbnail::RenderListRow(
                    compVariant,
                    i,
                    thumbnailSizePx,
                    [this](ECS::EntityID id) { SelectMedia(id); },
                    contextMenuUtils.get(),
                    true,
                    selectedEntityID
                );
            }
            else {
                GUI::Thumbnail::RenderThumbnail(
                    compVariant,
                    i,
                    thumbnailSizePx,
                    filterSettings.displayMode,
                    [this](ECS::EntityID id) { SelectMedia(id); },
                    contextMenuUtils.get(),
                    true,
                    selectedEntityID
                );
            }

            itemIndex++;
        }

        if (filterSettings.displayMode == GUI::Thumbnail::DisplayMode::List) {
            GUI::Thumbnail::EndListMode();
        }

        ImGui::NewLine();
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
        needsSort = true;
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

    nlohmann::json MediaHistoryView::Serialize() const {
        nlohmann::json j = BaseView::Serialize();
        j["filterSettings"] = {
            {"mediaType", static_cast<int>(filterSettings.mediaType)},
            {"extensionFilter", filterSettings.extensionFilter},
            {"filterHasMetadata", filterSettings.filterHasMetadata},
            {"filterChannels", filterSettings.filterChannels},
            {"sortMode", static_cast<int>(filterSettings.sortMode)},
            {"sortAscending", filterSettings.sortAscending},
            {"displayMode", static_cast<int>(filterSettings.displayMode)},
            {"thumbnailSize", static_cast<int>(filterSettings.thumbnailSize)}
        };
        return j;
    }

    void MediaHistoryView::Deserialize(const nlohmann::json& j) {
        BaseView::Deserialize(j);
        if (j.contains("filterSettings")) {
            auto fs = j["filterSettings"];
            if (fs.contains("mediaType")) filterSettings.mediaType = static_cast<ThumbnailFilters::MediaTypeFilter>(fs["mediaType"].get<int>());
            if (fs.contains("extensionFilter")) filterSettings.extensionFilter = fs["extensionFilter"].get<std::string>();
            if (fs.contains("filterHasMetadata")) filterSettings.filterHasMetadata = fs["filterHasMetadata"].get<bool>();
            if (fs.contains("filterChannels")) filterSettings.filterChannels = fs["filterChannels"].get<int>();
            if (fs.contains("sortMode")) filterSettings.sortMode = static_cast<ThumbnailFilters::SortMode>(fs["sortMode"].get<int>());
            if (fs.contains("sortAscending")) filterSettings.sortAscending = fs["sortAscending"].get<bool>();
            if (fs.contains("displayMode")) filterSettings.displayMode = static_cast<GUI::Thumbnail::DisplayMode>(fs["displayMode"].get<int>());
            if (fs.contains("thumbnailSize")) filterSettings.thumbnailSize = static_cast<GUI::Thumbnail::ThumbnailSize>(fs["thumbnailSize"].get<int>());
            needsSort = true;
        }
    }

}