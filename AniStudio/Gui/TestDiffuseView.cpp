#include "TestDiffuseView.hpp"
#include <fstream>
#include <iostream>

TestDiffuseView::TestDiffuseView(sd_ctx_t *context) : sd_ctx(context) {}

void TestDiffuseView::SetOutputPath(const std::string &path) { outputPath = path; }

void TestDiffuseView::Render() {
    ImGui::Begin("Test Diffusion");

    // Ensure the string buffer is large enough
    static char promptBuffer[256];
    static char negativePromptBuffer[256];

    // Copy current prompt values to buffer
    strncpy(promptBuffer, prompt.c_str(), sizeof(promptBuffer));
    strncpy(negativePromptBuffer, negativePrompt.c_str(), sizeof(negativePromptBuffer));

    if (ImGui::InputText("Prompt", promptBuffer, IM_ARRAYSIZE(promptBuffer))) {
        prompt = std::string(promptBuffer); // Update prompt when edited
    }
    if (ImGui::InputText("Negative Prompt", negativePromptBuffer, IM_ARRAYSIZE(negativePromptBuffer))) {
        negativePrompt = std::string(negativePromptBuffer); // Update negative prompt when edited
    }

    ImGui::InputInt("Width", &width);
    ImGui::InputInt("Height", &height);

    if (ImGui::Button("Generate Image")) {
        inference.TxtToImgInference();
    }

    ImGui::End();
}

void TestDiffuseView::GenerateImage() {
    std::cout << "Generating image with prompt: " << prompt << ", width: " << width << ", height: " << height
              << ", output path: " << outputPath << std::endl;


    // Call the txt2img function from the stable diffusion API
    sd_image_t *image = txt2img(sd_ctx, prompt.c_str(), negativePrompt.empty() ? nullptr : negativePrompt.c_str(),
                                0,    // clip_skip
                                7.5f, // cfg_scale
                                1.0f, // guidance
                                width, height,
                                EULER,   // sample_method (assuming EULER is defined somewhere)
                                20,      // sample_steps
                                0,       // seed
                                1,       // batch_count
                                nullptr, // control_cond
                                0.0f,    // control_strength
                                0.0f,    // style_strength
                                true,    // normalize_input
                                "" // input_id_images_path
    );
    if (!image) {
        std::cerr << "txt2img failed: " << sd_get_system_info() << std::endl; // If an error function is available
    }

}
