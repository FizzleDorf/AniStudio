// AssetsView.cpp
#include "AssetsView.hpp"
#include "FilePathSystem.hpp"
#include "FileDialogUtil.hpp"
#include "Events.hpp"
#include "FileFormats.hpp"
#include "DragDropUtils.hpp"
#include "ImageSystem.hpp"
#include "VideoSystem.hpp"
#include "AudioSystem.hpp"
#include "TextureSystem.hpp"
#include "ImageUtils.hpp"
#include "VideoMetadataUtils.hpp"
#include "ThumbnailFilters.hpp"
#include "AudioComponent.hpp"
#include "Log.hpp"
#include <imgui.h>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <fstream>

namespace GUI {

    AssetsView::AssetsView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseView(mgr, vm), needsRefresh(true) {
        viewName = "AssetsView";
        contextMenuUtils = std::make_unique<Utils::ContextMenuUtils>(m_entityManager);
        ANI_LOG_DEBUG("Constructed");
    }

    void AssetsView::Init() {
        auto fileSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
        if (fileSys) {
            assetsPath = fileSys->GetPath("ProjectAssets");
            if (assetsPath.empty()) {
                ANI_LOG_DEBUG("ProjectAssets path empty, falling back to ./assets");
                assetsPath = "./assets";
            }
            else {
                ANI_LOG_DEBUG("Assets path from FilePathSystem: %s", assetsPath.c_str());
            }
        }
        else {
            ANI_LOG_WARN("FilePathSystem not available, falling back to ./assets");
            assetsPath = "./assets";
        }
        RefreshAssets();

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
    }

    void AssetsView::Update(float deltaT) {
        if (needsRefresh) {
            RefreshAssets();
            needsRefresh = false;
        }
    }

    void AssetsView::Render() {
        ImGui::Begin("Assets", &windowOpen, ImGuiWindowFlags_MenuBar);

        if (needsSort) {
            ApplyFiltersAndSort();
            needsSort = false;
        }

        RenderMenuBar();

        ImGui::Separator();

        if (assetFiles.empty()) {
            ImGui::Text("No assets found in directory.");
        }
        else {
            RenderAssetGrid();
        }

        ImGui::End();

        if (!windowOpen) {
            std::unordered_map<std::string, std::any> eventData;
            eventData["workspaceID"] = GetID();
            eventData["viewTypeName"] = viewName;
            ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
        }
    }

