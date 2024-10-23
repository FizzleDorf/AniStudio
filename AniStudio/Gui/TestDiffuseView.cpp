#include "TestDiffuseView.hpp"
#include <fstream>
#include <iostream>

TestDiffuseView::TestDiffuseView(sd_ctx_t *context) : sd_ctx(context) {}

void TestDiffuseView::SetOutputPath(const std::string &path) { outputPath = path; }

void LogCallback(sd_log_level_t level, const char *text, void *data) {
    switch (level) {
    case SD_LOG_DEBUG:
        std::cout << "[DEBUG]: " << text << std::endl;
        break;
    case SD_LOG_INFO:
        std::cout << "[INFO]: " << text << std::endl;
        break;
    case SD_LOG_WARN:
        std::cout << "[WARNING]: " << text << std::endl;
        break;
    case SD_LOG_ERROR:
        std::cerr << "[ERROR]: " << text << std::endl;
        break;
    default:
        std::cerr << "[UNKNOWN LOG LEVEL]: " << text << std::endl;
        break;
    }
}

void ProgressCallback(int step, int steps, float time, void *data) {
    std::cout << "Progress: Step " << step << " of " << steps << " | Time: " << time << "s" << std::endl;
}


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
        GenerateImage();
    }

    ImGui::End();
}

void TestDiffuseView::GenerateImage() {
    std::cout << "Generating image with prompt: " << prompt << ", width: " << width << ", height: " << height
              << ", output path: " << outputPath << std::endl;

    sd_set_log_callback(LogCallback, nullptr);
    sd_set_progress_callback(ProgressCallback, nullptr);


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
                                nullptr  // input_id_images_path
    );
    if (!image) {
        std::cerr << "txt2img failed: " << sd_get_system_info() << std::endl; // If an error function is available
    }

    // Check if image generation was successful
    if (image) {
        // Save the image to the designated output path
        std::ofstream ofs(outputPath, std::ios::binary);
        if (ofs) {
            ofs.write(reinterpret_cast<const char *>(image->data), image->width * image->height * image->channel);
            ofs.close();
            std::cout << "Image saved to " << outputPath << std::endl;
        } else {
            std::cerr << "Failed to open output file: " << outputPath << std::endl;
        }
    } else {
        std::cerr << "Failed to generate image." << std::endl;
    }
}
