#pragma once

#include "SDCPPComponents.h"
#include "pch.h"
#include "ECS.h"
#include "GUI.h"

namespace GUI {

class ConvertView : public BaseView {
public:
    ConvertView(ECS::EntityManager &mgr) : BaseView(mgr) {}
    ~ConvertView() = default;
    
    void Init() override {}
    void Render() override {
        ImGui::Begin("Convert Model to GGUF/Quant") {

            if (ImGui::BeginTable("ModelLoaderTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Model", ImGuiTableColumnFlags_WidthFixed, 52.0f);
                ImGui::TableSetupColumn("Load", ImGuiTableColumnFlags_WidthFixed, 52.0f);
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);

                // Row for "Checkpoint"
                ImGui::TableNextColumn();
                ImGui::Text("Input Model");
                ImGui::TableNextColumn();
                if (ImGui::Button("...##j6")) {
                    IGFD::FileDialogConfig config;
                    config.path = filePaths.checkpointDir;
                    ImGuiFileDialog::Instance()->OpenDialog("LoadFileDialog", "Choose Model",
                                                            ".safetensors, .ckpt, .pt, .gguf", config);
                }
                ImGui::SameLine();
                if (ImGui::Button("R##j6")) {
                    modelComp.modelName = "";
                    modelComp.modelPath = "";
                }
                ImGui::TableNextColumn();
                ImGui::Text("%s", modelComp.modelName.c_str());

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    void Convert() {
        mgr.RegisterSystem<SDCPPSystem>();
        EntityID entity = mgr.AddNewEntity();
        mgr.AddComponent<ModelComponent>(entity);
        mgr.AddComponent<SamplerComponent>(entity);

        mgr.GetComponent<ModelComponent>(entity) = modelComp;
        mgr.GetComponent<SamplerComponent>(entity) = samplerComp;

        Event event;

        ANI::Events::QueueEvent(event)
        
    }

	private:

        ModelComponent modelComp;
        SamplerComponent samplerComp;
}
} // namespace GUI