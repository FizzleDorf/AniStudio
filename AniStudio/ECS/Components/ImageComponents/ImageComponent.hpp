#pragma once
#include "BaseComponent.hpp"
#include "filepaths.hpp"
#include <GL/glew.h>
#include <stb_image.h>

namespace ECS {

struct ImageComponent : public BaseComponent {
    std::string fileName = "AniStudio";
    std::string filePath = filePaths.defaultProjectPath;
    unsigned char *imageData = nullptr;
    int width = 0;
    int height = 0;
    int channels = 0;
    GLuint textureID = 0;

    virtual ComponentMetadata GetMetadata() const override {
        ComponentMetadata metadata;
        metadata.name = "Image";
        metadata.category = "Image Processing";
        metadata.description = "Handles image loading and processing";
        metadata.canBeNode = true;
        metadata.showPreview = true;

        // Properties
        metadata.properties = {ComponentProperty{.name = "File Name",
                                                 .type = ComponentProperty::Type::String,
                                                 .category = "File",
                                                 .limits = {.defaultValue = std::string("AniStudio")}},
                               ComponentProperty{.name = "File Path",
                                                 .type = ComponentProperty::Type::String,
                                                 .category = "File",
                                                 .limits = {.defaultValue = filePaths.defaultProjectPath}}};

        // Pins
        metadata.pins = {PinDescription{.name = "Image Out",
                                        .type = ComponentProperty::Type::Image,
                                        .category = "Output",
                                        .isInput = false,
                                        .isRequired = true}};

        return metadata;
    }

    char fileNameBuffer[256];
    char filePathBuffer[1024];

    void RenderProperties(EntityManager &mgr, EntityID entity) override {
        if (ImGui::CollapsingHeader("Image", ImGuiTreeNodeFlags_DefaultOpen)) {
            // File section
            if (ImGui::CollapsingHeader("File", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (fileName != fileNameBuffer) {
                    strncpy(fileNameBuffer, fileName.c_str(), sizeof(fileNameBuffer));
                }
                if (filePath != filePathBuffer) {
                    strncpy(filePathBuffer, filePath.c_str(), sizeof(filePathBuffer));
                }

                if (ImGui::InputText("Name", fileNameBuffer, sizeof(fileNameBuffer))) {
                    fileName = fileNameBuffer;
                }
                if (ImGui::InputText("Path", filePathBuffer, sizeof(filePathBuffer))) {
                    filePath = filePathBuffer;
                }

                if (ImGui::Button("Browse...")) {
                    // File dialog implementation
                }
            }

            // Image info section
            if (ImGui::CollapsingHeader("Image Info", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Size: %dx%d", width, height);
                ImGui::Text("Channels: %d", channels);

                if (textureID != 0) {
                    ImGui::Image((void *)(intptr_t)textureID,
                                 ImVec2(std::min(width * 0.25f, 200.0f), std::min(height * 0.25f, 200.0f)));
                }
            }
        }
    }

    void Execute(EntityManager &mgr, EntityID entity) override {
        if (!imageData && !filePath.empty()) {
            imageData = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
            if (imageData) {
                glGenTextures(1, &textureID);
                glBindTexture(GL_TEXTURE_2D, textureID);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
        }
    }

    nlohmann::json Serialize() const override {
        auto j = BaseComponent::Serialize();
        j["fileName"] = fileName;
        j["filePath"] = filePath;
        j["width"] = width;
        j["height"] = height;
        j["channels"] = channels;
        return j;
    }

    void Deserialize(const nlohmann::json &j) override {
        BaseComponent::Deserialize(j);
        if (j.contains("fileName"))
            fileName = j["fileName"];
        if (j.contains("filePath"))
            filePath = j["filePath"];
        if (j.contains("width"))
            width = j["width"];
        if (j.contains("height"))
            height = j["height"];
        if (j.contains("channels"))
            channels = j["channels"];
    }

    ~ImageComponent() {
        if (imageData) {
            stbi_image_free(imageData);
        }
    }
};

struct InputImageComponent : public ImageComponent {
    ComponentMetadata GetMetadata() const override {
        ComponentMetadata metadata;
        metadata.name = "Input Image";
        metadata.category = "Image Processing";
        metadata.description = "Input image node";
        metadata.canBeNode = true;
        metadata.showPreview = true;

        // Properties - same as ImageComponent
        metadata.properties = {ComponentProperty{.name = "File Name",
                                                 .type = ComponentProperty::Type::String,
                                                 .category = "File",
                                                 .limits = {.defaultValue = std::string("AniStudio")}},
                               ComponentProperty{.name = "File Path",
                                                 .type = ComponentProperty::Type::String,
                                                 .category = "File",
                                                 .limits = {.defaultValue = filePaths.defaultProjectPath}}};

        // Only output pin for input node
        metadata.pins = {PinDescription{.name = "Image Out",
                                        .type = ComponentProperty::Type::Image,
                                        .category = "Output",
                                        .isInput = false,
                                        .isRequired = true}};

        return metadata;
    }
};

struct OutputImageComponent : public ImageComponent {
    ComponentMetadata GetMetadata() const override {
        ComponentMetadata metadata;
        metadata.name = "Output Image";
        metadata.category = "Image Processing";
        metadata.description = "Output image node";
        metadata.canBeNode = true;
        metadata.showPreview = true;

        // Properties - same as ImageComponent
        metadata.properties = {ComponentProperty{.name = "File Name",
                                                 .type = ComponentProperty::Type::String,
                                                 .category = "File",
                                                 .limits = {.defaultValue = std::string("AniStudio")}},
                               ComponentProperty{.name = "File Path",
                                                 .type = ComponentProperty::Type::String,
                                                 .category = "File",
                                                 .limits = {.defaultValue = filePaths.defaultProjectPath}}};

        // Only input pin for output node
        metadata.pins = {PinDescription{.name = "Image In",
                                        .type = ComponentProperty::Type::Image,
                                        .category = "Input",
                                        .isInput = true,
                                        .isRequired = true}};

        return metadata;
    }
};

} // namespace ECS