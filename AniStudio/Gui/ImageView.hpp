#ifndef IMAGEVIEW_HPP
#define IMAGEVIEW_HPP

#include "Base/BaseView.hpp"
#include "ImageComponent.hpp"
#include "ImageSystem.hpp"
#include <pch.h>

namespace GUI {

    class ImageView : public BaseView {
    public:
        ImageView(ECS::EntityManager& entityMgr)
            : BaseView(entityMgr),
            selectedEntityID(0),
            imgIndex(0),
            showHistory(true),
            zoom(1.0f),
            offsetX(0.0f),
            offsetY(0.0f)
        {
            viewName = "ImageView";

            // Ensure ImageSystem is registered
            auto imageSystem = mgr.GetSystem<ECS::ImageSystem>();
            if (!imageSystem) {
                mgr.RegisterSystem<ECS::ImageSystem>();
                imageSystem = mgr.GetSystem<ECS::ImageSystem>();
            }

            // Register callbacks to update the view when images change
            if (imageSystem) {
                imageSystem->RegisterImageAddedCallback([this](ECS::EntityID entityID) {
                    // If nothing is selected yet, select the new image
                    if (selectedEntityID == 0) {
                        selectedEntityID = entityID;
                        imgIndex = GetCurrentImageIndex();
                    }
                    });

                imageSystem->RegisterImageRemovedCallback([this](ECS::EntityID entityID) {
                    // If the selected image was removed, select a different one
                    if (selectedEntityID == entityID) {
                        // Get current index before removal
                        int currentIndex = GetCurrentImageIndex();

                        // After removal, we need to select another image
                        auto imageEntities = GetImageEntities();

                        if (imageEntities.empty()) {
                            selectedEntityID = 0;
                            imgIndex = 0;
                        }
                        else {
                            // Try to select image at same index, or the last one
                            if (currentIndex < imageEntities.size()) {
                                selectedEntityID = imageEntities[currentIndex];
                                imgIndex = currentIndex;
                            }
                            else {
                                selectedEntityID = imageEntities.back();
                                imgIndex = imageEntities.size() - 1;
                            }
                        }
                    }
                    });
            }
        }

        void Init() override {
            // Try to select first image if nothing is selected
            if (selectedEntityID == 0) {
                auto imageEntities = GetImageEntities();
                if (!imageEntities.empty()) {
                    selectedEntityID = imageEntities[0];
                    imgIndex = 0;
                }
            }
        }

        void Render() override {
            ImGui::SetNextWindowSize(ImVec2(1024, 1024), ImGuiCond_FirstUseEver);
            ImGui::Begin("Image Viewer", nullptr);

            // Show image information if one is selected
            if (selectedEntityID != 0 && mgr.HasComponent<ECS::ImageComponent>(selectedEntityID)) {
                const auto& imageComp = mgr.GetComponent<ECS::ImageComponent>(selectedEntityID);
                ImGui::Text("File: %s", imageComp.fileName.c_str());
                ImGui::Text("Dimensions: %dx%d", imageComp.width, imageComp.height);
                ImGui::Separator();
            }

            RenderSelector();

            if (ImGui::Button("Load Image(s)")) {
                IGFD::FileDialogConfig config;
                config.path = ".";
                // Enable multiple selection
                config.countSelectionMax = 0; // 0 means infinite selections
                ImGuiFileDialog::Instance()->OpenDialog("LoadImageDialog", "Choose Image(s)",
                    filters, config);
            }

            if (ImGuiFileDialog::Instance()->Display("LoadImageDialog", 32, ImVec2(700, 400))) {
                if (ImGuiFileDialog::Instance()->IsOk()) {
                    // Get multiple selections
                    std::map<std::string, std::string> selection = ImGuiFileDialog::Instance()->GetSelection();

                    std::vector<std::string> filePaths;
                    for (const auto& [fileName, filePath] : selection) {
                        filePaths.push_back(filePath);
                    }

                    LoadImages(filePaths);
                }
                ImGuiFileDialog::Instance()->Close();
            }

            ImGui::SameLine();

            if (selectedEntityID != 0 && ImGui::Button("Save Image")) {
                SaveSelectedImage();
            }

            ImGui::SameLine();

            if (selectedEntityID != 0 && ImGui::Button("Save Image As")) {
                IGFD::FileDialogConfig config;
                config.path = ".";
                ImGuiFileDialog::Instance()->OpenDialog("SaveImageAsDialog", "Save Image As",
                    filters, config);
            }

            if (ImGuiFileDialog::Instance()->Display("SaveImageAsDialog")) {
                if (ImGuiFileDialog::Instance()->IsOk() && selectedEntityID != 0) {
                    std::string savePath = ImGuiFileDialog::Instance()->GetFilePathName();
                    SaveSelectedImageAs(savePath);
                }
                ImGuiFileDialog::Instance()->Close();
            }

            // Option to show/hide history panel
            ImGui::SameLine();
            ImGui::Checkbox("Show History", &showHistory);

            if (showHistory) {
                RenderHistory();
            }

            ImGui::Separator();

            if (ImGui::BeginChild("ImageViewerChild", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar)) {
                RenderSelectedImage();
                ImGui::EndChild();
            }

            ImGui::End();
        }

