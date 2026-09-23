// MediaHistoryView.cpp
#include "Log.hpp"
#include "MediaHistoryView.hpp"
#include "Events.hpp"
#include "DragDropUtils.hpp"
#include "ContextMenuUtils.hpp"
#include "ViewManager.hpp"
#include "ImageSystem.hpp"
#include "VideoSystem.hpp"
#include "AudioSystem.hpp"
#include "ImageUtils.hpp"
#include "ThumbnailFilters.hpp"
#include "Log.hpp"

#include <imgui.h>
#include <algorithm>
#include <chrono>

namespace GUI {

    MediaHistoryView::MediaHistoryView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseView(mgr, vm) {
        viewName = "MediaHistoryView";
        contextMenuUtils = std::make_unique<Utils::ContextMenuUtils>(m_entityManager);
        ANI_LOG_DEBUG("Constructed");
    }

    MediaHistoryView::~MediaHistoryView() {
        if (imageSystem) {
            imageSystem->UnregisterCallbacksForOwner(this);
        }
        if (videoSystem) {
            videoSystem->UnregisterCallbacksForOwner(this);
        }
        if (audioSystem) {
            audioSystem->UnregisterCallbacksForOwner(this);
        }
        ANI_LOG_DEBUG("Destroyed");
    }

    void MediaHistoryView::Init() {
        imageSystem = m_entityManager.GetSystem<ECS::ImageSystem>();
        if (!imageSystem) {
            m_entityManager.RegisterSystem<ECS::ImageSystem>();
            imageSystem = m_entityManager.GetSystem<ECS::ImageSystem>();
            if (imageSystem) {
                ANI_LOG_DEBUG("Registered ImageSystem");
            }
            else {
                ANI_LOG_ERROR("Failed to register ImageSystem");
            }
        }
        if (imageSystem) {
            imageSystem->RegisterImageAddedCallback(this, [this](ECS::EntityID entity) { OnMediaAdded(entity); });
            imageSystem->RegisterImageRemovedCallback(this, [this](ECS::EntityID entity) { OnMediaRemoved(entity); });
        }

        videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
        if (!videoSystem) {
            m_entityManager.RegisterSystem<ECS::VideoSystem>();
            videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
            if (videoSystem) {
                ANI_LOG_DEBUG("Registered VideoSystem");
            }
            else {
                ANI_LOG_ERROR("Failed to register VideoSystem");
            }
        }
        if (videoSystem) {
            videoSystem->RegisterVideoAddedCallback(this, [this](ECS::EntityID entity) { OnMediaAdded(entity); });
            videoSystem->RegisterVideoRemovedCallback(this, [this](ECS::EntityID entity) { OnMediaRemoved(entity); });
        }

        audioSystem = m_entityManager.GetSystem<ECS::AudioSystem>();
        if (!audioSystem) {
            m_entityManager.RegisterSystem<ECS::AudioSystem>();
            audioSystem = m_entityManager.GetSystem<ECS::AudioSystem>();
            if (audioSystem) {
                ANI_LOG_DEBUG("Registered AudioSystem");
            }
            else {
                ANI_LOG_ERROR("Failed to register AudioSystem");
            }
        }
        if (audioSystem) {
            audioSystem->RegisterAudioAddedCallback(this, [this](ECS::EntityID entity) { OnMediaAdded(entity); });
            audioSystem->RegisterAudioRemovedCallback(this, [this](ECS::EntityID entity) { OnMediaRemoved(entity); });
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
                ANI_LOG_ERROR("SelectMediaEntity event error: %s", e.what());
            }
            });

        ANI_LOG_DEBUG("Initialized");
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
            changed |= ThumbnailFilters::RenderFiltersMenu(filterSettings, true, true);

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
                ANI_LOG_TRACE("ApplyFiltersAndSort: entity %u invalid", eid);
                return info;
            }

            if (m_entityManager.HasComponent<ECS::ImageComponent>(eid)) {
                const auto& comp = m_entityManager.GetComponent<ECS::ImageComponent>(eid);
                if (comp.width <= 0 || comp.height <= 0 || comp.imageData == nullptr) {
                    ANI_LOG_TRACE("ApplyFiltersAndSort: entity %u image has no dimensions or data", eid);
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
                    ANI_LOG_TRACE("ApplyFiltersAndSort: entity %u video has no dimensions", eid);
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
            else if (m_entityManager.HasComponent<ECS::AudioComponent>(eid)) {
                const auto& comp = m_entityManager.GetComponent<ECS::AudioComponent>(eid);
                if (comp.pcmData.empty()) {
                    ANI_LOG_TRACE("ApplyFiltersAndSort: entity %u audio has no PCM data", eid);
                    return info;
                }
                info.fileName = comp.fileName;
                info.filePath = comp.filePath;
                info.fileSize = std::filesystem::file_size(comp.filePath);
                info.dateTime = "";
                info.channels = comp.channels;
                info.sampleRate = comp.sampleRate;
                info.duration = comp.duration;
                info.hasMetadata = comp.hasAniStudioMetadata;
                info.isAudio = true;
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
            bool isAudio = m_entityManager.HasComponent<ECS::AudioComponent>(entityID);
            if (!isImage && !isVideo && !isAudio) continue;

            if (filterSettings.displayMode != GUI::Thumbnail::DisplayMode::List) {
                if (itemIndex > 0 && (itemIndex % columns) != 0) {
                    ImGui::SameLine(0, spacing);
                }
                else if (itemIndex > 0) {
                    ImGui::NewLine();
                }
            }

            std::variant<const ECS::ImageComponent*, const ECS::VideoComponent*, const ECS::AudioComponent*> compVariant;
            if (isImage) {
                compVariant = &m_entityManager.GetComponent<ECS::ImageComponent>(entityID);
            }
            else if (isVideo) {
                compVariant = &m_entityManager.GetComponent<ECS::VideoComponent>(entityID);
            }
            else {
                compVariant = &m_entityManager.GetComponent<ECS::AudioComponent>(entityID);
            }

            if (filterSettings.displayMode == GUI::Thumbnail::DisplayMode::List) {
                GUI::Thumbnail::RenderListRow(
                    compVariant,
                    i,
                    thumbnailSizePx,
                    [this](ECS::EntityID id) { SelectMedia(id); },
                    contextMenuUtils.get(),
                    true,
                    selectedEntityID,
                    &m_entityManager
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
                    selectedEntityID,
                    &m_entityManager
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
                if (!m_entityManager.IsEntityValid(id)) continue;
                if (m_entityManager.HasComponent<ECS::PreviewImageComponent>(id)) continue;
                if (std::find(mediaEntities.begin(), mediaEntities.end(), id) == mediaEntities.end()) {
                    mediaEntities.push_back(id);
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

        if (audioSystem) {
            for (auto id : audioSystem->GetAllAudioEntities()) {
                if (m_entityManager.IsEntityValid(id)) {
                    if (std::find(mediaEntities.begin(), mediaEntities.end(), id) == mediaEntities.end()) {
                        mediaEntities.push_back(id);
                    }
                }
            }
        }

        if (selectedEntityID != 0 && !m_entityManager.IsEntityValid(selectedEntityID)) {
            selectedEntityID = mediaEntities.empty() ? 0 : mediaEntities[0];
            ANI_LOG_TRACE("RefreshEntities: selection reset to %u", selectedEntityID);
        }
        needsSort = true;

       //  ANI_LOG_TRACE("RefreshEntities: %zu media entities", mediaEntities.size());
    }

    void MediaHistoryView::OnMediaAdded(ECS::EntityID entity) {
        ANI_LOG_TRACE("Media added: entity %u", entity);
        RefreshEntities();
    }

    void MediaHistoryView::OnMediaRemoved(ECS::EntityID entity) {
        ANI_LOG_TRACE("Media removed: entity %u", entity);
        RefreshEntities();
        UpdateSelectedAfterRemoval(entity);
    }

    void MediaHistoryView::UpdateSelectedAfterRemoval(ECS::EntityID removedEntity) {
        if (selectedEntityID == removedEntity) {
            selectedEntityID = mediaEntities.empty() ? 0 : mediaEntities[0];
            ANI_LOG_TRACE("Selection moved to %u after removal of %u",
                selectedEntityID, removedEntity);
        }
    }

    void MediaHistoryView::SelectMedia(ECS::EntityID entityID) {
        if (entityID == 0 || !m_entityManager.IsEntityValid(entityID)) {
            ANI_LOG_TRACE("SelectMedia: invalid entity %u", entityID);
            return;
        }
        selectedEntityID = entityID;

        std::unordered_map<std::string, std::any> eventData;
        eventData["workspaceID"] = GetID();
        eventData["entityID"] = entityID;
        ANI::Events::Ref().QueueEventWithData("SelectMediaEntity", eventData);

        ANI_LOG_TRACE("Selected entity %u", entityID);
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

} // namespace GUI