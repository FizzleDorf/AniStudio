#include "GuiDiffusion.hpp"
#include "../Engine/Engine.hpp"
#include "../Events/Events.hpp"
#include "ECS.h"
#include "stable-diffusion.h"

using namespace ECS;
using namespace ANI;

void GuiDiffusion::RenderCKPTLoader() {
    ImGui::Text("Checkpoint:");
    ImGui::Text("%s", modelComp.modelName.c_str());
    ImGui::SameLine();

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
    ImGui::Combo("Sampler Method", &int(samplerComp.current_sample_method), SamplerComponent::sample_method_items,
                 SamplerComponent::sample_method_item_count);
    ImGui::Combo("Scheduler Method", &int(samplerComp.current_scheduler_method),
                 SamplerComponent::scheduler_method_items, SamplerComponent::scheduler_method_item_count);
    ImGui::InputInt("Seed", &samplerComp.seed);
    ImGui::InputInt("Steps", &samplerComp.steps);
    ImGui::InputFloat("Denoise", &samplerComp.denoise, 0.01f, 0.1f, "%.2f");
    ImGui::Checkbox("Free Parameters Immediately", &samplerComp.free_params_immediately);
    ImGui::Combo("Type Method", &int(samplerComp.current_type_method), SamplerComponent::type_method_items,
                 SamplerComponent::type_method_item_count);
    // ImGui::Combo("RNG Type", &int(samplerComp.current_rng_type), SamplerComponent::type_rng_items,
    // SamplerComponent::type_rng_item_count);
}

void GuiDiffusion::HandleQueueEvent() {
    Event event;
    EntityID newEntity = mgr.AddNewEntity();

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
    mgr.GetComponent<DiffusionModelComponent>(newEntity).ckptPath = ckptComp.ckptPath;
    mgr.GetComponent<VaeComponent>(newEntity).vaePath = vaeComp.vaePath;
    mgr.GetComponent<SamplerComponent>(newEntity).steps = samplerComp.steps;
    mgr.GetComponent<SamplerComponent>(newEntity).denoise = samplerComp.denoise;
    mgr.GetComponent<SamplerComponent>(newEntity).current_sample_method = samplerComp.current_sample_method;
    mgr.GetComponent<SamplerComponent>(newEntity).current_scheduler_method = samplerComp.current_scheduler_method;
    mgr.GetComponent<SamplerComponent>(newEntity).current_type_method = samplerComp.current_type_method;
    mgr.GetComponent<CFGComponent>(newEntity).cfg = cfgComp.cfg;
    mgr.GetComponent<PromptComponent>(newEntity).posPrompt = promptComp.posPrompt;
    mgr.GetComponent<PromptComponent>(newEntity).negPrompt = promptComp.negPrompt;
    mgr.GetComponent<LatentComponent>(newEntity).latentWidth = latentComp.latentWidth;
    mgr.GetComponent<LatentComponent>(newEntity).latentHeight = latentComp.latentHeight;

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
}

void GuiDiffusion::RenderQueue() {
    if (ImGui::Button("Queue")) {
        HandleQueueEvent();
    }
}

void GuiDiffusion::Render() {
    ImGui::SetNextWindowSize(ImVec2(300, 800), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Image Generation")) {
        RenderQueue();
        RenderCKPTLoader();
        RenderLatents();
        RenderInputImage();
        RenderPrompts();
        RenderSampler();
    }
    ImGui::End();
}
