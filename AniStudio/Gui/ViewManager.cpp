#include "ViewManager.hpp"

void ViewManager::Init() {
    canvasView = std::make_unique<CanvasView>();
    debugView = std::make_unique<DebugView>();
    diffusionView = std::make_unique<DiffusionView>();
    meshView = std::make_unique<MeshView>();
    sequencerView = std::make_unique<SequencerView>();
    settingsView = std::make_unique<SettingsView>();
    upscaleView = std::make_unique<UpscaleView>();
    nodeGraphView = std::make_unique<NodeGraphView>();
}

void ViewManager::Render() {
    if (viewState.showDiffusionView)
        diffusionView->Render();
    if (viewState.showSettingsView)
        settingsView->Render();
    if (viewState.showUpscaleView)
        upscaleView->Render();
    if (viewState.showMeshView)
        meshView->Render();
    if (viewState.showNodeGraphView)
        nodeGraphView->Render();
    if (viewState.showSequencerView)
        sequencerView->Render();
    if (viewState.showDrawingCanvas)
        canvasView->Render();
    if (viewState.showDebugView)
        debugView->Render();
}

void ViewManager::Update(const float deltaT) { 
    canvasView->Update(deltaT); 
}