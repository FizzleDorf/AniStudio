#define IMGUI_DEFINE_MATH_OPERATORS
#include "builders.h"
#include <imgui_internal.h>
#include <sstream>

namespace ed = ax::NodeEditor;
namespace util = ax::NodeEditor::Utilities;

util::BlueprintNodeBuilder::BlueprintNodeBuilder(ImTextureID texture, int textureWidth, int textureHeight)
    : HeaderTextureId(texture), HeaderTextureWidth(textureWidth), HeaderTextureHeight(textureHeight), CurrentNodeId(0),
      CurrentStage(Stage::Invalid), HasHeader(false) {}

void util::BlueprintNodeBuilder::Begin(ed::NodeId id) {
    HasHeader = false;
    HeaderMin = HeaderMax = ImVec2();

    ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(8, 4, 8, 8));
    ed::BeginNode(id);

    ImGui::PushID(id.AsPointer());
    CurrentNodeId = id;

    ImGui::BeginGroup();
    SetStage(Stage::Begin);
}

void util::BlueprintNodeBuilder::End() {
    SetStage(Stage::End);
    ImGui::EndGroup();
    ed::EndNode();
    ImGui::PopID();
    ed::PopStyleVar();
    SetStage(Stage::Invalid);
}

void util::BlueprintNodeBuilder::Header(const ImVec4 &color) {
    HeaderColor = ImColor(color);
    SetStage(Stage::Header);
}

void util::BlueprintNodeBuilder::EndHeader() { SetStage(Stage::Content); }

void util::BlueprintNodeBuilder::Input(ed::PinId id) {
    if (CurrentStage == Stage::Begin)
        SetStage(Stage::Content);

    SetStage(Stage::Input);

    ImGui::BeginGroup();
    Pin(id, ed::PinKind::Input);
}

void util::BlueprintNodeBuilder::EndInput() {
    ImGui::EndGroup();
    EndPin();
}

void util::BlueprintNodeBuilder::Middle() {
    if (CurrentStage == Stage::Begin)
        SetStage(Stage::Content);

    SetStage(Stage::Middle);
}

void util::BlueprintNodeBuilder::Output(ed::PinId id) {
    if (CurrentStage == Stage::Begin)
        SetStage(Stage::Content);

    SetStage(Stage::Output);

    ImGui::BeginGroup();
    Pin(id, ed::PinKind::Output);
}

void util::BlueprintNodeBuilder::EndOutput() {
    ImGui::EndGroup();
    EndPin();
}

bool util::BlueprintNodeBuilder::SetStage(Stage stage) {
    if (stage == CurrentStage)
        return false;

    auto oldStage = CurrentStage;
    CurrentStage = stage;

    switch (oldStage) {
    case Stage::Begin:
        break;
    case Stage::Header:
        ImGui::EndGroup();
        HeaderMin = ImGui::GetItemRectMin();
        HeaderMax = ImGui::GetItemRectMax();
        break;
    case Stage::Content:
        break;
    case Stage::Input:
        ImGui::EndGroup();
        break;
    case Stage::Middle:
        ImGui::EndGroup();
        break;
    case Stage::Output:
        ImGui::EndGroup();
        break;
    case Stage::End:
        break;
    case Stage::Invalid:
        break;
    }

    switch (stage) {
    case Stage::Begin:
        ImGui::BeginGroup();
        break;
    case Stage::Header:
        HasHeader = true;
        ImGui::BeginGroup();
        break;
    case Stage::Content:
        ImGui::BeginGroup();
        break;
    case Stage::Input:
        ImGui::BeginGroup();
        ed::PushStyleVar(ed::StyleVar_PivotAlignment, ImVec2(0, 0.5f));
        ed::PushStyleVar(ed::StyleVar_PivotSize, ImVec2(0, 0));
        break;
    case Stage::Middle:
        ImGui::BeginGroup();
        break;
    case Stage::Output:
        ImGui::BeginGroup();
        ed::PushStyleVar(ed::StyleVar_PivotAlignment, ImVec2(1.0f, 0.5f));
        ed::PushStyleVar(ed::StyleVar_PivotSize, ImVec2(0, 0));
        break;
    case Stage::End:
        ContentMin = ImGui::GetItemRectMin();
        ContentMax = ImGui::GetItemRectMax();
        NodeMin = ImGui::GetItemRectMin();
        NodeMax = ImGui::GetItemRectMax();
        break;
    case Stage::Invalid:
        break;
    }

    return true;
}

void util::BlueprintNodeBuilder::Pin(ed::PinId id, ed::PinKind kind) { ed::BeginPin(id, kind); }

void util::BlueprintNodeBuilder::EndPin() { ed::EndPin(); }