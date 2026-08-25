// ThumbnailFilters.hpp
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <filesystem>
#include <algorithm>
#include <imgui.h>
#include "FileFormats.hpp"
#include "ThumbnailUtils.hpp"

namespace ThumbnailFilters {

    enum class MediaTypeFilter { All, Image, Video, Audio };
    enum class SortMode { Name, Size, Date, Duration, EntityID };

    struct Settings {
        MediaTypeFilter mediaType = MediaTypeFilter::All;
        std::string extensionFilter;
        bool filterHasMetadata = false;
        int filterChannels = 0;
        SortMode sortMode = SortMode::Name;
        bool sortAscending = true;
        GUI::Thumbnail::DisplayMode displayMode = GUI::Thumbnail::DisplayMode::Detailed;
        GUI::Thumbnail::ThumbnailSize thumbnailSize = GUI::Thumbnail::ThumbnailSize::Large;
    };

    struct MediaItemInfo {
        std::string fileName;
        std::string filePath;
        uint64_t fileSize = 0;
        std::string dateTime;
        double duration = 0.0;
        float fps = 0.0f;
        int width = 0;
        int height = 0;
        int channels = 0;
        bool hasMetadata = false;
        bool isImage = false;
        bool isVideo = false;
        bool isAudio = false;
        uint64_t entityID = 0;
    };

    inline std::string GetExtension(const std::string& path) {
        std::filesystem::path p(path);
        std::string ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext;
    }

    template<typename T, typename Func>
    void ApplyFiltersAndSort(
        std::vector<T>& items,
        const Settings& settings,
        Func getInfo
    ) {
        items.erase(std::remove_if(items.begin(), items.end(), [&](const T& item) {
            MediaItemInfo info = getInfo(item);
            if (settings.mediaType == MediaTypeFilter::Image && !info.isImage) return true;
            if (settings.mediaType == MediaTypeFilter::Video && !info.isVideo) return true;
            if (settings.mediaType == MediaTypeFilter::Audio && !info.isAudio) return true;
            if (!settings.extensionFilter.empty()) {
                std::string ext = GetExtension(info.filePath);
                std::string filterLower = settings.extensionFilter;
                std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);
                if (ext != filterLower) return true;
            }
            if (settings.filterHasMetadata && !info.hasMetadata) return true;
            if (settings.filterChannels != 0 && info.channels != settings.filterChannels) return true;
            return false;
            }), items.end());

