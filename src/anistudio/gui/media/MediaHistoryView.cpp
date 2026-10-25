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

#include <imgui.h>
#include <algorithm>
#include <chrono>

namespace GUI {

    static const ECS::ImageComponent* GetImageComponentAny(
        ECS::EntityManager& mgr, ECS::EntityID id) {
        if (mgr.HasComponent<ECS::ImageComponent>(id))
            return &mgr.GetComponent<ECS::ImageComponent>(id);
        if (mgr.HasComponent<ECS::PreviewImageComponent>(id))
            return &mgr.GetComponent<ECS::PreviewImageComponent>(id);
        if (mgr.HasComponent<ECS::InputImageComponent>(id))
            return &mgr.GetComponent<ECS::InputImageComponent>(id);
        if (mgr.HasComponent<ECS::OutputImageComponent>(id))
            return &mgr.GetComponent<ECS::OutputImageComponent>(id);
        if (mgr.HasComponent<ECS::MaskImageComponent>(id))
            return &mgr.GetComponent<ECS::MaskImageComponent>(id);
        if (mgr.HasComponent<ECS::EndImageComponent>(id))
            return &mgr.GetComponent<ECS::EndImageComponent>(id);
        return nullptr;
    }

    static const ECS::VideoComponent* GetVideoComponentAny(
        ECS::EntityManager& mgr, ECS::EntityID id) {
        if (mgr.HasComponent<ECS::VideoComponent>(id))
            return &mgr.GetComponent<ECS::VideoComponent>(id);
        if (mgr.HasComponent<ECS::PreviewVideoComponent>(id))
            return &mgr.GetComponent<ECS::PreviewVideoComponent>(id);
        if (mgr.HasComponent<ECS::InputVideoComponent>(id))
            return &mgr.GetComponent<ECS::InputVideoComponent>(id);
        if (mgr.HasComponent<ECS::OutputVideoComponent>(id))
            return &mgr.GetComponent<ECS::OutputVideoComponent>(id);
        return nullptr;
    }

    static const ECS::AudioComponent* GetAudioComponentAny(
        ECS::EntityManager& mgr, ECS::EntityID id) {
        if (mgr.HasComponent<ECS::AudioComponent>(id))
            return &mgr.GetComponent<ECS::AudioComponent>(id);
        if (mgr.HasComponent<ECS::InputAudioComponent>(id))
            return &mgr.GetComponent<ECS::InputAudioComponent>(id);
        if (mgr.HasComponent<ECS::OutputAudioComponent>(id))
            return &mgr.GetComponent<ECS::OutputAudioComponent>(id);
        return nullptr;
    }

    MediaHistoryView::MediaHistoryView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseView(mgr, vm) {
        viewName = "MediaHistoryView";
        contextMenuUtils = std::make_unique<Utils::ContextMenuUtils>(m_entityManager);
        ANI_LOG_DEBUG("Constructed");
    }

    MediaHistoryView::~MediaHistoryView() {
        if (imageSystem) imageSystem->UnregisterCallbacksForOwner(this);
        if (videoSystem) videoSystem->UnregisterCallbacksForOwner(this);
        if (audioSystem) audioSystem->UnregisterCallbacksForOwner(this);
        ANI_LOG_DEBUG("Destroyed");
    }

