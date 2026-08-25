#pragma once

#include <string>
#include <functional>
#include <imgui.h>
#include "Types.hpp"
#include "ContextMenuUtils.hpp"
#include "OpenGLWrapper.hpp"
#include "ImageComponent.hpp"
#include "VideoComponent.hpp"
#include <variant>

namespace GUI {
    namespace Thumbnail {

        enum class DisplayMode {
            Compact,
            Detailed,
            List
        };

        enum class ThumbnailSize {
            Small,
            Medium,
            Large,
            ExtraLarge
        };

        float GetThumbnailSize(ThumbnailSize size);

        void RenderThumbnail(
            const std::variant<const ECS::ImageComponent*, const ECS::VideoComponent*>& component,
            size_t index,
            float thumbnailSize,
            DisplayMode mode,
            std::function<void(ECS::EntityID)> onSelect,
            Utils::ContextMenuUtils* contextMenuUtils,
            bool isEntityLoaded,
            ECS::EntityID activeEntityID = 0
        );

        void BeginListMode(float thumbnailSize);
        void EndListMode();
        void RenderListRow(
            const std::variant<const ECS::ImageComponent*, const ECS::VideoComponent*>& component,
            size_t index,
            float thumbnailSize,
            std::function<void(ECS::EntityID)> onSelect,
            Utils::ContextMenuUtils* contextMenuUtils,
            bool isEntityLoaded,
            ECS::EntityID activeEntityID = 0
        );

    }
}