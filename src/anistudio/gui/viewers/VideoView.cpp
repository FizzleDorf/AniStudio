#include "VideoView.hpp"
#include "TextureSystem.hpp"
#include <algorithm>
#include <iostream>

namespace GUI {

    VideoView::VideoView(ECS::EntityManager& entityMgr)
        : BaseView(entityMgr),
        selectedEntityID(0),
        videoIndex(0),
        showHistory(true),
        zoom(1.0f),
        offsetX(0.0f),
        offsetY(0.0f),
        isPlaying(false),
        playbackSpeed(1.0f),
        lastEntityCount(0),
        lastGeneratedVideoID(0)
    {
        viewName = "VideoView";
        std::cout << "[VideoView] Constructor called" << std::endl;
    }

    VideoView::~VideoView() {
        std::cout << "[VideoView] Destructor called - NO CALLBACKS TO UNREGISTER" << std::endl;
    }

    void VideoView::Init() {
        std::cout << "[VideoView] Initializing..." << std::endl;

        auto videoSystem = mgr.GetSystem<ECS::VideoSystem>();
        if (!videoSystem) {
            std::cout << "[VideoView] Registering VideoSystem..." << std::endl;
            mgr.RegisterSystem<ECS::VideoSystem>();
            videoSystem = mgr.GetSystem<ECS::VideoSystem>();
        }

        if (videoSystem) {
            auto textureSystem = mgr.GetSystem<ECS::TextureSystem>();
            if (textureSystem) {
                videoSystem->SetVideoTextureCallback(
                    [textureSystem](ECS::EntityID entityID, unsigned char* data,
                        int width, int height, int channels, GLuint* targetTexture) {
                            textureSystem->QueueVideoTextureCreation(entityID, data, width, height, channels, targetTexture);
                    }
                );
                std::cout << "[VideoView] Video texture callback connected to TextureSystem" << std::endl;
            }
            else {
                std::cerr << "[VideoView] WARNING: TextureSystem not found; video textures will not update!" << std::endl;
            }
        }

        RefreshVideoEntities();
        std::cout << "[VideoView] Initialization complete" << std::endl;
    }

    void VideoView::Update(float deltaT) {
        // Refresh entity list if count changed
        size_t currentCount = 0;
        for (auto entityID : mgr.GetAllEntities()) {
            if (mgr.HasComponent<ECS::VideoComponent>(entityID)) {
                currentCount++;
            }
        }

        if (currentCount != lastEntityCount) {
            RefreshVideoEntities();
            // If our selected entity is no longer valid, reset selection
            if (selectedEntityID != 0 && !mgr.IsEntityValid(selectedEntityID)) {
                selectedEntityID = 0;
                videoIndex = 0;
            }
            if (videoEntities.empty()) {
                selectedEntityID = 0;
                videoIndex = 0;
            }
            else if (selectedEntityID == 0) {
                selectedEntityID = videoEntities[0];
                videoIndex = 0;
            }
            else {
                auto it = std::find(videoEntities.begin(), videoEntities.end(), selectedEntityID);
                if (it != videoEntities.end()) {
                    videoIndex = static_cast<int>(std::distance(videoEntities.begin(), it));
                }
                else {
                    if (!videoEntities.empty()) {
                        selectedEntityID = videoEntities[0];
                        videoIndex = 0;
                    }
                    else {
                        selectedEntityID = 0;
                        videoIndex = 0;
                    }
                }
            }
        }

        // ensure selected entity is still valid and has the component
        if (selectedEntityID != 0 && mgr.IsEntityValid(selectedEntityID) &&
            mgr.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
            auto& videoComp = mgr.GetComponent<ECS::VideoComponent>(selectedEntityID);
            if (videoComp.isPlaying && videoComp.fmtCtx) {
                videoComp.frameAccumulator += deltaT * videoComp.playbackSpeed;
                float frameDuration = 1.0f / static_cast<float>(videoComp.fps);
                auto videoSystem = mgr.GetSystem<ECS::VideoSystem>();
                while (videoComp.frameAccumulator >= frameDuration) {
                    videoComp.frameAccumulator -= frameDuration;
                    if (videoSystem) {
                        // Re-check entity validity before advancing, in case it was removed during this loop
                        if (!mgr.IsEntityValid(selectedEntityID) || !mgr.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
                            videoComp.isPlaying = false;
                            break;
                        }
                        if (!videoSystem->AdvanceOneFrame(videoComp)) {
                            videoComp.isPlaying = false;
                            break;
                        }
                    }
                    else {
                        break;
                    }
                }
            }
        }
    }

