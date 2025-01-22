#pragma once
#include "Base/BaseView.hpp"
#include "Base/ViewManager.hpp"
#include "DiffusionView.hpp"
#include "ImGuiFileDialog.h"
#include "pch.h"
#include "stable-diffusion.h"
#include <SDcppSystem.hpp>
#include <components.h>

namespace GUI {

class QueueView : public BaseView {
public:
    QueueView(ECS::EntityManager &entityMgr, ViewManager &viewMgr) : BaseView(entityMgr), vMgr(viewMgr) {
        viewName = "QueueView";
    }
    ~QueueView() = default;

    // Overloaded Functions
    void Render() override;
    // nlohmann::json Serialize() const override;
    // void Deserialize(const nlohmann::json &j) override;
    // 
private:
    void QueueGeneration(int count);
    void StopCurrentGeneration();
    void ClearQueue();
    
    ViewManager &vMgr;
    int numQueues = 1;
    bool isProcessing = false;
};
} // namespace GUI