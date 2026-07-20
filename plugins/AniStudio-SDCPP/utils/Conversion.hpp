#pragma once

#include "stable-diffusion.h"
#include "pch.h"
#include "RngUtils.hpp"
#include "ContextUtils.hpp" 
#include "SaveUtils.hpp"
#include <filesystem>
#include <iostream>

namespace Utils
{
	class Conversion
	{
	public:
		// Note: Conversion doesn't use sd_ctx_t, but we keep the parameter for API consistency
		static bool ConvertToGGUF(const nlohmann::json &metadata, sd_ctx_t *sd_context = nullptr)
		{
			// Note: sd_context parameter is not used for conversion, but kept for API consistency
			(void)sd_context; // Mark as unused to avoid compiler warnings

			try
			{
				std::string inputPath, vaePath;
				std::string tensorTypeRules = ""; // Optional parameter for tensor type conversion rules
				sd_type_t type = SD_TYPE_F16;
				bool convertName = true;

				// Extract model paths from metadata
				if (metadata.contains("components") && metadata["components"].is_array())
				{
					for (const auto &comp : metadata["components"])
					{
						if (comp.contains("Model"))
						{
							auto model = comp["Model"];
							if (model.contains("modelPath") && !model["modelPath"].get<std::string>().empty())
								inputPath = model["modelPath"].get<std::string>();
						}

						if (comp.contains("Vae"))
						{
							auto vae = comp["Vae"];
							if (vae.contains("modelPath") && !vae["modelPath"].get<std::string>().empty())
								vaePath = vae["modelPath"].get<std::string>();
						}

						if (comp.contains("Sampler"))
						{
							auto sampler = comp["Sampler"];
							if (sampler.contains("current_type_method"))
								type = static_cast<sd_type_t>(sampler["current_type_method"].get<int>());
						}

						if (comp.contains("Conversion"))
						{
							auto conversion = comp["Conversion"];
							if (conversion.contains("tensorTypeRules") && !conversion["tensorTypeRules"].get<std::string>().empty())
								tensorTypeRules = conversion["tensorTypeRules"].get<std::string>();

							// Add support for convert_name parameter
							if (conversion.contains("convertName"))
								convertName = conversion["convertName"].get<bool>();
						}
					}
				}

				// Validate input path
				if (inputPath.empty())
				{
					throw std::runtime_error("Input model path is empty");
				}

				if (!std::filesystem::exists(inputPath))
				{
					throw std::runtime_error("Input model file does not exist: " + inputPath);
				}

				// Create output path with type suffix
				std::filesystem::path inPath(inputPath);
				std::string outPath = inPath.parent_path().string() + "/" +
					inPath.stem().string() + "_" +
					std::string(sd_type_name(type)) + ".gguf";

				// Perform conversion using UPDATED API from stable-diffusion.h
				// All 6 parameters are required according to the header
				bool result = convert(inputPath.c_str(),
					vaePath.empty() ? nullptr : vaePath.c_str(),
					outPath.c_str(),
					type,
					tensorTypeRules.empty() ? nullptr : tensorTypeRules.c_str(),
					convertName);

				if (!result)
				{
					throw std::runtime_error("Failed to convert Model: " + inputPath);
				}

				// Log conversion details
				std::cout << "Conversion details:" << std::endl;
				std::cout << "  Input: " << inputPath << std::endl;
				std::cout << "  VAE: " << (vaePath.empty() ? "none" : vaePath) << std::endl;
				std::cout << "  Output: " << outPath << std::endl;
				std::cout << "  Type: " << sd_type_name(type) << std::endl;
				std::cout << "  Tensor Rules: " << (tensorTypeRules.empty() ? "none" : tensorTypeRules) << std::endl;
				std::cout << "  Convert Name: " << (convertName ? "true" : "false") << std::endl;

				return true;
			}
			catch (const std::exception &e)
			{
				std::cerr << "Exception during conversion: " << e.what() << std::endl;
				return false;
			}
		}
	};
} // namespace Utils