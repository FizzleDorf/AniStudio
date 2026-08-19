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
#include <imgui.h>
#include <filesystem>
#include <iostream>

namespace GUI {

    AssetsView::AssetsView(ECS::EntityManager& mgr, ViewManager& vm)
        : BaseView(mgr, vm), needsRefresh(true) {
        viewName = "AssetsView";
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

        if (ImGui::Button("Refresh")) {
            RefreshAssets();
        }
        ImGui::SameLine();
        ImGui::Text("Path: %s", assetsPath.c_str());

        ImGui::SameLine();
        if (ImGui::Button("Clear Loaded")) {
            ClearLoadedAssets();
        }

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
            std::sort(assetFiles.begin(), assetFiles.end());
        }
        catch (const std::exception& e) {
            std::cerr << "[AssetsView] Error scanning assets: " << e.what() << std::endl;
        }
    }

    void AssetsView::RenderAssetGrid() {
        float windowWidth = ImGui::GetContentRegionAvail().x;
        float thumbnailSize = 128.0f;
        float cellSize = thumbnailSize + ImGui::GetStyle().ItemSpacing.x;
        int columns = std::max(1, static_cast<int>(windowWidth / cellSize));

        ImGui::Columns(columns, nullptr, false);

        for (const auto& path : assetFiles) {
            ImGui::BeginGroup();

            std::string filename = path.filename().string();
            std::string ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            ImGui::Text("%s", filename.c_str());

            ImGui::PushID(path.string().c_str());

            if (ImGui::Button("Load", ImVec2(thumbnailSize, thumbnailSize))) {
                LoadAsset(path);
            }

            if (ImGui::BeginDragDropSource()) {
                nlohmann::json payload;
                payload["filePath"] = path.string();
                payload["fileType"] = GUI::DragDrop::GuessMediaType(path.string());
                ImGui::SetDragDropPayload(GUI::DragDrop::PAYLOAD_FILE_PATH,
                    payload.dump().c_str(), payload.dump().size() + 1);
                ImGui::Text("Dragging: %s", path.filename().string().c_str());
                ImGui::EndDragDropSource();
            }

            ImGui::PopID();
            ImGui::EndGroup();
            ImGui::NextColumn();
        }

        ImGui::Columns(1);

        std::vector<std::string> droppedFiles;
        if (GUI::DragDrop::AcceptFileDrop(droppedFiles)) {
            for (const auto& f : droppedFiles) {
                LoadAsset(std::filesystem::path(f));
            }
        }
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
    }

} // namespace GUI