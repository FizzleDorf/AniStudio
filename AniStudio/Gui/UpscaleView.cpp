#include "UpscaleView.hpp"

namespace GUI {

void UpscaleView::Render() {
    ImGui::SetNextWindowSize(ImVec2(300, 800), ImGuiCond_FirstUseEver);
    ImGui::Begin("Upscale View");
    if (ImGui::BeginTable("ModelLoaderTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("Model", ImGuiTableColumnFlags_WidthFixed, 52.0f); // Fixed width for Model
        ImGui::TableSetupColumn("Load", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        ImGui::TableNextColumn();
        // Display and allow editing of the model component parameters
        ImGui::Text("Esrgan Model:");
        ImGui::TableNextColumn();
        if (ImGui::Button("...##d9")) {
            IGFD::FileDialogConfig config;
            config.path = filePaths.upscaleDir;
            ImGuiFileDialog::Instance()->OpenDialog("LoadUpscaleDialog", "Choose Upscale Model",
                                                    ".safetensors,.ckpt,.pt,.gguf", config);
        }
        ImGui::SameLine();
        if (ImGui::Button("R##t3")) {
            modelComp.modelName = "";
            modelComp.modelPath = "";
        }
        ImGui::TableNextColumn();
        ImGui::Text("%s", modelComp.modelName.c_str());
        ImGui::EndTable();
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
} // namespace GUI