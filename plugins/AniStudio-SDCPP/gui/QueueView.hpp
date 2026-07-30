// QueueView.hpp
#pragma once
#include "BaseView.hpp"
#include "ViewManager.hpp"
#include "SDcppSystem.hpp"
#include <vector>
#include <string>

namespace GUI {

    class QueueView : public BaseView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Queue Manager",
            "category": "Diffusion",
            "description": "Manage and save/load the global task queue."
        })";
        }

        QueueView(ECS::EntityManager& mgr, ViewManager& vm);
        ~QueueView();

        void Init() override;
        void Render() override;

    private:
        int m_selectedViewIndex = 0;
        std::vector<BaseView*> m_availableViews;
        int numQueues = 1;
        bool isPaused = false;

        void RefreshViewList();
        void RenderViewSelector();
        void RenderQueueList();
        void RenderMenuBar();

        void QueueFromSelectedView();
        void SaveQueue();
        void LoadQueue();
        void QuickSave();
        void QuickLoad();

        void RenderQueueItemContextMenu(const ECS::SDCPPSystem::QueueItem& item, size_t index);
    };

} // namespace GUI