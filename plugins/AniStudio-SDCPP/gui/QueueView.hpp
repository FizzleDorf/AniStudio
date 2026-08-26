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
        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& j) override;

    private:
        static constexpr const char* DIFFUSION_VIEWS[] = {
            "Txt2ImgView",
            "Img2ImgView",
            "EditView",
            "UpscaleView",
            "ConvertView",
            "Img2VidView",
            "Txt2VidView"
        };
        static constexpr size_t DIFFUSION_VIEWS_COUNT = sizeof(DIFFUSION_VIEWS) / sizeof(DIFFUSION_VIEWS[0]);

        int m_selectedViewIndex = 0;
        std::vector<BaseView*> m_availableViews;
        int numQueues = 1;
        bool isPaused = false;
        bool m_queueLoaded = false;

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
        bool IsDiffusionView(BaseView* view) const;
    };

} // namespace GUI