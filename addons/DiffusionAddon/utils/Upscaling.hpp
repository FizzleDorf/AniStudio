#pragma once

#include "stable-diffusion.h"
#include "PngMetadataUtils.hpp"
#include "RngUtils.hpp"
#include "ContextUtils.hpp"
#include "SaveUtils.hpp"
#include "FilePathService.hpp"
#include "pch.h"
#include <stb_image.h>
#include <stb_image_write.h>
#include <iostream>
#include <filesystem>
#include <thread>
#include <cstdlib>

namespace Utils
{
    void SaveImage(const unsigned char* data, int width, int height, int channels,
        const nlohmann::json& metadata, const std::string& fullPath);

    class Upscaling
    {
    public:
        static bool RunUpscaling(const nlohmann::json& metadata, const std::string& fullPath, sd_ctx_t* sd_context = nullptr)
        {
            (void)sd_context; // Mark as unused

            upscaler_ctx_t* upscaler_context = nullptr;
            unsigned char* inputData = nullptr;
            sd_image_t* upscaled_images = nullptr;
            int num_upscaled = 0;

            try
            {
                // Extract parameters from metadata
                std::string inputImagePath = "";
                std::string modelPath = Utils::FilePathService::GetPath("Upscale");
                std::string outputPath = Utils::FilePathService::GetPath("DefaultProject");
                std::string outputFilename = "upscale_AniStudio.png";
                uint32_t upscaleFactor = 4;
                bool direct = false;
                int n_threads = 4;
                int tile_size = 64;
                const char* backend = nullptr;
                const char* params_backend = nullptr;

                // Parse metadata to extract parameters
                if (metadata.contains("components") && metadata["components"].is_array())
                {
                    for (const auto& comp : metadata["components"])
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
                        if (comp.contains("OutputImage"))
                        {
                            nlohmann::json outputImageData = comp["OutputImage"];
                            if (outputImageData.contains("filePath") && !outputImageData["filePath"].is_null() && !outputImageData["filePath"].get<std::string>().empty())
                            {
                                outputPath = outputImageData["filePath"].get<std::string>();
                                std::cout << "Found output path: " << outputPath << std::endl;
                            }
                            if (outputImageData.contains("fileName") && !outputImageData["fileName"].is_null() && !outputImageData["fileName"].get<std::string>().empty())
                            {
                                outputFilename = outputImageData["fileName"].get<std::string>();
                                std::cout << "Found output filename: " << outputFilename << std::endl;
                            }
                        }

                        // Esrgan component
                        if (comp.contains("Esrgan"))
                        {
                            nlohmann::json esrganData = comp["Esrgan"];

                            if (esrganData.contains("modelPath") && !esrganData["modelPath"].is_null() && !esrganData["modelPath"].get<std::string>().empty())
                            {
                                modelPath = esrganData["modelPath"].get<std::string>();
                            }
                            else if (esrganData.contains("modelName") && !esrganData["modelName"].is_null() && !esrganData["modelName"].get<std::string>().empty())
                            {
                                std::string modelName = esrganData["modelName"].get<std::string>();
                                std::string upscaleDir = Utils::FilePathService::GetPath("Upscale");
                                if (!upscaleDir.empty() && upscaleDir[0] != '\0') {
                                    modelPath = (std::filesystem::path(upscaleDir) / modelName).string();
                                }
                            }

                            if (esrganData.contains("upscaleFactor"))
                                upscaleFactor = esrganData["upscaleFactor"].get<uint32_t>();

                            if (esrganData.contains("direct"))
                                direct = esrganData["direct"].get<bool>();
                            if (esrganData.contains("n_threads"))
                                n_threads = esrganData["n_threads"].get<int>();
                            if (esrganData.contains("tile_size"))
                                tile_size = esrganData["tile_size"].get<int>();
                            if (esrganData.contains("backend") && !esrganData["backend"].is_null())
                                backend = esrganData["backend"].get<std::string>().c_str();
                            if (esrganData.contains("params_backend") && !esrganData["params_backend"].is_null())
                                params_backend = esrganData["params_backend"].get<std::string>().c_str();
                        }

                        // Sampler component (some upscalers might use SD context)
                        if (comp.contains("Sampler"))
                        {
                            nlohmann::json samplerData = comp["Sampler"];
                            if (samplerData.contains("n_threads"))
                                n_threads = samplerData["n_threads"].get<int>();
                        }
                    }
                }

                // Validate parameters
                if (inputImagePath.empty())
                {
                    throw std::runtime_error("Input image path is empty!");
                }

                if (modelPath.empty())
                {
                    throw std::runtime_error("ESRGAN model path is empty!");
                }

                // Check if model file exists
                if (!std::filesystem::exists(modelPath)) {
                    std::cerr << "Warning: ESRGAN model file not found: " << modelPath << std::endl;
                    std::string upscaleDir = Utils::FilePathService::GetPath("Upscale");
                    if (!upscaleDir.empty()) {
                        std::string altPath = (std::filesystem::path(upscaleDir) / std::filesystem::path(modelPath).filename()).string();
                        if (std::filesystem::exists(altPath)) {
                            std::cout << "Found model at alternative path: " << altPath << std::endl;
                            modelPath = altPath;
                        }
                    }
                }

                // Load input image - force 3 channels for consistency
                int inputWidth, inputHeight, inputChannels;
                std::cout << "Loading input image from: " << inputImagePath << std::endl;
                inputData = stbi_load(inputImagePath.c_str(), &inputWidth, &inputHeight, &inputChannels, 3);
                if (!inputData)
                {
                    std::string error = std::string("Failed to load input image: ") + inputImagePath + " - " +
                        (stbi_failure_reason() ? stbi_failure_reason() : "unknown reason");
                    throw std::runtime_error(error);
                }

                inputChannels = 3;
                std::cout << "Input image loaded: " << inputWidth << "x" << inputHeight
                    << " with " << inputChannels << " channels (forced RGB)" << std::endl;

                // Determine output file path
                std::string outputFilePath;
                if (!fullPath.empty())
                {
                    outputFilePath = fullPath;
                }
                else
                {
                    std::filesystem::path outputDir(outputPath);
                    std::filesystem::path outputFile(outputFilename);
                    if (outputPath.empty() || outputPath[0] == '\0' || !std::filesystem::exists(outputDir)) {
                        std::string outputFolder = Utils::FilePathService::GetPath("OutputFolder");
                        if (!outputFolder.empty() && outputFolder[0] != '\0' && std::filesystem::exists(outputFolder)) {
                            outputDir = outputFolder;
                        }
                    }
                    outputFilePath = Utils::PngMetadata::CreateUniqueFilename(
                        outputFile.string(), outputDir.string());
                }

                std::cout << "Output will be saved to: " << outputFilePath << std::endl;

                // Initialize upscaler context (updated API)
                std::cout << "Initializing upscaler with model: " << modelPath << std::endl;
                std::cout << "Parameters: threads=" << n_threads
                    << ", direct=" << direct
                    << ", tile_size=" << tile_size << std::endl;

                upscaler_context = new_upscaler_ctx(modelPath.c_str(),
                    direct,
                    n_threads,
                    tile_size,
                    backend,
                    params_backend);

                if (!upscaler_context)
                {
                    throw std::runtime_error("Failed to initialize upscaler context!");
                }

                // Check actual upscale factor from context
                int actual_upscale_factor = get_upscale_factor(upscaler_context);
                if (actual_upscale_factor > 0 && actual_upscale_factor != upscaleFactor) {
                    std::cout << "Note: Model supports upscale factor " << actual_upscale_factor
                        << ", overriding requested factor " << upscaleFactor << std::endl;
                    upscaleFactor = actual_upscale_factor;
                }

                // Create input image struct
                sd_image_t input_image = {
                    static_cast<uint32_t>(inputWidth),
                    static_cast<uint32_t>(inputHeight),
                    static_cast<uint32_t>(inputChannels),
                    inputData
                };

                bool success = upscale(upscaler_context, input_image, upscaleFactor,
                    &upscaled_images, &num_upscaled);
                if (!success || num_upscaled == 0 || !upscaled_images)
                {
                    throw std::runtime_error("Upscaling failed - no output image produced");
                }

                // Use the first output image (the library usually returns exactly one)
                sd_image_t& upscaled_image = upscaled_images[0];
                std::cout << "Upscaling successful: " << upscaled_image.width << "x"
                    << upscaled_image.height << "x" << upscaled_image.channel << std::endl;

                // Save the upscaled image
                SaveImage(upscaled_image.data, upscaled_image.width, upscaled_image.height,
                    upscaled_image.channel, metadata, outputFilePath);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                // Cleanup resources
                if (inputData)
                {
                    stbi_image_free(inputData);
                    inputData = nullptr;
                }
                if (upscaled_images)
                {
                    // Use the library's dedicated free function
                    free_sd_images(upscaled_images, num_upscaled);
                    upscaled_images = nullptr;
                }
                if (upscaler_context)
                {
                    free_upscaler_ctx(upscaler_context);
                    upscaler_context = nullptr;
                }

                return true;
            }
            catch (const std::exception& e)
            {
                std::cerr << "Exception during upscaling: " << e.what() << std::endl;

                if (inputData)
                {
                    stbi_image_free(inputData);
                    inputData = nullptr;
                }
                if (upscaled_images)
                {
                    free_sd_images(upscaled_images, num_upscaled);
                    upscaled_images = nullptr;
                }
                if (upscaler_context)
                {
                    free_upscaler_ctx(upscaler_context);
                    upscaler_context = nullptr;
                }

                return false;
            }
        }
    };
} // namespace Utils