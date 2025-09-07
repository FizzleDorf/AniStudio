/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license information, please contact legal@kframe.ai.
 */

#pragma once

#include "stable-diffusion.h"
#include "pch.h"
#include "RngUtils.hpp"
#include "ContextUtils.hpp" 
#include "SaveUtils.hpp"
#include <filesystem>

namespace Utils
{
	class Conversion
	{
	public:
		static bool ConvertToGGUF(const nlohmann::json &metadata)
		{
			try
			{
				std::string inputPath, vaePath;
				sd_type_t type = SD_TYPE_F16;

				// Extract model paths from metadata
				if (metadata.contains("components") && metadata["components"].is_array())
				{
					for (const auto &comp : metadata["components"])
					{
						if (comp.contains("Model"))
						{
							auto model = comp["Model"];
							if (model.contains("modelPath"))
								inputPath = model["modelPath"];
						}

						if (comp.contains("Vae"))
						{
							auto vae = comp["Vae"];
							if (vae.contains("modelPath"))
								vaePath = vae["modelPath"];
						}

						if (comp.contains("Sampler"))
						{
							auto sampler = comp["Sampler"];
							if (sampler.contains("current_type_method"))
								type = static_cast<sd_type_t>(sampler["current_type_method"].get<int>());
						}
					}
				}

				// Validate input path
				if (inputPath.empty())
				{
					throw std::runtime_error("Input model path is empty");
				}

				// Create output path with type suffix
				std::filesystem::path inPath(inputPath);
				std::string outPath = inPath.parent_path().string() + "/" +
					inPath.stem().string() + "_" +
					std::string(sd_type_name(type)) + ".gguf";

				// Perform conversion
				bool result;
				if (vaePath.empty())
				{
					// Convert without VAE
					result = convert(inputPath.c_str(), nullptr, outPath.c_str(), type);
				}
				else
				{
					// Convert with VAE
					result = convert(inputPath.c_str(), vaePath.c_str(), outPath.c_str(), type);
				}

				if (!result)
				{
					throw std::runtime_error("Failed to convert Model: " + inputPath);
				}

				std::cout << "Successfully converted model to: " << outPath << std::endl;
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