        ~ImageView() {
            // Nothing to clean up - ImageSystem handles resource cleanup
        }

    private:
        ECS::EntityID selectedEntityID;
        int imgIndex;
        bool showHistory;

        // Values for zoom and panning
        float zoom;
        float offsetX;
        float offsetY;

        // File filters
        const char* filters = "Image files{.png,.jpg,.jpeg,.bmp,.tga}"
            ".png,.jpg,.jpeg,.bmp,.tga"
            "{.png},PNG"
            "{.jpg,.jpeg},JPEG"
            "{.bmp},BMP"
            "{.tga},TGA";

        // Get current list of image entities directly from the ImageSystem
        std::vector<ECS::EntityID> GetImageEntities() const {
            auto imageSystem = mgr.GetSystem<ECS::ImageSystem>();
            if (imageSystem) {
                return imageSystem->GetAllImageEntities();
            }
            return {}; // Empty vector if no system or no entities
        }

        // Get the index of the currently selected image
        int GetCurrentImageIndex() const {
            if (selectedEntityID == 0) return -1;

            auto imageEntities = GetImageEntities();
            auto it = std::find(imageEntities.begin(), imageEntities.end(), selectedEntityID);
            if (it != imageEntities.end()) {
                return static_cast<int>(std::distance(imageEntities.begin(), it));
            }
            return -1; // Not found
        }

        void RenderSelector() {
            auto imageEntities = GetImageEntities();

            if (imageEntities.empty()) {
                ImGui::Text("No images loaded.");
                return;
            }

            // Navigation buttons
            if (ImGui::Button("First")) {
                if (!imageEntities.empty()) {
                    imgIndex = 0;
                    selectedEntityID = imageEntities[imgIndex];
                }
            }

            ImGui::SameLine();

            if (ImGui::Button("Last")) {
                if (!imageEntities.empty()) {
                    imgIndex = static_cast<int>(imageEntities.size() - 1);
                    selectedEntityID = imageEntities[imgIndex];
                }
            }

            ImGui::SameLine();

            // Image index selection
            if (ImGui::InputInt("Current Image", &imgIndex)) {
                if (imageEntities.empty()) {
                    imgIndex = 0;
                    return;
                }

                const int size = imageEntities.size();
                if (size == 1) {
                    imgIndex = 0;
                }
                else {
                    if (imgIndex < 0) {
                        imgIndex = size - 1;
                    }
                    imgIndex = (imgIndex % size + size) % size;
                }

                selectedEntityID = imageEntities[imgIndex];
            }
        }

        void RenderHistory() {
            ImGui::Begin("History", &showHistory);

            auto imageEntities = GetImageEntities();

            if (imageEntities.empty()) {
                ImGui::Text("No images available.");
                ImGui::End();
                return;
            }

            // Create scrollable history panel
            for (size_t i = 0; i < imageEntities.size(); ++i) {
                ImGui::BeginGroup();

                ECS::EntityID entityID = imageEntities[i];
                if (!mgr.HasComponent<ECS::ImageComponent>(entityID)) {
                    ImGui::EndGroup();
                    continue;
                }

                const auto& imageComp = mgr.GetComponent<ECS::ImageComponent>(entityID);

                // Highlight the selected image
                if (static_cast<int>(i) == imgIndex) {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));
                }

                ImGui::Text("%zu: %s", i, imageComp.fileName.c_str());

                if (static_cast<int>(i) == imgIndex) {
                    ImGui::PopStyleColor();
                }

                // Calculate image dimensions for the thumbnail
                float aspectRatio = static_cast<float>(imageComp.width) / static_cast<float>(imageComp.height);
                ImVec2 maxSize(128.0f, 128.0f);
                ImVec2 imageSize;

                if (aspectRatio > 1.0f) {
                    imageSize = ImVec2(maxSize.x, maxSize.x / aspectRatio);
                }
                else {
                    imageSize = ImVec2(maxSize.y * aspectRatio, maxSize.y);
                }

                // Make the image clickable
                if (imageComp.textureID != 0) {
                    if (ImGui::ImageButton(("##img" + std::to_string(i)).c_str(),
                        reinterpret_cast<void*>(static_cast<intptr_t>(imageComp.textureID)),
                        imageSize)) {
                        imgIndex = static_cast<int>(i);
                        selectedEntityID = entityID;
                    }
                }
                else {
                    // Fallback if no texture
                    if (ImGui::Button(("Select##" + std::to_string(i)).c_str(), imageSize)) {
                        imgIndex = static_cast<int>(i);
                        selectedEntityID = entityID;
                    }
                }

