#ifndef BASEMEDIAVIEW_HPP
#define BASEMEDIAVIEW_HPP

#include "GUI.h"
#include "BaseView.hpp"
#include "ContextMenuUtils.hpp"
#include <vector>
#include <string>

namespace GUI {

    class BaseMediaView : public BaseView {
    public:
        BaseMediaView(ECS::EntityManager& mgr, ViewManager& vm);
        virtual ~BaseMediaView();

        virtual void Init() override = 0;
        virtual void Update(float deltaT) override = 0;
        virtual void Render() override = 0;

        virtual void LoadMedia(const std::vector<std::string>& filePaths) = 0;
        virtual void SaveSelectedMedia() = 0;
        virtual void SaveSelectedMediaAs(const std::string& filePath) = 0;
        virtual void RemoveSelectedMedia() = 0;
        virtual void RefreshEntities() = 0;

        virtual void SetZoom(float newZoom);
        virtual void DrawGrid(int width, int height);
        virtual std::string TruncateFilename(const std::string& filename, float maxTextWidth);

        virtual ECS::EntityID GetSelectedEntity() const;
        virtual void SetSelectedEntity(ECS::EntityID entity);

        virtual void ToggleHistoryView(bool show);
        virtual bool IsHistoryVisible() const;

        void HandleFileDropTarget();
        void HandleEntityDropTarget();
        void HandleClipboardPaste();

        bool IsAltKeyDown() const;

    protected:
        ECS::EntityID selectedEntityID;
        int index;
        float zoom;
        float offsetX;
        float offsetY;
        size_t lastEntityCount;
        std::vector<ECS::EntityID> mediaEntities;
        bool historyViewVisible;
        WorkspaceID historyWorkspaceID;

        std::unique_ptr<Utils::ContextMenuUtils> contextMenuUtils;

        virtual void OnMediaAdded(ECS::EntityID entity) = 0;
        virtual void OnMediaRemoved(ECS::EntityID entity) = 0;
        virtual void UpdateSelectionAfterRemoval(ECS::EntityID removedEntity);

        void RenderMediaContextMenu(ECS::EntityID entityID);
        void RenderMediaContextMenuForPath(const std::string& filePath);

        virtual std::string GetHistoryViewTypeName() const = 0;

        bool isDragging;
        ImVec2 dragStartPos;
    };

} // namespace GUI

#endif