        std::sort(items.begin(), items.end(), [&](const T& a, const T& b) {
            MediaItemInfo infoA = getInfo(a);
            MediaItemInfo infoB = getInfo(b);
            int cmp = 0;
            switch (settings.sortMode) {
            case SortMode::Name:
                cmp = infoA.fileName.compare(infoB.fileName);
                break;
            case SortMode::Size:
                cmp = (infoA.fileSize < infoB.fileSize) ? -1 : (infoA.fileSize > infoB.fileSize) ? 1 : 0;
                break;
            case SortMode::Date:
                cmp = infoA.dateTime.compare(infoB.dateTime);
                break;
            case SortMode::Duration:
                cmp = (infoA.duration < infoB.duration) ? -1 : (infoA.duration > infoB.duration) ? 1 : 0;
                break;
            case SortMode::EntityID:
                cmp = (infoA.entityID < infoB.entityID) ? -1 : (infoA.entityID > infoB.entityID) ? 1 : 0;
                break;
            }
            return settings.sortAscending ? cmp < 0 : cmp > 0;
            });
    }

    inline bool RenderSortMenu(Settings& settings) {
        bool changed = false;
        if (ImGui::BeginMenu("Sort")) {
            if (ImGui::BeginMenu("Sort By")) {
                const char* sortItems[] = { "Name", "Size", "Date", "Duration", "Entity ID" };
                for (int i = 0; i < IM_ARRAYSIZE(sortItems); ++i) {
                    if (ImGui::MenuItem(sortItems[i], nullptr, settings.sortMode == static_cast<SortMode>(i))) {
                        settings.sortMode = static_cast<SortMode>(i);
                        changed = true;
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Ascending", nullptr, settings.sortAscending)) {
                settings.sortAscending = true;
                changed = true;
            }
            if (ImGui::MenuItem("Descending", nullptr, !settings.sortAscending)) {
                settings.sortAscending = false;
                changed = true;
            }
            ImGui::EndMenu();
        }
        return changed;
    }

    inline bool RenderFiltersMenu(Settings& settings, bool showAudio = true, bool showExtension = true) {
        bool changed = false;
        if (ImGui::BeginMenu("Filters")) {
            if (ImGui::BeginMenu("Media Type")) {
                if (ImGui::MenuItem("All", nullptr, settings.mediaType == MediaTypeFilter::All)) {
                    settings.mediaType = MediaTypeFilter::All;
                    changed = true;
                }
                if (ImGui::MenuItem("Image", nullptr, settings.mediaType == MediaTypeFilter::Image)) {
                    settings.mediaType = MediaTypeFilter::Image;
                    changed = true;
                }
                if (ImGui::MenuItem("Video", nullptr, settings.mediaType == MediaTypeFilter::Video)) {
                    settings.mediaType = MediaTypeFilter::Video;
                    changed = true;
                }
                if (showAudio && ImGui::MenuItem("Audio", nullptr, settings.mediaType == MediaTypeFilter::Audio)) {
                    settings.mediaType = MediaTypeFilter::Audio;
                    changed = true;
                }
                ImGui::EndMenu();
            }

            if (showExtension) {
                const auto& formats = FileFormats::GetAllFormats();
                std::vector<std::string> exts;
                for (const auto& pair : formats) exts.push_back(pair.first);
                std::sort(exts.begin(), exts.end());
                exts.insert(exts.begin(), "");

                if (ImGui::BeginMenu("Extension")) {
                    for (const auto& ext : exts) {
                        const char* label = ext.empty() ? "All" : ext.c_str();
                        if (ImGui::MenuItem(label, nullptr, settings.extensionFilter == ext)) {
                            settings.extensionFilter = ext;
                            changed = true;
                        }
                    }
                    ImGui::EndMenu();
                }
            }

            if (ImGui::MenuItem("Has AniStudio Data", nullptr, settings.filterHasMetadata)) {
                settings.filterHasMetadata = !settings.filterHasMetadata;
                changed = true;
            }

            if (ImGui::BeginMenu("Channels")) {
                const char* channelLabels[] = { "Any", "RGB (3)", "RGBA (4)", "Grayscale (1)", "2ch" };
                int channelValues[] = { 0, 3, 4, 1, 2 };
                for (int i = 0; i < IM_ARRAYSIZE(channelLabels); ++i) {
                    if (ImGui::MenuItem(channelLabels[i], nullptr, settings.filterChannels == channelValues[i])) {
                        settings.filterChannels = channelValues[i];
                        changed = true;
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
        return changed;
    }

    inline bool RenderViewMenu(Settings& settings) {
        bool changed = false;
        if (ImGui::BeginMenu("View")) {
            if (ImGui::BeginMenu("Display Mode")) {
                const char* modeItems[] = { "Compact", "Detailed", "List" };
                for (int i = 0; i < IM_ARRAYSIZE(modeItems); ++i) {
                    if (ImGui::MenuItem(modeItems[i], nullptr, settings.displayMode == static_cast<GUI::Thumbnail::DisplayMode>(i))) {
                        settings.displayMode = static_cast<GUI::Thumbnail::DisplayMode>(i);
                        changed = true;
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Thumbnail Size")) {
                const char* sizeItems[] = { "Small", "Medium", "Large", "Extra Large" };
                for (int i = 0; i < IM_ARRAYSIZE(sizeItems); ++i) {
                    if (ImGui::MenuItem(sizeItems[i], nullptr, settings.thumbnailSize == static_cast<GUI::Thumbnail::ThumbnailSize>(i))) {
                        settings.thumbnailSize = static_cast<GUI::Thumbnail::ThumbnailSize>(i);
                        changed = true;
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        return changed;
    }

}