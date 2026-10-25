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
#include "ImageComponent.hpp"
#include "VideoComponent.hpp"
#include "AudioComponent.hpp"

namespace ThumbnailFilters {

    enum class MediaTypeFilter { All, Image, Video, Audio };
    enum class SortMode { Name, Size, Date, Duration, EntityID };

    enum class MediaCategory : uint32_t {
        None = 0,
        Regular = 1u << 0,
        Input = 1u << 1,
        Output = 1u << 2,
        Preview = 1u << 3,
        Mask = 1u << 4,
        End = 1u << 5,
        All = 0xFFFFFFFFu
    };

    inline uint32_t CategoryBit(MediaCategory c) {
        return static_cast<uint32_t>(c);
    }

    // Returns the category mask for an entity, or 0 if the entity should
    // not appear in Media History at all.
    //
    // Rules:
    //   1. An entity that ONLY carries an OutputImageComponent and has no
    //      loaded image data is a diffusion task entity. Hide it.
    //   2. An entity whose image-shaped component has no loaded pixel data
    //      (imageData == nullptr) is not yet media. Hide it. This covers
    //      task-entity clones that inherited a stale ImageComponent from
    //      the source view, and asset entities still decoding.
    //      Exception: PreviewImageComponent updates constantly through
    //      PreviewView; show those once they have dimensions.
    //   3. Video and audio follow the same "loaded" rule.
    //   4. Everything else that has an image/video/audio component shows.
    template<typename Mgr>
    uint32_t CategoryMaskForEntity(Mgr& mgr, ECS::EntityID id) {
        if (!mgr.IsEntityValid(id)) return 0;

        // ---- Image side -------------------------------------------------
        // Check base first, then derived. Only one image-shaped component
        // is used per entity by convention; the order of precedence is
        // base -> preview -> input -> output -> mask -> end.
        bool hasImage = false;
        bool imageLoaded = false;
        bool isPreviewImage = false;
        uint32_t imageCategory = 0;

        if (mgr.template HasComponent<ECS::ImageComponent>(id)) {
            hasImage = true;
            imageCategory = CategoryBit(MediaCategory::Regular);
            auto& c = mgr.template GetComponent<ECS::ImageComponent>(id);
            if (c.width > 0 && c.height > 0 && c.imageData != nullptr)
                imageLoaded = true;
        }
        else if (mgr.template HasComponent<ECS::PreviewImageComponent>(id)) {
            hasImage = true;
            isPreviewImage = true;
            imageCategory = CategoryBit(MediaCategory::Preview);
            auto& c = mgr.template GetComponent<ECS::PreviewImageComponent>(id);
            if (c.width > 0 && c.height > 0 && c.imageData != nullptr)
                imageLoaded = true;
        }
        else if (mgr.template HasComponent<ECS::InputImageComponent>(id)) {
            hasImage = true;
            imageCategory = CategoryBit(MediaCategory::Input);
            auto& c = mgr.template GetComponent<ECS::InputImageComponent>(id);
            if (c.width > 0 && c.height > 0 && c.imageData != nullptr)
                imageLoaded = true;
        }
        else if (mgr.template HasComponent<ECS::OutputImageComponent>(id)) {
            hasImage = true;
            imageCategory = CategoryBit(MediaCategory::Output);
            auto& c = mgr.template GetComponent<ECS::OutputImageComponent>(id);
            if (c.width > 0 && c.height > 0 && c.imageData != nullptr)
                imageLoaded = true;
        }
        else if (mgr.template HasComponent<ECS::MaskImageComponent>(id)) {
            hasImage = true;
            imageCategory = CategoryBit(MediaCategory::Mask);
            auto& c = mgr.template GetComponent<ECS::MaskImageComponent>(id);
            if (c.width > 0 && c.height > 0 && c.imageData != nullptr)
                imageLoaded = true;
        }
        else if (mgr.template HasComponent<ECS::EndImageComponent>(id)) {
            hasImage = true;
            imageCategory = CategoryBit(MediaCategory::End);
            auto& c = mgr.template GetComponent<ECS::EndImageComponent>(id);
            if (c.width > 0 && c.height > 0 && c.imageData != nullptr)
                imageLoaded = true;
        }

        // ---- Video side -------------------------------------------------
        bool hasVideo = false;
        bool videoLoaded = false;
        uint32_t videoCategory = 0;

        if (mgr.template HasComponent<ECS::VideoComponent>(id)) {
            hasVideo = true;
            videoCategory = CategoryBit(MediaCategory::Regular);
            auto& c = mgr.template GetComponent<ECS::VideoComponent>(id);
            if (c.width > 0 && c.height > 0) videoLoaded = true;
        }
        else if (mgr.template HasComponent<ECS::PreviewVideoComponent>(id)) {
            hasVideo = true;
            videoCategory = CategoryBit(MediaCategory::Preview);
            auto& c = mgr.template GetComponent<ECS::PreviewVideoComponent>(id);
            if (c.width > 0 && c.height > 0) videoLoaded = true;
        }
        else if (mgr.template HasComponent<ECS::InputVideoComponent>(id)) {
            hasVideo = true;
            videoCategory = CategoryBit(MediaCategory::Input);
            auto& c = mgr.template GetComponent<ECS::InputVideoComponent>(id);
            if (c.width > 0 && c.height > 0) videoLoaded = true;
        }
        else if (mgr.template HasComponent<ECS::OutputVideoComponent>(id)) {
            hasVideo = true;
            videoCategory = CategoryBit(MediaCategory::Output);
            auto& c = mgr.template GetComponent<ECS::OutputVideoComponent>(id);
            if (c.width > 0 && c.height > 0) videoLoaded = true;
        }

        // ---- Audio side -------------------------------------------------
        bool hasAudio = false;
        bool audioLoaded = false;
        uint32_t audioCategory = 0;

        if (mgr.template HasComponent<ECS::AudioComponent>(id)) {
            hasAudio = true;
            audioCategory = CategoryBit(MediaCategory::Regular);
            auto& c = mgr.template GetComponent<ECS::AudioComponent>(id);
            if (!c.pcmData.empty()) audioLoaded = true;
        }
        else if (mgr.template HasComponent<ECS::InputAudioComponent>(id)) {
            hasAudio = true;
            audioCategory = CategoryBit(MediaCategory::Input);
            auto& c = mgr.template GetComponent<ECS::InputAudioComponent>(id);
            if (!c.pcmData.empty()) audioLoaded = true;
        }
        else if (mgr.template HasComponent<ECS::OutputAudioComponent>(id)) {
            hasAudio = true;
            audioCategory = CategoryBit(MediaCategory::Output);
            auto& c = mgr.template GetComponent<ECS::OutputAudioComponent>(id);
            if (!c.pcmData.empty()) audioLoaded = true;
        }

        // If nothing media-shaped is present, hide the entity entirely.
        if (!hasImage && !hasVideo && !hasAudio) return 0;

        // If the *only* thing present is an image/video/audio that is not
        // loaded yet, hide it. That filters:
        //   - task entities (OutputImageComponent with no imageData)
        //   - task-entity clones that inherited an ImageComponent from the
        //     source view but haven't decoded anything
        //   - asset entities still decoding
        //   - video entities whose metadata hasn't arrived
        // Preview entities are exempt (they stream frames continuously and
        // are meant to appear as they cycle).
        if (hasImage && !imageLoaded && !isPreviewImage &&
            !hasVideo && !hasAudio) {
            return 0;
        }
        if (hasVideo && !videoLoaded && !hasImage && !hasAudio) return 0;
        if (hasAudio && !audioLoaded && !hasImage && !hasVideo) return 0;

        // Combine categories if multiple media types are present on the
        // same entity (rare but possible: an image with an attached audio
        // track). Prefer the image category so the grid thumbnails it as
        // an image.
        uint32_t mask = 0;
        if (hasImage && imageLoaded)     mask |= imageCategory;
        if (hasVideo && videoLoaded)     mask |= videoCategory;
        if (hasAudio && audioLoaded)     mask |= audioCategory;

        if (mask == 0) {
            // Nothing is loaded yet. Since we already passed the "only
            // media-shaped thing is unloaded" check above, this can only
            // happen if a mixed entity (e.g. image + audio) has one loaded
            // and the other unloaded. Fall back to whichever *is* loaded.
            if (hasImage && isPreviewImage) mask |= CategoryBit(MediaCategory::Preview);
        }

        if (mask == 0) return 0;
        return mask;
    }

