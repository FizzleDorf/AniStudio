#pragma once

#include <string>
#include <filesystem>
#include <functional>
#include <imgui.h>
#include "Types.hpp"
#include "ContextMenuUtils.hpp"
#include "OpenGLWrapper.hpp"

namespace GUI {
    namespace Thumbnail {

        enum class DisplayMode {
            Compact,
            Detailed
        };

        struct ThumbnailData {
            std::string filePath;
            std::string fileName;
            ECS::EntityID entityID = 0;
            ECS::EntityID activeEntityID = 0;
            GLuint textureID = 0;
            int width = 0;
            int height = 0;
            int channels = 0;
            uint64_t fileSize = 0;
            std::string fileDate;
            std::string fileTime;
            bool hasExif = false;
            bool hasLSB = false;
            float fps = 0.0f;
            bool isVideo = false;
        };

        void RenderThumbnail(
            const ThumbnailData& data,
            size_t index,
            float thumbnailSize,
            DisplayMode mode,
            std::function<void(ECS::EntityID)> onSelect,
            Utils::ContextMenuUtils* contextMenuUtils,
            bool isEntityLoaded
        );

    } // namespace Thumbnail
} // namespace GUI