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
            if (ImGui::BeginMenu("View")) {
                const char* modeItems[] = { "Compact", "Detailed" };
                int modeIdx = static_cast<int>(currentDisplayMode);
                if (ImGui::Combo("Display Mode", &modeIdx, modeItems, IM_ARRAYSIZE(modeItems))) {
                    currentDisplayMode = static_cast<Thumbnail::DisplayMode>(modeIdx);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Filters")) {
                const char* typeItems[] = { "All", "Image", "Video", "Audio" };
                int typeIdx = static_cast<int>(currentTypeFilter);
                if (ImGui::Combo("Type", &typeIdx, typeItems, IM_ARRAYSIZE(typeItems))) {
                    currentTypeFilter = static_cast<FileTypeFilter>(typeIdx);
                }
                const char* sortItems[] = { "Name", "Size", "Date" };
                int sortIdx = static_cast<int>(currentSort);
                if (ImGui::Combo("Sort By", &sortIdx, sortItems, IM_ARRAYSIZE(sortItems))) {
                    currentSort = static_cast<SortMode>(sortIdx);
                    ApplyFiltersAndSort();
                }
                if (ImGui::MenuItem(sortAscending ? "Ascending" : "Descending")) {
                    sortAscending = !sortAscending;
                    ApplyFiltersAndSort();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Actions")) {
                if (ImGui::MenuItem("Refresh")) {
                    RefreshAssets();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
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
            ApplyFiltersAndSort();
            LoadNewAssets();
        }
        catch (const std::exception& e) {
            std::cerr << "[AssetsView] Error scanning assets: " << e.what() << std::endl;
        }
    }

    void AssetsView::LoadNewAssets() {
        for (const auto& path : assetFiles) {
            std::string pathStr = path.string();
            if (loadedPaths.find(pathStr) == loadedPaths.end()) {
                LoadAsset(path);
            }
        }
    }

    void AssetsView::ApplyFiltersAndSort() {
        std::sort(assetFiles.begin(), assetFiles.end(), [this](const std::filesystem::path& a, const std::filesystem::path& b) {
            int cmp = 0;
            switch (currentSort) {
            case SortMode::Name:
                cmp = a.filename().string().compare(b.filename().string());
                break;
            case SortMode::Size:
                try {
                    auto sizeA = std::filesystem::file_size(a);
                    auto sizeB = std::filesystem::file_size(b);
                    cmp = (sizeA < sizeB) ? -1 : (sizeA > sizeB) ? 1 : 0;
                }
                catch (...) { cmp = 0; }
                break;
            case SortMode::Date:
                try {
                    auto timeA = std::filesystem::last_write_time(a);
                    auto timeB = std::filesystem::last_write_time(b);
                    cmp = (timeA < timeB) ? -1 : (timeA > timeB) ? 1 : 0;
                }
                catch (...) { cmp = 0; }
                break;
            }
            return sortAscending ? cmp < 0 : cmp > 0;
            });
    }

    void AssetsView::RenderAssetGrid() {
        float windowWidth = ImGui::GetContentRegionAvail().x;
        const float thumbnailSize = 150.0f;
        float cellSize;
        if (currentDisplayMode == Thumbnail::DisplayMode::Detailed) {
            cellSize = thumbnailSize + 170;
        }
        else {
            cellSize = thumbnailSize + ImGui::GetStyle().ItemSpacing.x + 10;
        }
        int columns = std::max(1, static_cast<int>(windowWidth / cellSize));

        ImGui::Columns(columns, nullptr, false);

        for (size_t i = 0; i < assetFiles.size(); ++i) {
            const auto& path = assetFiles[i];
            std::string ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            const auto& formats = FileFormats::GetAllFormats();
            auto fmtIt = formats.find(ext);
            if (fmtIt == formats.end()) continue;
            bool filterMatch = true;
            if (currentTypeFilter != FileTypeFilter::All) {
                if (currentTypeFilter == FileTypeFilter::Image && !fmtIt->second.isImage) filterMatch = false;
                else if (currentTypeFilter == FileTypeFilter::Video && !fmtIt->second.isVideo) filterMatch = false;
                else if (currentTypeFilter == FileTypeFilter::Audio && !fmtIt->second.isAudio) filterMatch = false;
            }
            if (!filterMatch) continue;

            ECS::EntityID entityID = 0;
            auto it = pathToEntity.find(path.string());
            if (it != pathToEntity.end() && m_entityManager.IsEntityValid(it->second)) {
                entityID = it->second;
            }

            Thumbnail::ThumbnailData data = BuildThumbnailData(path, entityID);
            data.activeEntityID = selectedEntityID;
            Thumbnail::RenderThumbnail(
                data,
                i,
                thumbnailSize,
                currentDisplayMode,
                [this](ECS::EntityID id) { SelectAssetEntity(id); },
                contextMenuUtils.get(),
                entityID != 0
            );
            ImGui::NextColumn();
        }

        ImGui::Columns(1);

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

    Thumbnail::ThumbnailData AssetsView::BuildThumbnailData(const std::filesystem::path& path, ECS::EntityID entityID) {
        Thumbnail::ThumbnailData data;
        data.filePath = path.string();
        data.fileName = path.filename().string();
        data.entityID = entityID;

        if (entityID != 0) {
            if (m_entityManager.HasComponent<ECS::ImageComponent>(entityID)) {
                auto& comp = m_entityManager.GetComponent<ECS::ImageComponent>(entityID);
                data.textureID = comp.textureID;
                data.width = comp.width;
                data.height = comp.height;
                data.channels = comp.channels;
                data.hasExif = comp.hasExifData;
                data.hasLSB = comp.hasLSBData;
                data.isVideo = false;
            }
            else if (m_entityManager.HasComponent<ECS::VideoComponent>(entityID)) {
                auto& comp = m_entityManager.GetComponent<ECS::VideoComponent>(entityID);
                data.textureID = comp.currentTexture;
                data.width = comp.width;
                data.height = comp.height;
                data.fps = comp.fps;
                data.isVideo = true;
            }
        }

        try {
            data.fileSize = std::filesystem::file_size(path);
        }
        catch (...) { data.fileSize = 0; }

        try {
            auto ftime = std::filesystem::last_write_time(path);
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

} // namespace GUI