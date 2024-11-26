#include "UpscaleView.hpp"

void UpscaleView::Render() {
    ImGui::SetNextWindowSize(ImVec2(300, 800), ImGuiCond_FirstUseEver);
    ImGui::Begin("Upscale View");

    // Display and allow editing of the model component parameters
    ImGui::Text("Esrgan Model:");
    ImGui::Text("%s", modelComp.modelName.c_str());

    if (ImGui::Button("...##d9")) {
        IGFD::FileDialogConfig config;
        config.path = filePaths.upscaleDir;
        ImGuiFileDialog::Instance()->OpenDialog("LoadUpscaleDialog", "Choose Upscale Model",
                                                ".safetensors,.ckpt,.pt,.gguf", config);
    }

    if (ImGuiFileDialog::Instance()->Display("LoadUpscaleDialog", 32, ImVec2(700, 400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
            std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();

            modelComp.modelName = selectedFile;
            modelComp.modelPath = fullPath;

            std::cout << "Selected file: " << modelComp.modelName << std::endl;
            std::cout << "Full path: " << modelComp.modelPath << std::endl;
        }

        ImGuiFileDialog::Instance()->Close();
    }

    // Display and edit the scaling factor
    ImGui::InputFloat("Scale", &modelComp.scale);

    ImGui::End();
}