    void VideoView::Render() {
        if (!ImGui::GetCurrentContext()) {
            std::cerr << "[VideoView] ERROR: No ImGui context!" << std::endl;
            return;
        }

        ImGui::SetNextWindowSize(ImVec2(1024, 768), ImGuiCond_FirstUseEver);

        std::string windowName = "Video Viewer##" + std::to_string(GetID());
        bool windowOpen = true;

        if (!ImGui::Begin(windowName.c_str(), &windowOpen)) {
            ImGui::End();
            return;
        }

        try {
            RenderVideoInfo();
            RenderSelector();
            RenderControls();
            RenderPlaybackControls();

            ImGui::SameLine();
            ImGui::Checkbox("Show History", &showHistory);

            if (showHistory) {
                RenderHistory();
            }

            ImGui::Separator();

            if (ImGui::BeginChild("VideoViewerChild", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar)) {
                RenderSelectedVideo();
            }
            ImGui::EndChild();
        }
        catch (const std::exception& e) {
            std::cerr << "[VideoView] Exception in Render: " << e.what() << std::endl;
            ImGui::Text("Error rendering VideoView: %s", e.what());
        }

        ImGui::End();

        if (!windowOpen) {
            std::unordered_map<std::string, std::any> eventData;
            eventData["workspaceID"] = GetID();
            eventData["viewTypeName"] = viewName;
            ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
        }
    }

    void VideoView::RefreshVideoEntities() {
        try {
            videoEntities.clear();
            for (auto entityID : mgr.GetAllEntities()) {
                if (mgr.HasComponent<ECS::VideoComponent>(entityID)) {
                    videoEntities.push_back(entityID);
                }
            }
            lastEntityCount = videoEntities.size();
            std::cout << "[VideoView] Refreshed entities, found " << videoEntities.size() << " videos" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "[VideoView] Exception refreshing entities: " << e.what() << std::endl;
            videoEntities.clear();
            lastEntityCount = 0;
        }
    }

