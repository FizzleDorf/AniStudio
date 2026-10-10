#include "ImGuiRenderSettingsTab.hpp"
#include "SettingsSystem.hpp"
#include "FilePathComponent.hpp"
#include "UISchema.hpp"
#include <imgui.h>

namespace ECS {

    void ImGuiRenderSettingsTab::ApplyToImGui(ImGuiRenderSettingsComponent& comp, ImGuiContext* ctx) {
        if (!ctx) return;
        ImGui::SetCurrentContext(ctx);
        ImGuiIO& io = ImGui::GetIO();

        io.ConfigWindowsResizeFromEdges = comp.configWindowsResizeFromEdges;
        io.ConfigWindowsMoveFromTitleBarOnly = comp.configWindowsMoveFromTitleBarOnly;
        io.ConfigDragClickToInputText = comp.configDragClickToInputText;
        io.ConfigNavMoveSetMousePos = comp.configNavMoveSetMousePos;
        io.ConfigNavCaptureKeyboard = comp.configNavCaptureKeyboard;
        io.ConfigNavEscapeClearFocusItem = comp.configNavEscapeClearFocusItem;
        io.ConfigDockingWithShift = comp.configDockingWithShift;
        io.ConfigDockingAlwaysTabBar = comp.configDockingAlwaysTabBar;
        io.ConfigDockingTransparentPayload = comp.configDockingTransparentPayload;
        io.ConfigViewportsNoAutoMerge = comp.configViewportsNoAutoMerge;
        io.ConfigViewportsNoTaskBarIcon = comp.configViewportsNoTaskBarIcon;
        io.ConfigViewportsNoDecoration = comp.configViewportsNoDecoration;
        io.ConfigViewportsNoDefaultParent = comp.configViewportsNoDefaultParent;
        io.ConfigInputTextCursorBlink = comp.configInputTextCursorBlink;
        io.ConfigInputTextEnterKeepActive = comp.configInputTextEnterKeepActive;

        auto setFlag = [&io](ImGuiConfigFlags flag, bool on) {
            if (on) io.ConfigFlags |= flag; else io.ConfigFlags &= ~flag;
            };
        setFlag(ImGuiConfigFlags_NavEnableKeyboard, comp.configNavEnableKeyboard);
        setFlag(ImGuiConfigFlags_NavEnableGamepad, comp.configNavEnableGamepad);
        setFlag(ImGuiConfigFlags_DockingEnable, comp.configDockingEnable);
        setFlag(ImGuiConfigFlags_ViewportsEnable, comp.configViewportsEnable);
    }

    ImGuiRenderSettingsTab::ImGuiRenderSettingsTab(SettingsSystem& sys, ImGuiRenderSettingsComponent& comp)
        : m_settingsSystem(sys), m_comp(comp) {
    }

    void ImGuiRenderSettingsTab::Render() {
        if (!m_imguiContext) return;
        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(m_imguiContext));

        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
            "Warning: Some changes require application restart");
        ImGui::Separator();

        auto props = m_comp.GetPropertyMap();
        auto* filePathComp = m_settingsSystem.GetFilePathComponent();
        const std::unordered_map<std::string, std::string>* pathMap =
            filePathComp ? &filePathComp->GetCurrentMap() : nullptr;

        bool changed = UISchema::RenderSchema(
            m_comp.GetSchema(),
            props,
            nullptr,
            m_comp.GetCompName(),
            (int)m_settingsSystem.GetSettingsEntity(),
            pathMap);

        if (changed) ApplyToImGui(m_comp, static_cast<ImGuiContext*>(m_imguiContext));
    }

}