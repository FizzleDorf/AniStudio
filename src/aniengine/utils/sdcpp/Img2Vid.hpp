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
#include "PngMetadataUtils.hpp"
#include "pch.h"
#include "RngUtils.hpp"
#include "ContextUtils.hpp" 
#include "SaveUtils.hpp"
#include <stb_image.h>
#include <stb_image_write.h>

namespace Utils
{
	// Forward declarations for shared utilities
	uint64_t generateRandomSeed();
	void SaveImage(const unsigned char *data, int width, int height, int channels,
		const nlohmann::json &metadata, const std::string &fullPath);
	sd_ctx_t *InitializeStableDiffusionContext(const nlohmann::json &metadata);

	class Img2Vid
	{
	public:
		static bool RunImg2Vid(const nlohmann::json &metadata, std::string fullPath)
		{
			sd_ctx_t *sd_context = nullptr;
			unsigned char *inputData = nullptr;
			sd_image_t *result_image = nullptr;

			try
			{
				// Extract parameters from metadata
				std::string inputImagePath = "";
				std::string outputPath = Utils::FilePaths::defaultProjectPath;
				std::string outputFilename = "img2vid_output.mp4";
				int width = 512, height = 512;
				int video_frames = 25, motion_bucket_id = 127, fps = 6;
				float augmentation_level = 0.0f, min_cfg = 1.0f, cfg_scale = 2.5f;
				float strength = 0.8f;
				int sample_steps = 20, seed = -1;
				sample_method_t sample_method = EULER;

				// Debug logging for metadata
				std::cout << "Img2Vid metadata:" << std::endl;
				std::cout << metadata.dump(2) << std::endl;

				// Parse metadata to extract parameters
				if (metadata.contains("components") && metadata["components"].is_array())
				{
					for (const auto &comp : metadata["components"])
					{
						// Input image path
						if (comp.contains("InputImage"))
						{
							nlohmann::json inputImageData = comp["InputImage"];

							if (inputImageData.contains("filePath") && !inputImageData["filePath"].is_null() && !inputImageData["filePath"].get<std::string>().empty())
							{
								inputImagePath = inputImageData["filePath"].get<std::string>();
								std::cout << "Found input path: " << inputImagePath << std::endl;
							}
						}

						// Output settings
						if (comp.contains("OutputVideo"))
						{
							nlohmann::json outputVideoData = comp["OutputVideo"];

							if (outputVideoData.contains("filePath") && !outputVideoData["filePath"].is_null() && !outputVideoData["filePath"].get<std::string>().empty())
							{
								outputPath = outputVideoData["filePath"].get<std::string>();
								std::cout << "Found output path: " << outputPath << std::endl;
							}

							if (outputVideoData.contains("fileName") && !outputVideoData["fileName"].is_null() && !outputVideoData["fileName"].get<std::string>().empty())
							{
								outputFilename = outputVideoData["fileName"].get<std::string>();
								std::cout << "Found output filename: " << outputFilename << std::endl;
							}
						}

						// Video parameters
						if (comp.contains("VideoParams"))
						{
							nlohmann::json videoData = comp["VideoParams"];

							if (videoData.contains("video_frames") && !videoData["video_frames"].is_null())
								video_frames = videoData["video_frames"].get<int>();
							if (videoData.contains("motion_bucket_id") && !videoData["motion_bucket_id"].is_null())
								motion_bucket_id = videoData["motion_bucket_id"].get<int>();
							if (videoData.contains("fps") && !videoData["fps"].is_null())
								fps = videoData["fps"].get<int>();
							if (videoData.contains("augmentation_level") && !videoData["augmentation_level"].is_null())
								augmentation_level = videoData["augmentation_level"].get<float>();
							if (videoData.contains("min_cfg") && !videoData["min_cfg"].is_null())
								min_cfg = videoData["min_cfg"].get<float>();
						}

						// Sampler component
						if (comp.contains("Sampler"))
						{
							nlohmann::json samplerData = comp["Sampler"];

							if (samplerData.contains("cfg") && !samplerData["cfg"].is_null())
								cfg_scale = samplerData["cfg"].get<float>();
							if (samplerData.contains("steps") && !samplerData["steps"].is_null())
								sample_steps = samplerData["steps"].get<int>();
							if (samplerData.contains("seed") && !samplerData["seed"].is_null())
								seed = samplerData["seed"].get<int>();
							if (samplerData.contains("denoise") && !samplerData["denoise"].is_null())
								strength = samplerData["denoise"].get<float>();
							if (samplerData.contains("current_sample_method") && !samplerData["current_sample_method"].is_null())
								sample_method = static_cast<sample_method_t>(samplerData["current_sample_method"].get<int>());
						}

						// Latent component for dimensions
						if (comp.contains("Latent"))
						{
							nlohmann::json latentData = comp["Latent"];

							if (latentData.contains("latentWidth") && !latentData["latentWidth"].is_null())
								width = latentData["latentWidth"].get<int>();
							if (latentData.contains("latentHeight") && !latentData["latentHeight"].is_null())
								height = latentData["latentHeight"].get<int>();
						}
					}
				}

				// Validate parameters
				if (inputImagePath.empty())
				{
					throw std::runtime_error("Input image path is empty!");
				}

				// Check if input image file exists
				if (!std::filesystem::exists(inputImagePath))
				{
					throw std::runtime_error("Input image file does not exist: " + inputImagePath);
				}

				// Load input image
				int inputWidth, inputHeight, inputChannels;
				std::cout << "Loading input image from: " << inputImagePath << std::endl;

				// Force 3 channels (RGB) for consistency with stable-diffusion
				inputData = stbi_load(inputImagePath.c_str(), &inputWidth, &inputHeight, &inputChannels, 3);
				if (!inputData)
				{
					std::string error = std::string("Failed to load input image: ") + inputImagePath + " - " +
						(stbi_failure_reason() ? stbi_failure_reason() : "unknown reason");
					throw std::runtime_error(error);
				}

				// Force channels to 3 since we requested RGB
				inputChannels = 3;

				std::cout << "Input image loaded successfully: " << inputWidth << "x" << inputHeight
					<< " with " << inputChannels << " channels (forced RGB)" << std::endl;

				// Validate image dimensions
				if (inputWidth <= 0 || inputHeight <= 0)
				{
					throw std::runtime_error("Invalid input image dimensions: " + std::to_string(inputWidth) + "x" + std::to_string(inputHeight));
				}

				// Create output path
				std::filesystem::path outputDir(outputPath);
				std::filesystem::path outputFile(outputFilename);
				std::string uniqueFilePath = Utils::PngMetadata::CreateUniqueFilename(
					outputFile.string(), outputDir.string());

				// Initialize SD context
				std::cout << "Initializing Stable Diffusion context..." << std::endl;
				sd_context = InitializeStableDiffusionContext(metadata);
				if (!sd_context)
				{
					throw std::runtime_error("Failed to initialize Stable Diffusion context!");
				}

				// Create input image struct
				sd_image_t input_image = {
					static_cast<uint32_t>(inputWidth),
					static_cast<uint32_t>(inputHeight),
					3, // Force 3 channels (RGB)
					inputData };

				// Ensure valid seed
				if (seed < 0)
				{
					seed = static_cast<int>(generateRandomSeed());
					std::cout << "Generated random seed: " << seed << std::endl;
				}

				// Perform img2vid
				std::cout << "Calling img2vid..." << std::endl;
				result_image = img2vid(
					sd_context,
					input_image,
					width,
					height,
					video_frames,
					motion_bucket_id,
					fps,
					augmentation_level,
					min_cfg,
					cfg_scale,
					sample_method,
					sample_steps,
					strength,
					static_cast<int64_t>(seed));

				if (!result_image)
				{
					throw std::runtime_error("img2vid failed - no output produced");
				}

				if (!result_image->data)
				{
					free(result_image);
					throw std::runtime_error("img2vid produced invalid data");
				}

				std::cout << "img2vid successful: " << result_image->width << "x" << result_image->height
					<< "x" << result_image->channel << ", saving frames to: " << fullPath << std::endl;

				// For video output, we need to handle multiple frames
				// The result_image contains all frames concatenated
				// We need to save them as individual frames or as a video file

				// Calculate frame dimensions
				uint32_t frame_width = result_image->width;
				uint32_t frame_height = result_image->height / video_frames; // Frames are stacked vertically
				uint32_t frame_channels = result_image->channel;

				std::cout << "Video output: " << video_frames << " frames of "
					<< frame_width << "x" << frame_height << "x" << frame_channels << std::endl;

				// Save individual frames
				for (int frame_idx = 0; frame_idx < video_frames; ++frame_idx)
				{
					// Calculate frame data offset
					size_t frame_size = frame_width * frame_height * frame_channels;
					unsigned char* frame_data = result_image->data + (frame_idx * frame_size);

					// Create frame filename
					std::filesystem::path frameDir(outputPath);
					std::string frameFilename = "frame_" + std::to_string(frame_idx) + ".png";
					std::string frameFullPath = (frameDir / frameFilename).string();

					// Save frame
					SaveImage(frame_data, frame_width, frame_height, frame_channels, metadata, frameFullPath);
				}

				// TODO: Optionally combine frames into video file using FFmpeg
				// This would require additional video encoding functionality

				std::this_thread::sleep_for(std::chrono::milliseconds(50));

				// Cleanup resources
				if (inputData)
				{
					stbi_image_free(inputData);
					inputData = nullptr;
				}

				if (result_image)
				{
					free(result_image);
					result_image = nullptr;
				}

				if (sd_context)
				{
					free_sd_ctx(sd_context);
					sd_context = nullptr;
				}

				return true;
			}
			catch (const std::exception &e)
			{
				std::cerr << "Exception during img2vid: " << e.what() << std::endl;

				// Clean up resources
				if (inputData)
				{
					stbi_image_free(inputData);
					inputData = nullptr;
				}

				if (result_image)
				{
					free(result_image);
					result_image = nullptr;
				}

				if (sd_context)
				{
					free_sd_ctx(sd_context);
					sd_context = nullptr;
				}

				return false;
			}
		}
	};
} // namespace Utils