#include "GuiDiffusion.hpp"
#include "stable-diffusion.h"
#include "Systems.h"
#include "ECS.h"

using namespace ECS;

static const char *sample_method_items[] = {"Euler a", "Euler",     "Heun",  "Dpm 2",    "Dpmpp 2 a",
                                            "Dpmpp 2m", "Dpm++ 2m v2", "Ipndm", "Ipndm v", "Lcm"};
static const int sample_method_item_count = sizeof(sample_method_items) / sizeof(sample_method_items[0]);
static int current_sample_method = 1; // Default selected enum value

static const char *scheduler_method_items[] = {"Default", "Discrete", "Karras",   "Exponential",
                                               "Ays",     "Gits",     "N schedules"};
static const int scheduler_method_item_count = sizeof(scheduler_method_items) / sizeof(scheduler_method_items[0]);
static int current_scheduler_method = 0; // Default selected enum value

 static const char *type_method_items[] = {
     "SD_TYPE_F32",   "SD_TYPE_F16",    "SD_TYPE_Q4_0",   "SD_TYPE_Q4_1",   "SD_TYPE_Q5_0",  "SD_TYPE_Q5_1",
     "SD_TYPE_Q8_0",     "SD_TYPE_Q8_1",     "SD_TYPE_Q2_K",    "SD_TYPE_Q3_K",   "SD_TYPE_Q4_K",    "SD_TYPE_Q5_K",
    "SD_TYPE_Q6_K",     "SD_TYPE_Q8_K",     "SD_TYPE_IQ2_XXS", "SD_TYPE_IQ2_XS", "SD_TYPE_IQ3_XXS", "SD_TYPE_IQ1_S",
    "SD_TYPE_IQ4_NL",   "SD_TYPE_IQ3_S",    "SD_TYPE_IQ2_S",   "SD_TYPE_IQ4_XS", "SD_TYPE_I8",      "SD_TYPE_I16",
     "SD_TYPE_I32",    "SD_TYPE_I64",    "SD_TYPE_F64",     "SD_TYPE_IQ1_M",  "SD_TYPE_BF16",    "SD_TYPE_Q4_0_4_4",
     "SD_TYPE_Q4_0_4_8", "SD_TYPE_Q4_0_8_8", "SD_TYPE_COUNT",
 };
static const int type_method_item_count = sizeof(type_method_items) / sizeof(type_method_items[0]);
 static int current_type_method = 0; // Default selected enum value

char PosBuffer[9999] = "";
char NegBuffer[9999] = "";

void GuiDiffusion::StartGui() { 

    t2IEntity = mgr->AddNewEntity();
    std::cout << "Initialized entity with ID: " << t2IEntity << std::endl;
    
    mgr->AddComponent<CFGComponent>(t2IEntity);
    if (mgr->HasComponent<CFGComponent>(t2IEntity)) {
        cfgComp = &mgr->GetComponent<CFGComponent>(t2IEntity);
        std::cout << "t2IEntity has CFGComponent with value: " << cfgComp->cfg << std::endl;
    }
    mgr->AddComponent<DiffusionModelComponent>(t2IEntity);
    if (mgr->HasComponent<DiffusionModelComponent>(t2IEntity)) {
        ckptComp = &mgr->GetComponent<DiffusionModelComponent>(t2IEntity);
        std::cout << "t2IEntity has DiffusionModelComponent with model path: " << ckptComp->ckptPath
                  << std::endl;
    }
    mgr->AddComponent<LatentComponent>(t2IEntity);
    if (mgr->HasComponent<LatentComponent>(t2IEntity)) {
        latentComp = &mgr->GetComponent<LatentComponent>(t2IEntity);
        std::cout << "t2IEntity has LatentComponent with Width: " << latentComp->latentWidth << ", Height: " << latentComp->latentHeight
                  << std::endl;
    }
    mgr->AddComponent<LoraComponent>(t2IEntity);
    if (mgr->HasComponent<LoraComponent>(t2IEntity)) {
        loraComp = &mgr->GetComponent<LoraComponent>(t2IEntity);
        std::cout << "t2IEntity has LoraComponent with Strength: " << loraComp->loraStrength << ", Clip Strength: " << loraComp->loraClipStrength
                  << ", Lora Reference: " << loraComp->loraPath << std::endl;
    }
    mgr->AddComponent<PromptComponent>(t2IEntity);
    if (mgr->HasComponent<PromptComponent>(t2IEntity)) {
        promptComp = &mgr->GetComponent<PromptComponent>(t2IEntity);
        std::cout << "t2IEntity has PromptComponent with Positive Prompt: " << promptComp->posPrompt
                  << ", Negative Prompt: " << promptComp->negPrompt << std::endl;
    }
    mgr->AddComponent<SamplerComponent>(t2IEntity);
    if (mgr->HasComponent<SamplerComponent>(t2IEntity)) {
        samplerComp = &mgr->GetComponent<SamplerComponent>(t2IEntity);
        std::cout << "t2IEntity has SamplerComponent with Steps: " << samplerComp->steps
                  << ", Scheduler: " << samplerComp->scheduler << ", Sampler: " << samplerComp->sampler
                  << ", Denoise: " << samplerComp->denoise << std::endl;
    }
}

const int MIN_Width = 380;

