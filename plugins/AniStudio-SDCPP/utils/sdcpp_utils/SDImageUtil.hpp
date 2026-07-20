#pragma once
#include "stable-diffusion.h"

namespace Utils {
	// Load image from file to sd_image_t
	inline sd_image_t LoadImageToSDImage(const std::string& filePath) {
		sd_image_t result = { 0, 0, 0, nullptr };
		if (filePath.empty() || !std::filesystem::exists(filePath)) {
			return result;
		}
		int width, height, channels;
		unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
		if (data) {
			result.width = width;
			result.height = height;
			result.channel = channels;
			result.data = data;
		}
		return result;
	}

	// Free sd_image_t data
	inline void FreeSDImage(sd_image_t& img) {
		if (img.data) {
			stbi_image_free(img.data);
			img.data = nullptr;
		}
	}

	// Parse image component and add to resources
	static bool ParseImageComponent(const nlohmann::json& comp, sd_image_t& target_image,
		std::vector<std::unique_ptr<sd_image_t>>& imageResources) {
		try {
			if (comp.contains("filePath") && !comp["filePath"].is_null() &&
				!comp["filePath"].get<std::string>().empty()) {
				std::string imagePath = comp["filePath"].get<std::string>();
				sd_image_t image = LoadImageToSDImage(imagePath);
				if (image.data) {
					auto image_ptr = std::make_unique<sd_image_t>(image);
					target_image = *image_ptr;
					imageResources.push_back(std::move(image_ptr));
					return true;
				}
			}
			return false;
		}
		catch (...) {
			return false;
		}
	}
}