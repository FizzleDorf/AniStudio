#include "GuiDiffusion.hpp"
#include "../Engine/Engine.hpp"
#include "../Events/Events.hpp"
#include "ECS.h"
#include "stable-diffusion.h"

using namespace ECS;
using namespace ANI;

void GuiDiffusion::StartGui() {
    // Add a new Entity
    //entity = mgr.AddNewEntity();

    //std::cout << "Initialized entity with ID: " << entity << std::endl;

    //mgr.AddComponent<ModelComponent>(entity);
    //mgr.AddComponent<CLipLComponent>(entity);
    //mgr.AddComponent<CLipGComponent>(entity);
    //mgr.AddComponent<T5XXLComponent>(entity);
    //mgr.AddComponent<DiffusionModelComponent>(entity);
    //mgr.AddComponent<VaeComponent>(entity);
    //mgr.AddComponent<LoraComponent>(entity);
    //mgr.AddComponent<LatentComponent>(entity);
    //mgr.AddComponent<ImageComponent>(entity);
    //mgr.AddComponent<SamplerComponent>(entity);
    //mgr.AddComponent<CFGComponent>(entity);
    //mgr.AddComponent<PromptComponent>(entity);

    //// Get components
    //modelComp = &mgr.GetComponent<ModelComponent>(entity);
    //clipLComp = &mgr.GetComponent<CLipLComponent>(entity);
    //clipGComp = &mgr.GetComponent<CLipGComponent>(entity);
    //t5xxlComp = &mgr.GetComponent<T5XXLComponent>(entity);
    //ckptComp = &mgr.GetComponent<DiffusionModelComponent>(entity);
    //loraComp = &mgr.GetComponent<LoraComponent>(entity);
    //latentComp = &mgr.GetComponent<LatentComponent>(entity);
    //imageComp = &mgr.GetComponent<ImageComponent>(entity);
    //promptComp = &mgr.GetComponent<PromptComponent>(entity);
    //samplerComp = &mgr.GetComponent<SamplerComponent>(entity);
    //cfgComp = &mgr.GetComponent<CFGComponent>(entity);
    //vaeComp = &mgr.GetComponent<VaeComponent>(entity);
}

