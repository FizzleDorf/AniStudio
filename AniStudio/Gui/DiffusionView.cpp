#include "DiffusionView.hpp"
#include "ImageView.hpp"
#include "../Events/Events.hpp"
#include "ECS.h"
#include "stable-diffusion.h"

using namespace ECS;
using namespace ANI;

ImageView imageView;
EntityID currentEntity;

void DiffusionView::RenderModelLoader() {
    ImGui::Text("Checkpoint:");
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

    ImGui::InputInt("# Threads (CPU Only)", &samplerComp.n_threads);
}

void DiffusionView::RenderFilePath() {
    static char fileName[256] = "";  // Buffer to hold the file name
    static char outputDir[256] = ""; // Buffer to hold the output directory

    // Editable input for the file name
    if (ImGui::InputText("Filename", fileName, IM_ARRAYSIZE(fileName))) {
        isFilenameChanged = true;
    }

    ImGui::SameLine();

    // Button to open the file dialog for choosing the output directory
    if (ImGui::Button("Choose Directory")) {
        IGFD::FileDialogConfig config;
        config.path = filePaths.defaultProjectPath; // Set the initial directory
        ImGuiFileDialog::Instance()->OpenDialog("LoadDirDialog", "Choose Directory", nullptr, config);
    }

    // Handle the file dialog display and selection
    if (ImGuiFileDialog::Instance()->Display("LoadDirDialog", 32, ImVec2(700, 400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            // Update the output directory from the selected path
            std::string selectedDir = ImGuiFileDialog::Instance()->GetCurrentPath();
            strncpy(outputDir, selectedDir.c_str(), IM_ARRAYSIZE(outputDir) - 1);
            outputDir[IM_ARRAYSIZE(outputDir) - 1] = '\0'; // Ensure null termination

            filePaths.checkpointDir = selectedDir; // Update the stored directory
            isFilenameChanged = true;              // Trigger file path update
        }
        ImGuiFileDialog::Instance()->Close();
    }

    // Editable display of the output directory
    if (ImGui::InputText("Output Directory", outputDir, IM_ARRAYSIZE(outputDir))) {
        filePaths.checkpointDir = outputDir; // Update the directory in filePaths
        isFilenameChanged = true;            // Trigger file path update
    }

    // Update ImageComponent properties if filename or filepath changes
    if (isFilenameChanged) {
        std::string newFileName(fileName);
        std::string outputDirectory(outputDir);

        // Ensure the file name has a .png extension
        if (!newFileName.empty()) {
            std::filesystem::path filePath(newFileName);
            if (filePath.extension() != ".png") {
                filePath.replace_extension(".png");
                newFileName = filePath.filename().string();
            }
        }

        // Construct the full file path if both directory and filename are valid
        if (!outputDirectory.empty() && !newFileName.empty()) {
            std::filesystem::path fullPath = std::filesystem::path(outputDirectory) / newFileName;

            imageComp.fileName = newFileName;
            imageComp.filePath = fullPath.string();

            std::cout << "ImageComponent updated:" << std::endl;
            std::cout << "  FileName: " << imageComp.fileName << std::endl;
            std::cout << "  FilePath: " << imageComp.filePath << std::endl;
        } else {
            std::cerr << "Invalid directory or filename!" << std::endl;
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
    if (ImGui::InputTextMultiline("Positive Prompt", promptComp.PosBuffer, sizeof(promptComp.PosBuffer))) {
        promptComp.posPrompt = promptComp.PosBuffer;
    }
    if (ImGui::InputTextMultiline("Negative Prompt", promptComp.NegBuffer, sizeof(promptComp.NegBuffer))) {
        promptComp.negPrompt = promptComp.NegBuffer;
    }
}

void DiffusionView::RenderSampler() {
    ImGui::Combo("Sampler", &int(samplerComp.current_sample_method), sample_method_items, sample_method_item_count);
    ImGui::Combo("Scheduler", &int(samplerComp.current_scheduler_method), scheduler_method_items,
                 scheduler_method_item_count);
    ImGui::InputInt("Seed", &samplerComp.seed);
    ImGui::InputFloat("CFG", &cfgComp.cfg);
    ImGui::InputFloat("Guidance", &cfgComp.guidance);
    ImGui::InputInt("Steps", &samplerComp.steps);
    ImGui::InputFloat("Denoise", &samplerComp.denoise, 0.01f, 0.1f, "%.2f");
}

void DiffusionView::HandleT2IEvent() {
    mgr.RegisterSystem<SDCPPSystem>();
    Event event;
    EntityID newEntity = mgr.AddNewEntity();
    currentEntity = newEntity;
    std::cout << "Initialized entity with ID: " << newEntity << "\n";

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

    std::cout << "Assigning components to new Entity: " << newEntity << "\n";
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

    std::cout << "ModelComponent.modelPath: " << mgr.GetComponent<ModelComponent>(newEntity).modelPath << "\n";
    std::cout << "CLipLComponent.encoderPath: " << mgr.GetComponent<CLipLComponent>(newEntity).modelPath << "\n";
    std::cout << "CLipGComponent.encoderPath: " << mgr.GetComponent<CLipGComponent>(newEntity).modelPath << "\n";
    std::cout << "T5XXLComponent.encoderPath: " << mgr.GetComponent<T5XXLComponent>(newEntity).modelPath << "\n";
    std::cout << "DiffusionModelComponent.ckptPath: " << mgr.GetComponent<DiffusionModelComponent>(newEntity).modelPath
              << "\n";
    std::cout << "VaeComponent.vaePath: " << mgr.GetComponent<VaeComponent>(newEntity).modelPath << "\n";
    std::cout << "SamplerComponent.steps: " << mgr.GetComponent<SamplerComponent>(newEntity).steps << "\n";
    std::cout << "SamplerComponent.denoise: " << mgr.GetComponent<SamplerComponent>(newEntity).denoise << "\n";
    std::cout << "SamplerComponent.current_sample_method: "
              << mgr.GetComponent<SamplerComponent>(newEntity).current_sample_method << "\n";
    std::cout << "SamplerComponent.current_scheduler_method: "
              << mgr.GetComponent<SamplerComponent>(newEntity).current_scheduler_method << "\n";
    std::cout << "SamplerComponent.current_type_method: "
              << mgr.GetComponent<SamplerComponent>(newEntity).current_type_method << "\n";
    std::cout << "CFGComponent.cfg: " << mgr.GetComponent<CFGComponent>(newEntity).cfg << "\n";
    std::cout << "PromptComponent.posPrompt: " << mgr.GetComponent<PromptComponent>(newEntity).posPrompt << "\n";
    std::cout << "PromptComponent.negPrompt: " << mgr.GetComponent<PromptComponent>(newEntity).negPrompt << "\n";
    std::cout << "LatentComponent.latentWidth: " << mgr.GetComponent<LatentComponent>(newEntity).latentWidth << "\n";
    std::cout << "LatentComponent.latentHeight: " << mgr.GetComponent<LatentComponent>(newEntity).latentHeight << "\n";

    event.entityID = newEntity;
    event.type = EventType::InferenceRequest;
    ANI::Events::Ref().QueueEvent(event);
}

void DiffusionView::HandleUpscaleEvent() {
    Event event;
    EntityID newEntity = mgr.AddNewEntity();

    std::cout << "Initialized entity with ID: " << newEntity << "\n";

    event.entityID = newEntity;
    event.type = EventType::InferenceRequest;
    ANI::Events::Ref().QueueEvent(event);
}

void DiffusionView::RenderQueue() {
    if (ImGui::Button("Queue")) {
        HandleT2IEvent();
    }

    ImGui::Checkbox("Free Parameters Immediately", &samplerComp.free_params_immediately);
    ImGui::Combo("Quant Type", &int(samplerComp.current_type_method), type_method_items, type_method_item_count);
    ImGui::Combo("RNG Type", &int(samplerComp.current_rng_type), type_rng_items, type_rng_item_count);
}

void DiffusionView::RenderControlnets() {
    ImGui::Text("Controlnet:");
    ImGui::SameLine();
    ImGui::Text("%s", controlComp.modelName.c_str());

    if (ImGui::Button("...##5b")) {
        IGFD::FileDialogConfig config;
        config.path = filePaths.controlnetDir;
        ImGuiFileDialog::Instance()->OpenDialog("LoadFileDialog", "Choose Model", ".safetensors, .ckpt, .pt, .gguf",
                                                config);
    }

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
    ImGui::InputFloat("Strength", &controlComp.cnStrength, 0.01f, 0.1f, "%.2f");
    ImGui::InputFloat("Start", &controlComp.applyStart, 0.01f, 0.1f, "%.2f");
    ImGui::InputFloat("End", &controlComp.applyEnd, 0.01f, 0.1f, "%.2f");
}
void DiffusionView::RenderEmbeddings() {
    ImGui::Text("Embedding:");
    ImGui::SameLine();
    ImGui::Text("%s", embedComp.modelName.c_str());

    if (ImGui::Button("...##v9")) {
        IGFD::FileDialogConfig config;
        config.path = filePaths.embedDir;
        ImGuiFileDialog::Instance()->OpenDialog("LoadFileDialog", "Choose Model", ".safetensors, .ckpt, .pt, .gguf",
                                                config);
    }

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
}
void DiffusionView::RenderDiffusionModelLoader() {
    ImGui::Text("Unet: ");
    ImGui::SameLine();
    ImGui::Text("%s", ckptComp.modelName.c_str());

    if (ImGui::Button("...##n2")) {
        IGFD::FileDialogConfig config;
        config.path = filePaths.unetDir;
        ImGuiFileDialog::Instance()->OpenDialog("LoadUnetDialog", "Choose Model", ".safetensors, .ckpt, .pt, .gguf",
                                                config);
    }

    if (ImGuiFileDialog::Instance()->Display("LoadUnetDialog", 32, ImVec2(700, 400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
            std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();

            ckptComp.modelName = selectedFile;
            ckptComp.modelPath = fullPath;
            std::cout << "Selected file: " << ckptComp.modelName << std::endl;
            std::cout << "Full path: " << ckptComp.modelPath << std::endl;
            std::cout << "New model path set: " << ckptComp.modelPath << std::endl;
        }

        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::Text("Clip L: ");
    ImGui::SameLine();
    ImGui::Text("%s", clipLComp.modelName.c_str());

    if (ImGui::Button("...##b7")) {
        IGFD::FileDialogConfig config;
        config.path = filePaths.encoderDir;
        ImGuiFileDialog::Instance()->OpenDialog("LoadClipLDialog", "Choose Model", ".safetensors, .ckpt, .pt, .gguf",
                                                config);
    }

    if (ImGuiFileDialog::Instance()->Display("LoadClipLDialog", 32, ImVec2(700, 400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
            std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();

            clipLComp.modelName = selectedFile;
            clipLComp.modelPath = fullPath;
            std::cout << "Selected file: " << clipLComp.modelName << std::endl;
            std::cout << "Full path: " << clipLComp.modelPath << std::endl;
            std::cout << "New model path set: " << clipLComp.modelPath << std::endl;
        }

        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::Text("Clip G: ");
    ImGui::SameLine();
    ImGui::Text("%s", clipGComp.modelName.c_str());

    if (ImGui::Button("...##g7")) {
        IGFD::FileDialogConfig config;
        config.path = filePaths.encoderDir;
        ImGuiFileDialog::Instance()->OpenDialog("LoadClipGDialog", "Choose Model", ".safetensors, .ckpt, .pt, .gguf",
                                                config);
    }

    if (ImGuiFileDialog::Instance()->Display("LoadClipGDialog", 32, ImVec2(700, 400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
            std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();

            clipGComp.modelName = selectedFile;
            clipGComp.modelPath = fullPath;
            std::cout << "Selected file: " << clipGComp.modelName << std::endl;
            std::cout << "Full path: " << clipGComp.modelPath << std::endl;
            std::cout << "New model path set: " << clipGComp.modelPath << std::endl;
        }

        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::Text("T5XXL: ");
    ImGui::SameLine();
    ImGui::Text("%s", t5xxlComp.modelName.c_str());

    if (ImGui::Button("...##x6")) {
        IGFD::FileDialogConfig config;
        config.path = filePaths.encoderDir;
        ImGuiFileDialog::Instance()->OpenDialog("LoadT5XXLDialog", "Choose Model", ".safetensors, .ckpt, .pt, .gguf",
                                                config);
    }

    if (ImGuiFileDialog::Instance()->Display("LoadT5XXLDialog", 32, ImVec2(700, 400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
            std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();

            t5xxlComp.modelName = selectedFile;
            t5xxlComp.modelPath = fullPath;
            std::cout << "Selected file: " << t5xxlComp.modelName << std::endl;
            std::cout << "Full path: " << t5xxlComp.modelPath << std::endl;
            std::cout << "New model path set: " << t5xxlComp.modelPath << std::endl;
        }

        ImGuiFileDialog::Instance()->Close();
    }
}
void DiffusionView::RenderVaeLoader() {
    ImGui::Text("Vae: ");
    ImGui::SameLine();
    ImGui::Text("%s", vaeComp.modelName.c_str());

    if (ImGui::Button("...##4b")) {
        IGFD::FileDialogConfig config;
        config.path = filePaths.vaeDir;
        ImGuiFileDialog::Instance()->OpenDialog("LoadVaeDialog", "Choose Model", ".safetensors, .ckpt, .pt, .gguf",
                                                config);
    }

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

void DiffusionView::Render() {
    ImGui::SetNextWindowSize(ImVec2(300, 800), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Image Generation")) {
        if (ImGui::BeginTabBar("Image")) {
            if (ImGui::BeginTabItem("Txt2Img")) {
                RenderFilePath();
                RenderQueue();
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

            if (ImGui::BeginTabItem("HighResFix")) {
                RenderFilePath();
                RenderQueue();
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

            if (ImGui::BeginTabItem("Img2Img")) {
                RenderFilePath();
                RenderQueue();
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

            if (ImGui::BeginTabItem("Upscale")) {
                RenderFilePath();
                RenderQueue();
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
    imageView.Render();
}