void GuiDiffusion::RenderCKPTLoader() {
    ImGui::BeginChild(1, ImVec2(MIN_Width, 300), true);
    ImGui::Text("Checkpoint");
    ImGui::PushItemWidth(200);
    ImGui::Text("%s", ckptComp->ckptName.c_str());
    ImGui::PopItemWidth();
    ImGui::SameLine();
    // Open the file dialog when the button is clicked
    if(ImGui::Button("...")) {
        IGFD::FileDialogConfig config;
        ImGuiFileDialog::Instance()->OpenDialog("LoadFileDialog", "Choose Model", ".safetensors, .pth, .gguf", config);
    }

    // Display the dialog
    if (ImGuiFileDialog::Instance()->Display("LoadFileDialog")) {
        // When a file is selected and "OK" is clicked
        if (ImGuiFileDialog::Instance()->IsOk()) {
            ckptComp->ckptName = ImGuiFileDialog::Instance()->GetFilePathName();
            ckptComp->ckptPath = ImGuiFileDialog::Instance()->GetCurrentPath();
            std::cout << "New model's name: " << ckptComp->ckptName << std::endl; // Debug log
            std::cout << "New model path set: " << ckptComp->ckptPath << std::endl; // Debug log
        }

        // Close the dialog
        ImGuiFileDialog::Instance()->Close();
    }
    ImGui::NewLine();
    ImGui::PushItemWidth(200);
    ImGui::Combo("SD_Type", &current_type_method, type_method_items, type_method_item_count);
    ImGui::PopItemWidth();
    ImGui::EndChild();
}

void GuiDiffusion::RenderLatents() {
    ImGui::BeginChild(2, ImVec2(MIN_Width, 280), true);
    ImGui::Text("Latent Image(s)");
    ImGui::PushItemWidth(200);
    ImGui::InputInt("Width", &(latentComp->latentWidth));
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(200);
    ImGui::InputInt("Height", &(latentComp->latentHeight));
    ImGui::PopItemWidth();
    ImGui::NewLine();
    ImGui::PushItemWidth(200);
    static int i11 = 1;
    ImGui::InputInt("Batch Size", &i11);
    ImGui::PopItemWidth();
    ImGui::EndChild();
}

void GuiDiffusion::RenderInputImage() {
    ImGui::BeginChild(3, ImVec2(MIN_Width, MIN_Width), true);
    ImGui::Text("Image Input");
    ImGui::PushItemWidth(200);
    static char str14[128] = "Path/to/Image";
    ImGui::InputText("", str14, IM_ARRAYSIZE(str14));
    ImGui::PopItemWidth();
    ImGui::NewLine();
    ImGui::Text("Resize Options");
    static bool should_resize = false;
    ImGui::Checkbox("should resize?", &should_resize);
    ImGui::NewLine();
    ImGui::Text("Resize To:");
    ImGui::PushItemWidth(200);
    static int i19 = 123;
    ImGui::InputInt("Image Width", &i19);
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(200);
    static int i20 = 123;
    ImGui::InputInt("Image Height", &i20);
    ImGui::PopItemWidth();
    ImGui::EndChild();
}

void GuiDiffusion::RenderSampler() {
    ImGui::BeginChild(4, ImVec2(MIN_Width, 300), true);
    ImGui::Text("Sampler");
    ImGui::NewLine();
    
    ImGui::PushItemWidth(200);
    ImGui::Combo("Sampler", &current_sample_method, sample_method_items, sample_method_item_count);
    ImGui::PopItemWidth();
    
    ImGui::PushItemWidth(200);
    ImGui::Combo("Scheduler", &current_scheduler_method, scheduler_method_items, scheduler_method_item_count);
    ImGui::PopItemWidth();
   
    ImGui::NewLine();

    ImGui::PushItemWidth(200);
    ImGui::InputInt("Steps", &(samplerComp->steps));
    ImGui::PopItemWidth();

    ImGui::PushItemWidth(200);
    ImGui::InputFloat("CFG", &(cfgComp->cfg), 0.5f, 1.0f, "%.3f");
    ImGui::PopItemWidth();

    ImGui::PushItemWidth(200);
    ImGui::InputFloat("Denoise", &(samplerComp->denoise), 0.01f, 1.0f, "%.3f");
    ImGui::PopItemWidth();

    ImGui::EndChild();
}

void GuiDiffusion::RenderPrompts() {
    ImGui::BeginChild(5, ImVec2(MIN_Width, 400), true);

    ImGui::Text("Prompts");

    ImGui::NewLine();

    ImGui::PushItemWidth(200);
    if (ImGui::InputTextMultiline("Positive", PosBuffer, IM_ARRAYSIZE(PosBuffer))) {
        promptComp->posPrompt = std::string(PosBuffer);
    }
    ImGui::PopItemWidth();

    ImGui::NewLine();

    ImGui::PushItemWidth(200);
    if (ImGui::InputTextMultiline("Negative", NegBuffer, IM_ARRAYSIZE(NegBuffer))) {
        promptComp->negPrompt = std::string(NegBuffer);
    }
    ImGui::PopItemWidth();

    ImGui::EndChild();
}


void GuiDiffusion::Render() {
    if (ImGui::Begin("Image Generation")) {

        Queue();

        if (ImGui::CollapsingHeader("Ckpt Loader"))
            RenderCKPTLoader();
        if (ImGui::CollapsingHeader("Input Latent"))
            RenderLatents();
        // RenderInputImage();
        if (ImGui::CollapsingHeader("Prompts"))
            RenderPrompts();
        if (ImGui::CollapsingHeader("Sampler"))
            RenderSampler();
    }
    ImGui::End();
}


void GuiDiffusion::Queue() {
    if (ImGui::Button("Run Inference")) {
        // Queue the entity for inference when the button is pressed
        auto *sdSystem = &(mgr-><SDCPPSystem>());
        sdSystem->QueueInference(t2IEntity);
    }
}
