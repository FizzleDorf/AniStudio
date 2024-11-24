#include "GuiDiffusion.hpp"
#include "../Engine/Engine.hpp"
#include "../Events/Events.hpp"
#include "ECS.h"
#include "stable-diffusion.h"

using namespace ECS;
using namespace ANI;

ImageView imageView;
EntityID currentEntity;

void GuiDiffusion::RenderModelLoader() {
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

void GuiDiffusion::RenderLatents() {
    ImGui::InputInt("Width", &latentComp.latentWidth);
    ImGui::InputInt("Height", &latentComp.latentHeight);
    ImGui::InputInt("Batch Size", &latentComp.batchSize);
}

void GuiDiffusion::RenderInputImage() {
    ImGui::Text("Image input placeholder"); // Adjust to render the actual image component if needed
}

void GuiDiffusion::RenderPrompts() {
    if (ImGui::InputTextMultiline("Positive Prompt", promptComp.PosBuffer, sizeof(promptComp.PosBuffer))) {
        promptComp.posPrompt = promptComp.PosBuffer;
    }
    if (ImGui::InputTextMultiline("Negative Prompt", promptComp.NegBuffer, sizeof(promptComp.NegBuffer))) {
        promptComp.negPrompt = promptComp.NegBuffer;
    }
}

void GuiDiffusion::RenderSampler() {
    ImGui::Combo("Sampler", &int(samplerComp.current_sample_method), sample_method_items,
                 sample_method_item_count);
    ImGui::Combo("Scheduler", &int(samplerComp.current_scheduler_method),
                 scheduler_method_items, scheduler_method_item_count);
    ImGui::InputInt("Seed", &samplerComp.seed);
    ImGui::InputFloat("CFG", &cfgComp.cfg);
    ImGui::InputFloat("Guidance", &cfgComp.guidance);
    ImGui::InputInt("Steps", &samplerComp.steps);
    ImGui::InputFloat("Denoise", &samplerComp.denoise, 0.01f, 0.1f, "%.2f");
    
}

void GuiDiffusion::HandleT2IEvent() {
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
    mgr.AddComponent<LoraComponent>(newEntity);
    mgr.AddComponent<LatentComponent>(newEntity);
    mgr.AddComponent<SamplerComponent>(newEntity);
    mgr.AddComponent<CFGComponent>(newEntity);
    mgr.AddComponent<PromptComponent>(newEntity);
    mgr.AddComponent<EmbeddingComponent>(newEntity);
    mgr.AddComponent<ImageComponent>(newEntity);

    std::cout << "Assigning components to new Entity: " << newEntity << "\n";

    mgr.GetComponent<ModelComponent>(newEntity) = modelComp;
    mgr.GetComponent<CLipLComponent>(newEntity) = clipLComp;
    mgr.GetComponent<CLipGComponent>(newEntity) = clipGComp;
    mgr.GetComponent<T5XXLComponent>(newEntity) = t5xxlComp;
    mgr.GetComponent<DiffusionModelComponent>(newEntity) = ckptComp;
    mgr.GetComponent<VaeComponent>(newEntity) = vaeComp;
    mgr.GetComponent<TaesdComponent>(newEntity) = taesdComp;
    mgr.GetComponent<LoraComponent>(newEntity) = loraComp;
    mgr.GetComponent<LatentComponent>(newEntity) = latentComp;
    mgr.GetComponent<SamplerComponent>(newEntity) = samplerComp;
    mgr.GetComponent<CFGComponent>(newEntity) = cfgComp;
    mgr.GetComponent<PromptComponent>(newEntity) = promptComp;
    mgr.GetComponent<EmbeddingComponent>(newEntity) = embedComp;

    std::cout << "ModelComponent.modelPath: " << mgr.GetComponent<ModelComponent>(newEntity).modelPath << "\n";
    std::cout << "CLipLComponent.encoderPath: " << mgr.GetComponent<CLipLComponent>(newEntity).encoderPath << "\n";
    std::cout << "CLipGComponent.encoderPath: " << mgr.GetComponent<CLipGComponent>(newEntity).encoderPath << "\n";
    std::cout << "T5XXLComponent.encoderPath: " << mgr.GetComponent<T5XXLComponent>(newEntity).encoderPath << "\n";
    std::cout << "DiffusionModelComponent.ckptPath: " << mgr.GetComponent<DiffusionModelComponent>(newEntity).ckptPath
              << "\n";
    std::cout << "VaeComponent.vaePath: " << mgr.GetComponent<VaeComponent>(newEntity).vaePath << "\n";
    std::cout << "SamplerComponent.steps: " << mgr.GetComponent<SamplerComponent>(newEntity).steps << "\n";
    std::cout << "SamplerComponent.denoise: " << mgr.GetComponent<SamplerComponent>(newEntity).denoise << "\n";
    std::cout << "SamplerComponent.current_sample_method: " << mgr.GetComponent<SamplerComponent>(newEntity).current_sample_method << "\n";
    std::cout << "SamplerComponent.current_scheduler_method: " << mgr.GetComponent<SamplerComponent>(newEntity).current_scheduler_method << "\n";
    std::cout << "SamplerComponent.current_type_method: " << mgr.GetComponent<SamplerComponent>(newEntity).current_type_method << "\n";
    std::cout << "CFGComponent.cfg: " << mgr.GetComponent<CFGComponent>(newEntity).cfg << "\n";
    std::cout << "PromptComponent.posPrompt: " << mgr.GetComponent<PromptComponent>(newEntity).posPrompt << "\n";
    std::cout << "PromptComponent.negPrompt: " << mgr.GetComponent<PromptComponent>(newEntity).negPrompt << "\n";
    std::cout << "LatentComponent.latentWidth: " << mgr.GetComponent<LatentComponent>(newEntity).latentWidth << "\n";
    std::cout << "LatentComponent.latentHeight: " << mgr.GetComponent<LatentComponent>(newEntity).latentHeight << "\n";

    event.entityID = newEntity;
    event.type = EventType::InferenceRequest;
    ANI::Events::Ref().QueueEvent(event);
    imageView.SetImageComponent(mgr.GetComponent<ImageComponent>(newEntity));
}

void GuiDiffusion::HandleUpscaleEvent() {
    Event event;
    EntityID newEntity = mgr.AddNewEntity();

    std::cout << "Initialized entity with ID: " << newEntity << "\n";

    event.entityID = newEntity;
    event.type = EventType::InferenceRequest;
    ANI::Events::Ref().QueueEvent(event);
}

void GuiDiffusion::RenderQueue() {
    if (ImGui::Button("Queue")) {
        HandleT2IEvent();
    }

    ImGui::Checkbox("Free Parameters Immediately", &samplerComp.free_params_immediately);
    ImGui::Combo("Quant Type", &int(samplerComp.current_type_method), type_method_items, type_method_item_count);
    ImGui::Combo("RNG Type", &int(samplerComp.current_rng_type), type_rng_items, type_rng_item_count);
}

void GuiDiffusion::RenderControlnets() {
    ImGui::Text("Controlnet:");
    ImGui::SameLine();
    ImGui::Text("%s", controlComp.controlName.c_str());

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

            controlComp.controlName = selectedFile;
            controlComp.controlPath = fullPath;
            std::cout << "Selected file: " << controlComp.controlName << std::endl;
            std::cout << "Full path: " << controlComp.controlPath << std::endl;
            std::cout << "New model path set: " << controlComp.controlPath << std::endl;
        }

        ImGuiFileDialog::Instance()->Close();
    }
    ImGui::InputFloat("Strength", &controlComp.cnStrength, 0.01f, 0.1f, "%.2f");
    ImGui::InputFloat("Start", &controlComp.applyStart, 0.01f, 0.1f, "%.2f");
    ImGui::InputFloat("End", &controlComp.applyEnd, 0.01f, 0.1f, "%.2f");
}
void GuiDiffusion::RenderEmbeddings() {
    ImGui::Text("Embedding:");
    ImGui::SameLine();
    ImGui::Text("%s", embedComp.embedName.c_str());

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

            embedComp.embedName = selectedFile;
            embedComp.embedPath = fullPath;
            std::cout << "Selected file: " << embedComp.embedName << std::endl;
            std::cout << "Full path: " << embedComp.embedPath << std::endl;
            std::cout << "New model path set: " << embedComp.embedPath << std::endl;
        }

        ImGuiFileDialog::Instance()->Close();
    }
}
void GuiDiffusion::RenderDiffusionModelLoader() { 
    ImGui::Text("Unet: ");
    ImGui::SameLine();
    ImGui::Text("%s", ckptComp.ckptName.c_str());

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

            ckptComp.ckptName = selectedFile;
            ckptComp.ckptPath = fullPath;
            std::cout << "Selected file: " << ckptComp.ckptName << std::endl;
            std::cout << "Full path: " << ckptComp.ckptPath << std::endl;
            std::cout << "New model path set: " << ckptComp.ckptPath << std::endl;
        }

        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::Text("Clip L: ");
    ImGui::SameLine();
    ImGui::Text("%s", clipLComp.encoderName.c_str());

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

            clipLComp.encoderName = selectedFile;
            clipLComp.encoderPath = fullPath;
            std::cout << "Selected file: " << clipLComp.encoderName << std::endl;
            std::cout << "Full path: " << clipLComp.encoderPath << std::endl;
            std::cout << "New model path set: " << clipLComp.encoderPath << std::endl;
        }

        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::Text("Clip G: ");
    ImGui::SameLine();
    ImGui::Text("%s", clipGComp.encoderName.c_str());

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

            clipGComp.encoderName = selectedFile;
            clipGComp.encoderPath = fullPath;
            std::cout << "Selected file: " << clipGComp.encoderName << std::endl;
            std::cout << "Full path: " << clipGComp.encoderPath << std::endl;
            std::cout << "New model path set: " << clipGComp.encoderPath << std::endl;
        }

        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::Text("T5XXL: ");
    ImGui::SameLine();
    ImGui::Text("%s", t5xxlComp.encoderName.c_str());

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

            t5xxlComp.encoderName = selectedFile;
            t5xxlComp.encoderPath = fullPath;
            std::cout << "Selected file: " << t5xxlComp.encoderName << std::endl;
            std::cout << "Full path: " << t5xxlComp.encoderPath << std::endl;
            std::cout << "New model path set: " << t5xxlComp.encoderPath << std::endl;
        }

        ImGuiFileDialog::Instance()->Close();
    }
}
void GuiDiffusion::RenderVaeLoader() {
    ImGui::Text("Vae: ");
    ImGui::SameLine();
    ImGui::Text("%s", vaeComp.vaeName.c_str());

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

            vaeComp.vaeName = selectedFile;
            vaeComp.vaePath = fullPath;
            std::cout << "Selected file: " << vaeComp.vaeName << std::endl;
            std::cout << "Full path: " << vaeComp.vaePath << std::endl;
            std::cout << "New model path set: " << vaeComp.vaePath << std::endl;
        }

        ImGuiFileDialog::Instance()->Close();
    }
}

void GuiDiffusion::Render() {
    ImGui::SetNextWindowSize(ImVec2(300, 800), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Image Generation")) {
        if (ImGui::BeginTabBar("Image")) {
            if (ImGui::BeginTabItem("Txt2Img")) {            
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
        
        
        if (ImGui::BeginTabItem("Img2Img")) {
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
