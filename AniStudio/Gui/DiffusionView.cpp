#include "DiffusionView.hpp"
#include "../Events/Events.hpp"
#include "Constants.hpp"
#define NOMINMAX
#include <exiv2/exiv2.hpp>

using namespace ECS;
using namespace ANI;

static void LogCallback(sd_log_level_t level, const char *text, void *data) {
    switch (level) {
    case SD_LOG_DEBUG:
        std::cout << "[DEBUG]: " << text;
        break;
    case SD_LOG_INFO:
        std::cout << "[INFO]: " << text;
        break;
    case SD_LOG_WARN:
        std::cout << "[WARNING]: " << text;
        break;
    case SD_LOG_ERROR:
        std::cerr << "[ERROR]: " << text;
        break;
    default:
        std::cerr << "[UNKNOWN LOG LEVEL]: " << text;
        break;
    }
}
static ProgressData progressData;
static void ProgressCallback(int step, int steps, float time, void *data) {
    progressData.currentStep = step;
    progressData.totalSteps = steps;
    progressData.currentTime = time;
    progressData.isProcessing = (steps > 0);
    std::cout << "Progress: Step " << step << " of " << steps << " | Time: " << time << "s" << std::endl;
}

namespace GUI {

DiffusionView::DiffusionView(EntityManager &entityMgr) : BaseView(entityMgr) {
    viewName = "DiffusionView";
    sd_set_log_callback(LogCallback, nullptr);
    sd_set_progress_callback(ProgressCallback, nullptr);
}

void DiffusionView::RenderModelLoader() {

    ImGui::Combo("Quant Type", &int(samplerComp.current_type_method), type_method_items, type_method_item_count);

    if (ImGui::BeginTable("ModelLoaderTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Model", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("Load", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        // ImGui::TableHeadersRow();

        // Row for "Checkpoint"
        ImGui::TableNextColumn();
        ImGui::Text("Model");
        ImGui::TableNextColumn();
        if (ImGui::Button("...##j6")) {
            IGFD::FileDialogConfig config;
            config.path = filePaths.checkpointDir;
            ImGuiFileDialog::Instance()->OpenDialog("LoadFileDialog", "Choose Model", ".safetensors, .ckpt, .pt, .gguf",
                                                    config);
        }
        ImGui::SameLine();
        if (ImGui::Button("R##j6")) {
            modelComp.modelName = "";
            modelComp.modelPath = "";
        }
        ImGui::TableNextColumn();
        ImGui::Text("%s", modelComp.modelName.c_str());

        RenderVaeLoader();
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
}

static char fileName[256] = "AniStudio"; // Buffer for file name
static char outputDir[256] = "";         // Buffer for output directory

void DiffusionView::RenderFilePath() {
    if (ImGui::BeginTable("Output Name", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Param", ImGuiTableColumnFlags_WidthFixed, 54.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        // ImGui::TableHeadersRow();
        ImGui::TableNextColumn();
        ImGui::Text("FileName"); // Row for "Filename"
        ImGui::TableNextColumn();
        if (ImGui::InputText("##Filename", fileName, IM_ARRAYSIZE(fileName))) {
            isFilenameChanged = true;
        }
        ImGui::EndTable();
    }
    if (ImGui::BeginTable("Output Dir", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Param", ImGuiTableColumnFlags_WidthFixed, 54.0f); // Fixed width for Model
        ImGui::TableSetupColumn("Load", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
        // ImGui::TableHeadersRow();
        ImGui::TableNextColumn();
        ImGui::Text("Dir Path"); // Row for "Output Directory"
        ImGui::TableNextColumn();
        if (ImGui::Button("...##w8")) {
            IGFD::FileDialogConfig config;
            config.path = filePaths.defaultProjectPath; // Set the initial directory
            ImGuiFileDialog::Instance()->OpenDialog("LoadDirDialog", "Choose Directory", nullptr, config);
        }
        ImGui::SameLine();
        if (ImGui::Button("R##w9")) {
            imageComp.fileName = "<none>";
            imageComp.filePath = "filePaths.defaultProjectPath";
        }
        ImGui::TableNextColumn();
        ImGui::Text("%s", imageComp.filePath.c_str());

        ImGui::EndTable();
    }

    // Handle the file dialog display and selection
    if (ImGuiFileDialog::Instance()->Display("LoadDirDialog", 32, ImVec2(700, 400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string selectedDir = ImGuiFileDialog::Instance()->GetCurrentPath();
            if (!selectedDir.empty()) {
                strncpy(outputDir, selectedDir.c_str(), IM_ARRAYSIZE(outputDir) - 1);
                outputDir[IM_ARRAYSIZE(outputDir) - 1] = '\0'; // Null termination
                imageComp.filePath = selectedDir;
                isFilenameChanged = true;
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    // Update ImageComponent properties if filename or filepath changes
    if (isFilenameChanged) {
        std::string newFileName(fileName);

        // Ensure the file name has a .png extension
        if (!newFileName.empty()) {
            std::filesystem::path filePath(newFileName);
            if (filePath.extension() != ".png") {
                filePath.replace_extension(".png");
                newFileName = filePath.filename().string();
            }
        }

        // Update the ImageComponent
        if (!newFileName.empty()) {
            imageComp.fileName = newFileName;
            std::cout << "ImageComponent updated:\n";
            std::cout << "  FileName: " << imageComp.fileName << '\n';
            std::cout << "  FilePath: " << imageComp.filePath << '\n';
        } else {
            std::cerr << "Invalid directory or filename!" << '\n';
        }

        isFilenameChanged = false; // Reset the flag
    }
}

void DiffusionView::RenderLatents() {

    ImGui::Combo("RNG Type", &int(samplerComp.current_rng_type), type_rng_items, type_rng_item_count);

    if (ImGui::BeginTable("PromptTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Param", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Width");
        ImGui::TableNextColumn();
        ImGui::InputInt("##Width", &latentComp.latentWidth, 8, 8);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Height");
        ImGui::TableNextColumn();
        ImGui::InputInt("##Height", &latentComp.latentHeight, 8, 8);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Height");
        ImGui::TableNextColumn();
        ImGui::InputInt("##Batch Size", &latentComp.batchSize);
        ImGui::EndTable();
    }
}

void DiffusionView::RenderInputImage() {
    ImGui::Text("Image input placeholder"); // Adjust to render the actual image component if needed
}

void DiffusionView::RenderPrompts() {
    if (ImGui::BeginTable("PromptTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Param", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        // ImGui::TableHeadersRow();

        // Positive Prompt
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Positive");
        ImGui::TableNextColumn();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        if (ImGui::InputTextMultiline("##Positive Prompt", promptComp.PosBuffer, sizeof(promptComp.PosBuffer),
                                      ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 8))) { // Adjust height
            promptComp.posPrompt = promptComp.PosBuffer;
        }
        ImGui::PopStyleVar(); // Restore frame padding

        // Negative Prompt
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Negative");
        ImGui::TableNextColumn();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        if (ImGui::InputTextMultiline("##Negative Prompt", promptComp.NegBuffer, sizeof(promptComp.NegBuffer),
                                      ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 8))) { // Adjust height
            promptComp.negPrompt = promptComp.NegBuffer;
        }
        ImGui::PopStyleVar(); // Restore frame padding

        ImGui::EndTable();
    }
}

void DiffusionView::RenderSampler() {
    if (ImGui::BeginTable("SamplerSettingsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Param", ImGuiTableColumnFlags_WidthFixed, 64.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        // ImGui::TableHeadersRow();

        auto addRow = [](const char *label, auto renderInput) {
            ImGui::TableNextRow();

            // Column 1: Label
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(label);

            // Column 2: Input Field
            ImGui::TableSetColumnIndex(1);
            renderInput();
        };

        // Add rows for each control
        addRow("Sampler", [&]() {
            ImGui::Combo("##Sampler", reinterpret_cast<int *>(&samplerComp.current_sample_method), sample_method_items,
                         sample_method_item_count);
        });

        addRow("Scheduler", [&]() {
            ImGui::Combo("##Scheduler", reinterpret_cast<int *>(&samplerComp.current_scheduler_method),
                         scheduler_method_items, scheduler_method_item_count);
        });

        addRow("Seed", [&]() {
            ImGui::InputInt("##Seed", &samplerComp.seed);
            // renderControlUI<int>(*seedControl);
        });
        addRow("CFG", [&]() { ImGui::InputFloat("##CFG", &cfgComp.cfg, 0.5f, 0.5f, "%.2f"); });
        addRow("Guidance", [&]() { ImGui::InputFloat("##Guidance", &cfgComp.guidance, 0.05f, 0.1f, "%.2f"); });
        addRow("Steps", [&]() { ImGui::InputInt("##Steps", &samplerComp.steps); });
        addRow("Denoise", [&]() { ImGui::InputFloat("##Denoise", &samplerComp.denoise, 0.01f, 0.1f, "%.2f"); });

        ImGui::EndTable();
    }
}

void DiffusionView::HandleT2IEvent() {
    std::cout << "Adding new entity..." << std::endl;
    EntityID newEntity = mgr.AddNewEntity();
    if (newEntity == 0) {
        std::cerr << "Failed to create new entity!" << std::endl;
        return;
    }

    std::cout << "Initialized entity with ID: " << newEntity << std::endl;

    try {
        loraComp.modelPath = filePaths.loraDir;

        // Add components
        mgr.AddComponent<ModelComponent>(newEntity);
        mgr.AddComponent<CLipLComponent>(newEntity);
        mgr.AddComponent<CLipGComponent>(newEntity);
        mgr.AddComponent<T5XXLComponent>(newEntity);
        mgr.AddComponent<DiffusionModelComponent>(newEntity);
        mgr.AddComponent<VaeComponent>(newEntity);
        mgr.AddComponent<TaesdComponent>(newEntity);
        mgr.AddComponent<ControlnetComponent>(newEntity);
        mgr.AddComponent<LoraComponent>(newEntity);
        mgr.AddComponent<LatentComponent>(newEntity);
        mgr.AddComponent<SamplerComponent>(newEntity);
        mgr.AddComponent<CFGComponent>(newEntity);
        mgr.AddComponent<PromptComponent>(newEntity);
        mgr.AddComponent<EmbeddingComponent>(newEntity);
        mgr.AddComponent<LayerSkipComponent>(newEntity);
        mgr.AddComponent<ImageComponent>(newEntity);

        // Assign component data
        mgr.GetComponent<ModelComponent>(newEntity) = modelComp;
        mgr.GetComponent<CLipLComponent>(newEntity) = clipLComp;
        mgr.GetComponent<CLipGComponent>(newEntity) = clipGComp;
        mgr.GetComponent<T5XXLComponent>(newEntity) = t5xxlComp;
        mgr.GetComponent<DiffusionModelComponent>(newEntity) = ckptComp;
        mgr.GetComponent<VaeComponent>(newEntity) = vaeComp;
        mgr.GetComponent<TaesdComponent>(newEntity) = taesdComp;
        mgr.GetComponent<ControlnetComponent>(newEntity) = controlComp;
        mgr.GetComponent<LoraComponent>(newEntity) = loraComp;
        mgr.GetComponent<LatentComponent>(newEntity) = latentComp;
        mgr.GetComponent<SamplerComponent>(newEntity) = samplerComp;
        mgr.GetComponent<CFGComponent>(newEntity) = cfgComp;
        mgr.GetComponent<PromptComponent>(newEntity) = promptComp;
        mgr.GetComponent<EmbeddingComponent>(newEntity) = embedComp;
        mgr.GetComponent<LayerSkipComponent>(newEntity) = layerSkipComp;
        mgr.GetComponent<ImageComponent>(newEntity) = imageComp;

        std::cout << "Components successfully assigned to entity " << newEntity << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "Error adding components: " << e.what() << std::endl;
        return;
    }

    // Queue event
    Event event;
    event.entityID = newEntity;
    event.type = EventType::InferenceRequest;
    ANI::Events::Ref().QueueEvent(event);
    std::cout << "Inference request queued for entity: " << newEntity << std::endl;
}

void DiffusionView::HandleUpscaleEvent() {
    Event event;
    EntityID newEntity = mgr.AddNewEntity();

    std::cout << "Initialized entity with ID: " << newEntity << "\n";

    event.entityID = newEntity;
    event.type = EventType::InferenceRequest;
    ANI::Events::Ref().QueueEvent(event);
}

void DiffusionView::RenderOther() {
    ImGui::Checkbox("Free Params", &samplerComp.free_params_immediately);
    ImGui::InputInt("# Threads (CPU Only)", &samplerComp.n_threads);
}

void DiffusionView::RenderControlnets() {
    if (ImGui::BeginTable("Controlnet", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Model", ImGuiTableColumnFlags_WidthFixed, 64.0f);
        ImGui::TableSetupColumn("Load", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        // ImGui::TableHeadersRow();
        ImGui::TableNextColumn();
        ImGui::Text("Controlnet");
        ImGui::TableNextColumn();
        if (ImGui::Button("...##5b")) {
            IGFD::FileDialogConfig config;
            config.path = filePaths.controlnetDir;
            ImGuiFileDialog::Instance()->OpenDialog("LoadFileDialog", "Choose Model", ".safetensors, .ckpt, .pt, .gguf",
                                                    config);
        }
        ImGui::SameLine();
        if (ImGui::Button("R##5c")) {
            controlComp.modelName = "";
            controlComp.modelPath = "";
        }
        ImGui::TableNextColumn();
        ImGui::Text("%s", controlComp.modelName.c_str());

        if (ImGuiFileDialog::Instance()->Display("LoadFileDialog", 32, ImVec2(700, 400))) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
                std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();

                controlComp.modelName = selectedFile;
                controlComp.modelPath = fullPath;
                std::cout << "Selected file: " << controlComp.modelName << std::endl;
                std::cout << "Full path: " << controlComp.modelPath << std::endl;
                std::cout << "New model path set: " << controlComp.modelPath << std::endl;
            }

            ImGuiFileDialog::Instance()->Close();
        }
        ImGui::EndTable();
    }

    if (ImGui::BeginTable("Control Settings", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Param", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        // ImGui::TableHeadersRow();
        ImGui::TableNextColumn();
        ImGui::Text("Strength");
        ImGui::TableNextColumn();
        ImGui::InputFloat("##Strength", &controlComp.cnStrength, 0.01f, 0.1f, "%.2f");
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Start");
        ImGui::TableNextColumn();
        ImGui::InputFloat("##Start", &controlComp.applyStart, 0.01f, 0.1f, "%.2f");
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("End");
        ImGui::TableNextColumn();
        ImGui::InputFloat("##End", &controlComp.applyEnd, 0.01f, 0.1f, "%.2f");
        ImGui::EndTable();
    }
}
void DiffusionView::RenderEmbeddings() {
    if (ImGui::BeginTable("Controlnet", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Model", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("Load", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        // ImGui::TableHeadersRow();
        ImGui::TableNextColumn();
        ImGui::Text("Embedding:");
        ImGui::TableNextColumn();
        if (ImGui::Button("...##v9")) {
            IGFD::FileDialogConfig config;
            config.path = filePaths.embedDir;
            ImGuiFileDialog::Instance()->OpenDialog("LoadFileDialog", "Choose Model", ".safetensors, .ckpt, .pt, .gguf",
                                                    config);
        }
        ImGui::SameLine();
        if (ImGui::Button("R##v0")) {
            embedComp.modelName = "";
            embedComp.modelPath = "";
        }
        ImGui::TableNextColumn();
        ImGui::Text("%s", embedComp.modelName.c_str());

        if (ImGuiFileDialog::Instance()->Display("LoadFileDialog", 32, ImVec2(700, 400))) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
                std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();

                embedComp.modelName = selectedFile;
                embedComp.modelPath = fullPath;
                std::cout << "Selected file: " << embedComp.modelName << std::endl;
                std::cout << "Full path: " << embedComp.modelPath << std::endl;
                std::cout << "New model path set: " << embedComp.modelPath << std::endl;
            }

            ImGuiFileDialog::Instance()->Close();
        }
        ImGui::EndTable();
    }
}

void DiffusionView::RenderDiffusionModelLoader() {

    ImGui::Combo("Quant Type", &int(samplerComp.current_type_method), type_method_items, type_method_item_count);

    if (ImGui::BeginTable("ModelLoaderTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("Model", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("Load", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Unet:"); // Row for Unet
        ImGui::TableNextColumn();
        if (ImGui::Button("...##n2")) {
            IGFD::FileDialogConfig config;
            config.path = filePaths.unetDir;
            ImGuiFileDialog::Instance()->OpenDialog("LoadUnetDialog", "Choose Model", ".safetensors,.ckpt,.pt,.gguf",
                                                    config);
        }
        ImGui::SameLine();
        if (ImGui::Button("R##n3")) {
            ckptComp.modelName = "";
            ckptComp.modelPath = "";
        }
        ImGui::TableNextColumn();
        ImGui::TextWrapped("%s", ckptComp.modelName.c_str());

        if (ImGuiFileDialog::Instance()->Display("LoadUnetDialog", 32, ImVec2(700, 400))) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
                std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();

                ckptComp.modelName = selectedFile;
                ckptComp.modelPath = fullPath;
                std::cout << "Selected file: " << ckptComp.modelName << std::endl;
                std::cout << "Full path: " << ckptComp.modelPath << std::endl;
            }
            ImGuiFileDialog::Instance()->Close();
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Clip L:"); // Row for Clip L
        ImGui::TableNextColumn();
        if (ImGui::Button("...##b7")) {
            IGFD::FileDialogConfig config;
            config.path = filePaths.encoderDir;
            ImGuiFileDialog::Instance()->OpenDialog("LoadClipLDialog", "Choose Model", ".safetensors,.ckpt,.pt,.gguf",
                                                    config);
        }
        ImGui::SameLine();
        if (ImGui::Button("R##n6")) {
            clipLComp.modelName = "";
            clipLComp.modelPath = "";
        }
        ImGui::TableNextColumn();
        ImGui::TextWrapped("%s", clipLComp.modelName.c_str());

        if (ImGuiFileDialog::Instance()->Display("LoadClipLDialog", 32, ImVec2(700, 400))) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
                std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();

                clipLComp.modelName = selectedFile;
                clipLComp.modelPath = fullPath;
                std::cout << "Selected file: " << clipLComp.modelName << std::endl;
                std::cout << "Full path: " << clipLComp.modelPath << std::endl;
            }
            ImGuiFileDialog::Instance()->Close();
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Clip G:"); // Row for Clip G
        ImGui::TableNextColumn();
        if (ImGui::Button("...##g7")) {
            IGFD::FileDialogConfig config;
            config.path = filePaths.encoderDir;
            ImGuiFileDialog::Instance()->OpenDialog("LoadClipGDialog", "Choose Model", ".safetensors,.ckpt,.pt,.gguf",
                                                    config);
        }
        ImGui::SameLine();
        if (ImGui::Button("R##n5")) {
            clipGComp.modelName = "";
            clipGComp.modelPath = "";
        }
        ImGui::TableNextColumn();
        ImGui::TextWrapped("%s", clipGComp.modelName.c_str());

        if (ImGuiFileDialog::Instance()->Display("LoadClipGDialog", 32, ImVec2(700, 400))) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
                std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();

                clipGComp.modelName = selectedFile;
                clipGComp.modelPath = fullPath;
                std::cout << "Selected file: " << clipGComp.modelName << std::endl;
                std::cout << "Full path: " << clipGComp.modelPath << std::endl;
            }
            ImGuiFileDialog::Instance()->Close();
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("T5XXL:"); // Row for T5XXL
        ImGui::TableNextColumn();
        if (ImGui::Button("...##x6")) {
            IGFD::FileDialogConfig config;
            config.path = filePaths.encoderDir;
            ImGuiFileDialog::Instance()->OpenDialog("LoadT5XXLDialog", "Choose Model", ".safetensors,.ckpt,.pt,.gguf",
                                                    config);
        }
        ImGui::SameLine();
        if (ImGui::Button("R##n4")) {
            t5xxlComp.modelName = "";
            t5xxlComp.modelPath = "";
        }
        ImGui::TableNextColumn();
        ImGui::TextWrapped("%s", t5xxlComp.modelName.c_str());

        if (ImGuiFileDialog::Instance()->Display("LoadT5XXLDialog", 32, ImVec2(700, 400))) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
                std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();

                t5xxlComp.modelName = selectedFile;
                t5xxlComp.modelPath = fullPath;
                std::cout << "Selected file: " << t5xxlComp.modelName << std::endl;
                std::cout << "Full path: " << t5xxlComp.modelPath << std::endl;
            }
            ImGuiFileDialog::Instance()->Close();
        }

        // Row for Vae
        RenderVaeLoader();
    }
}

void DiffusionView::RenderVaeLoader() {
    ImGui::TableNextColumn();
    ImGui::Text("Vae: ");
    ImGui::TableNextColumn();
    if (ImGui::Button("...##4b")) {
        IGFD::FileDialogConfig config;
        config.path = filePaths.vaeDir;
        ImGuiFileDialog::Instance()->OpenDialog("LoadVaeDialog", "Choose Model", ".safetensors, .ckpt, .pt, .gguf",
                                                config);
    }
    ImGui::SameLine();
    if (ImGui::Button("R##f7")) {
        vaeComp.modelName = "";
        vaeComp.modelPath = "";
    }
    ImGui::TableNextColumn();
    ImGui::Text("%s", vaeComp.modelName.c_str());

    if (ImGuiFileDialog::Instance()->Display("LoadVaeDialog", 32, ImVec2(700, 400))) {
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
    ImGui::EndTable();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    RenderVaeOptions();
}

void DiffusionView::RenderVaeOptions() {
    if (ImGui::BeginTable("VaeOptionsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("Param", ImGuiTableColumnFlags_WidthFixed, 64.0f); // Fixed width for Model
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextColumn();

        ImGui::Text("Tiled Vae");

        ImGui::TableNextColumn();

        ImGui::Checkbox("##TileVae", &vaeComp.isTiled);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        ImGui::Text("Keep Vae on CPU");
        ImGui::TableNextColumn();

        ImGui::Checkbox("##KeepVaeLoaded", &vaeComp.keep_vae_on_cpu);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        ImGui::Text("Vae Decode Only");
        ImGui::TableNextColumn();

        ImGui::Checkbox("##VaeDecodeOnly", &vaeComp.vae_decode_only);

        ImGui::EndTable();
    }
}

void DiffusionView::RenderQueueList() {

    ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Queue")) {

        // Get current progress values
        int currentStep = progressData.currentStep;
        int totalSteps = progressData.totalSteps;
        float time = progressData.currentTime;
        bool isProcessing = progressData.isProcessing;

        if (isProcessing && totalSteps > 0) {
            float progress = static_cast<float>(currentStep) / totalSteps;
            std::ostringstream ss;
            ss << "Processing: " << currentStep << "/" << totalSteps << " steps (" << std::fixed << std::setprecision(1)
               << time << "s)";
            ImGui::Text("%s", ss.str().c_str());
            ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 0));
        } else {
            ImGui::Text("Waiting...");
            ImGui::ProgressBar(0.0f, ImVec2(-FLT_MIN, 0));
        }
        ImGui::Separator();

            if (ImGui::Button("Queue", ImVec2(-FLT_MIN, 0))) {
                for (int i = 0; i < numQueues; i++) {
                    HandleT2IEvent();
                    // seedControl->activate();
                }
            }
            if (ImGui::InputInt("Queue #", &numQueues, 1, 4)) {
                if (numQueues < 1) {
                    numQueues = 1;
                }
            }
            if (ImGui::Button("Pause", ImVec2(-FLT_MIN, 0))) {
                Event event;
                event.type = EventType::PauseInference;
                ANI::Events::Ref().QueueEvent(event);
            }

            if (ImGui::Button("Resume", ImVec2(-FLT_MIN, 0))) {
                Event event;
                event.type = EventType::ResumeInference;
                ANI::Events::Ref().QueueEvent(event);
            }

            if (ImGui::Button("Stop Current", ImVec2(-FLT_MIN, 0))) {
                Event event;
                event.type = EventType::StopCurrentTask;
                ANI::Events::Ref().QueueEvent(event);
            }

            if (ImGui::Button("Clear Queue", ImVec2(-FLT_MIN, 0))) {
                Event event;
                event.type = EventType::ClearInferenceQueue;
                ANI::Events::Ref().QueueEvent(event);
            }
            ImGui::Separator();
        if (ImGui::BeginTable("InferenceQueue", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 42.0f);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 42.0f);
            ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthFixed, 48.0f);
            ImGui::TableSetupColumn("Move", ImGuiTableColumnFlags_WidthFixed, 42.0f);
            ImGui::TableSetupColumn("Prompt", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            auto &sdSystem = mgr.GetSystem<SDCPPSystem>();
            if (sdSystem) {
                auto queueItems = sdSystem->GetQueueSnapshot();
                for (size_t i = 0; i < queueItems.size(); i++) {
                    const auto &item = queueItems[i];
                    auto &prompt = mgr.GetComponent<PromptComponent>(item.entityID).posPrompt;

                    ImGui::TableNextRow();

                    // ID column
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", static_cast<int>(item.entityID));

                    // Status column
                    ImGui::TableNextColumn();
                    if (item.processing) {
                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Active");
                    } else {
                        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "Queued");
                    }

                    // Controls column
                    ImGui::TableNextColumn();
                    if (!item.processing) {
                        if (ImGui::Button(("Remove##" + std::to_string(i)).c_str())) {
                            sdSystem->RemoveFromQueue(i);
                        }
                    }

                    // Move column
                    ImGui::TableNextColumn();
                    if (!item.processing) {
                        if (i > 0) {
                            if (ImGui::ArrowButton(("up##" + std::to_string(i)).c_str(), ImGuiDir_Up)) {
                                sdSystem->MoveInQueue(i, i - 1);
                            }
                            if (i < queueItems.size() - 1) {
                                ImGui::SameLine();
                            }
                        }
                        if (i < queueItems.size() - 1) {
                            if (ImGui::ArrowButton(("down##" + std::to_string(i)).c_str(), ImGuiDir_Down)) {
                                sdSystem->MoveInQueue(i, i + 1);
                            }
                        }
                    }

                    // Prompt column
                    ImGui::TableNextColumn();
                    if (prompt.length() > 50) {
                        ImGui::Text("%s...", prompt.substr(0, 47).c_str());
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("%s", prompt.c_str());
                        }
                    } else {
                        ImGui::Text("%s", prompt.c_str());
                    }
                }
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void DiffusionView::Render() {

    // Queue Controls (detached from the options)
    RenderQueueList();

    ImGui::SetNextWindowSize(ImVec2(300, 800), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Image Generation")) {

        if (ImGui::CollapsingHeader("Metadata Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
            RenderMetadataControls();
        }
        if (ImGui::BeginTabBar("Image")) {
            if (ImGui::BeginTabItem("Txt2Img")) {
                // Output FileName
                if (ImGui::CollapsingHeader("Output Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                    RenderFilePath();
                }
                // Checkpoint loader
                if (ImGui::CollapsingHeader("Model Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (ImGui::BeginTabBar("Model Loader")) {
                        if (ImGui::BeginTabItem("Full")) {
                            RenderModelLoader();
                            ImGui::EndTabItem();
                        }
                        // Separated Model loader
                        if (ImGui::BeginTabItem("Separate")) {
                            RenderDiffusionModelLoader();
                            ImGui::EndTabItem();
                        }
                    }
                    ImGui::EndTabBar();
                }
                // Latent Params
                if (ImGui::CollapsingHeader("Latent Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                    RenderLatents();
                }
                // Prompt Inputs
                if (ImGui::CollapsingHeader("Prompt Inputs", ImGuiTreeNodeFlags_DefaultOpen)) {
                    RenderPrompts();
                }
                // Sampler Inputs
                if (ImGui::CollapsingHeader("Sampler Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                    RenderSampler();
                }
                // Controlnet Loader and Params
                if (ImGui::CollapsingHeader("ControlNet")) {
                    RenderControlnets();
                }
                // Embedding Inputs
                if (ImGui::CollapsingHeader("Embeddings")) {
                    RenderEmbeddings();
                }
                // Other Settings
                if (ImGui::CollapsingHeader("Other Settings")) {
                    RenderOther();
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Img2Img")) {

                // Output FileName
                if (ImGui::CollapsingHeader("Output Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                    RenderFilePath();
                }
                // Checkpoint loader
                if (ImGui::CollapsingHeader("Model Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (ImGui::BeginTabBar("Model Loader")) {
                        if (ImGui::BeginTabItem("Full")) {
                            RenderModelLoader();
                            ImGui::EndTabItem();
                        }
                        // Separated Model loader
                        if (ImGui::BeginTabItem("Separate")) {
                            RenderDiffusionModelLoader();
                            ImGui::EndTabItem();
                        }
                    }
                    ImGui::EndTabBar();
                }
                // Latent Params
                if (ImGui::CollapsingHeader("Latent Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                    RenderLatents();
                }
                // Prompt Inputs
                if (ImGui::CollapsingHeader("Prompt Inputs", ImGuiTreeNodeFlags_DefaultOpen)) {
                    RenderPrompts();
                }
                // Sampler Inputs
                if (ImGui::CollapsingHeader("Sampler Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                    RenderSampler();
                }
                // Controlnet Loader and Params
                if (ImGui::CollapsingHeader("ControlNet")) {
                    RenderControlnets();
                }
                // Embedding Inputs
                if (ImGui::CollapsingHeader("Embeddings")) {
                    RenderEmbeddings();
                }
                // Other Settings
                if (ImGui::CollapsingHeader("Other Settings")) {
                    RenderOther();
                }
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

nlohmann::json DiffusionView::Serialize() const {
    nlohmann::json j = BaseView::Serialize();
    j["modelComp"] = modelComp.Serialize();
    j["clipLComp"] = clipLComp.Serialize();
    j["clipGComp"] = clipGComp.Serialize();
    j["t5xxlComp"] = t5xxlComp.Serialize();
    j["ckptComp"] = ckptComp.Serialize();
    j["latentComp"] = latentComp.Serialize();
    j["loraComp"] = loraComp.Serialize();
    j["promptComp"] = promptComp.Serialize();
    j["samplerComp"] = samplerComp.Serialize();
    j["cfgComp"] = cfgComp.Serialize();
    j["vaeComp"] = vaeComp.Serialize();
    j["imageComp"] = imageComp.Serialize();
    j["embedComp"] = embedComp.Serialize();
    j["controlComp"] = controlComp.Serialize();
    j["layerSkipComp"] = layerSkipComp.Serialize();
    return j;
}

void DiffusionView::Deserialize(const nlohmann::json &j) {
    BaseView::Deserialize(j);

    // Try new format first (components object)
    if (j.contains("components")) {
        auto &components = j["components"];
        if (components.contains("ModelComponent"))
            modelComp.Deserialize(components["ModelComponent"]);
        if (components.contains("CLipLComponent"))
            clipLComp.Deserialize(components["CLipLComponent"]);
        if (components.contains("CLipGComponent"))
            clipGComp.Deserialize(components["CLipGComponent"]);
        if (components.contains("T5XXLComponent"))
            t5xxlComp.Deserialize(components["T5XXLComponent"]);
        if (components.contains("DiffusionModelComponent"))
            ckptComp.Deserialize(components["DiffusionModelComponent"]);
        if (components.contains("LatentComponent"))
            latentComp.Deserialize(components["LatentComponent"]);
        if (components.contains("LoraComponent"))
            loraComp.Deserialize(components["LoraComponent"]);
        if (components.contains("PromptComponent"))
            promptComp.Deserialize(components["PromptComponent"]);
        if (components.contains("SamplerComponent"))
            samplerComp.Deserialize(components["SamplerComponent"]);
        if (components.contains("CFGComponent"))
            cfgComp.Deserialize(components["CFGComponent"]);
        if (components.contains("VaeComponent"))
            vaeComp.Deserialize(components["VaeComponent"]);
        if (components.contains("EmbeddingComponent"))
            embedComp.Deserialize(components["EmbeddingComponent"]);
        if (components.contains("ControlnetComponent"))
            controlComp.Deserialize(components["ControlnetComponent"]);
        if (components.contains("LayerSkipComponent"))
            layerSkipComp.Deserialize(components["LayerSkipComponent"]);
        if (components.contains("ImageComponent"))
            imageComp.Deserialize(components["ImageComponent"]);
    }
    // Fall back to old format for backward compatibility
    else {
        if (j.contains("modelComp"))
            modelComp.Deserialize(j["modelComp"]);
        if (j.contains("clipLComp"))
            clipLComp.Deserialize(j["clipLComp"]);
        if (j.contains("clipGComp"))
            clipGComp.Deserialize(j["clipGComp"]);
        if (j.contains("t5xxlComp"))
            t5xxlComp.Deserialize(j["t5xxlComp"]);
        if (j.contains("ckptComp"))
            ckptComp.Deserialize(j["ckptComp"]);
        if (j.contains("latentComp"))
            latentComp.Deserialize(j["latentComp"]);
        if (j.contains("loraComp"))
            loraComp.Deserialize(j["loraComp"]);
        if (j.contains("promptComp"))
            promptComp.Deserialize(j["promptComp"]);
        if (j.contains("samplerComp"))
            samplerComp.Deserialize(j["samplerComp"]);
        if (j.contains("cfgComp"))
            cfgComp.Deserialize(j["cfgComp"]);
        if (j.contains("vaeComp"))
            vaeComp.Deserialize(j["vaeComp"]);
        if (j.contains("imageComp"))
            imageComp.Deserialize(j["imageComp"]);
        if (j.contains("embedComp"))
            embedComp.Deserialize(j["embedComp"]);
        if (j.contains("controlComp"))
            controlComp.Deserialize(j["controlComp"]);
        if (j.contains("layerSkipComp"))
            layerSkipComp.Deserialize(j["layerSkipComp"]);
    }

    // Update prompt buffers after deserialization
    if (!promptComp.posPrompt.empty()) {
        strncpy(promptComp.PosBuffer, promptComp.posPrompt.c_str(), sizeof(promptComp.PosBuffer) - 1);
        promptComp.PosBuffer[sizeof(promptComp.PosBuffer) - 1] = '\0';
    }
    if (!promptComp.negPrompt.empty()) {
        strncpy(promptComp.NegBuffer, promptComp.negPrompt.c_str(), sizeof(promptComp.NegBuffer) - 1);
        promptComp.NegBuffer[sizeof(promptComp.NegBuffer) - 1] = '\0';
    }
}

void DiffusionView::SaveMetadataToJson(const std::string &filepath) {
    try {
        nlohmann::json metadata = Serialize();
        std::ofstream file(filepath);
        if (file.is_open()) {
            file << metadata.dump(4);
            file.close();
            std::cout << "Metadata saved to: " << filepath << std::endl;
        } else {
            std::cerr << "Failed to open file for writing: " << filepath << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << "Error saving metadata: " << e.what() << std::endl;
    }
}

void DiffusionView::LoadMetadataFromJson(const std::string &filepath) {
    try {
        std::ifstream file(filepath);
        if (file.is_open()) {
            nlohmann::json metadata;
            file >> metadata;
            Deserialize(metadata);
            file.close();
            std::cout << "Metadata loaded from: " << filepath << std::endl;
        } else {
            std::cerr << "Failed to open file for reading: " << filepath << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << "Error loading metadata: " << e.what() << std::endl;
    }
}

void DiffusionView::LoadMetadataFromPNG(const std::string &imagePath) {
    FILE *fp = fopen(imagePath.c_str(), "rb");
    if (!fp) {
        std::cerr << "Failed to open PNG file: " << imagePath << std::endl;
        return;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        fclose(fp);
        return;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, nullptr, nullptr);
        fclose(fp);
        return;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        return;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    // Get text chunks
    png_textp text_ptr;
    int num_text;
    if (png_get_text(png, info, &text_ptr, &num_text) > 0) {
        for (int i = 0; i < num_text; i++) {
            if (strcmp(text_ptr[i].key, "parameters") == 0) {
                try {
                    nlohmann::json metadata = nlohmann::json::parse(text_ptr[i].text);

                    // Extract components from metadata
                    if (metadata.contains("components")) {
                        auto &components = metadata["components"];

                        if (components.contains("ModelComponent"))
                            modelComp.Deserialize(components["ModelComponent"]);
                        if (components.contains("CLipLComponent"))
                            clipLComp.Deserialize(components["CLipLComponent"]);
                        if (components.contains("CLipGComponent"))
                            clipGComp.Deserialize(components["CLipGComponent"]);
                        if (components.contains("T5XXLComponent"))
                            t5xxlComp.Deserialize(components["T5XXLComponent"]);
                        if (components.contains("DiffusionModelComponent"))
                            ckptComp.Deserialize(components["DiffusionModelComponent"]);
                        if (components.contains("LatentComponent"))
                            latentComp.Deserialize(components["LatentComponent"]);
                        if (components.contains("LoraComponent"))
                            loraComp.Deserialize(components["LoraComponent"]);
                        if (components.contains("PromptComponent"))
                            promptComp.Deserialize(components["PromptComponent"]);
                        if (components.contains("SamplerComponent"))
                            samplerComp.Deserialize(components["SamplerComponent"]);
                        if (components.contains("CFGComponent"))
                            cfgComp.Deserialize(components["CFGComponent"]);
                        if (components.contains("VaeComponent"))
                            vaeComp.Deserialize(components["VaeComponent"]);
                        if (components.contains("EmbeddingComponent"))
                            embedComp.Deserialize(components["EmbeddingComponent"]);
                        if (components.contains("ControlnetComponent"))
                            controlComp.Deserialize(components["ControlnetComponent"]);
                        if (components.contains("LayerSkipComponent"))
                            layerSkipComp.Deserialize(components["LayerSkipComponent"]);
                        if (components.contains("ImageComponent"))
                            imageComp.Deserialize(components["ImageComponent"]);

                        std::cout << "Successfully loaded metadata from PNG" << std::endl;
                    }
                } catch (const std::exception &e) {
                    std::cerr << "Error parsing PNG metadata: " << e.what() << std::endl;
                }
                break;
            }
        }
    }

    png_destroy_read_struct(&png, &info, nullptr);
    fclose(fp);
}

void DiffusionView::LoadMetadataFromExif(const std::string &imagePath) {
    try {
        auto image = Exiv2::ImageFactory::open(imagePath);
        if (image.get() != nullptr) {
            image->readMetadata();
            Exiv2::ExifData &exifData = image->exifData();

            // Look for our metadata in the UserComment tag
            auto it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.UserComment"));
            if (it != exifData.end()) {
                std::string jsonStr = it->toString();
                if (!jsonStr.empty()) {
                    nlohmann::json metadata = nlohmann::json::parse(jsonStr);
                    Deserialize(metadata);
                    std::cout << "Metadata loaded from image EXIF: " << imagePath << std::endl;
                }
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Error loading EXIF metadata: " << e.what() << std::endl;
    }
}

void DiffusionView::RenderMetadataControls() {

    if (ImGui::Button("Save Metadata", ImVec2(-FLT_MIN, 0))) {
        IGFD::FileDialogConfig config;
        config.path = filePaths.defaultProjectPath;
        ImGuiFileDialog::Instance()->OpenDialog("SaveMetadataDialog", "Save Metadata", ".json", config);
    }

    if (ImGui::Button("Load Metadata", ImVec2(-FLT_MIN, 0))) {
        IGFD::FileDialogConfig config;
        config.path = filePaths.defaultProjectPath;
        ImGuiFileDialog::Instance()->OpenDialog("LoadMetadataDialog", "Load Metadata", ".json,.png,.jpg,.jpeg", config);
    }

    // Handle Save Dialog
    if (ImGuiFileDialog::Instance()->Display("SaveMetadataDialog", 32, ImVec2(700, 400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filepath = ImGuiFileDialog::Instance()->GetFilePathName();
            SaveMetadataToJson(filepath);
        }
        ImGuiFileDialog::Instance()->Close();
    }

    // Handle Load Dialog
    if (ImGuiFileDialog::Instance()->Display("LoadMetadataDialog", 32, ImVec2(700, 400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filepath = ImGuiFileDialog::Instance()->GetFilePathName();
            std::string extension = std::filesystem::path(filepath).extension().string();

            if (extension == ".json") {
                LoadMetadataFromJson(filepath);
            } else if (extension == ".png" || extension == ".jpg" || extension == ".jpeg") {
                LoadMetadataFromExif(filepath);
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
}
} // namespace GUI