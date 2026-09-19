#include "ThumbnailUtils.hpp"
#include "DragDropUtils.hpp"
#include "TextureComponent.hpp"
#include <imgui.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

namespace GUI {
    namespace Thumbnail {

        static bool s_listTableOpen = false;

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
            if (!entityManager->HasComponent<ECS::TextureComponent>(entityID)) return 0;
            return entityManager->GetComponent<ECS::TextureComponent>(entityID).textureID;
        }

        void BeginListMode(float thumbnailSize) {
            if (s_listTableOpen) return;

            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 4.0f));

            float nameMinWidth = ImGui::CalcTextSize("Name").x + 20.0f;
            float sizeMinWidth = ImGui::CalcTextSize("Size").x + 20.0f;
            float channelsMinWidth = ImGui::CalcTextSize("Channels").x + 20.0f;
            float dimsMinWidth = ImGui::CalcTextSize("Dimensions").x + 20.0f;
            float durationMinWidth = ImGui::CalcTextSize("Duration").x + 20.0f;
            float fpsMinWidth = ImGui::CalcTextSize("FPS").x + 20.0f;
            float sampleRateMinWidth = ImGui::CalcTextSize("Sample Rate").x + 20.0f;
            float dateMinWidth = ImGui::CalcTextSize("Date/Time").x + 20.0f;
            float statusMinWidth = ImGui::CalcTextSize("Status").x + 20.0f;

            bool tableOpen = ImGui::BeginTable("ThumbnailList", 10,
                ImGuiTableFlags_BordersV | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable);

            if (tableOpen) {
                ImGui::TableSetupColumn("Thumb", ImGuiTableColumnFlags_WidthFixed, thumbnailSize);
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, nameMinWidth);
                ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, sizeMinWidth);
                ImGui::TableSetupColumn("Channels", ImGuiTableColumnFlags_WidthFixed, channelsMinWidth);
                ImGui::TableSetupColumn("Dimensions", ImGuiTableColumnFlags_WidthFixed, dimsMinWidth);
                ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthFixed, durationMinWidth);
                ImGui::TableSetupColumn("FPS", ImGuiTableColumnFlags_WidthFixed, fpsMinWidth);
                ImGui::TableSetupColumn("Sample Rate", ImGuiTableColumnFlags_WidthFixed, sampleRateMinWidth);
                ImGui::TableSetupColumn("Date/Time", ImGuiTableColumnFlags_WidthFixed, dateMinWidth);
                ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, statusMinWidth);

                ImGui::TableHeadersRow();
                s_listTableOpen = true;
            }
            else {
                ImGui::PopStyleVar();
            }
        }

        void EndListMode() {
            if (s_listTableOpen) {
                ImGui::EndTable();
                ImGui::PopStyleVar();
                s_listTableOpen = false;
            }
        }

        void RenderListRow(
            const std::variant<const ECS::ImageComponent*, const ECS::VideoComponent*, const ECS::AudioComponent*>& component,
            size_t index,
            float thumbnailSize,
            std::function<void(ECS::EntityID)> onSelect,
            Utils::ContextMenuUtils* contextMenuUtils,
            bool isEntityLoaded,
            ECS::EntityID activeEntityID,
            ECS::EntityManager* entityManager
        ) {
            if (!s_listTableOpen) return;

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
            ECS::EntityID entityID = 0;
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
                entityID = img->GetID();
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
                entityID = vid->GetID();
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
                entityID = aud->GetID();
                channels = aud->channels;
                sampleRate = aud->sampleRate;
                duration = aud->duration;
                fileSize = std::filesystem::file_size(filePath);
                hasExif = aud->hasExifData;
                hasLSB = aud->hasLSBData;
                hasAniStudioMetadata = aud->hasAniStudioMetadata;
            }

            float rowHeight = thumbnailSize + 8.0f;
            ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);

            if (entityID == activeEntityID && entityID != 0) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(255, 255, 0, 80));
            }

            ImGui::TableSetColumnIndex(0);
            ImVec2 cellPos = ImGui::GetCursorPos();

            ImGui::PushID(("row_select_" + std::to_string(index)).c_str());

            ImGui::Selectable("##row", false, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0.0f, rowHeight));
            if (ImGui::IsItemClicked()) {
                if (onSelect) onSelect(entityID);
            }

            ImVec2 selectableMin = ImGui::GetItemRectMin();
            ImVec2 selectableMax = ImGui::GetItemRectMax();

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

            ImGui::SetCursorPos(cellPos);
            float thumbSize = thumbnailSize - 4.0f;
            float thumbColWidth = ImGui::GetContentRegionAvail().x;

            if (isAudio) {
                float offsetX = (thumbColWidth - thumbSize) * 0.5f;
                float offsetY = (rowHeight - thumbSize) * 0.5f;
                ImGui::SetCursorPos(ImVec2(cellPos.x + offsetX, cellPos.y + offsetY));
                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImVec2 pos = ImGui::GetCursorScreenPos();
                dl->AddRectFilled(pos, pos + ImVec2(thumbSize, thumbSize), IM_COL32(60, 60, 80, 200));
                float iconSize = thumbSize * 0.6f;
                float iconX = pos.x + (thumbSize - iconSize) * 0.5f;
                float iconY = pos.y + (thumbSize - iconSize) * 0.5f;
                dl->AddText(ImVec2(iconX, iconY), IM_COL32(255, 255, 255, 255), "?");
            }
            else if (textureID != 0 && width > 0 && height > 0) {
                float aspect = (height > 0) ? (float)width / height : 1.0f;
                ImVec2 size;
                if (aspect > 1.0f) {
                    size.x = thumbSize;
                    size.y = thumbSize / aspect;
                }
                else {
                    size.x = thumbSize * aspect;
                    size.y = thumbSize;
                }
                float offsetX = (thumbColWidth - size.x) * 0.5f;
                float offsetY = (rowHeight - size.y) * 0.5f;
                ImGui::SetCursorPos(ImVec2(cellPos.x + offsetX, cellPos.y + offsetY));
                ImGui::Image((ImTextureID)(intptr_t)textureID, size);
            }
            else {
                float offsetX = (thumbColWidth - thumbSize) * 0.5f;
                float offsetY = (rowHeight - thumbSize) * 0.5f;
                ImGui::SetCursorPos(ImVec2(cellPos.x + offsetX, cellPos.y + offsetY));
                ImGui::Image((ImTextureID)(intptr_t)0, ImVec2(thumbSize, thumbSize));
                ImVec2 textSize = ImGui::CalcTextSize("?");
                float textX = cellPos.x + offsetX + (thumbSize - textSize.x) * 0.5f;
                float textY = cellPos.y + offsetY + (thumbSize - textSize.y) * 0.5f;
                ImGui::GetWindowDrawList()->AddText(ImVec2(textX, textY), IM_COL32(200, 200, 200, 255), "?");
            }

            ImGui::PopID();

            auto centerText = [](const std::string& text) {
                float avail = ImGui::GetContentRegionAvail().x;
                float textWidth = ImGui::CalcTextSize(text.c_str()).x;
                float offset = (avail - textWidth) * 0.5f;
                if (offset > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
                ImGui::TextWrapped("%s", text.c_str());
                };

            ImGui::TableSetColumnIndex(1);
            std::string displayName = fileName;
            if (displayName.length() > 30) displayName = displayName.substr(0, 28) + "...";
            centerText(displayName);

            ImGui::TableSetColumnIndex(2);
            centerText(FormatFileSize(fileSize));

            ImGui::TableSetColumnIndex(3);
            centerText(GetChannelString(channels));

            ImGui::TableSetColumnIndex(4);
            if (isImage || isVideo) {
                if (width > 0 && height > 0)
                    centerText(std::to_string(width) + "x" + std::to_string(height));
                else
                    centerText("");
            }
            else {
                centerText("");
            }

            ImGui::TableSetColumnIndex(5);
            if ((isVideo || isAudio) && duration > 0)
                centerText(FormatDuration(duration));
            else
                centerText("");

            ImGui::TableSetColumnIndex(6);
            if (isVideo && fps > 0)
                centerText(std::to_string(fps).substr(0, 4));
            else
                centerText("");

            ImGui::TableSetColumnIndex(7);
            if (isAudio && sampleRate > 0)
                centerText(std::to_string(sampleRate) + " Hz");
            else
                centerText("");

            ImGui::TableSetColumnIndex(8);
            std::string dateTime = CombineDateTime(fileDate, fileTime);
            if (!dateTime.empty())
                centerText(dateTime);
            else
                centerText("");

            ImGui::TableSetColumnIndex(9);
            float avail = ImGui::GetContentRegionAvail().x;

            auto getStatusColor = [hasAniStudioMetadata](bool flag) -> ImVec4 {
                if (flag) {
                    if (hasAniStudioMetadata) {
                        return ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                    }
                    else {
                        return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                    }
                }
                else {
                    return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                }
                };

            ImVec4 exifColor = getStatusColor(hasExif);
            ImVec4 lsbColor = getStatusColor(hasLSB);

            std::string statusText = "EXIF | LSB";
            float textWidth = ImGui::CalcTextSize(statusText.c_str()).x;
            float offset = (avail - textWidth) * 0.5f;
            if (offset > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

            ImGui::TextColored(exifColor, "EXIF");
            ImGui::SameLine(0, 0);
            ImGui::Text(" | ");
            ImGui::SameLine(0, 0);
            ImGui::TextColored(lsbColor, "LSB");

            if (ImGui::IsItemHovered()) {
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
        }

        void RenderThumbnail(
            const std::variant<const ECS::ImageComponent*, const ECS::VideoComponent*, const ECS::AudioComponent*>& component,
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
            ECS::EntityID entityID = 0;
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
                entityID = img->GetID();
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
                entityID = vid->GetID();
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
                entityID = aud->GetID();
                channels = aud->channels;
                sampleRate = aud->sampleRate;
                duration = aud->duration;
                fileSize = std::filesystem::file_size(filePath);
                hasExif = aud->hasExifData;
                hasLSB = aud->hasLSBData;
                hasAniStudioMetadata = aud->hasAniStudioMetadata;
            }

            if (mode == DisplayMode::Compact) {
                float childHeight = thumbnailSize + ImGui::GetFontSize() + 4.0f;
                ImGui::BeginChild(("compact_child_" + std::to_string(index)).c_str(), ImVec2(thumbnailSize + 4, childHeight), true);

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

                ImGui::BeginGroup();

                ImVec2 thumbSize(thumbnailSize, thumbnailSize);
                if (isAudio) {
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    dl->AddRectFilled(pos, pos + thumbSize, IM_COL32(60, 60, 80, 200));
                    float iconSize = thumbSize.x * 0.6f;
                    float iconX = pos.x + (thumbSize.x - iconSize) * 0.5f;
                    float iconY = pos.y + (thumbSize.y - iconSize) * 0.5f;
                    dl->AddText(ImVec2(iconX, iconY), IM_COL32(255, 255, 255, 255), "?");
                    if (ImGui::IsItemClicked()) {
                        if (onSelect) onSelect(entityID);
                    }
                }
                else if (textureID != 0 && width > 0 && height > 0) {
                    float aspect = (height > 0) ? (float)width / height : 1.0f;
                    ImVec2 size = thumbSize;
                    if (aspect > 1.0f) size.y = thumbSize.x / aspect;
                    else size.x = thumbSize.y * aspect;
                    ImVec2 imagePos = ImGui::GetCursorPos() + ImVec2((thumbSize.x - size.x) * 0.5f, (thumbSize.y - size.y) * 0.5f);
                    ImGui::SetCursorPos(imagePos);

                    ImGui::Image((ImTextureID)(intptr_t)textureID, size);
                    if (ImGui::IsItemClicked()) {
                        if (onSelect) onSelect(entityID);
                    }
                }
                else {
                    ImGui::Image((ImTextureID)(intptr_t)0, thumbSize);
                    if (ImGui::IsItemClicked()) {
                        if (onSelect) onSelect(entityID);
                    }
                    ImVec2 textSize = ImGui::CalcTextSize("?");
                    float textX = ImGui::GetItemRectMin().x + (thumbSize.x - textSize.x) * 0.5f;
                    float textY = ImGui::GetItemRectMin().y + (thumbSize.y - textSize.y) * 0.5f;
                    ImGui::GetWindowDrawList()->AddText(ImVec2(textX, textY), IM_COL32(200, 200, 200, 255), "?");
                }

                if (entityID == activeEntityID && entityID != 0) {
                    ImVec2 cursorPos = ImGui::GetItemRectMin();
                    ImVec2 itemSize = ImGui::GetItemRectSize();
                    ImGui::GetWindowDrawList()->AddRect(cursorPos, cursorPos + itemSize, IM_COL32(255, 255, 0, 180), 2.0f, 0, 2.0f);
                }

                if (ImGui::BeginPopupContextItem(("thumb_menu_" + std::to_string(index)).c_str())) {
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

                std::string displayName = fileName;
                if (displayName.length() > 20) displayName = displayName.substr(0, 18) + "...";
                ImGui::Text("%s", displayName.c_str());

                ImGui::EndGroup();
                ImGui::EndChild();

                if (ImGui::IsItemHovered()) {
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
            }
            else if (mode == DisplayMode::Detailed) {
                const float childWidth = thumbnailSize + 160;
                const float childHeight = thumbnailSize + 60;
                ImGui::BeginChild(("thumb_child_" + std::to_string(index)).c_str(), ImVec2(childWidth, childHeight), true);

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

                if (entityID == activeEntityID && entityID != 0) {
                    ImVec2 cursorPos = ImGui::GetItemRectMin();
                    ImVec2 itemSize = ImGui::GetItemRectSize();
                    ImGui::GetWindowDrawList()->AddRect(cursorPos, cursorPos + itemSize, IM_COL32(255, 255, 0, 180), 2.0f, 0, 2.0f);
                }

                std::string displayName = fileName;
                if (displayName.length() > 24) displayName = displayName.substr(0, 22) + "...";
                ImGui::TextWrapped("%s", displayName.c_str());

                float imageSize = thumbnailSize - 10;
                ImVec2 cursor = ImGui::GetCursorPos();

                if (isAudio) {
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    dl->AddRectFilled(pos, pos + ImVec2(imageSize, imageSize), IM_COL32(60, 60, 80, 200));
                    float iconSize = imageSize * 0.6f;
                    float iconX = pos.x + (imageSize - iconSize) * 0.5f;
                    float iconY = pos.y + (imageSize - iconSize) * 0.5f;
                    dl->AddText(ImVec2(iconX, iconY), IM_COL32(255, 255, 255, 255), "?");
                    if (ImGui::IsItemClicked()) {
                        if (onSelect) onSelect(entityID);
                    }
                }
                else if (textureID != 0 && width > 0 && height > 0) {
                    float aspect = (height > 0) ? (float)width / height : 1.0f;
                    ImVec2 size;
                    if (aspect > 1.0f) {
                        size.x = imageSize;
                        size.y = imageSize / aspect;
                    }
                    else {
                        size.x = imageSize * aspect;
                        size.y = imageSize;
                    }
                    ImGui::SetCursorPos(cursor + ImVec2((imageSize - size.x) * 0.5f, 0));

                    ImGui::Image((ImTextureID)(intptr_t)textureID, size);
                    if (ImGui::IsItemClicked()) {
                        if (onSelect) onSelect(entityID);
                    }
                }
                else {
                    ImGui::SetCursorPos(cursor);
                    ImGui::Image((ImTextureID)(intptr_t)0, ImVec2(imageSize, imageSize));
                    if (ImGui::IsItemClicked()) {
                        if (onSelect) onSelect(entityID);
                    }
                    ImVec2 textSize = ImGui::CalcTextSize("?");
                    float textX = ImGui::GetItemRectMin().x + (imageSize - textSize.x) * 0.5f;
                    float textY = ImGui::GetItemRectMin().y + (imageSize - textSize.y) * 0.5f;
                    ImGui::GetWindowDrawList()->AddText(ImVec2(textX, textY), IM_COL32(200, 200, 200, 255), "?");
                }

                ImGui::SameLine(0, 4);
                ImGui::BeginGroup();

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

                auto getStatusColor = [hasAniStudioMetadata](bool flag) -> ImVec4 {
                    if (flag) {
                        if (hasAniStudioMetadata) {
                            return ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                        }
                        else {
                            return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                        }
                    }
                    else {
                        return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                    }
                    };

                ImVec4 exifColor = getStatusColor(hasExif);
                ImVec4 lsbColor = getStatusColor(hasLSB);

                ImGui::TextColored(exifColor, "EXIF");
                ImGui::SameLine();
                ImGui::Text("|");
                ImGui::SameLine();
                ImGui::TextColored(lsbColor, "LSB");

                ImGui::EndGroup();

                if (ImGui::BeginPopupContextWindow()) {
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

                ImGui::EndChild();

                if (ImGui::IsItemHovered()) {
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
            }
        }

    }
}