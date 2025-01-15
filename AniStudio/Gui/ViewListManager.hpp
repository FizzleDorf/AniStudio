#pragma once
#include "Base/BaseView.hpp"
#include "ImGuiFileDialog.h"
#include "pch.h"
#include "guis.h"
#include <components.h>

namespace GUI {

struct ViewListTemplate {
    std::string name;
    std::vector<ViewTypeID> viewTypes;
    bool isDefault;
};

class ViewListManager : public BaseView {
public:
    ViewListManager(ECS::EntityManager &entityMgr, ViewManager &viewMgr) : BaseView(entityMgr), viewMgr(viewMgr) {
        viewName = "ViewListManager";
    }
    ~ViewListManager() = default;

    void Init() override;
    void Render() override;

    void SaveViewList(const std::string &name);
    void LoadViewList(const std::string &name);
    void DeleteViewList(const std::string &name);
    void CreateDefaultTemplates();
    void SaveDefaultTemplates();
    void LoadTemplates();

private:
    void RenderTemplateList();
    void RenderViewTypeSelector();
    void RenderCurrentViewList();
    void RenderTemplateControls();

    ViewManager &viewMgr;

    std::vector<ViewListTemplate> templates;
    std::string selectedTemplate;
    std::string newTemplateName;
    bool isEditingTemplate = false;

    const std::string templatesPath = "../data/templates/viewlists.json";
    const std::string lastSelectedPath = "../data/defaults/last_viewlist.json";

    // Default templates
    const std::vector<ViewListTemplate> defaultTemplates = {
        {"Default", {ViewType<DiffusionView>()}, true},
        // {"Image Editor", {ViewType<DiffusionView>(), ViewType<CanvasView>()}, true},
        {"Debug", {ViewType<DebugView>()}, true}};
};

} // namespace GUI