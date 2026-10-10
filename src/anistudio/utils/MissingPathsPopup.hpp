#pragma once
#include <string>

namespace ECS { class SettingsSystem; }

namespace Utils {
    void CheckMissingPaths(ECS::SettingsSystem* settingsSystem);
    void RenderMissingPathsPopup();
}