#pragma once

#include "stable-diffusion.h"
#include "PngMetadataUtils.hpp"
#include "pch.h"
#include "RngUtils.hpp"
#include "ContextUtils.hpp" 
#include "SaveUtils.hpp"
#include "FilePaths.hpp"  // Added include for new FilePaths
#include <stb_image.h>
#include <stb_image_write.h>

namespace Utils
{
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
			unsigned char *endInputData = nullptr;
			sd_image_t *result_images = nullptr;
			int num_frames_out = 0;
			sd_vid_gen_params_t vid_params;
			sd_vid_gen_params_init(&vid_params);

			try
			{
				// Get FilePaths instance
				FilePaths& filePaths = FilePaths::GetInstance();

				std::string inputImagePath = "";
				std::string endImagePath = "";
				std::string outputPath = filePaths.GetPath("DefaultProject");  // Updated to use new API
				std::string outputFilename = "img2vid_output";
				std::string posPrompt = "";
				std::string negPrompt = "";
				int latentWidth = 0;
				int latentHeight = 0;

				if (metadata.contains("components") && metadata["components"].is_array())
				{
					for (const auto &comp : metadata["components"])
					{
						if (comp.contains("InputImage"))
						{
							nlohmann::json inputImageData = comp["InputImage"];
							if (inputImageData.contains("filePath") && !inputImageData["filePath"].is_null())
								inputImagePath = inputImageData["filePath"].get<std::string>();
						}

						if (comp.contains("EndImage"))
						{
							nlohmann::json endImageData = comp["EndImage"];
							if (endImageData.contains("filePath") && !endImageData["filePath"].is_null())
								endImagePath = endImageData["filePath"].get<std::string>();
						}

						if (comp.contains("OutputVideo"))
						{
							nlohmann::json outputVideoData = comp["OutputVideo"];
							if (outputVideoData.contains("filePath") && !outputVideoData["filePath"].is_null())
								outputPath = outputVideoData["filePath"].get<std::string>();
							if (outputVideoData.contains("fileName") && !outputVideoData["fileName"].is_null())
								outputFilename = outputVideoData["fileName"].get<std::string>();
						}

						if (comp.contains("Prompt"))
						{
							nlohmann::json promptData = comp["Prompt"];
							if (promptData.contains("posPrompt") && !promptData["posPrompt"].is_null())
								posPrompt = promptData["posPrompt"].get<std::string>();
							if (promptData.contains("negPrompt") && !promptData["negPrompt"].is_null())
								negPrompt = promptData["negPrompt"].get<std::string>();
						}

						if (comp.contains("ClipSkip"))
						{
							nlohmann::json clipSkipData = comp["ClipSkip"];
							if (clipSkipData.contains("clipSkip") && !clipSkipData["clipSkip"].is_null())
								vid_params.clip_skip = clipSkipData["clipSkip"].get<int>();
						}

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

						if (comp.contains("Sampler"))
						{
							nlohmann::json samplerData = comp["Sampler"];
							if (samplerData.contains("seed") && !samplerData["seed"].is_null())
								vid_params.seed = static_cast<int64_t>(samplerData["seed"].get<int>());
							if (samplerData.contains("denoise") && !samplerData["denoise"].is_null())
								vid_params.strength = samplerData["denoise"].get<float>();
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
						}

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
						}

						if (comp.contains("Latent"))
						{
							nlohmann::json latentData = comp["Latent"];
							if (latentData.contains("latentWidth") && !latentData["latentWidth"].is_null())
								latentWidth = latentData["latentWidth"].get<int>();
							if (latentData.contains("latentHeight") && !latentData["latentHeight"].is_null())
								latentHeight = latentData["latentHeight"].get<int>();
						}
					}
				}

				vid_params.prompt = posPrompt.c_str();
				vid_params.negative_prompt = negPrompt.c_str();

				if (inputImagePath.empty()) {
					throw std::runtime_error("Input image path is empty!");
				}

				if (!std::filesystem::exists(inputImagePath)) {
					throw std::runtime_error("Input image file not found: " + inputImagePath);
				}

				std::cout << "=== Img2Vid Debug ===" << std::endl;
				std::cout << "Input image path: " << inputImagePath << std::endl;
				std::cout << "End image path: " << endImagePath << std::endl;

				int inputWidth, inputHeight, inputChannels;
				inputData = stbi_load(inputImagePath.c_str(), &inputWidth, &inputHeight, &inputChannels, 3);
				if (!inputData) {
					throw std::runtime_error("Failed to load input image: " + inputImagePath);
				}
				inputChannels = 3;

				std::cout << "Loaded input image: " << inputWidth << "x" << inputHeight << std::endl;

				if (latentWidth <= 0 || latentHeight <= 0) {
					latentWidth = inputWidth;
					latentHeight = inputHeight;
				}