                ImGui::EndGroup();

                // Flow thumbnails horizontally
                if (i < imageEntities.size() - 1) {
                    ImGui::SameLine();

                    // Check if we need to wrap to next line
                    float totalWidth = ImGui::GetContentRegionAvail().x;
                    float nextWidth = std::min(maxSize.x, maxSize.y * aspectRatio) + ImGui::GetStyle().ItemSpacing.x;

                    if (ImGui::GetCursorPosX() + nextWidth > totalWidth) {
                        ImGui::NewLine();
                    }
                }
            }

            ImGui::End();
        }

        void RenderSelectedImage() {
            if (selectedEntityID == 0 || !mgr.HasComponent<ECS::ImageComponent>(selectedEntityID)) {
                ImGui::Text("No image selected.");
                return;
            }

            const auto& imageComp = mgr.GetComponent<ECS::ImageComponent>(selectedEntityID);

            // Handle zooming with mouse wheel when child window is hovered
            if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.0f) {
                SetZoom(zoom + ImGui::GetIO().MouseWheel * 0.1f);
            }

            // Calculate the window padding
            const ImVec2 windowPadding = ImGui::GetStyle().WindowPadding;
            const ImVec2 windowPos = ImGui::GetWindowPos();
            const ImVec2 contentPos = ImVec2(windowPos.x + windowPadding.x, windowPos.y + windowPadding.y);

            // Calculate image size and position
            ImVec2 imageSize = ImVec2(imageComp.width * zoom, imageComp.height * zoom);
            ImVec2 imagePos = ImVec2(offsetX + windowPadding.x, offsetY + windowPadding.y);

            // Draw grid before the image
            DrawGrid(imageComp.width, imageComp.height);

            // Set cursor position and render image
            ImGui::SetCursorPos(imagePos);
            if (imageComp.textureID != 0) {
                ImGui::Image((void*)(intptr_t)imageComp.textureID, imageSize,
                    ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, 1));

                // Handle panning only when the image is hovered
                if (ImGui::IsItemHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                    offsetX += ImGui::GetIO().MouseDelta.x;
                    offsetY += ImGui::GetIO().MouseDelta.y;
                }
            }
            else {
                ImGui::Text("No image data or texture available.");
            }

            // Ensure the content size is set to accommodate the image
            ImGui::SetCursorPos(ImVec2(imagePos.x + imageSize.x, imagePos.y + imageSize.y));
        }

        void DrawGrid(int imageWidth, int imageHeight) {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            const float gridStep = 100.0f * zoom;

            const ImVec2 windowPos = ImGui::GetWindowPos();
            const ImVec2 windowSize = ImGui::GetWindowSize();
            const ImVec2 contentMin = windowPos;
            const ImVec2 contentMax = ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y);

            // Calculate grid start positions
            float startX = contentMin.x - fmodf(ImGui::GetScrollX(), gridStep);
            float startY = contentMin.y - fmodf(ImGui::GetScrollY(), gridStep);

            // Draw vertical grid lines
            for (float x = startX; x < contentMax.x; x += gridStep) {
                draw_list->AddLine(ImVec2(x, contentMin.y), ImVec2(x, contentMax.y), IM_COL32(255, 255, 255, 50));
            }

            // Draw horizontal grid lines
            for (float y = startY; y < contentMax.y; y += gridStep) {
                draw_list->AddLine(ImVec2(contentMin.x, y), ImVec2(contentMax.x, y), IM_COL32(255, 255, 255, 50));
            }
        }

        void SetZoom(float newZoom) {
            // Limit zoom level to a reasonable range
            zoom = std::clamp(newZoom, 0.1f, 5.0f);
        }

        void LoadImages(const std::vector<std::string>& filePaths) {
            auto imageSystem = mgr.GetSystem<ECS::ImageSystem>();
            if (!imageSystem) return;

            std::vector<ECS::EntityID> newEntities = imageSystem->LoadImages(filePaths);

            // Select the last loaded image
            if (!newEntities.empty()) {
                selectedEntityID = newEntities.back();
                imgIndex = GetCurrentImageIndex();
            }
        }

        void SaveSelectedImage() {
            if (selectedEntityID == 0) return;

            auto imageSystem = mgr.GetSystem<ECS::ImageSystem>();
            if (!imageSystem) return;

            // Use the existing file path
            const auto& imageComp = mgr.GetComponent<ECS::ImageComponent>(selectedEntityID);
            imageSystem->SaveImage(selectedEntityID, imageComp.filePath);
        }

        void SaveSelectedImageAs(const std::string& filePath) {
            if (selectedEntityID == 0) return;

            auto imageSystem = mgr.GetSystem<ECS::ImageSystem>();
            if (!imageSystem) return;

            imageSystem->SaveImage(selectedEntityID, filePath);
        }
    };

} // namespace GUI

#endif // IMAGEVIEW_HPP