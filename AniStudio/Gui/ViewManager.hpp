#pragma once
#include "ECS.h"
#include "ViewState.hpp"
#include <Guis.h>
#include <vector>
#include <memory>

class CanvasView;
class DebugView;
class DiffusionView;
class MeshView;
class SequencerView;
class SettingsView;
class UpscaleView;
class NodeGraphView;

using namespace ECS;

class ViewManager {
    public:
        ViewManager() {}
        ~ViewManager() {}
        void Init();
        void Update(const float deltaT);
        void Render();

    private:  
        std::unique_ptr<CanvasView> canvasView = nullptr;
        std::unique_ptr<DebugView> debugView = nullptr;
        std::unique_ptr<DiffusionView> diffusionView = nullptr;
        std::unique_ptr<MeshView> meshView = nullptr;
        std::unique_ptr<SequencerView> sequencerView = nullptr;
        std::unique_ptr<SettingsView> settingsView = nullptr;
        std::unique_ptr<UpscaleView> upscaleView = nullptr;
        std::unique_ptr<NodeGraphView> nodeGraphView = nullptr;
};