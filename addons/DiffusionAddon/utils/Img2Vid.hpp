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
			sd_image_t *result_images = nullptr;
			int num_frames_out = 0;
			sd_vid_gen_params_t vid_params;
			sd_vid_gen_params_init(&vid_params);

			try
			{

				// Extract parameters from metadata - FIXED: Use local strings
				std::string inputImagePath = "";
				std::string outputPath = Utils::FilePaths::defaultProjectPath;
				std::string outputFilename = "img2vid_output";
				std::string posPrompt = "";
				std::string negPrompt = "";

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

						// Prompt component
						if (comp.contains("Prompt"))
						{
							nlohmann::json promptData = comp["Prompt"];

							if (promptData.contains("posPrompt") && !promptData["posPrompt"].is_null())
								posPrompt = promptData["posPrompt"].get<std::string>();
							if (promptData.contains("negPrompt") && !promptData["negPrompt"].is_null())
								negPrompt = promptData["negPrompt"].get<std::string>();
						}

						// ClipSkip component
						if (comp.contains("ClipSkip"))
						{
							nlohmann::json clipSkipData = comp["ClipSkip"];

							if (clipSkipData.contains("clipSkip") && !clipSkipData["clipSkip"].is_null())
								vid_params.clip_skip = clipSkipData["clipSkip"].get<int>();
						}

						// Video parameters
						if (comp.contains("VideoParams"))
						{
							nlohmann::json videoData = comp["VideoParams"];

							if (videoData.contains("video_frames") && !videoData["video_frames"].is_null())
								vid_params.video_frames = videoData["video_frames"].get<int>();
							if (videoData.contains("vace_strength") && !videoData["vace_strength"].is_null())
								vid_params.vace_strength = videoData["vace_strength"].get<float>();
							if (videoData.contains("moe_boundary") && !videoData["moe_boundary"].is_null())
								vid_params.moe_boundary = videoData["moe_boundary"].get<float>();
						}

						// Sampler component - extract core sampling parameters
						if (comp.contains("Sampler"))
						{
							nlohmann::json samplerData = comp["Sampler"];

							if (samplerData.contains("seed") && !samplerData["seed"].is_null())
								vid_params.seed = static_cast<int64_t>(samplerData["seed"].get<int>());
							if (samplerData.contains("denoise") && !samplerData["denoise"].is_null())
								vid_params.strength = samplerData["denoise"].get<float>();

							// Extract sample_params fields for BOTH sample_params and high_noise_sample_params
							if (samplerData.contains("steps") && !samplerData["steps"].is_null())
							{
								vid_params.sample_params.sample_steps = samplerData["steps"].get<int>();
								vid_params.high_noise_sample_params.sample_steps = samplerData["steps"].get<int>();
							}
							if (samplerData.contains("eta") && !samplerData["eta"].is_null())
							{
								vid_params.sample_params.eta = samplerData["eta"].get<float>();
								vid_params.high_noise_sample_params.eta = samplerData["eta"].get<float>();
							}

							// Extract method selections
							if (samplerData.contains("current_sample_method") && !samplerData["current_sample_method"].is_null())
							{
								vid_params.sample_params.sample_method = static_cast<sample_method_t>(samplerData["current_sample_method"].get<int>());
								vid_params.high_noise_sample_params.sample_method = vid_params.sample_params.sample_method;
							}
							if (samplerData.contains("current_scheduler_method") && !samplerData["current_scheduler_method"].is_null())
							{
								vid_params.sample_params.scheduler = static_cast<scheduler_t>(samplerData["current_scheduler_method"].get<int>());
								vid_params.high_noise_sample_params.scheduler = vid_params.sample_params.scheduler;
							}
						}

						// HighNoiseSampler component - override high_noise_sample_params if present
						if (comp.contains("HighNoiseSampler"))
						{
							nlohmann::json highNoiseSamplerData = comp["HighNoiseSampler"];

							if (highNoiseSamplerData.contains("high_noise_sample_method") && !highNoiseSamplerData["high_noise_sample_method"].is_null())
								vid_params.high_noise_sample_params.sample_method = static_cast<sample_method_t>(highNoiseSamplerData["high_noise_sample_method"].get<int>());
							if (highNoiseSamplerData.contains("high_noise_scheduler_method") && !highNoiseSamplerData["high_noise_scheduler_method"].is_null())
								vid_params.high_noise_sample_params.scheduler = static_cast<scheduler_t>(highNoiseSamplerData["high_noise_scheduler_method"].get<int>());
							if (highNoiseSamplerData.contains("high_noise_steps") && !highNoiseSamplerData["high_noise_steps"].is_null())
								vid_params.high_noise_sample_params.sample_steps = highNoiseSamplerData["high_noise_steps"].get<int>();
							if (highNoiseSamplerData.contains("high_noise_eta") && !highNoiseSamplerData["high_noise_eta"].is_null())
								vid_params.high_noise_sample_params.eta = highNoiseSamplerData["high_noise_eta"].get<float>();
						}

						// HighNoiseSampler component - override high_noise_sample_params if present
						if (comp.contains("HighNoiseSampler"))
						{
							nlohmann::json highNoiseSamplerData = comp["HighNoiseSampler"];

							if (highNoiseSamplerData.contains("high_noise_sample_method") && !highNoiseSamplerData["high_noise_sample_method"].is_null())
								vid_params.high_noise_sample_params.sample_method = static_cast<sample_method_t>(highNoiseSamplerData["high_noise_sample_method"].get<int>());
							if (highNoiseSamplerData.contains("high_noise_scheduler_method") && !highNoiseSamplerData["high_noise_scheduler_method"].is_null())
								vid_params.high_noise_sample_params.scheduler = static_cast<scheduler_t>(highNoiseSamplerData["high_noise_scheduler_method"].get<int>());
							if (highNoiseSamplerData.contains("high_noise_steps") && !highNoiseSamplerData["high_noise_steps"].is_null())
								vid_params.high_noise_sample_params.sample_steps = highNoiseSamplerData["high_noise_steps"].get<int>();
							if (highNoiseSamplerData.contains("high_noise_eta") && !highNoiseSamplerData["high_noise_eta"].is_null())
								vid_params.high_noise_sample_params.eta = highNoiseSamplerData["high_noise_eta"].get<float>();
						}

						// Guidance component - map to BOTH sample_params and high_noise_sample_params
						if (comp.contains("Guidance"))
						{
							nlohmann::json guidanceData = comp["Guidance"];

							if (guidanceData.contains("txt_cfg") && !guidanceData["txt_cfg"].is_null())
							{
								vid_params.sample_params.guidance.txt_cfg = guidanceData["txt_cfg"].get<float>();
								vid_params.high_noise_sample_params.guidance.txt_cfg = guidanceData["txt_cfg"].get<float>();
							}
							if (guidanceData.contains("img_cfg") && !guidanceData["img_cfg"].is_null())
							{
								vid_params.sample_params.guidance.img_cfg = guidanceData["img_cfg"].get<float>();
								vid_params.high_noise_sample_params.guidance.img_cfg = guidanceData["img_cfg"].get<float>();
							}
							if (guidanceData.contains("distilled_guidance") && !guidanceData["distilled_guidance"].is_null())
							{
								vid_params.sample_params.guidance.distilled_guidance = guidanceData["distilled_guidance"].get<float>();
								vid_params.high_noise_sample_params.guidance.distilled_guidance = guidanceData["distilled_guidance"].get<float>();
							}
						}

						// SLG component - map to BOTH sample_params and high_noise_sample_params
						if (comp.contains("SLG"))
						{
							nlohmann::json slgData = comp["SLG"];

							if (slgData.contains("layer_start") && !slgData["layer_start"].is_null())
							{
								vid_params.sample_params.guidance.slg.layer_start = slgData["layer_start"].get<float>();
								vid_params.high_noise_sample_params.guidance.slg.layer_start = slgData["layer_start"].get<float>();
							}
							if (slgData.contains("layer_end") && !slgData["layer_end"].is_null())
							{
								vid_params.sample_params.guidance.slg.layer_end = slgData["layer_end"].get<float>();
								vid_params.high_noise_sample_params.guidance.slg.layer_end = slgData["layer_end"].get<float>();
							}
							if (slgData.contains("scale") && !slgData["scale"].is_null())
							{
								vid_params.sample_params.guidance.slg.scale = slgData["scale"].get<float>();
								vid_params.high_noise_sample_params.guidance.slg.scale = slgData["scale"].get<float>();
							}

							// Handle layers array for SLG - apply to both sample params
							if (slgData.contains("layers") && slgData["layers"].is_array() &&
								slgData.contains("layer_count") && !slgData["layer_count"].is_null())
							{
								size_t layer_count = slgData["layer_count"].get<size_t>();
								if (layer_count > 0 && slgData["layers"].size() >= layer_count)
								{
									// Allocate and copy layers array for both sample params
									vid_params.sample_params.guidance.slg.layers = new int[layer_count];
									vid_params.high_noise_sample_params.guidance.slg.layers = new int[layer_count];

									vid_params.sample_params.guidance.slg.layer_count = layer_count;
									vid_params.high_noise_sample_params.guidance.slg.layer_count = layer_count;

									for (size_t i = 0; i < layer_count; i++)
									{
										int layer_val = slgData["layers"][i].get<int>();
										vid_params.sample_params.guidance.slg.layers[i] = layer_val;
										vid_params.high_noise_sample_params.guidance.slg.layers[i] = layer_val;
									}
								}
							}
						}

						// Latent component for dimensions
						if (comp.contains("Latent"))
						{
							nlohmann::json latentData = comp["Latent"];

							if (latentData.contains("latentWidth") && !latentData["latentWidth"].is_null())
								vid_params.width = latentData["latentWidth"].get<int>();
							if (latentData.contains("latentHeight") && !latentData["latentHeight"].is_null())
								vid_params.height = latentData["latentHeight"].get<int>();
						}
					}
				}

				// Set prompt strings - ALWAYS use c_str(), NEVER nullptr
				vid_params.prompt = posPrompt.c_str();
				vid_params.negative_prompt = negPrompt.c_str();

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

				// Set input image in video parameters
				vid_params.init_image = {
					static_cast<uint32_t>(inputWidth),
					static_cast<uint32_t>(inputHeight),
					3, // Force 3 channels (RGB)
					inputData
				};

				// Initialize SD context
				std::cout << "Initializing Stable Diffusion context..." << std::endl;
				sd_context = InitializeStableDiffusionContext(metadata);
				if (!sd_context)
				{
					throw std::runtime_error("Failed to initialize Stable Diffusion context!");
				}

				// Ensure valid seed
				if (vid_params.seed < 0)
				{
					vid_params.seed = static_cast<int64_t>(generateRandomSeed());
					std::cout << "Generated random seed: " << vid_params.seed << std::endl;
				}

				// Perform img2vid using NEW structured API
				std::cout << "Calling generate_video for img2vid generation..." << std::endl;
				result_images = generate_video(sd_context, &vid_params, &num_frames_out);

				if (!result_images)
				{
					throw std::runtime_error("generate_video failed - no output produced");
				}

				if (!result_images[0].data)
				{
					free(result_images);
					throw std::runtime_error("generate_video produced invalid data");
				}

				std::cout << "img2vid successful: " << result_images[0].width << "x" << result_images[0].height
					<< "x" << result_images[0].channel << ", " << num_frames_out << " frames generated" << std::endl;

				// Save individual frames
				for (int frame_idx = 0; frame_idx < num_frames_out; ++frame_idx)
				{
					// Create frame filename
					std::filesystem::path frameDir(outputPath);
					std::string frameFilename = outputFilename + "_frame_" + std::to_string(frame_idx) + ".png";
					std::string frameFullPath = (frameDir / frameFilename).string();

					// Save frame
					SaveImage(result_images[frame_idx].data,
						result_images[frame_idx].width,
						result_images[frame_idx].height,
						result_images[frame_idx].channel,
						metadata,
						frameFullPath);
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(50));

				// Cleanup SLG layers arrays if they were allocated
				if (vid_params.sample_params.guidance.slg.layers != nullptr)
				{
					delete[] vid_params.sample_params.guidance.slg.layers;
					vid_params.sample_params.guidance.slg.layers = nullptr;
				}
				if (vid_params.high_noise_sample_params.guidance.slg.layers != nullptr)
				{
					delete[] vid_params.high_noise_sample_params.guidance.slg.layers;
					vid_params.high_noise_sample_params.guidance.slg.layers = nullptr;
				}

				// Cleanup resources
				if (inputData)
				{
					stbi_image_free(inputData);
					inputData = nullptr;
				}

				if (result_images)
				{
					// Free each frame's data
					for (int i = 0; i < num_frames_out; ++i)
					{
						if (result_images[i].data)
						{
							free(result_images[i].data);
						}
					}
					free(result_images);
					result_images = nullptr;
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

				// Clean up SLG layers arrays if they were allocated
				if (vid_params.sample_params.guidance.slg.layers != nullptr)
				{
					delete[] vid_params.sample_params.guidance.slg.layers;
					vid_params.sample_params.guidance.slg.layers = nullptr;
				}
				if (vid_params.high_noise_sample_params.guidance.slg.layers != nullptr)
				{
					delete[] vid_params.high_noise_sample_params.guidance.slg.layers;
					vid_params.high_noise_sample_params.guidance.slg.layers = nullptr;
				}

				// Clean up resources
				if (inputData)
				{
					stbi_image_free(inputData);
					inputData = nullptr;
				}

				if (result_images)
				{
					for (int i = 0; i < num_frames_out; ++i)
					{
						if (result_images[i].data)
						{
							free(result_images[i].data);
						}
					}
					free(result_images);
					result_images = nullptr;
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