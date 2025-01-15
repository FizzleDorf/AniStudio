#include "DiffusionView.hpp"
#include "../Events/Events.hpp"


using namespace ECS;
using namespace ANI;

namespace GUI {

void DiffusionView::RenderModelLoader() {
    if (ImGui::BeginTable("ModelLoaderTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Model", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("Load", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

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

        ImGui::EndTable();
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
        ImGui::TableHeadersRow();
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
        ImGui::TableHeadersRow();
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
    ImGui::InputInt("Width", &latentComp.latentWidth);
    ImGui::InputInt("Height", &latentComp.latentHeight);
    ImGui::InputInt("Batch Size", &latentComp.batchSize);
}

void DiffusionView::RenderInputImage() {
    ImGui::Text("Image input placeholder"); // Adjust to render the actual image component if needed
}

void DiffusionView::RenderPrompts() {
    if (ImGui::BeginTable("SamplerSettingsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Param", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Positive");
        ImGui::TableNextColumn();
        if (ImGui::InputTextMultiline("##Positive Prompt", promptComp.PosBuffer, sizeof(promptComp.PosBuffer))) {
            promptComp.posPrompt = promptComp.PosBuffer;
        }
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Negative");
        ImGui::TableNextColumn();
        if (ImGui::InputTextMultiline("##Negative Prompt", promptComp.NegBuffer, sizeof(promptComp.NegBuffer))) {
            promptComp.negPrompt = promptComp.NegBuffer;
        }
        ImGui::EndTable();
    }
}

void DiffusionView::RenderSampler() {
    if (ImGui::BeginTable("SamplerSettingsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Param", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

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
        addRow("CFG", [&]() { ImGui::InputFloat("##CFG", &cfgComp.cfg); });
        addRow("Guidance", [&]() { ImGui::InputFloat("##Guidance", &cfgComp.guidance); });
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

    // Log component values for debugging
    std::cout << "ModelComponent.modelPath: " << mgr.GetComponent<ModelComponent>(newEntity).modelPath << std::endl;
    std::cout << "PromptComponent.posPrompt: " << mgr.GetComponent<PromptComponent>(newEntity).posPrompt << std::endl;
    std::cout << "LatentComponent.latentWidth: " << mgr.GetComponent<LatentComponent>(newEntity).latentWidth
              << std::endl;

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
    ImGui::Combo("Quant Type", &int(samplerComp.current_type_method), type_method_items, type_method_item_count);
    ImGui::Combo("RNG Type", &int(samplerComp.current_rng_type), type_rng_items, type_rng_item_count);
}

void DiffusionView::RenderControlnets() {
    if (ImGui::BeginTable("Controlnet", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Model", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("Load", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
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
        ImGui::TableHeadersRow();
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
        ImGui::TableHeadersRow();
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

    if (ImGui::BeginTable("ModelLoaderTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("Model", ImGuiTableColumnFlags_WidthFixed, 52.0f); // Fixed width for Model
        ImGui::TableSetupColumn("Load", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
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

        ImGui::EndTable();
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
}

void DiffusionView::RenderQueueList() {
    if (ImGui::BeginTable("QueueTable", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (ImGui::Button("Queue")) {
            for (int i = 0; i < numQueues; i++) {
                HandleT2IEvent();
                // seedControl->activate();
            }
        }
        if (ImGui::Button("Stop Current")) {
        }
        if (ImGui::Button("Clear")) {
        }
        if (ImGui::InputInt("Queue #", &numQueues, 1, 4)) {
            if (numQueues < 1) {
                numQueues = 1;
            }
        }
        ImGui::EndTable();
    }

    if (ImGui::BeginTable("InferenceQueue", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 42.0f);
        ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthFixed, 42.0f);
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

void DiffusionView::Render() {
    ImGui::SetNextWindowSize(ImVec2(300, 800), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Image Generation")) {
        if (ImGui::BeginTabBar("Image")) {
            if (ImGui::BeginTabItem("Txt2Img")) {
                // Queue Controls
                RenderQueueList();
                RenderOther();
                // Output FileName
                RenderFilePath();
                // Checkpoint loader
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
                // Latent Params
                RenderLatents();
                // Prompt Inputs
                RenderPrompts();
                // Sampler Inputs
                RenderSampler();
                // Controlnet Loader and Params
                RenderControlnets();
                // Embedding Inputs
                RenderEmbeddings();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Img2Img")) {
                RenderFilePath();
                RenderOther();
                RenderInputImage();
                RenderModelLoader();
                RenderDiffusionModelLoader();
                RenderVaeLoader();
                RenderLatents();
                RenderPrompts();
                RenderSampler();
                RenderControlnets();
                RenderEmbeddings();
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
}

void DiffusionView::Deserialize(const nlohmann::json &j) {
    BaseView::Deserialize(j);
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

} // namespace GUI