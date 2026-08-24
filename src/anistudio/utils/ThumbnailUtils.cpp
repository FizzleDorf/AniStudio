#include "ThumbnailUtils.hpp"
#include "DragDropUtils.hpp"
#include "ImageUtils.hpp"
#include "VideoUtils.hpp"
#include <imgui.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <filesystem>

namespace GUI {
    namespace Thumbnail {

        static std::unordered_map<std::string, nlohmann::json> s_metadataCache;

        static std::string NormalizePath(const std::string& path) {
            std::error_code ec;
            std::filesystem::path absPath = std::filesystem::absolute(path, ec);
            if (ec) return path;
            return absPath.string();
        }

        static const nlohmann::json& GetCachedMetadata(const std::string& filePath, bool isVideo) {
            std::string normalizedPath = NormalizePath(filePath);
            auto it = s_metadataCache.find(normalizedPath);
            if (it != s_metadataCache.end()) {
                return it->second;
            }

            nlohmann::json meta;
            if (isVideo) {
                meta = Utils::VideoMetadataUtils::ReadMetadataFromVideo(filePath);
            }
            else {
                meta = Utils::ImageUtils::ReadMetadataFromImage(filePath);
            }

            auto result = s_metadataCache.emplace(normalizedPath, std::move(meta));
            return result.first->second;
        }

        static bool HasValidComponents(const nlohmann::json& obj) {
            if (!obj.is_object()) return false;
            if (obj.contains("components") && obj["components"].is_array()) {
                for (const auto& comp : obj["components"]) {
                    if (comp.is_object() && !comp.empty()) {
                        for (auto it = comp.begin(); it != comp.end(); ++it) {
                            if (!it.value().is_null() && !it.value().empty()) {
                                return true;
                            }
                        }
                    }
                }
            }
            return false;
        }

        static nlohmann::json ExtractAniStudioData(const nlohmann::json& meta) {
            if (meta.is_null() || !meta.is_object()) return meta;

            // Direct components at root
            if (HasValidComponents(meta)) return meta;

            // dataType == "entity" wrapper
            if (meta.contains("dataType") && meta["dataType"] == "entity" && meta.contains("data")) {
                const auto& data = meta["data"];
                if (data.is_object()) {
                    if (data.contains("parameters") && data["parameters"].is_object()) {
                        const auto& params = data["parameters"];
                        if (HasValidComponents(params)) return params;
                    }
                    if (HasValidComponents(data)) return data;
                }
            }

            // Single-key wrapper (like {"parameters": {...}})
            if (meta.is_object() && meta.size() == 1) {
                auto it = meta.begin();
                const auto& value = it.value();
                if (value.is_object()) {
                    if (HasValidComponents(value)) return value;
                    if (value.contains("parameters") && value["parameters"].is_object()) {
                        const auto& params = value["parameters"];
                        if (HasValidComponents(params)) return params;
                    }
                }
            }

            return meta;
        }

        static bool IsAniStudioCompatible(const nlohmann::json& meta) {
            nlohmann::json extracted = ExtractAniStudioData(meta);
            return HasValidComponents(extracted);
        }

        static bool HasExifData(const nlohmann::json& obj) {
            if (!obj.is_object()) return false;
            if (HasValidComponents(obj)) return true;
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                if (it.key() != "components" && it.value().is_object() && !it.value().empty()) {
                    return true;
                }
                if (it.value().is_string() && !it.value().get<std::string>().empty()) {
                    return true;
                }
            }
            return false;
        }

        static bool GetCachedExif(const std::string& filePath, bool isVideo) {
            const auto& meta = GetCachedMetadata(filePath, isVideo);
            if (meta.is_null() || meta.empty()) return false;

            nlohmann::json extracted = ExtractAniStudioData(meta);
            if (HasExifData(extracted)) return true;
            if (HasExifData(meta)) return true;

            return false;
        }

        static bool GetCachedLSB(const std::string& filePath, bool isVideo) {
            const auto& meta = GetCachedMetadata(filePath, isVideo);
            if (meta.is_null() || meta.empty()) return false;

            std::vector<std::string> stealthKeys = { "LSB", "Stealth", "Hidden", "steganography", "lsb" };

            auto hasLSB = [&](const nlohmann::json& obj) {
                if (!obj.is_object()) return false;
                for (const auto& key : stealthKeys) {
                    if (obj.contains(key)) return true;
                }
                if (obj.contains("components") && obj["components"].is_array()) {
                    for (const auto& comp : obj["components"]) {
                        if (comp.is_object()) {
                            for (const auto& key : stealthKeys) {
                                if (comp.contains(key)) return true;
                            }
                        }
                    }
                }
                return false;
                };

            nlohmann::json extracted = ExtractAniStudioData(meta);
            if (hasLSB(extracted)) return true;
            if (hasLSB(meta)) return true;

            return false;
        }

        static int GetCachedMetadataStatus(const std::string& filePath, bool isVideo) {
            const auto& meta = GetCachedMetadata(filePath, isVideo);
            if (meta.is_null() || meta.empty()) return 0;
            if (IsAniStudioCompatible(meta)) return 1;
            return 2;
        }

