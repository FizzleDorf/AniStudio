#pragma once

#include "Constants.hpp"
#include "ECS.h"
#include "GUI.h"
#include "ImGuiFileDialog.h"
#include "SDCPPComponents.h"
#include "pch.h"

namespace GUI {

class ConvertView : public BaseView {
public:
    ConvertView(ECS::EntityManager &entityMgr) : BaseView(entityMgr) { viewName = "ConvertView"; }
    ~ConvertView() = default;

    void Init() override {}

    void Render() override {
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
        ImGui::Begin("Convert Model to GGUF/Quant");

        if (ImGui::BeginTable("ModelLoaderTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Model", ImGuiTableColumnFlags_WidthFixed, 52.0f);
            ImGui::TableSetupColumn("Load", ImGuiTableColumnFlags_WidthFixed, 52.0f);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            // Row for "Input Model"
            ImGui::TableNextColumn();
            ImGui::Text("Input Model");
            ImGui::TableNextColumn();
            if (ImGui::Button("...##j6")) {
                IGFD::FileDialogConfig config;
                config.path = filePaths.checkpointDir;
                ImGuiFileDialog::Instance()->OpenDialog("ConvertModelDialog", "Choose Model",
                                                        ".safetensors, .ckpt, .pt, .gguf", config);
            }
            ImGui::SameLine();
            if (ImGui::Button("R##j6")) {
                modelComp.modelName = "";
                modelComp.modelPath = "";
            }
            ImGui::TableNextColumn();
            ImGui::Text("%s", modelComp.modelName.c_str());
            
            RenderVaeLoader();

            ImGui::EndTable();

        }

        if (ImGuiFileDialog::Instance()->Display("ConvertModelDialog", 32, ImVec2(700, 400))) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
                std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();

                modelComp.modelName = selectedFile;
                modelComp.modelPath = fullPath;
                std::cout << "Selected file: " << modelComp.modelName << std::endl;
                std::cout << "Full path: " << modelComp.modelPath << std::endl;
                std::cout << "New model path set: " << modelComp.modelPath << std::endl;
            }

            ImGuiFileDialog::Instance()->Close();
        }
        ImGui::Combo("Quant Type", reinterpret_cast<int *>(&samplerComp.current_type_method), type_method_items,
                     type_method_item_count);

        if (ImGui::Button("Convert")) {
            Convert();
        }

        ImGui::End();
    }

private:
    ECS::SamplerComponent samplerComp;
    ECS::ModelComponent modelComp;
    ECS::VaeComponent vaeComp;
    
    void Convert() {
        auto sdSystem = mgr.GetSystem<ECS::SDCPPSystem>();
        if (!sdSystem) {
            mgr.RegisterSystem<ECS::SDCPPSystem>();
        }
        ECS::EntityID entity = mgr.AddNewEntity();
        mgr.AddComponent<ECS::ModelComponent>(entity);
        mgr.AddComponent<ECS::SamplerComponent>(entity);
        mgr.AddComponent<ECS::VaeComponent>(entity);
        mgr.GetComponent<ECS::ModelComponent>(entity) = modelComp;
        mgr.GetComponent<ECS::SamplerComponent>(entity) = samplerComp;
        mgr.GetComponent<ECS::VaeComponent>(entity) = vaeComp;

        ANI::Event event;
        event.entityID = entity;
        event.type = ANI::EventType::ConvertToGGUF;
        ANI::Events::Ref().QueueEvent(event);
    }

    void RenderVaeLoader() {
        ImGui::TableNextColumn();
        ImGui::Text("Vae: ");
        ImGui::TableNextColumn();
        if (ImGui::Button("...##4b")) {
            IGFD::FileDialogConfig config;
            config.path = filePaths.vaeDir;
            ImGuiFileDialog::Instance()->OpenDialog("ConvertVaeDialog", "Choose Model", ".safetensors, .ckpt, .pt, .gguf",
                                                    config);
        }
        ImGui::SameLine();
        if (ImGui::Button("R##f7")) {
            vaeComp.modelName = "";
            vaeComp.modelPath = "";
        }
        ImGui::TableNextColumn();
        ImGui::Text("%s", vaeComp.modelName.c_str());

        if (ImGuiFileDialog::Instance()->Display("ConvertVaeDialog", 32, ImVec2(700, 400))) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
                std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();

                vaeComp.modelName = selectedFile;
                vaeComp.modelPath = fullPath;
                std::cout << "Selected file: " << vaeComp.modelName << std::endl;
                std::cout << "Full path: " << vaeComp.modelPath << std::endl;
                std::cout << "New model path set: " << vaeComp.modelPath << std::endl;
            }

            ImGuiFileDialog::Instance()->Close();
        }
    }
};

} // namespace GUI