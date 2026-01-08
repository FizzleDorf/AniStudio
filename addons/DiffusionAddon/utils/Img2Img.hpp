#pragma once

#include "stable-diffusion.h"
#include "PngMetadataUtils.hpp"
#include "pch.h"
#include "RngUtils.hpp"
#include "ContextUtils.hpp" 
#include "SaveUtils.hpp"
#include "FilePathService.hpp"
#include <stb_image.h>
#include <stb_image_write.h>

namespace Utils
{
	uint64_t generateRandomSeed();
	void SaveImage(const unsigned char *data, int width, int height, int channels,
		const nlohmann::json &metadata, const std::string &fullPath);
	sd_ctx_t *InitializeStableDiffusionContext(const nlohmann::json &metadata);

	class Img2Img
	{
	public:
		static bool RunImg2Img(const nlohmann::json &metadata, std::string fullPath)
		{
			sd_ctx_t *sd_context = nullptr;
			unsigned char *inputData = nullptr;
			unsigned char *maskData = nullptr;
			unsigned char *emptyMaskData = nullptr;
			sd_image_t *result_image = nullptr;

			try
			{

				std::string inputImagePath = "";
				std::string maskImagePath = "";
				std::string outputPath = Utils::FilePathService::GetPath("DefaultProject");
				std::string outputFilename = "img2img_output.png";
				std::string posPrompt = "";
				std::string negPrompt = "";
				int latentWidth = 0;
				int latentHeight = 0;

				sd_img_gen_params_t gen_params;
				sd_img_gen_params_init(&gen_params);

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

						if (comp.contains("MaskImage"))
						{
							nlohmann::json maskImageData = comp["MaskImage"];
							if (maskImageData.contains("filePath") && !maskImageData["filePath"].is_null())
								maskImagePath = maskImageData["filePath"].get<std::string>();
						}

						if (comp.contains("OutputImage"))
						{
							nlohmann::json outputImageData = comp["OutputImage"];
							if (outputImageData.contains("filePath") && !outputImageData["filePath"].is_null())
								outputPath = outputImageData["filePath"].get<std::string>();
							if (outputImageData.contains("fileName") && !outputImageData["fileName"].is_null())
								outputFilename = outputImageData["fileName"].get<std::string>();
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
								gen_params.clip_skip = clipSkipData["clipSkip"].get<int>();
						}

						if (comp.contains("Sampler"))
						{
							nlohmann::json samplerData = comp["Sampler"];
							if (samplerData.contains("seed") && !samplerData["seed"].is_null())
								gen_params.seed = static_cast<int64_t>(samplerData["seed"].get<int>());
							if (samplerData.contains("denoise") && !samplerData["denoise"].is_null())
								gen_params.strength = samplerData["denoise"].get<float>();
							if (samplerData.contains("batchSize") && !samplerData["batchSize"].is_null())
								gen_params.batch_count = samplerData["batchSize"].get<int>();
							if (samplerData.contains("steps") && !samplerData["steps"].is_null())
								gen_params.sample_params.sample_steps = samplerData["steps"].get<int>();
							if (samplerData.contains("eta") && !samplerData["eta"].is_null())
								gen_params.sample_params.eta = samplerData["eta"].get<float>();
							if (samplerData.contains("current_sample_method") && !samplerData["current_sample_method"].is_null())
								gen_params.sample_params.sample_method = static_cast<sample_method_t>(samplerData["current_sample_method"].get<int>());
							if (samplerData.contains("current_scheduler_method") && !samplerData["current_scheduler_method"].is_null())
								gen_params.sample_params.scheduler = static_cast<scheduler_t>(samplerData["current_scheduler_method"].get<int>());
						}

						if (comp.contains("Guidance"))
						{
							nlohmann::json guidanceData = comp["Guidance"];
							if (guidanceData.contains("txt_cfg") && !guidanceData["txt_cfg"].is_null())
								gen_params.sample_params.guidance.txt_cfg = guidanceData["txt_cfg"].get<float>();
							if (guidanceData.contains("img_cfg") && !guidanceData["img_cfg"].is_null())
								gen_params.sample_params.guidance.img_cfg = guidanceData["img_cfg"].get<float>();
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

				gen_params.prompt = posPrompt.c_str();
				gen_params.negative_prompt = negPrompt.c_str();
				gen_params.control_image = { 0, 0, 0, nullptr };

				if (inputImagePath.empty()) {
					throw std::runtime_error("Input image path is empty!");
				}

				if (!std::filesystem::exists(inputImagePath)) {
					throw std::runtime_error("Input image file not found: " + inputImagePath);
				}

				std::cout << "=== Img2Img Debug ===" << std::endl;
				std::cout << "Input image path: " << inputImagePath << std::endl;

				int inputWidth, inputHeight, inputChannels;
				inputData = stbi_load(inputImagePath.c_str(), &inputWidth, &inputHeight, &inputChannels, 3);
				if (!inputData) {
					throw std::runtime_error("Failed to load input image: " + inputImagePath);
				}
				inputChannels = 3;

				std::cout << "Loaded image dimensions: " << inputWidth << "x" << inputHeight << std::endl;

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

				gen_params.width = static_cast<uint32_t>(latentWidth);
				gen_params.height = static_cast<uint32_t>(latentHeight);

				sd_image_t input_image = {
					static_cast<uint32_t>(inputWidth),
					static_cast<uint32_t>(inputHeight),
					3,
					inputData };

				gen_params.init_image = input_image;

				if (maskImagePath.empty() || !std::filesystem::exists(maskImagePath))
				{
					size_t maskSize = inputWidth * inputHeight;
					emptyMaskData = new unsigned char[maskSize];
					std::memset(emptyMaskData, 255, maskSize);

					gen_params.mask_image.width = static_cast<uint32_t>(inputWidth);
					gen_params.mask_image.height = static_cast<uint32_t>(inputHeight);
					gen_params.mask_image.channel = 1;
					gen_params.mask_image.data = emptyMaskData;
				}
				else
				{
					int maskWidth, maskHeight, maskChannels;
					maskData = stbi_load(maskImagePath.c_str(), &maskWidth, &maskHeight, &maskChannels, 1);

					if (!maskData)
					{
						size_t maskSize = inputWidth * inputHeight;
						emptyMaskData = new unsigned char[maskSize];
						std::memset(emptyMaskData, 255, maskSize);

						gen_params.mask_image.width = static_cast<uint32_t>(inputWidth);
						gen_params.mask_image.height = static_cast<uint32_t>(inputHeight);
						gen_params.mask_image.channel = 1;
						gen_params.mask_image.data = emptyMaskData;
					}
					else
					{
						if (maskWidth != inputWidth || maskHeight != inputHeight)
						{
							stbi_image_free(maskData);
							maskData = nullptr;

							size_t maskSize = inputWidth * inputHeight;
							emptyMaskData = new unsigned char[maskSize];
							std::memset(emptyMaskData, 255, maskSize);

							gen_params.mask_image.width = static_cast<uint32_t>(inputWidth);
							gen_params.mask_image.height = static_cast<uint32_t>(inputHeight);
							gen_params.mask_image.channel = 1;
							gen_params.mask_image.data = emptyMaskData;
						}
						else
						{
							gen_params.mask_image.width = static_cast<uint32_t>(maskWidth);
							gen_params.mask_image.height = static_cast<uint32_t>(maskHeight);
							gen_params.mask_image.channel = 1;
							gen_params.mask_image.data = maskData;
						}
					}
				}

				std::cout << "Final settings:" << std::endl;
				std::cout << "  - Input image: " << inputWidth << "x" << inputHeight << std::endl;
				std::cout << "  - Target size: " << gen_params.width << "x" << gen_params.height << std::endl;
				std::cout << "  - Strength: " << gen_params.strength << std::endl;
				std::cout << "=====================" << std::endl;

				// Create output path with proper fallback
				std::filesystem::path outputDir(outputPath);
				std::filesystem::path outputFile(outputFilename);

				// Check if output directory exists, if not use a fallback
				if (outputPath.empty() || outputPath[0] == '\0' || !std::filesystem::exists(outputDir)) {
					// Try to use OutputFolder from FilePaths
					std::string outputFolder = Utils::FilePathService::GetPath("OutputFolder");
					if (!outputFolder.empty() && outputFolder[0] != '\0' && std::filesystem::exists(outputFolder)) {
						outputDir = outputFolder;
					}
					else {
						// Fallback to executable directory
						outputDir = Utils::FilePathService::GetExecutableDir();
					}
				}

				std::string uniqueFilePath = Utils::PngMetadata::CreateUniqueFilename(
					outputFile.string(), outputDir.string());

				sd_context = InitializeStableDiffusionContext(metadata);
				if (!sd_context) {
					throw std::runtime_error("Failed to initialize Stable Diffusion context!");
				}

				if (gen_params.seed < 0) {
					gen_params.seed = static_cast<int64_t>(generateRandomSeed());
				}

				result_image = generate_image(sd_context, &gen_params);
				if (!result_image || !result_image->data) {
					throw std::runtime_error("generate_image failed");
				}

				SaveImage(result_image->data, result_image->width, result_image->height,
					result_image->channel, metadata, fullPath);

				if (inputData) {
					stbi_image_free(inputData);
					inputData = nullptr;
				}
				if (maskData) {
					stbi_image_free(maskData);
					maskData = nullptr;
				}
				if (emptyMaskData) {
					delete[] emptyMaskData;
					emptyMaskData = nullptr;
				}
				if (result_image) {
					free(result_image);
					result_image = nullptr;
				}
				if (sd_context) {
					free_sd_ctx(sd_context);
					sd_context = nullptr;
				}

				return true;
			}
			catch (const std::exception &e)
			{
				std::cerr << "Exception during img2img: " << e.what() << std::endl;

				if (inputData) {
					stbi_image_free(inputData);
					inputData = nullptr;
				}
				if (maskData) {
					stbi_image_free(maskData);
					maskData = nullptr;
				}
				if (emptyMaskData) {
					delete[] emptyMaskData;
					emptyMaskData = nullptr;
				}
				if (result_image) {
					free(result_image);
					result_image = nullptr;
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