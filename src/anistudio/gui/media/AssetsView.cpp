// AssetsView.cpp
#include "AssetsView.hpp"
#include "FilePathSystem.hpp"
#include "FileDialogUtil.hpp"
#include "Events.hpp"
#include "FileFormats.hpp"
#include "DragDropUtils.hpp"
#include "ImageSystem.hpp"
#include "VideoSystem.hpp"
#include "TextureSystem.hpp"
#include "ImageUtils.hpp"
#include "VideoMetadataUtils.hpp"
#include "ThumbnailFilters.hpp"
#include <imgui.h>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <fstream>

namespace GUI {

    AssetsView::AssetsView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseView(mgr, vm), needsRefresh(true) {
        viewName = "AssetsView";
        contextMenuUtils = std::make_unique<Utils::ContextMenuUtils>(m_entityManager);
    }

    void AssetsView::Init() {
        auto fileSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
        if (fileSys) {
            assetsPath = fileSys->GetPath("ProjectAssets");
            if (assetsPath.empty()) {
                assetsPath = "./assets";
            }
        }
        else {
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
                std::cerr << "[AssetsView] SelectMediaEntity event error: " << e.what() << std::endl;
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
            std::cerr << "[AssetsView] Assets path does not exist: " << assetsPath << std::endl;
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
            LoadNewAssets();
            needsSort = true;
        }
        catch (const std::exception& e) {
            std::cerr << "[AssetsView] Error scanning assets: " << e.what() << std::endl;
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

            std::variant<const ECS::ImageComponent*, const ECS::VideoComponent*> compVariant;
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
            std::cout << "[AssetsView] Unsupported file type: " << ext << std::endl;
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
                    std::cout << "[AssetsView] Loaded image: " << filePath << " (Entity: " << entity << ")" << std::endl;
                }
            }
            else if (it->second.isVideo) {
                auto videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
                if (!videoSystem) {
                    m_entityManager.RegisterSystem<ECS::VideoSystem>();
                    videoSystem = m_entityManager.GetSystem<ECS::VideoSystem>();
                }

                if (videoSystem) {
                    auto textureSystem = m_entityManager.GetSystem<ECS::TextureSystem>();
                    if (textureSystem) {
                        videoSystem->SetVideoTextureCallback(
                            [textureSystem](ECS::EntityID entityID, unsigned char* data,
                                int width, int height, int channels, GLuint* targetTexture) {
                                    textureSystem->QueueVideoTextureCreation(entityID, data, width, height, channels, targetTexture);
                            }
                        );
                    }

                    ECS::EntityID entity = m_entityManager.AddNewEntity();
                    m_entityManager.AddComponent<ECS::VideoComponent>(entity);
                    videoSystem->SetVideo(entity, filePath);
                    loadedEntities.push_back(entity);
                    pathToEntity[filePath] = entity;
                    loadedPaths.insert(filePath);
                    std::cout << "[AssetsView] Loaded video: " << filePath << " (Entity: " << entity << ")" << std::endl;
                }
            }
            else if (it->second.isAudio) {
                std::cout << "[AssetsView] Audio file: " << filePath << " (not yet supported)" << std::endl;
            }
            else {
                std::cout << "[AssetsView] Unsupported file type: " << ext << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[AssetsView] Error loading asset: " << e.what() << std::endl;
        }
    }

    void AssetsView::ClearLoadedAssets() {
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
                else {
                    m_entityManager.DestroyEntity(entity);
                }
            }
        }
        loadedEntities.clear();
        pathToEntity.clear();
        loadedPaths.clear();
        std::cout << "[AssetsView] Cleared all loaded assets" << std::endl;
        needsRefresh = true;
    }

    bool AssetsView::CopyFileToAssets(const std::string& sourcePath) {
        std::filesystem::path src(sourcePath);
        std::filesystem::path dst = std::filesystem::path(assetsPath) / src.filename();
        if (std::filesystem::exists(dst)) {
            std::cerr << "[AssetsView] File already exists in assets: " << dst << std::endl;
            return false;
        }
        try {
            std::filesystem::copy(src, dst, std::filesystem::copy_options::overwrite_existing);
            std::cout << "[AssetsView] Copied " << sourcePath << " to " << dst << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[AssetsView] Failed to copy file: " << e.what() << std::endl;
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
            std::cerr << "[AssetsView] No metadata found in " << filePath << std::endl;
            return;
        }

        nlohmann::json wrapped;
        wrapped["dataType"] = "entity";
        wrapped["data"] = metadata;
        wrapped["source"] = "metadata";
        std::string jsonStr = wrapped.dump();
        ImGui::SetClipboardText(jsonStr.c_str());
        std::cout << "[AssetsView] Copied metadata from " << filePath << std::endl;
    }

    void AssetsView::SelectAssetEntity(ECS::EntityID entityID) {
        if (entityID == 0 || !m_entityManager.IsEntityValid(entityID)) return;
        selectedEntityID = entityID;
        std::unordered_map<std::string, std::any> eventData;
        eventData["workspaceID"] = GetID();
        eventData["entityID"] = entityID;
        ANI::Events::Ref().QueueEventWithData("SelectMediaEntity", eventData);
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

}