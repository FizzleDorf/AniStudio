#include "DiffusionView.hpp"
#include "../Events/Events.hpp"
#include "ECS.h"
#include "ImageView.hpp"
#include "stable-diffusion.h"
#include <filesystem>

using namespace ECS;
using namespace ANI;

ImageView imageView;
EntityID currentEntity;

void DiffusionView::RenderModelLoader() {
    if (ImGui::BeginTable("ModelLoaderTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        // Row for "Checkpoint"
        ImGui::TableNextColumn();
        ImGui::Text("Checkpoint:");
        ImGui::TableNextColumn();
        ImGui::Text("%s", modelComp.modelName.c_str());
        ImGui::TableNextColumn();
        if (ImGui::Button("...##j6")) {
            IGFD::FileDialogConfig config;
            config.path = filePaths.checkpointDir;
            ImGuiFileDialog::Instance()->OpenDialog("LoadFileDialog", "Choose Model", ".safetensors, .ckpt, .pt, .gguf",
                                                    config);
        }
        if (ImGui::Button("R##j6")) {
            modelComp.modelName = "";
            modelComp.modelPath = "";
        }
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

    ImGui::InputInt("# Threads (CPU Only)", &samplerComp.n_threads);
}

static char fileName[256] = "AniStudio"; // Buffer for file name
static char outputDir[256] = "";         // Buffer for output directory

void DiffusionView::RenderFilePath() {
    if (ImGui::BeginTable("Output", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        // Row for "Filename"
        ImGui::TableNextColumn();
        ImGui::Text("FileName:");
        ImGui::TableNextColumn();
        if (ImGui::InputText("##Filename", fileName, IM_ARRAYSIZE(fileName))) {
            isFilenameChanged = true;
        }

        // Row for "Output Directory"
        ImGui::TableNextColumn();
        ImGui::Text("Directory:");
        ImGui::TableNextColumn();
        ImGui::Text("%s", outputDir);
        ImGui::TableNextColumn();
        if (ImGui::Button("...##w8")) {
            IGFD::FileDialogConfig config;
            config.path = filePaths.defaultProjectPath; // Set the initial directory
            ImGuiFileDialog::Instance()->OpenDialog("LoadDirDialog", "Choose Directory", nullptr, config);
        }
        if (ImGui::Button("...##w8")) {
        }
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
    mgr.GetComponent<ImageComponent>(newEntity) = imageComp;

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

    if (ImGui::BeginTable("ModelLoaderTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("Model", ImGuiTableColumnFlags_WidthFixed, 54.0f); // Fixed width for Model
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 100.0f);        // Stretch to fill remaining space
        ImGui::TableSetupColumn("Load", ImGuiTableColumnFlags_WidthFixed, 40.0f);   // Fixed width for Load
        ImGui::TableHeadersRow();
        
        // Row for Unet
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Unet:");

        ImGui::TableNextColumn();
        ImGui::TextWrapped("%s", ckptComp.modelName.c_str());

        ImGui::TableNextColumn();
        if (ImGui::Button("...##n2")) {
            IGFD::FileDialogConfig config;
            config.path = filePaths.unetDir;
            ImGuiFileDialog::Instance()->OpenDialog("LoadUnetDialog", "Choose Model", ".safetensors,.ckpt,.pt,.gguf",
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
            }
            ImGuiFileDialog::Instance()->Close();
        }

        ImGui::SameLine();

        if (ImGui::Button("...##n3")) {
            ckptComp.modelName = "";
            ckptComp.modelPath = "";
        }
 
        // Row for Clip L
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Clip L:");

        ImGui::TableNextColumn();
        ImGui::TextWrapped("%s", clipLComp.modelName.c_str());

        ImGui::TableNextColumn();
        if (ImGui::Button("...##b7")) {
            IGFD::FileDialogConfig config;
            config.path = filePaths.encoderDir;
            ImGuiFileDialog::Instance()->OpenDialog("LoadClipLDialog", "Choose Model", ".safetensors,.ckpt,.pt,.gguf",
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
            }
            ImGuiFileDialog::Instance()->Close();
        }

        ImGui::SameLine();

        if (ImGui::Button("...##n6")) {
            clipLComp.modelName = "";
            clipLComp.modelPath = "";
        }

        // Row for Clip G
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Clip G:");

        ImGui::TableNextColumn();
        ImGui::TextWrapped("%s", clipGComp.modelName.c_str());

        ImGui::TableNextColumn();
        if (ImGui::Button("...##g7")) {
            IGFD::FileDialogConfig config;
            config.path = filePaths.encoderDir;
            ImGuiFileDialog::Instance()->OpenDialog("LoadClipGDialog", "Choose Model", ".safetensors,.ckpt,.pt,.gguf",
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
            }
            ImGuiFileDialog::Instance()->Close();
        }
        ImGui::SameLine();

        if (ImGui::Button("...##n5")) {
            clipGComp.modelName = "";
            clipGComp.modelPath = "";
        }

        // Row for T5XXL
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("T5XXL:");

        ImGui::TableNextColumn();
        ImGui::TextWrapped("%s", t5xxlComp.modelName.c_str());

        ImGui::TableNextColumn();
        if (ImGui::Button("...##x6")) {
            IGFD::FileDialogConfig config;
            config.path = filePaths.encoderDir;
            ImGuiFileDialog::Instance()->OpenDialog("LoadT5XXLDialog", "Choose Model", ".safetensors,.ckpt,.pt,.gguf",
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
            }
            ImGuiFileDialog::Instance()->Close();
        }
        ImGui::SameLine();

        if (ImGui::Button("...##n4")) {
            t5xxlComp.modelName = "";
            t5xxlComp.modelPath = "";
        }

        ImGui::EndTable();
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
                // Queue Controls
                RenderQueue();
                // Output FileName
                RenderFilePath();
                // Checkpoint loader
                RenderModelLoader();
                // Separated Model loader
                RenderDiffusionModelLoader();
                // Vae Loader
                RenderVaeLoader();
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