    void AssetsView::RenderMenuBar() {
        if (ImGui::BeginMenuBar()) {
            bool changed = false;
            changed |= ThumbnailFilters::RenderViewMenu(filterSettings);
            changed |= ThumbnailFilters::RenderSortMenu(filterSettings);
            changed |= ThumbnailFilters::RenderFiltersMenu(filterSettings, true, true);

            if (ImGui::BeginMenu("Actions")) {
                if (ImGui::MenuItem("Refresh")) {
                    RefreshAssets();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
            if (changed) {
                needsSort = true;
            }
        }
    }

    void AssetsView::RefreshAssets() {
        assetFiles.clear();
        if (assetsPath.empty() || !std::filesystem::exists(assetsPath)) {
            ANI_LOG_WARN("Assets path does not exist: %s", assetsPath.c_str());
            return;
        }
        try {
            const auto& formats = FileFormats::GetAllFormats();
            for (const auto& entry : std::filesystem::directory_iterator(assetsPath)) {
                if (entry.is_regular_file()) {
                    auto ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if (formats.find(ext) != formats.end()) {
                        assetFiles.push_back(entry.path());
                    }
                }
            }
            ANI_LOG_DEBUG("Found %zu asset file(s) in %s",
                assetFiles.size(), assetsPath.c_str());
            LoadNewAssets();
            needsSort = true;
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("Error scanning assets: %s", e.what());
        }
    }

    void AssetsView::ApplyFiltersAndSort() {
        auto getInfo = [this](const std::filesystem::path& path) -> ThumbnailFilters::MediaItemInfo {
            ThumbnailFilters::MediaItemInfo info;
            info.filePath = path.string();
            info.fileName = path.filename().string();
            std::string ext = ThumbnailFilters::GetExtension(info.filePath);
            auto& formats = FileFormats::GetAllFormats();
            auto it = formats.find(ext);
            if (it != formats.end()) {
                info.isImage = it->second.isImage;
                info.isVideo = it->second.isVideo;
                info.isAudio = it->second.isAudio;
            }
            auto entityIt = pathToEntity.find(info.filePath);
            if (entityIt != pathToEntity.end() && m_entityManager.IsEntityValid(entityIt->second)) {
                ECS::EntityID eid = entityIt->second;
                info.entityID = eid;
                if (m_entityManager.HasComponent<ECS::ImageComponent>(eid)) {
                    const auto& comp = m_entityManager.GetComponent<ECS::ImageComponent>(eid);
                    info.fileSize = comp.fileSize;
                    info.dateTime = comp.fileDate + " " + comp.fileTime;
                    info.channels = comp.channels;
                    info.hasMetadata = comp.hasAniStudioMetadata;
                    info.width = comp.width;
                    info.height = comp.height;
                }
                else if (m_entityManager.HasComponent<ECS::VideoComponent>(eid)) {
                    const auto& comp = m_entityManager.GetComponent<ECS::VideoComponent>(eid);
                    info.fileSize = comp.fileSize;
                    info.dateTime = comp.fileDate + " " + comp.fileTime;
                    info.channels = 4;
                    info.hasMetadata = comp.hasAniStudioMetadata;
                    info.width = comp.width;
                    info.height = comp.height;
                    info.duration = (comp.frameCount > 0) ? comp.frameCount / comp.fps : 0.0;
                    info.fps = static_cast<float>(comp.fps);
                }
                else if (m_entityManager.HasComponent<ECS::AudioComponent>(eid)) {
                    const auto& comp = m_entityManager.GetComponent<ECS::AudioComponent>(eid);
                    info.fileSize = std::filesystem::file_size(info.filePath);
                    info.channels = comp.channels;
                    info.sampleRate = comp.sampleRate;
                    info.duration = comp.duration;
                    info.hasMetadata = comp.hasAniStudioMetadata;
                }
            }
            else {
                try {
                    info.fileSize = std::filesystem::file_size(info.filePath);
                    auto ftime = std::filesystem::last_write_time(info.filePath);
                    auto s = std::chrono::duration_cast<std::chrono::seconds>(
                        ftime.time_since_epoch()).count();
                    std::time_t t = static_cast<std::time_t>(s);
                    std::tm tm;
#ifdef _WIN32
                    gmtime_s(&tm, &t);
#else
                    gmtime_r(&t, &tm);
#endif
                    char buf[32];
                    strftime(buf, sizeof(buf), "%Y:%m:%d %H:%M:%S", &tm);
                    info.dateTime = buf;
                }
                catch (...) {}
            }
            return info;
            };
        ThumbnailFilters::ApplyFiltersAndSort(assetFiles, filterSettings, getInfo);
    }

    void AssetsView::LoadNewAssets() {
        for (const auto& path : assetFiles) {
            std::string pathStr = path.string();
            if (loadedPaths.find(pathStr) == loadedPaths.end()) {
                ANI_LOG_TRACE("Loading new asset: %s", pathStr.c_str());
                LoadAsset(path);
            }
        }
    }

    void AssetsView::RenderAssetGrid() {
        float windowWidth = ImGui::GetContentRegionAvail().x;
        float thumbnailSizePx = GUI::Thumbnail::GetThumbnailSize(filterSettings.thumbnailSize);
        const float spacing = 12.0f;

        float itemWidth = 0.0f;
        float itemHeight = 0.0f;

        if (filterSettings.displayMode == GUI::Thumbnail::DisplayMode::Compact) {
            itemWidth = thumbnailSizePx + spacing;
            itemHeight = thumbnailSizePx + ImGui::GetFontSize() + spacing;
        }
        else if (filterSettings.displayMode == GUI::Thumbnail::DisplayMode::List) {
            itemWidth = windowWidth;
            itemHeight = thumbnailSizePx + 8.0f + spacing;
        }
        else {
            itemWidth = thumbnailSizePx + 160 + spacing;
            itemHeight = thumbnailSizePx + 60 + spacing;
        }

        int columns = std::max(1, static_cast<int>((windowWidth + spacing) / (itemWidth + spacing)));
        if (filterSettings.displayMode == GUI::Thumbnail::DisplayMode::List) {
            columns = 1;
        }

        int itemIndex = 0;

        if (filterSettings.displayMode == GUI::Thumbnail::DisplayMode::List) {
            GUI::Thumbnail::BeginListMode(thumbnailSizePx);
        }

        for (size_t i = 0; i < assetFiles.size(); ++i) {
            const auto& path = assetFiles[i];

            ECS::EntityID entityID = 0;
            auto it = pathToEntity.find(path.string());
            if (it != pathToEntity.end() && m_entityManager.IsEntityValid(it->second)) {
                entityID = it->second;
            }

            if (filterSettings.displayMode != GUI::Thumbnail::DisplayMode::List) {
                if (itemIndex > 0 && (itemIndex % columns) != 0) {
                    ImGui::SameLine(0, spacing);
                }
                else if (itemIndex > 0) {
                    ImGui::NewLine();
                }
            }

            std::variant<const ECS::ImageComponent*, const ECS::VideoComponent*, const ECS::AudioComponent*> compVariant;
            bool hasComponent = false;
            if (entityID != 0 && m_entityManager.IsEntityValid(entityID)) {
                if (m_entityManager.HasComponent<ECS::ImageComponent>(entityID)) {
                    compVariant = &m_entityManager.GetComponent<ECS::ImageComponent>(entityID);
                    hasComponent = true;
                }
                else if (m_entityManager.HasComponent<ECS::VideoComponent>(entityID)) {
                    compVariant = &m_entityManager.GetComponent<ECS::VideoComponent>(entityID);
                    hasComponent = true;
                }
                else if (m_entityManager.HasComponent<ECS::AudioComponent>(entityID)) {
                    compVariant = &m_entityManager.GetComponent<ECS::AudioComponent>(entityID);
                    hasComponent = true;
                }
            }

            if (hasComponent) {
                if (filterSettings.displayMode == GUI::Thumbnail::DisplayMode::List) {
                    GUI::Thumbnail::RenderListRow(
                        compVariant,
                        i,
                        thumbnailSizePx,
                        [this](ECS::EntityID id) { SelectAssetEntity(id); },
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
                        [this](ECS::EntityID id) { SelectAssetEntity(id); },
                        contextMenuUtils.get(),
                        true,
                        selectedEntityID
                    );
                }
            }

            itemIndex++;
        }

        if (filterSettings.displayMode == GUI::Thumbnail::DisplayMode::List) {
            GUI::Thumbnail::EndListMode();
        }

        ImGui::NewLine();

        std::vector<std::string> droppedFiles;
        if (GUI::DragDrop::AcceptFileDrop(droppedFiles)) {
            for (const auto& f : droppedFiles) {
                CopyFileToAssets(f);
            }
            needsRefresh = true;
        }

        ECS::EntityID droppedEntity;
        if (GUI::DragDrop::AcceptEntityDrop(droppedEntity)) {
            if (m_entityManager.IsEntityValid(droppedEntity)) {
                std::string filePath;
                if (m_entityManager.HasComponent<ECS::ImageComponent>(droppedEntity)) {
                    filePath = m_entityManager.GetComponent<ECS::ImageComponent>(droppedEntity).filePath;
                }
                else if (m_entityManager.HasComponent<ECS::VideoComponent>(droppedEntity)) {
                    filePath = m_entityManager.GetComponent<ECS::VideoComponent>(droppedEntity).filePath;
                }
                else if (m_entityManager.HasComponent<ECS::AudioComponent>(droppedEntity)) {
                    filePath = m_entityManager.GetComponent<ECS::AudioComponent>(droppedEntity).filePath;
                }
                if (!filePath.empty() && std::filesystem::exists(filePath)) {
                    if (CopyFileToAssets(filePath)) {
                        needsRefresh = true;
                    }
                }
            }
        }
    }

    void AssetsView::LoadAsset(const std::filesystem::path& path) {
        std::string filePath = path.string();
        if (loadedPaths.find(filePath) != loadedPaths.end()) {
            return;
        }

        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        const auto& formats = FileFormats::GetAllFormats();
        auto it = formats.find(ext);
        if (it == formats.end()) {
            ANI_LOG_WARN("Unsupported file type: %s", ext.c_str());
            return;
        }

        try {
            if (it->second.isImage) {
                auto imageSystem = m_entityManager.GetSystem<ECS::ImageSystem>();
                if (!imageSystem) {
                    m_entityManager.RegisterSystem<ECS::ImageSystem>();
                    imageSystem = m_entityManager.GetSystem<ECS::ImageSystem>();
                }

                if (imageSystem) {
                    ECS::EntityID entity = m_entityManager.AddNewEntity();
                    m_entityManager.AddComponent<ECS::ImageComponent>(entity);
                    imageSystem->SetImage(entity, filePath);
                    loadedEntities.push_back(entity);
                    pathToEntity[filePath] = entity;
                    loadedPaths.insert(filePath);
                    ANI_LOG_INFO("Loaded image: %s (Entity: %u)",
                        filePath.c_str(), entity);
                }
                else {
                    ANI_LOG_WARN("ImageSystem unavailable, skipping image: %s",
                        filePath.c_str());
                }
            }
            else if (it->second.isVideo) {
                auto videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
                if (!videoSystem) {
                    m_entityManager.RegisterSystem<ECS::VideoSystem>();
                    videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
                }

                if (videoSystem) {
                    ECS::EntityID entity = m_entityManager.AddNewEntity();
                    m_entityManager.AddComponent<ECS::VideoComponent>(entity);
                    videoSystem->LoadVideo(entity, filePath);
                    loadedEntities.push_back(entity);
                    pathToEntity[filePath] = entity;
                    loadedPaths.insert(filePath);
                    ANI_LOG_INFO("Loaded video: %s (Entity: %u)",
                        filePath.c_str(), entity);
                }
                else {
                    ANI_LOG_WARN("VideoSystem unavailable, skipping video: %s",
                        filePath.c_str());
                }
            }
            else if (it->second.isAudio) {
                auto audioSystem = m_entityManager.GetSystem<ECS::AudioSystem>();
                if (!audioSystem) {
                    m_entityManager.RegisterSystem<ECS::AudioSystem>();
                    audioSystem = m_entityManager.GetSystem<ECS::AudioSystem>();
                }

                if (audioSystem) {
                    ECS::EntityID entity = m_entityManager.AddNewEntity();
                    m_entityManager.AddComponent<ECS::AudioComponent>(entity);
                    audioSystem->LoadAudio(entity, filePath);
                    loadedEntities.push_back(entity);
                    pathToEntity[filePath] = entity;
                    loadedPaths.insert(filePath);
                    ANI_LOG_INFO("Loaded audio: %s (Entity: %u)",
                        filePath.c_str(), entity);
                }
                else {
                    ANI_LOG_WARN("AudioSystem unavailable, skipping audio: %s",
                        filePath.c_str());
                }
            }
            else {
                ANI_LOG_WARN("Unsupported file type: %s", ext.c_str());
            }
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("Error loading asset %s: %s",
                filePath.c_str(), e.what());
        }
    }

    void AssetsView::ClearLoadedAssets() {
        size_t clearedCount = 0;
        for (ECS::EntityID entity : loadedEntities) {
            if (m_entityManager.IsEntityValid(entity)) {
                if (m_entityManager.HasComponent<ECS::ImageComponent>(entity)) {
                    auto imageSystem = m_entityManager.GetSystem<ECS::ImageSystem>();
                    if (imageSystem) {
                        imageSystem->RemoveImage(entity);
                    }
                }
                else if (m_entityManager.HasComponent<ECS::VideoComponent>(entity)) {
                    auto videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
                    if (videoSystem) {
                        videoSystem->RemoveVideo(entity);
                    }
                }
                else if (m_entityManager.HasComponent<ECS::AudioComponent>(entity)) {
                    auto audioSystem = m_entityManager.GetSystem<ECS::AudioSystem>();
                    if (audioSystem) {
                        audioSystem->RemoveAudio(entity);
                    }
                }
                else {
                    m_entityManager.DestroyEntity(entity);
                }
                clearedCount++;
            }
        }
        loadedEntities.clear();
        pathToEntity.clear();
        loadedPaths.clear();
        ANI_LOG_INFO("Cleared %zu loaded asset(s)", clearedCount);
        needsRefresh = true;
    }

    bool AssetsView::CopyFileToAssets(const std::string& sourcePath) {
        std::filesystem::path src(sourcePath);
        std::filesystem::path dst = std::filesystem::path(assetsPath) / src.filename();
        if (std::filesystem::exists(dst)) {
            ANI_LOG_WARN("File already exists in assets: %s", dst.string().c_str());
            return false;
        }
        try {
            std::filesystem::copy(src, dst, std::filesystem::copy_options::overwrite_existing);
            ANI_LOG_DEBUG("Copied %s to %s",
                sourcePath.c_str(), dst.string().c_str());
            return true;
        }
        catch (const std::exception& e) {
            ANI_LOG_WARN("Failed to copy file: %s", e.what());
            return false;
        }
    }

    void AssetsView::CopyMetadataFromFile(const std::string& filePath) {
        nlohmann::json metadata;
        std::string ext = std::filesystem::path(filePath).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".webp") {
            metadata = Utils::ImageUtils::ReadMetadataFromImage(filePath);
        }
        else if (ext == ".mp4" || ext == ".webm" || ext == ".mkv" || ext == ".avi" || ext == ".mov") {
            metadata = Utils::VideoMetadataUtils::ReadMetadataFromVideo(filePath);
        }

        if (metadata.is_null() || metadata.empty()) {
            ANI_LOG_WARN("No metadata found in %s", filePath.c_str());
            return;
        }

        nlohmann::json wrapped;
        wrapped["dataType"] = "entity";
        wrapped["data"] = metadata;
        wrapped["source"] = "metadata";
        std::string jsonStr = wrapped.dump();
        ImGui::SetClipboardText(jsonStr.c_str());
        ANI_LOG_DEBUG(" Copied metadata from %s", filePath.c_str());
    }

    void AssetsView::SelectAssetEntity(ECS::EntityID entityID) {
        if (entityID == 0 || !m_entityManager.IsEntityValid(entityID)) {
            ANI_LOG_TRACE("SelectAssetEntity: invalid entity %u", entityID);
            return;
        }
        selectedEntityID = entityID;
        std::unordered_map<std::string, std::any> eventData;
        eventData["workspaceID"] = GetID();
        eventData["entityID"] = entityID;
        ANI::Events::Ref().QueueEventWithData("SelectMediaEntity", eventData);
        ANI_LOG_TRACE("Selected entity %u", entityID);
    }

    nlohmann::json AssetsView::Serialize() const {
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

    void AssetsView::Deserialize(const nlohmann::json& j) {
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