#include "UpscaleView.hpp"

void UpscaleView::Render() {
    ImGui::Begin("Upscale View");

    // Display and allow editing of model component parameters
    if (mgr.HasComponent<EsrganComponent>(entity)) {
        modelComp = mgr.GetComponent<EsrganComponent>(entity);

        ImGui::Text("Esrgan Model:");
        ImGui::Text("%s", modelComp.modelName.c_str());

        if (ImGui::Button("...")) {
            IGFD::FileDialogConfig config;
            config.path = filePaths.checkpointDir;
            ImGuiFileDialog::Instance()->OpenDialog("LoadFileDialog", "Choose Model", ".safetensors, .ckpt, .pt, .gguf",
                                                    config);
        }

        if (ImGuiFileDialog::Instance()->Display("LoadFileDialog", 32, ImVec2(700, 400))) {
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
        ImGui::InputFloat("Scale", &modelComp.scale);

    }


    ImGui::End();
}
