#include "GuiDiffusion.hpp"
#include "../Engine/Engine.hpp"
#include "../Events/Events.hpp"
#include "ECS.h"
#include "stable-diffusion.h"

using namespace ECS;
using namespace ANI;

void GuiDiffusion::StartGui() {
    entity = mgr.AddNewEntity();
    std::cout << "Initialized entity with ID: " << entity << std::endl;

    mgr.AddComponent<ModelComponent>(entity);
    if (mgr.HasComponent<ModelComponent>(entity)) {
        modelComp = &mgr.GetComponent<ModelComponent>(entity);
        std::cout << "entity has ModelComponent with model path: " << modelComp->modelPath << std::endl;
    }

    mgr.AddComponent<CFGComponent>(entity);
    if (mgr.HasComponent<CFGComponent>(entity)) {
        cfgComp = &mgr.GetComponent<CFGComponent>(entity);
        std::cout << "entity has CFGComponent with value: " << cfgComp->cfg << std::endl;
    }
    mgr.AddComponent<DiffusionModelComponent>(entity);
    if (mgr.HasComponent<DiffusionModelComponent>(entity)) {
        ckptComp = &mgr.GetComponent<DiffusionModelComponent>(entity);
        std::cout << "entity has DiffusionModelComponent with model path: " << ckptComp->ckptPath << std::endl;
    }
    mgr.AddComponent<LatentComponent>(entity);
    if (mgr.HasComponent<LatentComponent>(entity)) {
        latentComp = &mgr.GetComponent<LatentComponent>(entity);
        std::cout << "entity has LatentComponent with Width: " << latentComp->latentWidth
                  << ", Height: " << latentComp->latentHeight << std::endl;
    }
    mgr.AddComponent<LoraComponent>(entity);
    if (mgr.HasComponent<LoraComponent>(entity)) {
        loraComp = &mgr.GetComponent<LoraComponent>(entity);
        std::cout << "entity has LoraComponent with Strength: " << loraComp->loraStrength
                  << ", Clip Strength: " << loraComp->loraClipStrength << ", Lora Reference: " << loraComp->loraPath
                  << std::endl;
    }
    mgr.AddComponent<PromptComponent>(entity);
    if (mgr.HasComponent<PromptComponent>(entity)) {
        promptComp = &mgr.GetComponent<PromptComponent>(entity);
        std::cout << "entity has PromptComponent with Positive Prompt: " << promptComp->posPrompt
                  << ", Negative Prompt: " << promptComp->negPrompt << std::endl;
    }
    mgr.AddComponent<SamplerComponent>(entity);
    if (mgr.HasComponent<SamplerComponent>(entity)) {
        samplerComp = &mgr.GetComponent<SamplerComponent>(entity);
        std::cout << "entity has SamplerComponent with Steps: " << samplerComp->steps
                  << ", Scheduler: " << samplerComp->scheduler_method_items[samplerComp->current_scheduler_method]
                  << ", Sampler: " << samplerComp->sample_method_items[samplerComp->current_sample_method]
                  << ", Denoise: " << samplerComp->denoise << std::endl;
    }
    mgr.AddComponent<InferenceComponent>(entity);
    if (mgr.HasComponent<InferenceComponent>(entity)) {
        inferenceComp = &mgr.GetComponent<InferenceComponent>(entity);
        std::cout << "entity has InferenceComponent" << std::endl;
    }
    mgr.AddComponent<ImageComponent>(entity);
    if (mgr.HasComponent<ImageComponent>(entity)) {
        imageComp = &mgr.GetComponent<ImageComponent>(entity);
        std::cout << "entity has ImageComponent" << std::endl;
    }
}

void GuiDiffusion::RenderCKPTLoader() {
    ImGui::Text("Checkpoint:");
    ImGui::Text("%s", modelComp->modelName.c_str());
    ImGui::SameLine();

    if (ImGui::Button("...")) {
        IGFD::FileDialogConfig config;
        ImGuiFileDialog::Instance()->OpenDialog("LoadFileDialog", "Choose Model", ".safetensors, .ckpt, .pt, .gguf",
                                                config);
    }

    if (ImGuiFileDialog::Instance()->Display("LoadFileDialog", 32, ImVec2(700, 400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            modelComp->modelName = ImGuiFileDialog::Instance()->GetCurrentFileName();
            modelComp->modelPath = ImGuiFileDialog::Instance()->GetFilePathName();
            std::cout << "New model path set: " << modelComp->modelName << std::endl;
        }
        ImGuiFileDialog::Instance()->Close();
    }
    ImGui::Combo("SD Type", &int(samplerComp->current_type_method), samplerComp->type_method_items,
                 samplerComp->type_method_item_count);
}

void GuiDiffusion::RenderLatents() {
    ImGui::InputInt("Width", &latentComp->latentWidth);
    ImGui::InputInt("Height", &latentComp->latentHeight);
}

void GuiDiffusion::RenderInputImage() {
    ImGui::Text("Image input placeholder"); // Adjust to render the actual image component if needed
}

void GuiDiffusion::RenderPrompts() {
    if (ImGui::InputTextMultiline("Positive Prompt", promptComp->PosBuffer, sizeof(promptComp->PosBuffer))) {
        promptComp->posPrompt = promptComp->PosBuffer;
    }

    if (ImGui::InputTextMultiline("Negative Prompt", promptComp->NegBuffer, sizeof(promptComp->NegBuffer))) {
        promptComp->negPrompt = promptComp->NegBuffer;
    }
}

void GuiDiffusion::RenderSampler() {

    ImGui::SliderInt("Steps", &samplerComp->steps, 1, 100);
    ImGui::SliderFloat("Denoise", &samplerComp->denoise, 0.0f, 1.0f);
}

void GuiDiffusion::HandleQueueEvent() {
    Event event;
    EntityID tempEntity = mgr.AddNewEntity();
    if (mgr.HasComponent<ModelComponent>(entity) && mgr.HasComponent<DiffusionModelComponent>(entity)) {

        mgr.AddComponent<ModelComponent>(tempEntity);
        mgr.GetComponent<ModelComponent>(tempEntity).modelPath = mgr.GetComponent<ModelComponent>(entity).modelPath;

        mgr.AddComponent<DiffusionModelComponent>(tempEntity);
        mgr.GetComponent<DiffusionModelComponent>(tempEntity).ckptPath =
            mgr.GetComponent<DiffusionModelComponent>(entity).ckptPath;
    } else {
        std::cerr << "ModelComponent and DiffusionModelComponent not found; Aborting Queuing Inference!" << std::endl;
        return;
    }
    event.entityID = tempEntity;
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
        ImGui::Text("Dock and arrange each window as needed:");

        RenderQueue();
        RenderCKPTLoader();
        RenderLatents();
        RenderInputImage();
        RenderPrompts();
        RenderSampler();
    }
    ImGui::End();
}
