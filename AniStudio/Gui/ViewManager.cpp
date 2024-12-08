#include "ViewManager.hpp"

void ViewManager::Init(EntityManager &manager) {
    mgr = &manager;
    canvasView = std::make_unique<CanvasView>(1024, 1024);
    debugView = std::make_unique<DebugView>(*mgr);
    diffusionView = std::make_unique<DiffusionView>(filePaths);
    meshView = std::make_unique<MeshView>();
    sequencerView = std::make_unique<SequencerView>();
    settingsView = std::make_unique<SettingsView>();
    upscaleView = std::make_unique<UpscaleView>(filePaths);
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

void ViewManager::Update() {

}