void GuiDiffusion::RenderCKPTLoader() {
    ImGui::Text("Checkpoint:");
    ImGui::Text("%s", modelComp.modelName.c_str());
    ImGui::SameLine();

    if (ImGui::Button("...")) {
        IGFD::FileDialogConfig config;
        ImGuiFileDialog::Instance()->OpenDialog("LoadFileDialog", "Choose Model", ".safetensors, .ckpt, .pt, .gguf",
                                                config);
    }

    if (ImGuiFileDialog::Instance()->Display("LoadFileDialog", 32, ImVec2(700, 400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string selectedFile = ImGuiFileDialog::Instance()->GetCurrentFileName();
            std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();
            ;
            modelComp.modelName = selectedFile;
            modelComp.modelPath = fullPath;
            std::cout << "Selected file: " << modelComp.modelName << std::endl;
            std::cout << "Full path: " << modelComp.modelPath << std::endl;
            std::cout << "New model path set: " << modelComp.modelPath << std::endl;
        }

        ImGuiFileDialog::Instance()->Close();
    }
     ImGui::Combo("SD Type", &int(samplerComp.current_type_method), samplerComp.type_method_items,
                  samplerComp.type_method_item_count);
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

    ImGui::SliderInt("Steps", &samplerComp.steps, 1, 100);
    ImGui::SliderFloat("Denoise", &samplerComp.denoise, 0.0f, 1.0f);
}

void GuiDiffusion::HandleQueueEvent() {
    Event event;
    EntityID tempEntity = mgr.AddNewEntity();
    std::cout << "Initialized entity with ID: " << tempEntity << std::endl;
    mgr.AddComponent<ModelComponent>(tempEntity);
    mgr.AddComponent<CLipLComponent>(tempEntity);
    mgr.AddComponent<CLipGComponent>(tempEntity);
    mgr.AddComponent<T5XXLComponent>(tempEntity);
    mgr.AddComponent<DiffusionModelComponent>(tempEntity);
    mgr.AddComponent<VaeComponent>(tempEntity);
    mgr.AddComponent<LoraComponent>(tempEntity);
    mgr.AddComponent<LatentComponent>(tempEntity);
    mgr.AddComponent<ImageComponent>(tempEntity);
    mgr.AddComponent<SamplerComponent>(tempEntity);
    mgr.AddComponent<CFGComponent>(tempEntity);
    mgr.AddComponent<PromptComponent>(tempEntity);
    // TODO: add copy constructors to reduce this mess
    std::cout << "Assigning components to tempEntity: " << tempEntity << std::endl;

    // ModelComponent
    mgr.GetComponent<ModelComponent>(tempEntity).modelPath = modelComp.modelPath;
    std::cout << "ModelComponent.modelPath: " << mgr.GetComponent<ModelComponent>(tempEntity).modelPath << std::endl;

    // CLipLComponent
    mgr.GetComponent<CLipLComponent>(tempEntity).encoderPath = clipLComp.encoderPath;
    std::cout << "CLipLComponent.encoderPath: " << mgr.GetComponent<CLipLComponent>(tempEntity).encoderPath
              << std::endl;

    // CLipGComponent
    mgr.GetComponent<CLipGComponent>(tempEntity).encoderPath = clipGComp.encoderPath;
    std::cout << "CLipGComponent.encoderPath: " << mgr.GetComponent<CLipGComponent>(tempEntity).encoderPath
              << std::endl;

    // T5XXLComponent
    mgr.GetComponent<T5XXLComponent>(tempEntity).encoderPath = t5xxlComp.encoderPath;
    std::cout << "T5XXLComponent.encoderPath: " << mgr.GetComponent<T5XXLComponent>(tempEntity).encoderPath
              << std::endl;

    // DiffusionModelComponent
    mgr.GetComponent<DiffusionModelComponent>(tempEntity).ckptPath = ckptComp.ckptPath;
    std::cout << "DiffusionModelComponent.ckptPath: " << mgr.GetComponent<DiffusionModelComponent>(tempEntity).ckptPath
              << std::endl;

    // VaeComponent
    mgr.GetComponent<VaeComponent>(tempEntity).vaePath = vaeComp.vaePath;
    std::cout << "VaeComponent.vaePath: " << mgr.GetComponent<VaeComponent>(tempEntity).vaePath << std::endl;

    // SamplerComponent
    mgr.GetComponent<SamplerComponent>(tempEntity).steps = samplerComp.steps;
    std::cout << "SamplerComponent.steps: " << mgr.GetComponent<SamplerComponent>(tempEntity).steps << std::endl;

    mgr.GetComponent<SamplerComponent>(tempEntity).denoise = samplerComp.denoise;
    std::cout << "SamplerComponent.denoise: " << mgr.GetComponent<SamplerComponent>(tempEntity).denoise << std::endl;

    mgr.GetComponent<SamplerComponent>(tempEntity).current_sample_method = samplerComp.current_sample_method;
    std::cout << "SamplerComponent.current_sample_method: "
              << mgr.GetComponent<SamplerComponent>(tempEntity).current_sample_method << std::endl;

    mgr.GetComponent<SamplerComponent>(tempEntity).current_scheduler_method = samplerComp.current_scheduler_method;
    std::cout << "SamplerComponent.current_scheduler_method: "
              << mgr.GetComponent<SamplerComponent>(tempEntity).current_scheduler_method << std::endl;

    mgr.GetComponent<SamplerComponent>(tempEntity).current_type_method = samplerComp.current_type_method;
    std::cout << "SamplerComponent.current_type_method: "
              << mgr.GetComponent<SamplerComponent>(tempEntity).current_type_method << std::endl;

    // CFGComponent
    mgr.GetComponent<CFGComponent>(tempEntity).cfg = cfgComp.cfg;
    std::cout << "CFGComponent.cfg: " << mgr.GetComponent<CFGComponent>(tempEntity).cfg << std::endl;

    // PromptComponent
    mgr.GetComponent<PromptComponent>(tempEntity).posPrompt = promptComp.posPrompt;
    std::cout << "PromptComponent.posPrompt: " << mgr.GetComponent<PromptComponent>(tempEntity).posPrompt << std::endl;

    mgr.GetComponent<PromptComponent>(tempEntity).negPrompt = promptComp.negPrompt;
    std::cout << "PromptComponent.negPrompt: " << mgr.GetComponent<PromptComponent>(tempEntity).negPrompt << std::endl;


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
        RenderQueue();
        RenderCKPTLoader();
        RenderLatents();
        RenderInputImage();
        RenderPrompts();
        RenderSampler();
    }
    ImGui::End();
}
