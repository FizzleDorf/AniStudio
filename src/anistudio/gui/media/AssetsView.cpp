#include "AssetsView.hpp"
#include "FilePathSystem.hpp"
#include "FileDialogUtil.hpp"
#include "Events.hpp"
#include "ImageView.hpp"
#include "VideoView.hpp"
#include "FileFormats.hpp"
#include "DragDropUtils.hpp"
#include "ImageSystem.hpp"
#include "VideoSystem.hpp"
#include "TextureSystem.hpp"
#include "ContextMenuUtils.hpp"
#include <imgui.h>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <chrono>

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
    }

    void AssetsView::Update(float deltaT) {
        if (needsRefresh) {
            RefreshAssets();
            needsRefresh = false;
        }
    }

    void AssetsView::Render() {
        ImGui::Begin("Assets", &windowOpen);

        RenderFilters();

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

    void AssetsView::RenderFilters() {
        ImGui::Text("Path: %s", assetsPath.c_str());

        if (ImGui::Button("Refresh")) {
            RefreshAssets();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear Loaded")) {
            ClearLoadedAssets();
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        const char* typeItems[] = { "All", "Image", "Video", "Audio" };
        int typeIdx = static_cast<int>(currentTypeFilter);
        if (ImGui::Combo("Type", &typeIdx, typeItems, IM_ARRAYSIZE(typeItems))) {
            currentTypeFilter = static_cast<FileTypeFilter>(typeIdx);
            ApplyFiltersAndSort();
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        const char* sortItems[] = { "Name", "Size", "Date" };
        int sortIdx = static_cast<int>(currentSort);
        if (ImGui::Combo("Sort", &sortIdx, sortItems, IM_ARRAYSIZE(sortItems))) {
            currentSort = static_cast<SortMode>(sortIdx);
            ApplyFiltersAndSort();
        }

        ImGui::SameLine();
        if (ImGui::Button(sortAscending ? "Å£" : "Å•")) {
            sortAscending = !sortAscending;
            ApplyFiltersAndSort();
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
        }
        catch (const std::exception& e) {
            std::cerr << "[AssetsView] Error scanning assets: " << e.what() << std::endl;
        }
    }

    void AssetsView::ApplyFiltersAndSort() {
        auto it = std::remove_if(assetFiles.begin(), assetFiles.end(), [this](const std::filesystem::path& p) {
            if (currentTypeFilter == FileTypeFilter::All) return false;
            std::string ext = p.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            const auto& formats = FileFormats::GetAllFormats();
            auto fmtIt = formats.find(ext);
            if (fmtIt == formats.end()) return true;
            if (currentTypeFilter == FileTypeFilter::Image && !fmtIt->second.isImage) return true;
            if (currentTypeFilter == FileTypeFilter::Video && !fmtIt->second.isVideo) return true;
            if (currentTypeFilter == FileTypeFilter::Audio && !fmtIt->second.isAudio) return true;
            return false;
            });
        assetFiles.erase(it, assetFiles.end());

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
        float cellSize = thumbnailSize + ImGui::GetStyle().ItemSpacing.x + 10;
        int columns = std::max(1, static_cast<int>(windowWidth / cellSize));

        ImGui::Columns(columns, nullptr, false);

        for (size_t i = 0; i < assetFiles.size(); ++i) {
            RenderThumbnail(assetFiles[i], i, thumbnailSize);
            ImGui::NextColumn();
        }

        ImGui::Columns(1);

        std::vector<std::string> droppedFiles;
        if (GUI::DragDrop::AcceptFileDrop(droppedFiles)) {
            for (const auto& f : droppedFiles) {
                std::filesystem::path src(f);
                std::filesystem::path dst = std::filesystem::path(assetsPath) / src.filename();
                try {
                    if (std::filesystem::exists(dst)) {
                        std::cerr << "[AssetsView] File already exists in assets: " << dst << std::endl;
                        continue;
                    }
                    std::filesystem::copy(src, dst, std::filesystem::copy_options::overwrite_existing);
                    std::cout << "[AssetsView] Copied " << f << " to " << dst << std::endl;
                }
                catch (const std::exception& e) {
                    std::cerr << "[AssetsView] Failed to copy file: " << e.what() << std::endl;
                }
            }
            needsRefresh = true;
        }
    }

    void AssetsView::RenderThumbnail(const std::filesystem::path& path, size_t index, float thumbnailSize) {
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        const auto& formats = FileFormats::GetAllFormats();
        auto fmtIt = formats.find(ext);
        if (fmtIt == formats.end()) return;

        ECS::EntityID entityID = 0;
        for (auto e : loadedEntities) {
            if (m_entityManager.IsEntityValid(e)) {
                if (m_entityManager.HasComponent<ECS::ImageComponent>(e)) {
                    auto& comp = m_entityManager.GetComponent<ECS::ImageComponent>(e);
                    if (comp.filePath == path.string()) {
                        entityID = e;
                        break;
                    }
                }
                else if (m_entityManager.HasComponent<ECS::VideoComponent>(e)) {
                    auto& comp = m_entityManager.GetComponent<ECS::VideoComponent>(e);
                    if (comp.filePath == path.string()) {
                        entityID = e;
                        break;
                    }
                }
            }
        }

        ImGui::BeginGroup();

        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        ImVec2 itemSize(thumbnailSize + 10, thumbnailSize + 50);
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRect(cursorPos, cursorPos + itemSize, IM_COL32(60, 60, 60, 255), 2.0f);

        ImVec2 thumbSize(thumbnailSize - 10, thumbnailSize - 10);

        GLuint texID = 0;
        int width = 0, height = 0;
        if (entityID != 0) {
            if (m_entityManager.HasComponent<ECS::ImageComponent>(entityID)) {
                auto& comp = m_entityManager.GetComponent<ECS::ImageComponent>(entityID);
                texID = comp.textureID;
                width = comp.width;
                height = comp.height;
            }
            else if (m_entityManager.HasComponent<ECS::VideoComponent>(entityID)) {
                auto& comp = m_entityManager.GetComponent<ECS::VideoComponent>(entityID);
                texID = comp.currentTexture;
                width = comp.width;
                height = comp.height;
            }
        }

        if (texID != 0 && width > 0 && height > 0) {
            float aspect = (height > 0) ? (float)width / height : 1.0f;
            ImVec2 size = thumbSize;
            if (aspect > 1.0f) size.y = thumbSize.x / aspect;
            else size.x = thumbSize.y * aspect;
            ImVec2 imagePos = ImGui::GetCursorPos() + ImVec2((thumbSize.x - size.x) * 0.5f, 0);
            ImGui::SetCursorPos(imagePos);
            if (ImGui::ImageButton(("##asset" + std::to_string(index)).c_str(),
                (ImTextureID)(intptr_t)texID, size, ImVec2(0, 0), ImVec2(1, 1))) {
                if (entityID == 0) {
                    LoadAsset(path);
                }
            }
        }
        else {
            if (ImGui::ImageButton(("##assetplaceholder" + std::to_string(index)).c_str(),
                (ImTextureID)(intptr_t)0, thumbSize, ImVec2(0, 0), ImVec2(1, 1))) {
                LoadAsset(path);
            }
            ImVec2 textSize = ImGui::CalcTextSize(ext.c_str());
            float textX = ImGui::GetItemRectMin().x + (thumbSize.x - textSize.x) * 0.5f;
            float textY = ImGui::GetItemRectMin().y + (thumbSize.y - textSize.y) * 0.5f;
            drawList->AddText(ImVec2(textX, textY), IM_COL32(200, 200, 200, 255), ext.c_str());
        }

        if (entityID != 0) {
            if (ImGui::BeginDragDropSource()) {
                nlohmann::json payload;
                payload["entityID"] = entityID;
                ImGui::SetDragDropPayload(GUI::DragDrop::PAYLOAD_ENTITY,
                    payload.dump().c_str(), payload.dump().size() + 1);
                if (texID != 0) {
                    ImGui::Image((ImTextureID)(intptr_t)texID, ImVec2(64, 64));
                }
                ImGui::Text("%s", path.filename().string().c_str());
                ImGui::EndDragDropSource();
            }
        }
        else {
            if (ImGui::BeginDragDropSource()) {
                nlohmann::json payload;
                payload["filePath"] = path.string();
                payload["fileType"] = GUI::DragDrop::GuessMediaType(path.string());
                ImGui::SetDragDropPayload(GUI::DragDrop::PAYLOAD_FILE_PATH,
                    payload.dump().c_str(), payload.dump().size() + 1);
                ImGui::Text("%s", path.filename().string().c_str());
                ImGui::EndDragDropSource();
            }
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup(("AssetContextMenu##" + std::to_string(index)).c_str());
        }

        if (ImGui::BeginPopup(("AssetContextMenu##" + std::to_string(index)).c_str())) {
            if (entityID != 0) {
                contextMenuUtils->RenderEntityContextMenu(entityID);
            }
            else {
                if (ImGui::MenuItem("Load Asset")) {
                    LoadAsset(path);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Copy File Path")) {
                    ImGui::SetClipboardText(path.string().c_str());
                }
                if (ImGui::MenuItem("Open in Explorer")) {
                    std::string cmd = "explorer /select,\"" + path.string() + "\"";
                    system(cmd.c_str());
                }
            }
            ImGui::EndPopup();
        }

        std::string filename = path.filename().string();
        if (filename.length() > 20) {
            filename = filename.substr(0, 18) + "...";
        }
        ImGui::Text("%s", filename.c_str());

        try {
            auto size = std::filesystem::file_size(path);
            std::string sizeStr = (size > 1024 * 1024) ? std::to_string(size / (1024 * 1024)) + " MB" :
                (size > 1024) ? std::to_string(size / 1024) + " KB" : std::to_string(size) + " B";
            ImGui::Text("%s", sizeStr.c_str());
        }
        catch (...) {}

        try {
            auto ftime = std::filesystem::last_write_time(path);
            auto now = std::chrono::system_clock::now();
            auto diff = ftime - std::filesystem::file_time_type::clock::now();
            auto sys_time = now + std::chrono::duration_cast<std::chrono::system_clock::duration>(diff);
            std::time_t tt = std::chrono::system_clock::to_time_t(sys_time);
            std::tm tm = *std::localtime(&tt);
            char buffer[32];
            strftime(buffer, sizeof(buffer), "%Y-%m-%d", &tm);
            ImGui::Text("%s", buffer);
        }
        catch (...) {}

        ImGui::EndGroup();
    }

    void AssetsView::LoadAsset(const std::filesystem::path& path) {
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        std::string filePath = path.string();

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
        needsRefresh = true;
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
        std::cout << "[AssetsView] Cleared all loaded assets" << std::endl;
        needsRefresh = true;
    }

} // namespace GUI