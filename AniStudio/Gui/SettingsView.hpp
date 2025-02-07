#pragma once

#include "Base/BaseView.hpp"
#include "filepaths.hpp"
#include <nlohmann/json.hpp>
#include "ImGuiFileDialog.h"
#include "GuiStyleHelpers.hpp"

 namespace GUI {

class SettingsView : public BaseView {
public:
    SettingsView(EntityManager &mgr) : BaseView(mgr) { viewName = "Settings View";}
    ~SettingsView() {}

    void Render() override;

private:
    void RenderSettingsWindow();
    void RenderPathRow(const char *label, std::string &path);

    void ShowFontSelector(const char *label);
    bool ShowStyleSelector(const char *label);

    void ShowStyleEditor(ImGuiStyle *ref);
    

};
} // namespace GUI