				if (latentWidth != inputWidth || latentHeight != inputHeight) {
					std::cout << "WARNING: Latent dimensions (" << latentWidth << "x" << latentHeight
						<< ") don't match input image (" << inputWidth << "x" << inputHeight << ")" << std::endl;
					std::cout << "Auto-adjusting latent to match input image..." << std::endl;
					latentWidth = inputWidth;
					latentHeight = inputHeight;
				}

				vid_params.width = static_cast<uint32_t>(latentWidth);
				vid_params.height = static_cast<uint32_t>(latentHeight);

				sd_image_t input_image = {
					static_cast<uint32_t>(inputWidth),
					static_cast<uint32_t>(inputHeight),
					3,
					inputData };

				vid_params.init_image = input_image;

				if (!endImagePath.empty() && std::filesystem::exists(endImagePath)) {
					int endWidth, endHeight, endChannels;
					endInputData = stbi_load(endImagePath.c_str(), &endWidth, &endHeight, &endChannels, 3);
					if (endInputData) {
						endChannels = 3;
						if (endWidth == inputWidth && endHeight == inputHeight) {
							vid_params.end_image = {
								static_cast<uint32_t>(endWidth),
								static_cast<uint32_t>(endHeight),
								3,
								endInputData
							};
							std::cout << "Loaded end image: " << endWidth << "x" << endHeight << std::endl;
						}
						else {
							stbi_image_free(endInputData);
							endInputData = nullptr;
							std::cout << "End image dimensions don't match input, ignoring end image" << std::endl;
						}
					}
				}
				else {
					vid_params.end_image = { 0, 0, 0, nullptr };
				}

				vid_params.control_frames = nullptr;
				vid_params.control_frames_size = 0;

				std::cout << "Final settings:" << std::endl;
				std::cout << "  - Input image: " << inputWidth << "x" << inputHeight << std::endl;
				std::cout << "  - Target size: " << vid_params.width << "x" << vid_params.height << std::endl;
				std::cout << "  - Strength: " << vid_params.strength << std::endl;
				std::cout << "  - Video frames: " << vid_params.video_frames << std::endl;
				std::cout << "=====================" << std::endl;

				sd_context = InitializeStableDiffusionContext(metadata);
				if (!sd_context) {
					throw std::runtime_error("Failed to initialize Stable Diffusion context!");
				}

				if (vid_params.seed < 0) {
					vid_params.seed = static_cast<int64_t>(generateRandomSeed());
				}

				result_images = generate_video(sd_context, &vid_params, &num_frames_out);
				if (!result_images || !result_images[0].data) {
					throw std::runtime_error("generate_video failed");
				}

				std::cout << "Generated " << num_frames_out << " frames" << std::endl;

				// Ensure output directory exists with proper fallback
				std::filesystem::path frameDir(outputPath);

				// Check if output directory exists, if not use a fallback
				if (outputPath.empty() || outputPath[0] == '\0' || !std::filesystem::exists(frameDir)) {
					// Try to use OutputFolder from FilePaths
					std::string outputFolder = filePaths.GetPath("OutputFolder");
					if (!outputFolder.empty() && outputFolder[0] != '\0' && std::filesystem::exists(outputFolder)) {
						frameDir = outputFolder;
					}
					else {
						// Fallback to executable directory
						frameDir = filePaths.GetExecutableDir();
					}
				}

				// Create the directory if it doesn't exist
				std::filesystem::create_directories(frameDir);

				for (int frame_idx = 0; frame_idx < num_frames_out; ++frame_idx) {
					std::string frameFilename = outputFilename + "_frame_" + std::to_string(frame_idx) + ".png";
					std::string frameFullPath = (frameDir / frameFilename).string();

					SaveImage(result_images[frame_idx].data,
						result_images[frame_idx].width,
						result_images[frame_idx].height,
						result_images[frame_idx].channel,
						metadata,
						frameFullPath);
				}

				if (inputData) {
					stbi_image_free(inputData);
					inputData = nullptr;
				}
				if (endInputData) {
					stbi_image_free(endInputData);
					endInputData = nullptr;
				}
				if (result_images) {
					for (int i = 0; i < num_frames_out; ++i) {
						if (result_images[i].data) {
							free(result_images[i].data);
						}
					}
					free(result_images);
					result_images = nullptr;
				}
				if (sd_context) {
					free_sd_ctx(sd_context);
					sd_context = nullptr;
				}

				return true;
			}
			catch (const std::exception &e)
			{
				std::cerr << "Exception during img2vid: " << e.what() << std::endl;

				if (inputData) {
					stbi_image_free(inputData);
					inputData = nullptr;
				}
				if (endInputData) {
					stbi_image_free(endInputData);
					endInputData = nullptr;
				}
				if (result_images) {
					for (int i = 0; i < num_frames_out; ++i) {
						if (result_images[i].data) {
							free(result_images[i].data);
						}
					}
					free(result_images);
					result_images = nullptr;
				}
				if (sd_context) {
					free_sd_ctx(sd_context);
					sd_context = nullptr;
				}

				return false;
			}
		}
	};
}