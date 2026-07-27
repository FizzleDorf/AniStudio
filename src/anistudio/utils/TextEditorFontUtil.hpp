#pragma once
#include <imgui.h>
#include <string>
#include "FontSettingsComponent.hpp"
#include "TextEditorSettingsComponent.hpp"

namespace TextEditorUtil {

    void SetFontComponent(ECS::FontSettingsComponent* comp);
    void SetSettingsComponent(ECS::TextEditorSettingsComponent* comp);

    void pushEditorFont();
    void popEditorFont();

    void clearEditorFontCache();

} // namespace TextEditorUtil