    struct Settings {
        MediaTypeFilter mediaType = MediaTypeFilter::All;
        std::string extensionFilter;
        bool filterHasMetadata = false;
        int filterChannels = 0;
        SortMode sortMode = SortMode::Name;
        bool sortAscending = true;
        GUI::Thumbnail::DisplayMode displayMode = GUI::Thumbnail::DisplayMode::Detailed;
        GUI::Thumbnail::ThumbnailSize thumbnailSize = GUI::Thumbnail::ThumbnailSize::Large;
        uint32_t categoryFilterMask =
            CategoryBit(MediaCategory::Regular) |
            CategoryBit(MediaCategory::Input) |
            CategoryBit(MediaCategory::Output) |
            CategoryBit(MediaCategory::Preview) |
            CategoryBit(MediaCategory::Mask) |
            CategoryBit(MediaCategory::End);
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
        int sampleRate = 0;
        bool hasMetadata = false;
        bool isImage = false;
        bool isVideo = false;
        bool isAudio = false;
        uint64_t entityID = 0;
        uint32_t categoryMask = 0;
    };

    inline std::string GetExtension(const std::string& path) {
        std::filesystem::path p(path);
        std::string ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext;
    }

    template<typename T, typename Func>
    void ApplyFiltersAndSort(std::vector<T>& items, const Settings& settings, Func getInfo) {
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
            if (info.categoryMask != 0 &&
                (info.categoryMask & settings.categoryFilterMask) == 0) {
                return true;
            }
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

    inline bool RenderCategoryMenu(Settings& settings) {
        bool changed = false;
        if (ImGui::BeginMenu("Category")) {
            struct Row { const char* label; MediaCategory cat; };
            const Row rows[] = {
                { "Regular", MediaCategory::Regular },
                { "Input",   MediaCategory::Input   },
                { "Output",  MediaCategory::Output  },
                { "Preview", MediaCategory::Preview },
                { "Mask",    MediaCategory::Mask    },
                { "End",     MediaCategory::End     },
            };
            for (const auto& r : rows) {
                bool on = (settings.categoryFilterMask & CategoryBit(r.cat)) != 0;
                if (ImGui::MenuItem(r.label, nullptr, on)) {
                    if (on) settings.categoryFilterMask &= ~CategoryBit(r.cat);
                    else    settings.categoryFilterMask |= CategoryBit(r.cat);
                    changed = true;
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Show All")) {
                settings.categoryFilterMask = CategoryBit(MediaCategory::All);
                changed = true;
            }
            if (ImGui::MenuItem("Hide All")) {
                settings.categoryFilterMask = 0;
                changed = true;
            }
            if (ImGui::MenuItem("Reset to Default")) {
                settings.categoryFilterMask =
                    CategoryBit(MediaCategory::Regular) |
                    CategoryBit(MediaCategory::Input) |
                    CategoryBit(MediaCategory::Output) |
                    CategoryBit(MediaCategory::Preview) |
                    CategoryBit(MediaCategory::Mask) |
                    CategoryBit(MediaCategory::End);
                changed = true;
            }
            ImGui::EndMenu();
        }
        return changed;
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
                settings.sortAscending = true; changed = true;
            }
            if (ImGui::MenuItem("Descending", nullptr, !settings.sortAscending)) {
                settings.sortAscending = false; changed = true;
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
                    settings.mediaType = MediaTypeFilter::All; changed = true;
                }
                if (ImGui::MenuItem("Image", nullptr, settings.mediaType == MediaTypeFilter::Image)) {
                    settings.mediaType = MediaTypeFilter::Image; changed = true;
                }
                if (ImGui::MenuItem("Video", nullptr, settings.mediaType == MediaTypeFilter::Video)) {
                    settings.mediaType = MediaTypeFilter::Video; changed = true;
                }
                if (showAudio && ImGui::MenuItem("Audio", nullptr, settings.mediaType == MediaTypeFilter::Audio)) {
                    settings.mediaType = MediaTypeFilter::Audio; changed = true;
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
                            settings.extensionFilter = ext; changed = true;
                        }
                    }
                    ImGui::EndMenu();
                }
            }
            if (ImGui::MenuItem("Has AniStudio Data", nullptr, settings.filterHasMetadata)) {
                settings.filterHasMetadata = !settings.filterHasMetadata; changed = true;
            }
            if (ImGui::BeginMenu("Channels")) {
                const char* channelLabels[] = { "Any", "RGB (3)", "RGBA (4)", "Grayscale (1)", "2ch" };
                int channelValues[] = { 0, 3, 4, 1, 2 };
                for (int i = 0; i < IM_ARRAYSIZE(channelLabels); ++i) {
                    if (ImGui::MenuItem(channelLabels[i], nullptr, settings.filterChannels == channelValues[i])) {
                        settings.filterChannels = channelValues[i]; changed = true;
                    }
                }
                ImGui::EndMenu();
            }
            changed |= RenderCategoryMenu(settings);
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
                        settings.displayMode = static_cast<GUI::Thumbnail::DisplayMode>(i); changed = true;
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Thumbnail Size")) {
                const char* sizeItems[] = { "Small", "Medium", "Large", "Extra Large" };
                for (int i = 0; i < IM_ARRAYSIZE(sizeItems); ++i) {
                    if (ImGui::MenuItem(sizeItems[i], nullptr, settings.thumbnailSize == static_cast<GUI::Thumbnail::ThumbnailSize>(i))) {
                        settings.thumbnailSize = static_cast<GUI::Thumbnail::ThumbnailSize>(i); changed = true;
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        return changed;
    }

}