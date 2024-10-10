#include "GuiDiffusion.hpp"
#include "stable-diffusion.h"
#include "EntityManager.hpp"

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

std::string modelName = "Model.safetensors";

void GuiDiffusion::StartGui() { 

    t2IEntity = mgr->AddNewEntity();
    std::cout << "Initialized entity with ID: " << t2IEntity << std::endl;

    mgr->AddComponent<CFGComponent>(t2IEntity);
    if (mgr->HasComponent<CFGComponent>(t2IEntity)) {
        cfg = &mgr->GetComponent<CFGComponent>(t2IEntity).cfg;
        std::cout << "t2IEntity has CFGComponent with value: " << *cfg << std::endl;
    }
    mgr->AddComponent<DiffusionModelComponent>(t2IEntity);
    if (mgr->HasComponent<DiffusionModelComponent>(t2IEntity)) {
        modelPath = &mgr->GetComponent<DiffusionModelComponent>(t2IEntity).model_path;
        std::cout << "t2IEntity has DiffusionModelComponent with model path: " << *modelPath
                  << std::endl;
    }
    mgr->AddComponent<LatentComponent>(t2IEntity);
    if (mgr->HasComponent<LatentComponent>(t2IEntity)) {
        latentWidth = &mgr->GetComponent<LatentComponent>(t2IEntity).width;
        latentHeight = &mgr->GetComponent<LatentComponent>(t2IEntity).height;
        std::cout << "t2IEntity has LatentComponent with Width: " << *latentWidth << ", Height: " << *latentHeight
                  << std::endl;
    }
    mgr->AddComponent<LoraComponent>(t2IEntity);
    if (mgr->HasComponent<LoraComponent>(t2IEntity)) {
        strength = &mgr->GetComponent<LoraComponent>(t2IEntity).strength;
        clipStrength = &mgr->GetComponent<LoraComponent>(t2IEntity).clipStrength;
        loraReference = &mgr->GetComponent<LoraComponent>(t2IEntity).lora_reference;
        std::cout << "t2IEntity has LoraComponent with Strength: " << *strength << ", Clip Strength: " << *clipStrength
                  << ", Lora Reference: " << *loraReference << std::endl;
    }
    mgr->AddComponent<PromptComponent>(t2IEntity);
    if (mgr->HasComponent<PromptComponent>(t2IEntity)) {
        posPrompt = &mgr->GetComponent<PromptComponent>(t2IEntity).posPrompt;
        negPrompt = &mgr->GetComponent<PromptComponent>(t2IEntity).negPrompt;
        std::cout << "t2IEntity has PromptComponent with Positive Prompt: " << *posPrompt
                  << ", Negative Prompt: " << *negPrompt << std::endl;
    }
    mgr->AddComponent<SamplerComponent>(t2IEntity);
    if (mgr->HasComponent<SamplerComponent>(t2IEntity)) {
        samplerSteps = &mgr->GetComponent<SamplerComponent>(t2IEntity).steps;
        scheduler = &mgr->GetComponent<SamplerComponent>(t2IEntity).scheduler;
        sampler = &mgr->GetComponent<SamplerComponent>(t2IEntity).sampler;
        denoise = &mgr->GetComponent<SamplerComponent>(t2IEntity).denoise;
        std::cout << "t2IEntity has SamplerComponent with Steps: " << *samplerSteps << ", Scheduler: " << *scheduler
                  << ", Sampler: " << *sampler << ", Denoise: " << *denoise << std::endl;
    }
}

int MIN_Width = 380;

void GuiDiffusion::RenderCKPTLoader() {
    ImGui::BeginChild(1, ImVec2(MIN_Width, 300), true);
    ImGui::Text("Checkpoint");
    ImGui::PushItemWidth(200);
    ImGui::Text("%s", modelName.c_str());
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
            modelName = ImGuiFileDialog::Instance()->GetFilePathName();
            *modelPath = ImGuiFileDialog::Instance()->GetCurrentPath();
            std::cout << "New model's name: " << modelName << std::endl;    // Debug log
            std::cout << "New model path set: " << *modelPath << std::endl; // Debug log
        }

        // Close the dialog
        ImGuiFileDialog::Instance()->Close();
    }
    ImGui::NewLine();
    ImGui::PushItemWidth(200);
    static int item_current41 = 0;
    const char *items41[] = {"Never", "Gonna", "Give", "You", "Up"};
    ImGui::Combo("SD_Type", &current_type_method, type_method_items, type_method_item_count);
    ImGui::PopItemWidth();
    ImGui::EndChild();
}

void GuiDiffusion::RenderLatents() {
    ImGui::BeginChild(2, ImVec2(MIN_Width, 280), true);
    ImGui::Text("Latent Image(s)");
    ImGui::PushItemWidth(200);
    ImGui::InputInt("Width", latentWidth);
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(200);
    ImGui::InputInt("Height", latentHeight);
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
    ImGui::InputInt("Steps", samplerSteps);
    ImGui::PopItemWidth();

    ImGui::PushItemWidth(200);
    ImGui::InputFloat("CFG", cfg);
    ImGui::PopItemWidth();

    ImGui::PushItemWidth(200);
    ImGui::InputFloat("Denoise", denoise, 0.01f, 1.0f, "%.3f");
    ImGui::PopItemWidth();

    ImGui::EndChild();
}

void GuiDiffusion::RenderPrompts() {
    ImGui::BeginChild(5, ImVec2(MIN_Width, 400), true);

    ImGui::Text("Prompts");

    ImGui::NewLine();

    ImGui::PushItemWidth(200);
    if (ImGui::InputTextMultiline("Positive", PosBuffer, IM_ARRAYSIZE(PosBuffer))) {
        *posPrompt = std::string(PosBuffer);
    }
    ImGui::PopItemWidth();

    ImGui::NewLine();

    ImGui::PushItemWidth(200);
    if (ImGui::InputTextMultiline("Negative", NegBuffer, IM_ARRAYSIZE(NegBuffer))) {
        *negPrompt = std::string(NegBuffer);
    }
    ImGui::PopItemWidth();

    ImGui::EndChild();
}

void GuiDiffusion::RenderCommands() {

}

void GuiDiffusion::Render() {
    if (ImGui::Begin("Image Generation")) {

        RenderCommands();

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