    void VideoView::RenderVideoInfo() {
        if (selectedEntityID != 0 && mgr.IsEntityValid(selectedEntityID) &&
            mgr.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
            try {
                const auto& videoComp = mgr.GetComponent<ECS::VideoComponent>(selectedEntityID);
                ImGui::Text("File: %s", videoComp.fileName.c_str());
                ImGui::Text("Dimensions: %dx%d, FPS: %.2f, Frames: %d",
                    videoComp.width, videoComp.height, videoComp.fps, videoComp.frameCount);
                ImGui::Text("Current Frame: %d / %d", videoComp.currentFrame, videoComp.frameCount);
                ImGui::Text("Entity ID: %zu", selectedEntityID);

                if (selectedEntityID == lastGeneratedVideoID) {
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "✦ NEWLY GENERATED");
                }

                ImGui::Separator();
            }
            catch (const std::exception& e) {
                ImGui::Text("Error reading video info: %s", e.what());
            }
        }
    }

    void VideoView::RenderControls() {
        if (ImGui::Button("Load Video(s)")) {
            IGFD::FileDialogConfig config;
            config.path = ".";
            config.countSelectionMax = 0;
            ImGuiFileDialog::Instance()->OpenDialog("LoadVideoDialog", "Choose Video(s)",
                filters, config);
        }

        if (ImGuiFileDialog::Instance()->Display("LoadVideoDialog", 32, ImVec2(700, 400))) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::map<std::string, std::string> selection = ImGuiFileDialog::Instance()->GetSelection();
                std::vector<std::string> filePaths;
                for (const auto& [fileName, filePath] : selection) {
                    filePaths.push_back(filePath);
                }
                LoadVideos(filePaths);
            }
            ImGuiFileDialog::Instance()->Close();
        }

        ImGui::SameLine();

        if (selectedEntityID != 0 && ImGui::Button("Remove Video")) {
            RemoveSelectedVideo();
        }

        ImGui::SameLine();

        if (ImGui::Button("Refresh")) {
            RefreshVideoEntities();
        }
    }

    void VideoView::RenderSelector() {
        if (videoEntities.empty()) {
            ImGui::Text("No videos loaded.");
            return;
        }

        if (ImGui::Button("First")) {
            if (!videoEntities.empty()) {
                videoIndex = 0;
                selectedEntityID = videoEntities[videoIndex];
                PauseAllVideos();
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Previous")) {
            if (!videoEntities.empty()) {
                videoIndex = (videoIndex - 1 + static_cast<int>(videoEntities.size())) % static_cast<int>(videoEntities.size());
                selectedEntityID = videoEntities[videoIndex];
                PauseAllVideos();
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Next")) {
            if (!videoEntities.empty()) {
                videoIndex = (videoIndex + 1) % static_cast<int>(videoEntities.size());
                selectedEntityID = videoEntities[videoIndex];
                PauseAllVideos();
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Last")) {
            if (!videoEntities.empty()) {
                videoIndex = static_cast<int>(videoEntities.size() - 1);
                selectedEntityID = videoEntities[videoIndex];
                PauseAllVideos();
            }
        }

        ImGui::SameLine();

        if (ImGui::InputInt("Current Video", &videoIndex)) {
            if (!videoEntities.empty()) {
                const int size = static_cast<int>(videoEntities.size());
                if (size == 1) {
                    videoIndex = 0;
                }
                else {
                    videoIndex = ((videoIndex % size) + size) % size;
                }
                selectedEntityID = videoEntities[videoIndex];
                PauseAllVideos();
            }
        }

        ImGui::Text("Video %d of %zu", videoIndex + 1, videoEntities.size());
    }

    void VideoView::RenderPlaybackControls() {
        ImGui::Separator();

        if (selectedEntityID == 0 || !mgr.IsEntityValid(selectedEntityID) ||
            !mgr.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
            ImGui::Text("No video selected.");
            return;
        }

        try {
            auto& videoComp = mgr.GetComponent<ECS::VideoComponent>(selectedEntityID);

            int currentFrame = videoComp.currentFrame;
            if (ImGui::SliderInt("Frame", &currentFrame, 0, videoComp.frameCount - 1)) {
                auto videoSystem = mgr.GetSystem<ECS::VideoSystem>();
                if (videoSystem) {
                    videoSystem->SeekToFrame(videoComp, currentFrame);
                }
            }

            if (ImGui::Button(videoComp.isPlaying ? "Pause" : "Play")) {
                videoComp.isPlaying = !videoComp.isPlaying;
                if (videoComp.isPlaying) {
                    videoComp.frameAccumulator = 0.0f;
                }
            }

            ImGui::SameLine();

            if (ImGui::Button("Stop")) {
                videoComp.isPlaying = false;
                videoComp.frameAccumulator = 0.0f;
                auto videoSystem = mgr.GetSystem<ECS::VideoSystem>();
                if (videoSystem) {
                    videoSystem->SeekToFrame(videoComp, 0);
                }
            }

            ImGui::SameLine();
            ImGui::Checkbox("Loop", &videoComp.looping);

            ImGui::SameLine();
            if (ImGui::SliderFloat("Speed", &videoComp.playbackSpeed, 0.1f, 2.0f, "%.1fx")) {
            }

            ImGui::Separator();
        }
        catch (const std::exception& e) {
            ImGui::Text("Error with playback controls: %s", e.what());
        }
    }

    void VideoView::RenderHistory() {
        ImGui::Begin("Video History", &showHistory);

        if (videoEntities.empty()) {
            ImGui::Text("No videos available.");
            ImGui::End();
            return;
        }

        float currentRowWidth = 0.0f;

        for (size_t i = 0; i < videoEntities.size(); ++i) {
            ECS::EntityID entityID = videoEntities[i];

            if (!mgr.IsEntityValid(entityID) || !mgr.HasComponent<ECS::VideoComponent>(entityID)) {
                continue;
            }

            try {
                const auto& videoComp = mgr.GetComponent<ECS::VideoComponent>(entityID);

                ImGui::BeginGroup();

                if (static_cast<int>(i) == videoIndex) {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));
                }
                else if (entityID == lastGeneratedVideoID) {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
                }

                float aspectRatio = (videoComp.height > 0) ?
                    static_cast<float>(videoComp.width) / static_cast<float>(videoComp.height) : 1.0f;
                ImVec2 maxSize(128.0f, 128.0f);
                ImVec2 imageSize;

                if (aspectRatio > 1.0f) {
                    imageSize = ImVec2(maxSize.x, maxSize.x / aspectRatio);
                }
                else {
                    imageSize = ImVec2(maxSize.y * aspectRatio, maxSize.y);
                }

                std::string displayText = std::to_string(i) + ": " + TruncateFilename(videoComp.fileName, imageSize.x);
                if (entityID == lastGeneratedVideoID) {
                    displayText += " ✦";
                }
                ImGui::Text("%s", displayText.c_str());
                ImGui::Text("%.0f fps, %d frames", videoComp.fps, videoComp.frameCount);

                if (static_cast<int>(i) == videoIndex || entityID == lastGeneratedVideoID) {
                    ImGui::PopStyleColor();
                }

                if (videoComp.currentTexture != 0) {
                    if (ImGui::ImageButton(("##vid" + std::to_string(i)).c_str(),
                        (ImTextureID)(intptr_t)videoComp.currentTexture,
                        imageSize,
                        ImVec2(0, 0),
                        ImVec2(1, 1)
                    )) {
                        videoIndex = static_cast<int>(i);
                        selectedEntityID = entityID;
                        PauseAllVideos();
                    }
                }
                else {
                    if (ImGui::Button(("Select##" + std::to_string(i)).c_str(), imageSize)) {
                        videoIndex = static_cast<int>(i);
                        selectedEntityID = entityID;
                        PauseAllVideos();
                    }
                }

                ImGui::EndGroup();

                float buttonWidth = imageSize.x + ImGui::GetStyle().ItemSpacing.x;
                currentRowWidth += buttonWidth;

                if (i < videoEntities.size() - 1) {
                    float nextButtonWidth = std::min(maxSize.x, maxSize.y * aspectRatio) + ImGui::GetStyle().ItemSpacing.x;
                    if (currentRowWidth + nextButtonWidth > ImGui::GetContentRegionAvail().x) {
                        ImGui::NewLine();
                        currentRowWidth = 0.0f;
                    }
                    else {
                        ImGui::SameLine();
                    }
                }
            }
            catch (const std::exception& e) {
                ImGui::Text("Error with video %zu: %s", i, e.what());
            }
        }

        ImGui::End();
    }

    void VideoView::RenderSelectedVideo() {
        if (selectedEntityID == 0 || !mgr.IsEntityValid(selectedEntityID) ||
            !mgr.HasComponent<ECS::VideoComponent>(selectedEntityID)) {
            ImGui::Text("No video selected or entity invalid.");
            return;
        }

        try {
            const auto& videoComp = mgr.GetComponent<ECS::VideoComponent>(selectedEntityID);

            if (videoComp.currentTexture == 0 || videoComp.width <= 0 || videoComp.height <= 0) {
                ImGui::Text("Video loading... (Texture ID: %u, Size: %dx%d)",
                    videoComp.currentTexture, videoComp.width, videoComp.height);
                return;
            }

            if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.0f) {
                float newZoom = zoom + ImGui::GetIO().MouseWheel * 0.1f;
                SetZoom(newZoom);
            }

            ImVec2 imageSize = ImVec2(videoComp.width * zoom, videoComp.height * zoom);
            ImVec2 windowSize = ImGui::GetWindowSize();
            ImVec2 windowPadding = ImGui::GetStyle().WindowPadding;

            if (zoom <= 1.0f) {
                offsetX = (windowSize.x - imageSize.x) * 0.5f;
                offsetY = (windowSize.y - imageSize.y) * 0.5f;
            }

            ImVec2 imagePos = ImVec2(offsetX + windowPadding.x, offsetY + windowPadding.y);

            DrawGrid(videoComp.width, videoComp.height);

            ImGui::SetCursorPos(imagePos);
            ImGui::Dummy(imageSize);

            ImGui::SetCursorPos(imagePos);

            ImGui::Image(
                (ImTextureID)(intptr_t)videoComp.currentTexture,
                imageSize,
                ImVec2(0, 0),
                ImVec2(1, 1)
            );

            if (zoom > 1.0f && ImGui::IsItemHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                offsetX += ImGui::GetIO().MouseDelta.x;
                offsetY += ImGui::GetIO().MouseDelta.y;
            }

            ImGui::SetCursorPos(ImVec2(imagePos.x + imageSize.x, imagePos.y + imageSize.y));
            ImGui::Dummy(ImVec2(0, 0));
        }
        catch (const std::exception& e) {
            ImGui::Text("Error rendering video: %s", e.what());
        }
    }

    void VideoView::DrawGrid(int videoWidth, int videoHeight) {
        if (videoWidth <= 0 || videoHeight <= 0) return;

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const float gridStep = 100.0f * zoom;

        const ImVec2 windowPos = ImGui::GetWindowPos();
        const ImVec2 windowSize = ImGui::GetWindowSize();
        const ImVec2 contentMin = windowPos;
        const ImVec2 contentMax = ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y);

        float startX = contentMin.x - fmodf(ImGui::GetScrollX(), gridStep);
        float startY = contentMin.y - fmodf(ImGui::GetScrollY(), gridStep);

        for (float x = startX; x < contentMax.x; x += gridStep) {
            draw_list->AddLine(ImVec2(x, contentMin.y), ImVec2(x, contentMax.y), IM_COL32(255, 255, 255, 50));
        }

        for (float y = startY; y < contentMax.y; y += gridStep) {
            draw_list->AddLine(ImVec2(contentMin.x, y), ImVec2(contentMax.x, y), IM_COL32(255, 255, 255, 50));
        }
    }

    void VideoView::SetZoom(float newZoom) {
        zoom = std::clamp(newZoom, 0.1f, 5.0f);
    }

    void VideoView::LoadVideos(const std::vector<std::string>& filePaths) {
        std::cout << "[VideoView] Loading " << filePaths.size() << " videos..." << std::endl;

        auto videoSystem = mgr.GetSystem<ECS::VideoSystem>();
        if (!videoSystem) {
            std::cerr << "[VideoView] Error: VideoSystem not found!" << std::endl;
            return;
        }

        try {
            for (const auto& filePath : filePaths) {
                if (filePath.empty()) continue;

                ECS::EntityID entity = mgr.AddNewEntity();
                mgr.AddComponent<ECS::VideoComponent>(entity);
                videoSystem->SetVideo(entity, filePath);

                std::cout << "[VideoView] Started loading: " << filePath << " (Entity: " << entity << ")" << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[VideoView] Exception loading videos: " << e.what() << std::endl;
        }
    }

    void VideoView::RemoveSelectedVideo() {
        if (selectedEntityID == 0 || !mgr.IsEntityValid(selectedEntityID)) return;

        try {
            // Pause the video to stop any ongoing decoding
            auto& videoComp = mgr.GetComponent<ECS::VideoComponent>(selectedEntityID);
            videoComp.isPlaying = false;
            videoComp.frameAccumulator = 0.0f;

            auto videoSystem = mgr.GetSystem<ECS::VideoSystem>();
            if (videoSystem) {
                videoSystem->RemoveVideo(selectedEntityID);
            }

            // Reset selection and refresh the list
            selectedEntityID = 0;
            videoIndex = 0;
            RefreshVideoEntities();

            std::cout << "[VideoView] Video removed successfully" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "[VideoView] Exception removing video: " << e.what() << std::endl;
        }
    }

    void VideoView::PauseAllVideos() {
        try {
            for (auto entityID : videoEntities) {
                if (mgr.IsEntityValid(entityID) && mgr.HasComponent<ECS::VideoComponent>(entityID)) {
                    auto& videoComp = mgr.GetComponent<ECS::VideoComponent>(entityID);
                    videoComp.isPlaying = false;
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[VideoView] Exception pausing videos: " << e.what() << std::endl;
        }
    }

    std::string VideoView::TruncateFilename(const std::string& filename, float maxTextWidth) {
        if (filename.empty()) return "Unknown";

        float textWidth = ImGui::CalcTextSize(filename.c_str()).x;
        if (textWidth <= maxTextWidth) {
            return filename;
        }

        std::string truncated = "...";
        float ellipsisWidth = ImGui::CalcTextSize(truncated.c_str()).x;

        for (int i = static_cast<int>(filename.length()) - 1; i >= 0; --i) {
            truncated.insert(3, 1, filename[i]);
            float newWidth = ImGui::CalcTextSize(truncated.c_str()).x;
            if (newWidth > maxTextWidth) {
                truncated.erase(3, 1);
                return truncated;
            }
        }

        return truncated;
    }

} // namespace GUI