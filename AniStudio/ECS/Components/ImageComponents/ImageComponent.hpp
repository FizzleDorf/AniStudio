#pragma once

#include "BaseComponent.hpp"
#include "FilePaths.hpp"
#include <GL/glew.h>
#include <string>
#include <stb_image.h>

namespace ECS {
struct ImageComponent : public BaseComponent {
    std::string fileName = "AniStudio";                  // Default file name
    std::string filePath = Utils::FilePaths::defaultProjectPath; // Directory containing the Image
    unsigned char *imageData = nullptr;                  // Pointer to image data
    int width = 0;                                       // Image width
    int height = 0;                                      // Image height
    int channels = 0;                                    // Number of color channels
    GLuint textureID = 0;                                // OpenGL texture ID

    ImageComponent() {
        compName = "Image";
    }

    ~ImageComponent() {
        if (imageData) {
            stbi_image_free(imageData);
        }
    }

	// Serialize the component to JSON
	virtual nlohmann::json Serialize() const override {
		nlohmann::json j;
		j["compName"] = compName;
		j[compName] = {
			{"width", width},
			{"height", height},
			{"channels", channels},
			{"fileName", fileName},
			{"filePath", filePath}
		};
		return j;
	}

	// Deserialize the component from JSON
	virtual void Deserialize(const nlohmann::json& j) override {
		BaseComponent::Deserialize(j);

		nlohmann::json componentData;

		if (j.contains(compName)) {
			componentData = j.at(compName);
		}
		else {
			for (auto it = j.begin(); it != j.end(); ++it) {
				if (it.key() == compName) {
					componentData = it.value();
					break;
				}
			}
			if (componentData.empty()) {
				componentData = j;
			}
		}

		if (componentData.contains("width"))
			width = componentData["width"];
		if (componentData.contains("height"))
			height = componentData["height"];
		if (componentData.contains("channels"))
			channels = componentData["channels"];
		if (componentData.contains("width"))
			fileName = componentData["fileName"];
	}

    ImageComponent &operator=(const ImageComponent &other) {
        if (this != &other) {
            fileName = other.fileName;
            filePath = other.filePath;
            width = other.width;
            height = other.height;
            channels = other.channels;
        }
        return *this;
    }
};

struct InputImageComponent : public ImageComponent {
    InputImageComponent() {
        compName = "InputImage";
		fileName = "";
		filePath = "";
    }

	InputImageComponent &operator=(const InputImageComponent &other) {
		if (this != &other) {
			fileName = other.fileName;
			filePath = other.filePath;
			width = other.width;
			height = other.height;
			channels = other.channels;
		}
		return *this;
	}
};

struct OutputImageComponent : public ImageComponent {
    OutputImageComponent() {
        compName = "OutputImage";
    }

	OutputImageComponent &operator=(const OutputImageComponent &other) {
		if (this != &other) {
			fileName = other.fileName;
			filePath = other.filePath;
			width = other.width;
			height = other.height;
			channels = other.channels;
		}
		return *this;
	}

};

struct ControlNetImageComponent : public ImageComponent {
    ControlNetImageComponent() {
        compName = "ControlNetImageComponent";
    }
};

struct MaskImageComponent : public ImageComponent {
    MaskImageComponent() {
        compName = "MaskImageComponent";
		fileName = "";
		filePath = "";
    }

	MaskImageComponent &operator=(const MaskImageComponent &other) {
		if (this != &other) {
			fileName = other.fileName;
			filePath = other.filePath;
			width = other.width;
			height = other.height;
			channels = other.channels;
			value = other.value;
		}
		return *this;
	}

    float value = 0.75f;
};
} // namespace ECS