    void MediaHistoryView::Init() {
        imageSystem = m_entityManager.GetSystem<ECS::ImageSystem>();
        if (!imageSystem) {
            m_entityManager.RegisterSystem<ECS::ImageSystem>();
            imageSystem = m_entityManager.GetSystem<ECS::ImageSystem>();
        }
        if (imageSystem) {
            imageSystem->RegisterImageAddedCallback(this, [this](ECS::EntityID e) { OnMediaAdded(e); });
            imageSystem->RegisterImageRemovedCallback(this, [this](ECS::EntityID e) { OnMediaRemoved(e); });
        }

        videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
        if (!videoSystem) {
            m_entityManager.RegisterSystem<ECS::VideoSystem>();
            videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
        }
        if (videoSystem) {
            videoSystem->RegisterVideoAddedCallback(this, [this](ECS::EntityID e) { OnMediaAdded(e); });
            videoSystem->RegisterVideoRemovedCallback(this, [this](ECS::EntityID e) { OnMediaRemoved(e); });
        }

        audioSystem = m_entityManager.GetSystem<ECS::AudioSystem>();
        if (!audioSystem) {
            m_entityManager.RegisterSystem<ECS::AudioSystem>();
            audioSystem = m_entityManager.GetSystem<ECS::AudioSystem>();
        }
        if (audioSystem) {
            audioSystem->RegisterAudioAddedCallback(this, [this](ECS::EntityID e) { OnMediaAdded(e); });
            audioSystem->RegisterAudioRemovedCallback(this, [this](ECS::EntityID e) { OnMediaRemoved(e); });
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
                            if (m_entityManager.IsEntityValid(entity)) selectedEntityID = entity;
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

    void MediaHistoryView::Update(float deltaT) {}

    void MediaHistoryView::Render() {
        ImGui::Begin("Media History", &windowOpen, ImGuiWindowFlags_MenuBar);
        try {
            if (needsSort) {
                ApplyFiltersAndSort();
                needsSort = false;
            }
            RenderMenuBar();
            if (mediaEntities.empty()) {
                ImGui::Text("No media loaded.");
            }
            else {
                RenderMediaGrid();
            }
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("[MediaHistoryView] Render exception: %s", e.what());
        }
        catch (...) {
            ANI_LOG_ERROR("[MediaHistoryView] Render unknown exception");
        }
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
                if (ImGui::MenuItem("Refresh")) RefreshEntities();
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
            if (changed) needsSort = true;
        }
    }

    void MediaHistoryView::ApplyFiltersAndSort() {
        auto getInfo = [this](ECS::EntityID eid) -> ThumbnailFilters::MediaItemInfo {
            ThumbnailFilters::MediaItemInfo info;
            info.entityID = eid;
            info.categoryMask = ThumbnailFilters::CategoryMaskForEntity(m_entityManager, eid);
            if (info.categoryMask == 0) return info;
            if (!m_entityManager.IsEntityValid(eid)) return info;

            const auto* img = GetImageComponentAny(m_entityManager, eid);
            const auto* vid = GetVideoComponentAny(m_entityManager, eid);
            const auto* aud = GetAudioComponentAny(m_entityManager, eid);

            if (img) {
                if (img->width <= 0 || img->height <= 0 || img->imageData == nullptr) return info;
                info.fileName = img->fileName;
                info.filePath = img->filePath;
                info.fileSize = img->fileSize;
                info.dateTime = img->fileDate + " " + img->fileTime;
                info.channels = img->channels;
                info.hasMetadata = img->hasAniStudioMetadata;
                info.width = img->width;
                info.height = img->height;
                info.isImage = true;
            }
            else if (vid) {
                if (vid->width <= 0 || vid->height <= 0) return info;
                info.fileName = vid->fileName;
                info.filePath = vid->filePath;
                info.fileSize = vid->fileSize;
                info.dateTime = vid->fileDate + " " + vid->fileTime;
                info.channels = 4;
                info.hasMetadata = vid->hasAniStudioMetadata;
                info.width = vid->width;
                info.height = vid->height;
                info.duration = (vid->frameCount > 0) ? vid->frameCount / vid->fps : 0.0;
                info.fps = static_cast<float>(vid->fps);
                info.isVideo = true;
            }
            else if (aud) {
                if (aud->pcmData.empty()) return info;
                info.fileName = aud->fileName;
                info.filePath = aud->filePath;
                try { info.fileSize = std::filesystem::file_size(aud->filePath); }
                catch (...) { info.fileSize = 0; }
                info.dateTime = "";
                info.channels = aud->channels;
                info.sampleRate = aud->sampleRate;
                info.duration = aud->duration;
                info.hasMetadata = aud->hasAniStudioMetadata;
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
        if (filterSettings.displayMode == GUI::Thumbnail::DisplayMode::Compact) {
            itemWidth = thumbnailSizePx + spacing;
        }
        else if (filterSettings.displayMode == GUI::Thumbnail::DisplayMode::List) {
            itemWidth = availableWidth;
        }
        else {
            itemWidth = thumbnailSizePx + 160 + spacing;
        }

        int columns = std::max(1, static_cast<int>((availableWidth + spacing) / (itemWidth + spacing)));
        if (filterSettings.displayMode == GUI::Thumbnail::DisplayMode::List) columns = 1;

        int itemIndex = 0;
        for (size_t i = 0; i < mediaEntities.size(); ++i) {
            ECS::EntityID entityID = mediaEntities[i];
            if (!m_entityManager.IsEntityValid(entityID)) continue;

            const auto* img = GetImageComponentAny(m_entityManager, entityID);
            const auto* vid = GetVideoComponentAny(m_entityManager, entityID);
            const auto* aud = GetAudioComponentAny(m_entityManager, entityID);
            if (!img && !vid && !aud) continue;

            if (filterSettings.displayMode == GUI::Thumbnail::DisplayMode::List) {
                if (itemIndex > 0) ImGui::NewLine();
            }
            else {
                if (itemIndex > 0 && (itemIndex % columns) != 0) ImGui::SameLine(0, spacing);
            }

            std::variant<const ECS::ImageComponent*, const ECS::VideoComponent*, const ECS::AudioComponent*> compVariant;
            if (img) compVariant = img;
            else if (vid) compVariant = vid;
            else compVariant = aud;

            if (filterSettings.displayMode == GUI::Thumbnail::DisplayMode::List) {
                GUI::Thumbnail::RenderListRow(compVariant, entityID, i, thumbnailSizePx,
                    [this](ECS::EntityID id) { SelectMedia(id); },
                    contextMenuUtils.get(), true, selectedEntityID, &m_entityManager);
            }
            else {
                GUI::Thumbnail::RenderThumbnail(compVariant, entityID, i, thumbnailSizePx,
                    filterSettings.displayMode,
                    [this](ECS::EntityID id) { SelectMedia(id); },
                    contextMenuUtils.get(), true, selectedEntityID, &m_entityManager);
            }
            itemIndex++;
        }
    }

    void MediaHistoryView::RefreshEntities() {
        ANI_LOG_DEBUG("[MediaHistoryView] RefreshEntities begin");
        mediaEntities.clear();

        for (auto id : m_entityManager.GetAllEntities()) {
            if (!m_entityManager.IsEntityValid(id)) continue;
            const uint32_t category = ThumbnailFilters::CategoryMaskForEntity(m_entityManager, id);
            if (category == 0) continue;
            mediaEntities.push_back(id);
        }

        if (selectedEntityID != 0 && !m_entityManager.IsEntityValid(selectedEntityID)) {
            selectedEntityID = mediaEntities.empty() ? 0 : mediaEntities[0];
        }
        needsSort = true;
        ANI_LOG_DEBUG("[MediaHistoryView] RefreshEntities end count=%zu", mediaEntities.size());
    }

    void MediaHistoryView::OnMediaAdded(ECS::EntityID entity) {
        ANI_LOG_DEBUG("[MediaHistoryView] OnMediaAdded entity=%u begin", entity);
        RefreshEntities();
        ANI_LOG_DEBUG("[MediaHistoryView] OnMediaAdded entity=%u end", entity);
    }

    void MediaHistoryView::OnMediaRemoved(ECS::EntityID entity) {
        ANI_LOG_DEBUG("[MediaHistoryView] OnMediaRemoved entity=%u begin", entity);
        RefreshEntities();
        UpdateSelectedAfterRemoval(entity);
        ANI_LOG_DEBUG("[MediaHistoryView] OnMediaRemoved entity=%u end", entity);
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
            {"thumbnailSize", static_cast<int>(filterSettings.thumbnailSize)},
            {"categoryFilterMask", filterSettings.categoryFilterMask}
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
            if (fs.contains("categoryFilterMask")) filterSettings.categoryFilterMask = fs["categoryFilterMask"].get<uint32_t>();
            needsSort = true;
        }
    }

} // namespace GUI