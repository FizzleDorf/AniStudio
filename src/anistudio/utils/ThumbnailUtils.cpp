#include "ThumbnailUtils.hpp"
#include "DragDropUtils.hpp"
#include "TextureComponent.hpp"
#include "TextureSystem.hpp"
#include "IconFonts.hpp"
#include <imgui.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace GUI {
    namespace Thumbnail {

        float GetThumbnailSize(ThumbnailSize size) {
            switch (size) {
            case ThumbnailSize::Small: return 64.0f;
            case ThumbnailSize::Medium: return 100.0f;
            case ThumbnailSize::Large: return 150.0f;
            case ThumbnailSize::ExtraLarge: return 200.0f;
            default: return 100.0f;
            }
        }

        static std::string FormatFileSize(uint64_t bytes) {
            if (bytes > 1024 * 1024 * 1024) {
                char buf[32];
                snprintf(buf, sizeof(buf), "%.1f GB", bytes / (1024.0 * 1024.0 * 1024.0));
                return buf;
            }
            else if (bytes > 1024 * 1024) {
                char buf[32];
                snprintf(buf, sizeof(buf), "%.1f MB", bytes / (1024.0 * 1024.0));
                return buf;
            }
            else if (bytes > 1024) {
                char buf[32];
                snprintf(buf, sizeof(buf), "%.1f KB", bytes / 1024.0);
                return buf;
            }
            return std::to_string(bytes) + " B";
        }

        static std::string FormatDuration(double seconds) {
            if (seconds <= 0) return "0s";
            int totalSeconds = (int)seconds;
            int hours = totalSeconds / 3600;
            int minutes = (totalSeconds % 3600) / 60;
            int secs = totalSeconds % 60;
            if (hours > 0) {
                char buf[32];
                snprintf(buf, sizeof(buf), "%d:%02d:%02d", hours, minutes, secs);
                return buf;
            }
            else {
                char buf[32];
                snprintf(buf, sizeof(buf), "%d:%02d", minutes, secs);
                return buf;
            }
        }

        static std::string GetChannelString(int channels) {
            if (channels == 3) return "RGB";
            if (channels == 4) return "RGBA";
            if (channels == 1) return "Grayscale";
            if (channels == 2) return "2ch";
            return std::to_string(channels) + "ch";
        }

        static uint64_t SafeFileSize(const std::string& path) {
            if (path.empty()) return 0;
            try {
                return static_cast<uint64_t>(std::filesystem::file_size(path));
            }
            catch (...) {
                return 0;
            }
        }

        static std::string CombineDateTime(const std::string& date, const std::string& time) {
            if (date.empty()) return "";
            if (time.empty()) return date;
            return date + " " + time;
        }

        static bool IsImageLoaded(const ECS::ImageComponent* img) {
            return img != nullptr && img->width > 0 && img->height > 0 && img->imageData != nullptr;
        }

        static bool IsVideoLoaded(const ECS::VideoComponent* vid) {
            return vid != nullptr && vid->width > 0 && vid->height > 0;
        }

        static bool IsAudioLoaded(const ECS::AudioComponent* aud) {
            return aud != nullptr && !aud->pcmData.empty();
        }

        static GLuint GetEntityTextureID(ECS::EntityManager* entityManager, ECS::EntityID entityID) {
            if (!entityManager) return 0;
            if (!entityManager->IsEntityValid(entityID)) return 0;

            auto texSys = entityManager->GetSystem<ECS::TextureSystem>();
            if (!texSys) return 0;

            GLuint tex = texSys->GetTextureID(entityID);
            if (tex == 0) {
                texSys->EnsureTextureForEntity(entityID);
                tex = texSys->GetTextureID(entityID);
            }
            return tex;
        }

        static bool EntityHasVideo(ECS::EntityManager* mgr, ECS::EntityID id) {
            return mgr && mgr->IsEntityValid(id) &&
                (mgr->HasComponent<ECS::VideoComponent>(id) ||
                    mgr->HasComponent<ECS::PreviewVideoComponent>(id) ||
                    mgr->HasComponent<ECS::InputVideoComponent>(id) ||
                    mgr->HasComponent<ECS::OutputVideoComponent>(id));
        }

        static bool EntityHasAudio(ECS::EntityManager* mgr, ECS::EntityID id) {
            return mgr && mgr->IsEntityValid(id) &&
                (mgr->HasComponent<ECS::AudioComponent>(id) ||
                    mgr->HasComponent<ECS::InputAudioComponent>(id) ||
                    mgr->HasComponent<ECS::OutputAudioComponent>(id));
        }

        static bool EntityHasImage(ECS::EntityManager* mgr, ECS::EntityID id) {
            return mgr && mgr->IsEntityValid(id) &&
                (mgr->HasComponent<ECS::ImageComponent>(id) ||
                    mgr->HasComponent<ECS::PreviewImageComponent>(id) ||
                    mgr->HasComponent<ECS::InputImageComponent>(id) ||
                    mgr->HasComponent<ECS::OutputImageComponent>(id) ||
                    mgr->HasComponent<ECS::MaskImageComponent>(id) ||
                    mgr->HasComponent<ECS::EndImageComponent>(id));
        }

        static void DrawTypeBadges(ECS::EntityManager* mgr, ECS::EntityID id,
            const ImVec2& rectMin) {
            if (!mgr || id == 0) return;
            if (!mgr->IsEntityValid(id)) return;

            std::string badge;
            if (EntityHasVideo(mgr, id)) badge += Icon::Video();
            if (EntityHasAudio(mgr, id)) badge += Icon::VolumeHigh();
            if (EntityHasImage(mgr, id)) badge += Icon::Image();
            if (badge.empty()) return;

            ImVec2 textSize = ImGui::CalcTextSize(badge.c_str());
            ImVec2 boxMin(rectMin.x + 4.0f, rectMin.y + 4.0f);
            ImVec2 boxMax(boxMin.x + textSize.x + 6.0f, boxMin.y + textSize.y + 4.0f);
            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddRectFilled(boxMin, boxMax, IM_COL32(0, 0, 0, 160), 3.0f);
            ImGui::SetCursorScreenPos(ImVec2(boxMin.x + 3.0f, boxMin.y + 2.0f));
            ImGui::TextUnformatted(badge.c_str());
        }

        static void DrawAudioIcon(const ImVec2& rectMin, const ImVec2& rectMax) {
            const std::string icon = Icon::VolumeHigh();
            ImVec2 textSize = ImGui::CalcTextSize(icon.c_str());
            ImVec2 textPos(rectMin.x + (rectMax.x - rectMin.x - textSize.x) * 0.5f,
                rectMin.y + (rectMax.y - rectMin.y - textSize.y) * 0.5f);
            ImGui::SetCursorScreenPos(textPos);
            ImGui::TextUnformatted(icon.c_str());
        }

        static std::string TruncateToWidth(const std::string& text, float maxWidth) {
            if (maxWidth <= 0.0f) return "";
            ImVec2 full = ImGui::CalcTextSize(text.c_str());
            if (full.x <= maxWidth) return text;
            std::string result = text;
            while (!result.empty()) {
                result.pop_back();
                std::string candidate = result + "...";
                if (ImGui::CalcTextSize(candidate.c_str()).x <= maxWidth) {
                    return candidate;
                }
            }
            return "...";
        }

        void BeginListMode(float thumbnailSize) {
            (void)thumbnailSize;
        }

        void EndListMode() {
        }

        static void DrawLeafDetail(const char* label, const std::string& value) {
            if (value.empty()) return;
            ImGui::Bullet();
            ImGui::SameLine();
            ImGui::TextDisabled("%s", label);
            ImGui::SameLine();
            ImGui::TextUnformatted(value.c_str());
        }

        static void DrawLeafDetail(const char* label, const std::string& value, ImVec4 color) {
            if (value.empty()) return;
            ImGui::Bullet();
            ImGui::SameLine();
            ImGui::TextDisabled("%s", label);
            ImGui::SameLine();
            ImGui::TextColored(color, "%s", value.c_str());
        }

        static ImVec2 FitAspect(int width, int height, float boxSize) {
            float aspect = (height > 0) ? (float)width / (float)height : 1.0f;
            ImVec2 size(boxSize, boxSize);
            if (aspect > 1.0f) size.y = boxSize / aspect;
            else size.x = boxSize * aspect;
            return size;
        }

        void RenderListRow(
            const std::variant<const ECS::ImageComponent*, const ECS::VideoComponent*, const ECS::AudioComponent*>& component,
            ECS::EntityID entityID,
            size_t index,
            float thumbnailSize,
            std::function<void(ECS::EntityID)> onSelect,
            Utils::ContextMenuUtils* contextMenuUtils,
            bool isEntityLoaded,
            ECS::EntityID activeEntityID,
            ECS::EntityManager* entityManager
        ) {
            const ECS::ImageComponent* img = nullptr;
            const ECS::VideoComponent* vid = nullptr;
            const ECS::AudioComponent* aud = nullptr;
            bool isImage = false;
            bool isVideo = false;
            bool isAudio = false;

            if (auto* p = std::get_if<const ECS::ImageComponent*>(&component)) {
                img = *p;
                isImage = true;
            }
            else if (auto* p = std::get_if<const ECS::VideoComponent*>(&component)) {
                vid = *p;
                isVideo = true;
            }
            else if (auto* p = std::get_if<const ECS::AudioComponent*>(&component)) {
                aud = *p;
                isAudio = true;
            }

            if (!img && !vid && !aud) return;

            if (isImage && !IsImageLoaded(img)) return;
            if (isVideo && !IsVideoLoaded(vid)) return;
            if (isAudio && !IsAudioLoaded(aud)) return;

            std::string filePath, fileName;
            GLuint textureID = 0;
            int width = 0, height = 0, channels = 0;
            uint64_t fileSize = 0;
            std::string fileDate, fileTime;
            double duration = 0.0;
            bool hasExif = false, hasLSB = false, hasAniStudioMetadata = false;
            float fps = 0.0f;
            int sampleRate = 0;

            if (isImage) {
                filePath = img->filePath;
                fileName = img->fileName;
                textureID = GetEntityTextureID(entityManager, entityID);
                width = img->width;
                height = img->height;
                channels = img->channels;
                fileSize = img->fileSize;
                fileDate = img->fileDate;
                fileTime = img->fileTime;
                hasExif = img->hasExifData;
                hasLSB = img->hasLSBData;
                hasAniStudioMetadata = img->hasAniStudioMetadata;
            }
            else if (isVideo) {
                filePath = vid->filePath;
                fileName = vid->fileName;
                textureID = GetEntityTextureID(entityManager, entityID);
                width = vid->width;
                height = vid->height;
                channels = 4;
                fileSize = vid->fileSize;
                fileDate = vid->fileDate;
                fileTime = vid->fileTime;
                hasExif = vid->hasExifData;
                hasLSB = vid->hasLSBData;
                hasAniStudioMetadata = vid->hasAniStudioMetadata;
                duration = (vid->frameCount > 0) ? vid->frameCount / vid->fps : 0.0;
                fps = static_cast<float>(vid->fps);
            }
            else if (isAudio) {
                filePath = aud->filePath;
                fileName = aud->fileName;
                channels = aud->channels;
                sampleRate = aud->sampleRate;
                duration = aud->duration;
                fileSize = SafeFileSize(filePath);
                hasExif = aud->hasExifData;
                hasLSB = aud->hasLSBData;
                hasAniStudioMetadata = aud->hasAniStudioMetadata;
            }

            ImGui::PushID(("list_node_" + std::to_string(index)).c_str());

            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_OpenOnDoubleClick |
                ImGuiTreeNodeFlags_SpanAvailWidth;

            if (entityID == activeEntityID && entityID != 0) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            const bool isLeaf =
                fileDate.empty() && fileTime.empty() &&
                fileSize == 0 && width == 0 && height == 0 &&
                duration <= 0.0 && fps <= 0.0f && sampleRate == 0;

            if (isLeaf) {
                flags |= ImGuiTreeNodeFlags_Leaf;
            }

            const float lineHeight = ImGui::GetTextLineHeight();
            const float cellSize = lineHeight + 8.0f;

            ImVec2 rowStart = ImGui::GetCursorScreenPos();

            bool nodeOpen = ImGui::TreeNodeEx("##node", flags, "%s", "");

            bool itemHovered = ImGui::IsItemHovered();
            bool itemActive = (entityID == activeEntityID && entityID != 0);
            if (itemActive || itemHovered) {
                ImVec2 rowMin = rowStart;
                ImVec2 rowMax = ImVec2(rowStart.x + ImGui::GetContentRegionAvail().x,
                    rowStart.y + cellSize);
                ImU32 bg = itemActive
                    ? IM_COL32(80, 120, 200, 90)
                    : IM_COL32(120, 120, 120, 40);
                ImGui::GetWindowDrawList()->AddRectFilled(rowMin, rowMax, bg, 3.0f);
            }

            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                if (onSelect) onSelect(entityID);
            }

            if (isEntityLoaded && entityID != 0) {
                if (ImGui::BeginDragDropSource()) {
                    nlohmann::json payload;
                    payload["entityID"] = entityID;
                    ImGui::SetDragDropPayload(GUI::DragDrop::PAYLOAD_ENTITY,
                        payload.dump().c_str(), payload.dump().size() + 1);
                    if (textureID != 0) {
                        ImGui::Image((ImTextureID)(intptr_t)textureID, ImVec2(64, 64));
                    }
                    ImGui::Text("%s", fileName.c_str());
                    ImGui::EndDragDropSource();
                }
            }
            else {
                if (ImGui::BeginDragDropSource()) {
                    nlohmann::json payload;
                    payload["filePath"] = filePath;
                    payload["fileType"] = GUI::DragDrop::GuessMediaType(filePath);
                    ImGui::SetDragDropPayload(GUI::DragDrop::PAYLOAD_FILE_PATH,
                        payload.dump().c_str(), payload.dump().size() + 1);
                    ImGui::Text("%s", fileName.c_str());
                    ImGui::EndDragDropSource();
                }
            }

            if (ImGui::BeginPopupContextItem()) {
                if (isEntityLoaded && entityID != 0) {
                    contextMenuUtils->RenderEntityContextMenuItems(entityID);
                }
                else {
                    if (ImGui::MenuItem("Load Asset")) {
                        if (onSelect) onSelect(entityID);
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Copy File Path")) {
                        ImGui::SetClipboardText(filePath.c_str());
                    }
                }
                ImGui::EndPopup();
            }

            ImGui::SameLine();

            ImVec2 thumbMin = ImGui::GetCursorScreenPos();
            ImVec2 thumbMax = ImVec2(thumbMin.x + cellSize, thumbMin.y + cellSize);

            if (isAudio) {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                dl->AddRectFilled(thumbMin, thumbMax, IM_COL32(60, 60, 80, 200), 3.0f);
                std::string glyph = Icon::VolumeHigh();
                ImVec2 ts = ImGui::CalcTextSize(glyph.c_str());
                ImVec2 tp(thumbMin.x + (cellSize - ts.x) * 0.5f,
                    thumbMin.y + (cellSize - ts.y) * 0.5f);
                dl->AddText(tp, IM_COL32(220, 220, 220, 255), glyph.c_str());
            }
            else if (textureID != 0 && width > 0 && height > 0) {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImVec2 size = FitAspect(width, height, cellSize);
                ImVec2 imagePos(thumbMin.x + (cellSize - size.x) * 0.5f,
                    thumbMin.y + (cellSize - size.y) * 0.5f);
                dl->AddRectFilled(thumbMin, thumbMax, IM_COL32(30, 30, 40, 200), 3.0f);
                dl->AddImage((ImTextureID)(intptr_t)textureID, imagePos, imagePos + size);
            }
            else {
                ImGui::GetWindowDrawList()->AddRectFilled(
                    thumbMin, thumbMax, IM_COL32(60, 60, 80, 200), 3.0f);
            }

            ImGui::SetCursorScreenPos(ImVec2(thumbMax.x + 6.0f, thumbMin.y));
            ImVec2 iconMin = ImGui::GetCursorScreenPos();
            ImVec2 iconMax = ImVec2(iconMin.x + cellSize, iconMin.y + cellSize);
            {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                dl->AddRectFilled(iconMin, iconMax, IM_COL32(50, 50, 70, 220), 3.0f);

                std::string iconGlyph;
                if (isVideo) iconGlyph = Icon::Video();
                else if (isAudio) iconGlyph = Icon::VolumeHigh();
                else if (isImage) iconGlyph = Icon::Image();

                if (!iconGlyph.empty()) {
                    ImVec2 ts = ImGui::CalcTextSize(iconGlyph.c_str());
                    ImVec2 tp(iconMin.x + (cellSize - ts.x) * 0.5f,
                        iconMin.y + (cellSize - ts.y) * 0.5f);
                    dl->AddText(tp, IM_COL32(230, 230, 230, 255), iconGlyph.c_str());
                }
            }

            ImGui::SetCursorScreenPos(ImVec2(iconMax.x + 6.0f, iconMin.y));
            ImGui::TextUnformatted(fileName.c_str());

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("File: %s", fileName.c_str());
                ImGui::Text("Path: %s", filePath.c_str());
                if ((isImage || isVideo) && width > 0 && height > 0) {
                    ImGui::Text("Dimensions: %dx%d", width, height);
                }
                if (channels > 0) {
                    ImGui::Text("Channels: %s", GetChannelString(channels).c_str());
                }
                if (fileSize > 0) {
                    ImGui::Text("Size: %s", FormatFileSize(fileSize).c_str());
                }
                if (!fileDate.empty()) {
                    ImGui::Text("Date: %s", fileDate.c_str());
                }
                if (!fileTime.empty()) {
                    ImGui::Text("Time: %s", fileTime.c_str());
                }
                if (isVideo && fps > 0) {
                    ImGui::Text("FPS: %.1f", fps);
                }
                if ((isVideo || isAudio) && duration > 0) {
                    ImGui::Text("Duration: %s", FormatDuration(duration).c_str());
                }
                if (isAudio && sampleRate > 0) {
                    ImGui::Text("Sample Rate: %d Hz", sampleRate);
                }
                if (hasAniStudioMetadata) {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "AniStudio Metadata");
                }
                if (hasExif && !hasAniStudioMetadata) {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "EXIF Metadata");
                }
                if (hasLSB && !hasAniStudioMetadata) {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "LSB Metadata");
                }

                if (textureID != 0 && width > 0 && height > 0) {
                    ImVec2 previewDims = FitAspect(width, height, thumbnailSize);
                    ImGui::Image((ImTextureID)(intptr_t)textureID, previewDims);
                }

                ImGui::EndTooltip();
            }

            if (nodeOpen) {
                ImGui::Indent(cellSize * 2.0f + 12.0f);

                if (textureID != 0 && width > 0 && height > 0) {
                    ImVec2 previewDims = FitAspect(width, height, thumbnailSize);
                    ImGui::Image((ImTextureID)(intptr_t)textureID, previewDims);
                    ImGui::Spacing();
                }
                else if (isAudio) {
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", Icon::VolumeHigh());
                    ImGui::Spacing();
                }

                if (width > 0 && height > 0) {
                    DrawLeafDetail("Dimensions", std::to_string(width) + " x " + std::to_string(height));
                }
                if (channels > 0) {
                    DrawLeafDetail("Channels", GetChannelString(channels));
                }
                if (fileSize > 0) {
                    DrawLeafDetail("Size", FormatFileSize(fileSize));
                }
                if (!fileDate.empty() || !fileTime.empty()) {
                    DrawLeafDetail("Date", CombineDateTime(fileDate, fileTime));
                }
                if (isVideo && duration > 0.0) {
                    DrawLeafDetail("Duration", FormatDuration(duration));
                }
                if (isVideo && fps > 0.0f) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%.1f", fps);
                    DrawLeafDetail("FPS", buf);
                }
                if (isAudio && duration > 0.0) {
                    DrawLeafDetail("Duration", FormatDuration(duration));
                }
                if (isAudio && sampleRate > 0) {
                    DrawLeafDetail("Sample Rate", std::to_string(sampleRate) + " Hz");
                }

                {
                    std::string statusValue;
                    if (hasAniStudioMetadata) statusValue = "AniStudio";
                    else if (hasExif) statusValue = "EXIF";
                    else if (hasLSB) statusValue = "LSB";
                    if (!statusValue.empty()) {
                        ImVec4 color = hasAniStudioMetadata
                            ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f)
                            : ImVec4(1.0f, 1.0f, 0.2f, 1.0f);
                        DrawLeafDetail("Metadata", statusValue, color);
                    }
                }

                if (!filePath.empty()) {
                    DrawLeafDetail("Path", filePath);
                }

                ImGui::Unindent(cellSize * 2.0f + 12.0f);

                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        void RenderThumbnail(
            const std::variant<const ECS::ImageComponent*, const ECS::VideoComponent*, const ECS::AudioComponent*>& component,
            ECS::EntityID entityID,
            size_t index,
            float thumbnailSize,
            DisplayMode mode,
            std::function<void(ECS::EntityID)> onSelect,
            Utils::ContextMenuUtils* contextMenuUtils,
            bool isEntityLoaded,
            ECS::EntityID activeEntityID,
            ECS::EntityManager* entityManager
        ) {
            const ECS::ImageComponent* img = nullptr;
            const ECS::VideoComponent* vid = nullptr;
            const ECS::AudioComponent* aud = nullptr;
            bool isImage = false;
            bool isVideo = false;
            bool isAudio = false;

            if (auto* p = std::get_if<const ECS::ImageComponent*>(&component)) {
                img = *p;
                isImage = true;
            }
            else if (auto* p = std::get_if<const ECS::VideoComponent*>(&component)) {
                vid = *p;
                isVideo = true;
            }
            else if (auto* p = std::get_if<const ECS::AudioComponent*>(&component)) {
                aud = *p;
                isAudio = true;
            }

            if (!img && !vid && !aud) return;

            if (isImage && !IsImageLoaded(img)) return;
            if (isVideo && !IsVideoLoaded(vid)) return;
            if (isAudio && !IsAudioLoaded(aud)) return;

            std::string filePath, fileName;
            GLuint textureID = 0;
            int width = 0, height = 0, channels = 0;
            uint64_t fileSize = 0;
            std::string fileDate, fileTime;
            double duration = 0.0;
            bool hasExif = false, hasLSB = false, hasAniStudioMetadata = false;
            float fps = 0.0f;
            int sampleRate = 0;

            if (isImage) {
                filePath = img->filePath;
                fileName = img->fileName;
                textureID = GetEntityTextureID(entityManager, entityID);
                width = img->width;
                height = img->height;
                channels = img->channels;
                fileSize = img->fileSize;
                fileDate = img->fileDate;
                fileTime = img->fileTime;
                hasExif = img->hasExifData;
                hasLSB = img->hasLSBData;
                hasAniStudioMetadata = img->hasAniStudioMetadata;
            }
            else if (isVideo) {
                filePath = vid->filePath;
                fileName = vid->fileName;
                textureID = GetEntityTextureID(entityManager, entityID);
                width = vid->width;
                height = vid->height;
                channels = 4;
                fileSize = vid->fileSize;
                fileDate = vid->fileDate;
                fileTime = vid->fileTime;
                hasExif = vid->hasExifData;
                hasLSB = vid->hasLSBData;
                hasAniStudioMetadata = vid->hasAniStudioMetadata;
                duration = (vid->frameCount > 0) ? vid->frameCount / vid->fps : 0.0;
                fps = static_cast<float>(vid->fps);
            }
            else if (isAudio) {
                filePath = aud->filePath;
                fileName = aud->fileName;
                channels = aud->channels;
                sampleRate = aud->sampleRate;
                duration = aud->duration;
                fileSize = SafeFileSize(filePath);
                hasExif = aud->hasExifData;
                hasLSB = aud->hasLSBData;
                hasAniStudioMetadata = aud->hasAniStudioMetadata;
            }

            if (mode == DisplayMode::Compact) {
                const float cellWidth = thumbnailSize + 8.0f;

                ImGui::PushID(("compact_" + std::to_string(index)).c_str());
                ImGui::BeginGroup();

                ImVec2 cellMin = ImGui::GetCursorScreenPos();

                ImGui::InvisibleButton("##compact_cell",
                    ImVec2(thumbnailSize, thumbnailSize));
                bool clicked = ImGui::IsItemClicked();
                bool hovered = ImGui::IsItemHovered();

                if (isEntityLoaded && entityID != 0) {
                    if (ImGui::BeginDragDropSource()) {
                        nlohmann::json payload;
                        payload["entityID"] = entityID;
                        ImGui::SetDragDropPayload(GUI::DragDrop::PAYLOAD_ENTITY,
                            payload.dump().c_str(), payload.dump().size() + 1);
                        if (textureID != 0) {
                            ImGui::Image((ImTextureID)(intptr_t)textureID, ImVec2(64, 64));
                        }
                        ImGui::Text("%s", fileName.c_str());
                        ImGui::EndDragDropSource();
                    }
                }
                else {
                    if (ImGui::BeginDragDropSource()) {
                        nlohmann::json payload;
                        payload["filePath"] = filePath;
                        payload["fileType"] = GUI::DragDrop::GuessMediaType(filePath);
                        ImGui::SetDragDropPayload(GUI::DragDrop::PAYLOAD_FILE_PATH,
                            payload.dump().c_str(), payload.dump().size() + 1);
                        ImGui::Text("%s", fileName.c_str());
                        ImGui::EndDragDropSource();
                    }
                }

                ImDrawList* dl = ImGui::GetWindowDrawList();

                ImVec2 thumbMin = cellMin;
                ImVec2 thumbMax = ImVec2(cellMin.x + thumbnailSize,
                    cellMin.y + thumbnailSize);

                if (isAudio) {
                    dl->AddRectFilled(thumbMin, thumbMax, IM_COL32(60, 60, 80, 200), 4.0f);
                    DrawAudioIcon(thumbMin, thumbMax);
                }
                else if (textureID != 0 && width > 0 && height > 0) {
                    ImVec2 size = FitAspect(width, height, thumbnailSize);
                    ImVec2 imagePos(thumbMin.x + (thumbnailSize - size.x) * 0.5f,
                        thumbMin.y + (thumbnailSize - size.y) * 0.5f);
                    dl->AddImage((ImTextureID)(intptr_t)textureID, imagePos, imagePos + size);
                }
                else {
                    dl->AddRectFilled(thumbMin, thumbMax, IM_COL32(60, 60, 80, 200), 4.0f);
                }

                DrawTypeBadges(entityManager, entityID, thumbMin);

                if (entityID == activeEntityID && entityID != 0) {
                    dl->AddRect(thumbMin, thumbMax, IM_COL32(255, 255, 0, 200), 4.0f, 0, 2.0f);
                }
                else if (hovered) {
                    dl->AddRect(thumbMin, thumbMax, IM_COL32(255, 255, 255, 160), 4.0f, 0, 1.5f);
                }

                if (ImGui::BeginPopupContextItem()) {
                    if (isEntityLoaded && entityID != 0) {
                        contextMenuUtils->RenderEntityContextMenuItems(entityID);
                    }
                    else {
                        if (ImGui::MenuItem("Load Asset")) {
                            if (onSelect) onSelect(entityID);
                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem("Copy File Path")) {
                            ImGui::SetClipboardText(filePath.c_str());
                        }
                    }
                    ImGui::EndPopup();
                }

                ImGui::SetCursorScreenPos(ImVec2(cellMin.x, thumbMax.y + 4.0f));

                std::string displayName = TruncateToWidth(fileName, thumbnailSize);
                float textWidth = ImGui::CalcTextSize(displayName.c_str()).x;
                float textOffset = (thumbnailSize - textWidth) * 0.5f;
                if (textOffset < 0.0f) textOffset = 0.0f;

                ImGui::SetCursorScreenPos(ImVec2(cellMin.x + textOffset, thumbMax.y + 4.0f));
                ImGui::TextUnformatted(displayName.c_str());

                if (clicked) {
                    if (onSelect) onSelect(entityID);
                }

                if (hovered) {
                    ImGui::BeginTooltip();
                    ImGui::Text("File: %s", fileName.c_str());
                    ImGui::Text("Path: %s", filePath.c_str());
                    if (isImage || isVideo) {
                        if (width > 0 && height > 0) {
                            ImGui::Text("Dimensions: %dx%d", width, height);
                        }
                    }
                    if (channels > 0) {
                        ImGui::Text("Channels: %s", GetChannelString(channels).c_str());
                    }
                    if (fileSize > 0) {
                        ImGui::Text("Size: %s", FormatFileSize(fileSize).c_str());
                    }
                    if (!fileDate.empty()) {
                        ImGui::Text("Date: %s", fileDate.c_str());
                    }
                    if (!fileTime.empty()) {
                        ImGui::Text("Time: %s", fileTime.c_str());
                    }
                    if (isVideo && fps > 0) {
                        ImGui::Text("FPS: %.1f", fps);
                    }
                    if ((isVideo || isAudio) && duration > 0) {
                        ImGui::Text("Duration: %s", FormatDuration(duration).c_str());
                    }
                    if (isAudio && sampleRate > 0) {
                        ImGui::Text("Sample Rate: %d Hz", sampleRate);
                    }
                    if (hasAniStudioMetadata) {
                        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "AniStudio Metadata");
                    }
                    if (hasExif && !hasAniStudioMetadata) {
                        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "EXIF Metadata");
                    }
                    if (hasLSB && !hasAniStudioMetadata) {
                        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "LSB Metadata");
                    }
                    ImGui::EndTooltip();
                }

                ImGui::EndGroup();
                ImGui::PopID();
                (void)cellWidth;
            }
            else if (mode == DisplayMode::Detailed) {
                const float imageSize = thumbnailSize - 10.0f;
                const float pad = 6.0f;
                const float infoWidth = 190.0f;
                const float childWidth = pad + imageSize + pad + infoWidth + pad;
                const float childHeight = std::max(imageSize, 84.0f) + pad * 2.0f;

                ImGui::PushID(("detailed_" + std::to_string(index)).c_str());
                ImGui::BeginGroup();

                ImVec2 cellMin = ImGui::GetCursorScreenPos();

                ImGui::InvisibleButton("##detail_cell",
                    ImVec2(childWidth, childHeight));
                bool cellHovered = ImGui::IsItemHovered();
                bool cellClicked = ImGui::IsItemClicked();

                if (isEntityLoaded && entityID != 0) {
                    if (ImGui::BeginDragDropSource()) {
                        nlohmann::json payload;
                        payload["entityID"] = entityID;
                        ImGui::SetDragDropPayload(GUI::DragDrop::PAYLOAD_ENTITY,
                            payload.dump().c_str(), payload.dump().size() + 1);
                        if (textureID != 0) {
                            ImGui::Image((ImTextureID)(intptr_t)textureID, ImVec2(64, 64));
                        }
                        ImGui::Text("%s", fileName.c_str());
                        ImGui::EndDragDropSource();
                    }
                }
                else {
                    if (ImGui::BeginDragDropSource()) {
                        nlohmann::json payload;
                        payload["filePath"] = filePath;
                        payload["fileType"] = GUI::DragDrop::GuessMediaType(filePath);
                        ImGui::SetDragDropPayload(GUI::DragDrop::PAYLOAD_FILE_PATH,
                            payload.dump().c_str(), payload.dump().size() + 1);
                        ImGui::Text("%s", fileName.c_str());
                        ImGui::EndDragDropSource();
                    }
                }

                ImDrawList* dl = ImGui::GetWindowDrawList();

                ImVec2 cardMin = cellMin;
                ImVec2 cardMax = ImVec2(cellMin.x + childWidth, cellMin.y + childHeight);

                dl->AddRectFilled(cardMin, cardMax, IM_COL32(35, 35, 45, 220), 4.0f);

                ImU32 borderCol;
                float borderThick;
                if (entityID == activeEntityID && entityID != 0) {
                    borderCol = IM_COL32(255, 255, 0, 220);
                    borderThick = 2.0f;
                }
                else if (cellHovered) {
                    borderCol = IM_COL32(255, 255, 255, 200);
                    borderThick = 1.5f;
                }
                else {
                    borderCol = IM_COL32(90, 90, 110, 200);
                    borderThick = 1.0f;
                }
                dl->AddRect(cardMin, cardMax, borderCol, 4.0f, 0, borderThick);

                ImVec2 thumbMin(cardMin.x + pad, cardMin.y + pad);
                ImVec2 thumbMax(thumbMin.x + imageSize, thumbMin.y + imageSize);

                if (isAudio) {
                    dl->AddRectFilled(thumbMin, thumbMax, IM_COL32(60, 60, 80, 200), 4.0f);
                    DrawAudioIcon(thumbMin, thumbMax);
                }
                else if (textureID != 0 && width > 0 && height > 0) {
                    ImVec2 size = FitAspect(width, height, imageSize);
                    ImVec2 imagePos(thumbMin.x + (imageSize - size.x) * 0.5f,
                        thumbMin.y + (imageSize - size.y) * 0.5f);
                    dl->AddImage((ImTextureID)(intptr_t)textureID, imagePos, imagePos + size);
                }
                else {
                    dl->AddRectFilled(thumbMin, thumbMax, IM_COL32(60, 60, 80, 200), 4.0f);
                }

                DrawTypeBadges(entityManager, entityID, thumbMin);

                float textX = thumbMax.x + pad;
                float infoHeight = imageSize;
                float infoWidthAvail = cardMax.x - textX - pad;

                ImGui::SetCursorScreenPos(ImVec2(textX, cardMin.y + pad));

                ImGui::BeginChild(
                    ("##detail_info_" + std::to_string(index)).c_str(),
                    ImVec2(infoWidthAvail, infoHeight),
                    false,
                    ImGuiWindowFlags_HorizontalScrollbar
                );

                float availableTextWidth =
                    infoWidthAvail - ImGui::GetStyle().ScrollbarSize - 4.0f;

                std::string displayName = TruncateToWidth(fileName, availableTextWidth);
                ImGui::TextUnformatted(displayName.c_str());

                std::string sizeStr = FormatFileSize(fileSize);
                std::string channelStr = GetChannelString(channels);
                ImGui::Text("%s | %s", sizeStr.c_str(), channelStr.c_str());

                if (isImage || isVideo) {
                    if (width > 0 && height > 0) {
                        ImGui::Text("%dx%d", width, height);
                    }
                }
                if (!fileDate.empty()) ImGui::Text("%s", fileDate.c_str());
                if (!fileTime.empty()) ImGui::Text("%s", fileTime.c_str());

                if (isVideo) {
                    if (duration > 0) {
                        ImGui::Text("Duration: %s", FormatDuration(duration).c_str());
                    }
                    if (fps > 0) ImGui::Text("%.1f fps", fps);
                }
                else if (isAudio) {
                    if (duration > 0) {
                        ImGui::Text("Duration: %s", FormatDuration(duration).c_str());
                    }
                    if (sampleRate > 0) ImGui::Text("%d Hz", sampleRate);
                }

                ImVec4 exifColor;
                if (hasExif) {
                    exifColor = hasAniStudioMetadata
                        ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f)
                        : ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                }
                else {
                    exifColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                }

                ImVec4 lsbColor;
                if (hasLSB) {
                    lsbColor = hasAniStudioMetadata
                        ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f)
                        : ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                }
                else {
                    lsbColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                }

                ImGui::TextColored(exifColor, "EXIF");
                ImGui::SameLine();
                ImGui::Text("|");
                ImGui::SameLine();
                ImGui::TextColored(lsbColor, "LSB");

                ImGui::EndChild();

                if (ImGui::BeginPopupContextItem()) {
                    if (isEntityLoaded && entityID != 0) {
                        contextMenuUtils->RenderEntityContextMenuItems(entityID);
                    }
                    else {
                        if (ImGui::MenuItem("Load Asset")) {
                            if (onSelect) onSelect(entityID);
                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem("Copy File Path")) {
                            ImGui::SetClipboardText(filePath.c_str());
                        }
                    }
                    ImGui::EndPopup();
                }

                if (cellClicked) {
                    if (onSelect) onSelect(entityID);
                }

                if (cellHovered) {
                    ImGui::BeginTooltip();
                    ImGui::Text("File: %s", fileName.c_str());
                    ImGui::Text("Path: %s", filePath.c_str());
                    if (isImage || isVideo) {
                        if (width > 0 && height > 0) {
                            ImGui::Text("Dimensions: %dx%d", width, height);
                        }
                    }
                    if (channels > 0) {
                        ImGui::Text("Channels: %s", GetChannelString(channels).c_str());
                    }
                    if (fileSize > 0) {
                        ImGui::Text("Size: %s", FormatFileSize(fileSize).c_str());
                    }
                    if (!fileDate.empty()) {
                        ImGui::Text("Date: %s", fileDate.c_str());
                    }
                    if (!fileTime.empty()) {
                        ImGui::Text("Time: %s", fileTime.c_str());
                    }
                    if (isVideo && fps > 0) {
                        ImGui::Text("FPS: %.1f", fps);
                    }
                    if ((isVideo || isAudio) && duration > 0) {
                        ImGui::Text("Duration: %s", FormatDuration(duration).c_str());
                    }
                    if (isAudio && sampleRate > 0) {
                        ImGui::Text("Sample Rate: %d Hz", sampleRate);
                    }
                    if (hasAniStudioMetadata) {
                        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "AniStudio Metadata");
                    }
                    if (hasExif && !hasAniStudioMetadata) {
                        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "EXIF Metadata");
                    }
                    if (hasLSB && !hasAniStudioMetadata) {
                        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "LSB Metadata");
                    }
                    ImGui::EndTooltip();
                }

                ImGui::EndGroup();
                ImGui::PopID();
            }
        }

    }
}