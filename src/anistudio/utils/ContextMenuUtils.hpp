#pragma once

#include "EntityManager.hpp"
#include "Types.hpp"
#include <nlohmann/json.hpp>
#include <imgui.h>
#include <string>
#include <vector>
#include <map>

namespace Utils {

    class ContextMenuUtils {
    public:
        ContextMenuUtils(ECS::EntityManager& entityMgr);
        ~ContextMenuUtils() = default;

        void RenderEntityContextMenu(ECS::EntityID entityId);

        void RenderImageContextMenu(ECS::EntityID entityId);
        void RenderImageContextMenuWithPath(const std::string& imagePath);

        void RenderVideoContextMenu(ECS::EntityID entityId);
        void RenderVideoContextMenuWithPath(const std::string& videoPath);

        void RenderComponentContextMenu(ECS::EntityID entityId, ECS::ComponentTypeID componentId);
        void RenderComponentContextMenu(ECS::EntityID entityId, const std::string& componentName);

        void CopyEntity(ECS::EntityID entityId);
        void CopyComponent(ECS::EntityID entityId, ECS::ComponentTypeID componentId);
        void CopyComponent(ECS::EntityID entityId, const std::string& componentName);
        void CopyFilePath(ECS::EntityID entityId);
        void CopyImageData(ECS::EntityID entityId);
        void CopyVideoFrame(ECS::EntityID entityId);

        bool PasteEntity(ECS::EntityID targetEntityId);
        bool PasteComponent(ECS::EntityID targetEntityId, const std::string& componentName);
        bool PasteValue(ECS::EntityID targetEntityId, const std::string& componentName, const std::string& propertyName);

        bool HasClipboardEntity() const;
        std::string GetClipboardPreview() const;
        void ClearClipboard();

        std::vector<std::string> GetClipboardComponents() const;
        std::vector<std::string> GetCommonProperties(ECS::EntityID targetEntityId, const std::string& componentName) const;

        nlohmann::json GetClipboardData() const;
        void RenderPasteMenu(ECS::EntityID entityId);

    private:
        ECS::EntityManager& entityManager;

        nlohmann::json ParseImageMetadata(const std::string& imagePath);
        std::vector<std::string> ExtractComponentsFromMetadata(const nlohmann::json& metadata);

        void RenderMetadataComponentMenu(const nlohmann::json& metadata);
        void RenderEntityComponentMenu(ECS::EntityID entityId);
        void RenderValueCopyMenu(ECS::EntityID entityId, ECS::ComponentTypeID componentId, const std::string& componentName);
        void RenderMetadataValueMenu(const nlohmann::json& metadata);

        void CopyComponentFromMetadata(const nlohmann::json& metadata, const std::string& componentName);
        void CopyEntityFromMetadata(const nlohmann::json& metadata);

        void SetClipboardData(const nlohmann::json& data);

        std::string GetClipboardValue(const std::string& componentName, const std::string& propertyName) const;
        nlohmann::json GetClipboardComponent(const std::string& componentName) const;

        bool SetClipboardDIB(unsigned char* data, int width, int height, int channels);
    };

} // namespace Utils