// ViewState.h
#pragma once

struct ViewState {
    bool showDiffusionView = true;
    bool showDrawingCanvas = true;
    bool showSettingsView = true;
    bool shoeUpscaleView = true;
    bool showImageView = true;
};

extern ViewState viewState;
