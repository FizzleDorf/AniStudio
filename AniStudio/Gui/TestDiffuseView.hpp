#ifndef TEST_DIFFUSE_VIEW_HPP
#define TEST_DIFFUSE_VIEW_HPP

#include "stable-diffusion.h"
#include "SDCPPComponents.h"
#include <imgui.h>
#include <string>

class TestDiffuseView {
public:
    TestDiffuseView(sd_ctx_t *context);

    void Render();                               
    void SetOutputPath(const std::string &path);

private:
    InferenceComponent inference;
    sd_ctx_t *sd_ctx;
    std::string prompt;
    std::string negativePrompt;
    int width = 512;
    int height = 512;
    std::string outputPath;

    void GenerateImage();
};

#endif // TEST_DIFFUSE_VIEW_HPP
