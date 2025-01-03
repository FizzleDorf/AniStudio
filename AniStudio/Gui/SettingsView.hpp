#pragma once

#include "imgui.h"
#include "ECS.h"
#include "ViewState.hpp"
#include "filepaths.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <iostream>

// FlagOption struct for ComfyUI Path Args
struct FlagOption {
    const char* flag;
    const char* description;
    char value[256];
};

// BoolOption struct for ComfyUI Bool Args
struct BoolOption {
    const char* flag;
    const char* description;
    bool enabled;
};

class SettingsView {
public:
    void Render();
    

private:
    void InstallVenv();
    void InstallComfyUI();
    nlohmann::json SerializeOptions();
    void ShowBoolOptionsTable(BoolOption* options, int count, const char* tableTitle);
    void ShowFlagPathsTable(FlagOption* options, int count, const char* tableTitle);
    void RenderSettingsWindow();
    void RunComfyUI();
};

extern BoolOption boolOptions[];
extern FlagOption inputOptions[];

