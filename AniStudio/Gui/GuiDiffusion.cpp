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
    EntityID newEntity = mgr.AddNewEntity();
    std::cout << "Initialized entity with ID: " << newEntity << std::endl;
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

    // TODO: add copy constructors to reduce this mess
    std::cout << "Assigning components to new Entity: " << newEntity << std::endl;

    // ModelComponent
    mgr.GetComponent<ModelComponent>(newEntity).modelPath = modelComp.modelPath;
    std::cout << "ModelComponent.modelPath: " << mgr.GetComponent<ModelComponent>(newEntity).modelPath << std::endl;

    // CLipLComponent
    mgr.GetComponent<CLipLComponent>(newEntity).encoderPath = clipLComp.encoderPath;
    std::cout << "CLipLComponent.encoderPath: " << mgr.GetComponent<CLipLComponent>(newEntity).encoderPath
              << std::endl;

    // CLipGComponent
    mgr.GetComponent<CLipGComponent>(newEntity).encoderPath = clipGComp.encoderPath;
    std::cout << "CLipGComponent.encoderPath: " << mgr.GetComponent<CLipGComponent>(newEntity).encoderPath
              << std::endl;

    // T5XXLComponent
    mgr.GetComponent<T5XXLComponent>(newEntity).encoderPath = t5xxlComp.encoderPath;
    std::cout << "T5XXLComponent.encoderPath: " << mgr.GetComponent<T5XXLComponent>(newEntity).encoderPath
              << std::endl;

    // DiffusionModelComponent
    mgr.GetComponent<DiffusionModelComponent>(newEntity).ckptPath = ckptComp.ckptPath;
    std::cout << "DiffusionModelComponent.ckptPath: " << mgr.GetComponent<DiffusionModelComponent>(newEntity).ckptPath
              << std::endl;

    // VaeComponent
    mgr.GetComponent<VaeComponent>(newEntity).vaePath = vaeComp.vaePath;
    std::cout << "VaeComponent.vaePath: " << mgr.GetComponent<VaeComponent>(newEntity).vaePath << std::endl;

    // SamplerComponent
    mgr.GetComponent<SamplerComponent>(newEntity).steps = samplerComp.steps;
    std::cout << "SamplerComponent.steps: " << mgr.GetComponent<SamplerComponent>(newEntity).steps << std::endl;

    mgr.GetComponent<SamplerComponent>(newEntity).denoise = samplerComp.denoise;
    std::cout << "SamplerComponent.denoise: " << mgr.GetComponent<SamplerComponent>(newEntity).denoise << std::endl;

    mgr.GetComponent<SamplerComponent>(newEntity).current_sample_method = samplerComp.current_sample_method;
    std::cout << "SamplerComponent.current_sample_method: "
              << mgr.GetComponent<SamplerComponent>(newEntity).current_sample_method << std::endl;

    mgr.GetComponent<SamplerComponent>(newEntity).current_scheduler_method = samplerComp.current_scheduler_method;
    std::cout << "SamplerComponent.current_scheduler_method: "
              << mgr.GetComponent<SamplerComponent>(newEntity).current_scheduler_method << std::endl;

    mgr.GetComponent<SamplerComponent>(newEntity).current_type_method = samplerComp.current_type_method;
    std::cout << "SamplerComponent.current_type_method: "
              << mgr.GetComponent<SamplerComponent>(newEntity).current_type_method << std::endl;

    // CFGComponent
    mgr.GetComponent<CFGComponent>(newEntity).cfg = cfgComp.cfg;
    std::cout << "CFGComponent.cfg: " << mgr.GetComponent<CFGComponent>(newEntity).cfg << std::endl;

    // PromptComponent
    mgr.GetComponent<PromptComponent>(newEntity).posPrompt = promptComp.posPrompt;
    std::cout << "PromptComponent.posPrompt: " << mgr.GetComponent<PromptComponent>(newEntity).posPrompt << "\n";

    mgr.GetComponent<PromptComponent>(newEntity).negPrompt = promptComp.negPrompt;
    std::cout << "PromptComponent.negPrompt: " << mgr.GetComponent<PromptComponent>(newEntity).negPrompt << "\n";

    mgr.GetComponent<LatentComponent>(newEntity).latentWidth = latentComp.latentWidth;
    std::cout << "LatentComponent.latentWidth: " << mgr.GetComponent<LatentComponent>(newEntity).latentWidth << "\n";

    mgr.GetComponent<LatentComponent>(newEntity).latentHeight = latentComp.latentHeight;
    std::cout << "LatentComponent.latentHeight: " << mgr.GetComponent<LatentComponent>(newEntity).latentHeight << "\n";

    // Set the Event params and send the request
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