        void RenderThumbnail(
            const ThumbnailData& data,
            size_t index,
            float thumbnailSize,
            DisplayMode mode,
            std::function<void(ECS::EntityID)> onSelect,
            Utils::ContextMenuUtils* contextMenuUtils,
            bool isEntityLoaded
        ) {
            if (mode == DisplayMode::Compact) {
                ImGui::BeginGroup();

                ImVec2 thumbSize(thumbnailSize, thumbnailSize);
                if (data.textureID != 0 && data.width > 0 && data.height > 0) {
                    float aspect = (data.height > 0) ? (float)data.width / data.height : 1.0f;
                    ImVec2 size = thumbSize;
                    if (aspect > 1.0f) size.y = thumbSize.x / aspect;
                    else size.x = thumbSize.y * aspect;
                    ImVec2 imagePos = ImGui::GetCursorPos() + ImVec2((thumbSize.x - size.x) * 0.5f, (thumbSize.y - size.y) * 0.5f);
                    ImGui::SetCursorPos(imagePos);
                    if (ImGui::ImageButton(("##thumb" + std::to_string(index)).c_str(),
                        (ImTextureID)(intptr_t)data.textureID, size, ImVec2(0, 0), ImVec2(1, 1))) {
                        if (onSelect) onSelect(data.entityID);
                    }
                }
                else {
                    if (ImGui::ImageButton(("##thumbplaceholder" + std::to_string(index)).c_str(),
                        (ImTextureID)(intptr_t)0, thumbSize, ImVec2(0, 0), ImVec2(1, 1))) {
                        if (onSelect) onSelect(data.entityID);
                    }
                    ImVec2 textSize = ImGui::CalcTextSize("?");
                    float textX = ImGui::GetItemRectMin().x + (thumbSize.x - textSize.x) * 0.5f;
                    float textY = ImGui::GetItemRectMin().y + (thumbSize.y - textSize.y) * 0.5f;
                    ImGui::GetWindowDrawList()->AddText(ImVec2(textX, textY), IM_COL32(200, 200, 200, 255), "?");
                }

                if (data.entityID == data.activeEntityID && data.entityID != 0) {
                    ImVec2 cursorPos = ImGui::GetItemRectMin();
                    ImVec2 itemSize = ImGui::GetItemRectSize();
                    ImGui::GetWindowDrawList()->AddRect(cursorPos, cursorPos + itemSize, IM_COL32(255, 255, 0, 180), 2.0f, 0, 2.0f);
                }

                if (isEntityLoaded && data.entityID != 0) {
                    if (ImGui::BeginDragDropSource()) {
                        nlohmann::json payload;
                        payload["entityID"] = data.entityID;
                        ImGui::SetDragDropPayload(GUI::DragDrop::PAYLOAD_ENTITY,
                            payload.dump().c_str(), payload.dump().size() + 1);
                        if (data.textureID != 0) {
                            ImGui::Image((ImTextureID)(intptr_t)data.textureID, ImVec2(64, 64));
                        }
                        ImGui::Text("%s", data.fileName.c_str());
                        ImGui::EndDragDropSource();
                    }
                }
                else {
                    if (ImGui::BeginDragDropSource()) {
                        nlohmann::json payload;
                        payload["filePath"] = data.filePath;
                        payload["fileType"] = GUI::DragDrop::GuessMediaType(data.filePath);
                        ImGui::SetDragDropPayload(GUI::DragDrop::PAYLOAD_FILE_PATH,
                            payload.dump().c_str(), payload.dump().size() + 1);
                        ImGui::Text("%s", data.fileName.c_str());
                        ImGui::EndDragDropSource();
                    }
                }

                if (ImGui::BeginPopupContextItem(("thumb_menu_" + std::to_string(index)).c_str())) {
                    if (isEntityLoaded && data.entityID != 0) {
                        contextMenuUtils->RenderEntityContextMenuItems(data.entityID);
                    }
                    else {
                        if (ImGui::MenuItem("Load Asset")) {
                            if (onSelect) onSelect(data.entityID);
                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem("Copy File Path")) {
                            ImGui::SetClipboardText(data.filePath.c_str());
                        }
                    }
                    ImGui::EndPopup();
                }

                std::string displayName = data.fileName;
                if (displayName.length() > 20) displayName = displayName.substr(0, 18) + "...";
                ImGui::Text("%s", displayName.c_str());

                ImGui::EndGroup();
            }
            else {
                const float childWidth = thumbnailSize + 160;
                const float childHeight = thumbnailSize + 60;
                ImGui::BeginChild(("thumb_child_" + std::to_string(index)).c_str(), ImVec2(childWidth, childHeight), true);

                if (data.entityID == data.activeEntityID && data.entityID != 0) {
                    ImVec2 cursorPos = ImGui::GetItemRectMin();
                    ImVec2 itemSize = ImGui::GetItemRectSize();
                    ImGui::GetWindowDrawList()->AddRect(cursorPos, cursorPos + itemSize, IM_COL32(255, 255, 0, 180), 2.0f, 0, 2.0f);
                }

                if (isEntityLoaded && data.entityID != 0) {
                    if (ImGui::BeginDragDropSource()) {
                        nlohmann::json payload;
                        payload["entityID"] = data.entityID;
                        ImGui::SetDragDropPayload(GUI::DragDrop::PAYLOAD_ENTITY,
                            payload.dump().c_str(), payload.dump().size() + 1);
                        if (data.textureID != 0) {
                            ImGui::Image((ImTextureID)(intptr_t)data.textureID, ImVec2(64, 64));
                        }
                        ImGui::Text("%s", data.fileName.c_str());
                        ImGui::EndDragDropSource();
                    }
                }
                else {
                    if (ImGui::BeginDragDropSource()) {
                        nlohmann::json payload;
                        payload["filePath"] = data.filePath;
                        payload["fileType"] = GUI::DragDrop::GuessMediaType(data.filePath);
                        ImGui::SetDragDropPayload(GUI::DragDrop::PAYLOAD_FILE_PATH,
                            payload.dump().c_str(), payload.dump().size() + 1);
                        ImGui::Text("%s", data.fileName.c_str());
                        ImGui::EndDragDropSource();
                    }
                }

                if (ImGui::BeginPopupContextWindow()) {
                    if (isEntityLoaded && data.entityID != 0) {
                        contextMenuUtils->RenderEntityContextMenuItems(data.entityID);
                    }
                    else {
                        if (ImGui::MenuItem("Load Asset")) {
                            if (onSelect) onSelect(data.entityID);
                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem("Copy File Path")) {
                            ImGui::SetClipboardText(data.filePath.c_str());
                        }
                    }
                    ImGui::EndPopup();
                }

                std::string displayName = data.fileName;
                if (displayName.length() > 24) displayName = displayName.substr(0, 22) + "...";
                ImGui::TextWrapped("%s", displayName.c_str());

                float imageSize = thumbnailSize - 10;
                ImVec2 cursor = ImGui::GetCursorPos();

                if (data.textureID != 0 && data.width > 0 && data.height > 0) {
                    float aspect = (data.height > 0) ? (float)data.width / data.height : 1.0f;
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
                    ImGui::Image((ImTextureID)(intptr_t)data.textureID, size);
                    if (ImGui::IsItemClicked()) {
                        if (onSelect) onSelect(data.entityID);
                    }
                }
                else {
                    ImGui::SetCursorPos(cursor);
                    ImGui::Image((ImTextureID)(intptr_t)0, ImVec2(imageSize, imageSize));
                    if (ImGui::IsItemClicked()) {
                        if (onSelect) onSelect(data.entityID);
                    }
                    ImVec2 textSize = ImGui::CalcTextSize("?");
                    float textX = ImGui::GetItemRectMin().x + (imageSize - textSize.x) * 0.5f;
                    float textY = ImGui::GetItemRectMin().y + (imageSize - textSize.y) * 0.5f;
                    ImGui::GetWindowDrawList()->AddText(ImVec2(textX, textY), IM_COL32(200, 200, 200, 255), "?");
                }

                ImGui::SameLine(0, 4);
                ImGui::BeginGroup();
                if (data.fileSize > 0) {
                    std::string sizeStr = (data.fileSize > 1024 * 1024) ? std::to_string(data.fileSize / (1024 * 1024)) + " MB" :
                        (data.fileSize > 1024) ? std::to_string(data.fileSize / 1024) + " KB" : std::to_string(data.fileSize) + " B";
                    ImGui::Text("%s", sizeStr.c_str());
                }
                if (data.width > 0 && data.height > 0) {
                    ImGui::Text("%dx%d", data.width, data.height);
                }
                if (data.channels > 0) {
                    ImGui::Text("%dch", data.channels);
                }
                if (!data.fileDate.empty()) ImGui::Text("%s", data.fileDate.c_str());
                if (!data.fileTime.empty()) ImGui::Text("%s", data.fileTime.c_str());
                if (data.isVideo && data.fps > 0) ImGui::Text("%.1f fps", data.fps);

                bool isAniStudio = (GetCachedMetadataStatus(data.filePath, data.isVideo) == 1);
                bool hasExif = GetCachedExif(data.filePath, data.isVideo);
                bool hasLSB = GetCachedLSB(data.filePath, data.isVideo);

                ImVec4 exifColor;
                ImVec4 lsbColor;

                if (isAniStudio) {
                    exifColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                    lsbColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                }
                else if (hasExif) {
                    exifColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                }
                else {
                    exifColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                }

                if (isAniStudio) {
                    lsbColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                }
                else if (hasLSB) {
                    lsbColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                }
                else {
                    lsbColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                }

                ImGui::TextColored(exifColor, "EXIF");
                ImGui::SameLine();
                ImGui::Text("|");
                ImGui::SameLine();
                ImGui::TextColored(lsbColor, "LSB");

                ImGui::EndGroup();

                ImGui::EndChild();
            }
        }

    }
}