#pragma once

struct ViewState {
    bool showDiffusionView = true;
    bool showDrawingCanvas = false;
    bool showMeshView = false;
    bool showSettingsView = false;
    bool showUpscaleView = false;
    bool showImageView = true;
    bool showNodeGraphView = false;
    bool showSequencerView = false;
    bool showDebugView = true;
};

extern ViewState viewState;
