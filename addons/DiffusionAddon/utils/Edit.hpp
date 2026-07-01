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
    uint64_t generateRandomSeed();
    void SaveImage(const unsigned char* data, int width, int height, int channels,
        const nlohmann::json& metadata, const std::string& fullPath);

    class Edit
    {
    public:
        static bool RunEdit(const nlohmann::json& metadata, const std::string& fullPath, sd_ctx_t* sd_context = nullptr)
        {
            bool contextProvided = (sd_context != nullptr);
            std::vector<unsigned char*> refImageData;
            sd_image_t* result_images = nullptr;        // array returned by generate_image
            int num_result_images = 0;
            sd_img_gen_params_t gen_params;
            sd_img_gen_params_init(&gen_params);

            // Additional structures for advanced features
            std::vector<sd_image_t> imagesToCleanup;
            std::vector<sd_image_t> idImagesStorage;
            std::vector<sd_image_t> refImagesStorage; // For multiple reference images
            int* slg_layers = nullptr;

            try
            {
                // Extract parameters from metadata
                std::vector<std::string> refImagePaths;
                std::string outputPath = Utils::FilePathService::GetPath("DefaultProject");
                std::string outputFilename = "edit_output.png";
                std::string posPrompt = "";
                std::string negPrompt = "";

                // VAE tiling parameters
                sd_tiling_params_t vae_tiling_params = { false, 64, 64, 0.0f, 64.0f, 64.0f };

                // PhotoMaker parameters
                sd_pm_params_t pm_params = { nullptr, 0, nullptr, 0.0f };

                // ControlNet image
                sd_image_t control_image = { 0, 0, 0, nullptr };
                float control_strength = 0.0f;

                // Cache parameters
                sd_cache_params_t cache_params;
                sd_cache_params_init(&cache_params);

                // Debug logging for metadata
                std::cout << "Edit metadata:" << std::endl;
                std::cout << metadata.dump(2) << std::endl;

                // Parse metadata to extract parameters
                if (metadata.contains("components") && metadata["components"].is_array())
                {
                    for (const auto& comp : metadata["components"])
                    {
                        // --- Reference Images ---
                        if (comp.contains("ReferenceImages"))
                        {
                            nlohmann::json refImagesData = comp["ReferenceImages"];
                            if (refImagesData.is_array())
                            {
                                for (const auto& refImg : refImagesData)
                                {
                                    if (refImg.contains("filePath") && !refImg["filePath"].is_null() && !refImg["filePath"].get<std::string>().empty())
                                    {
                                        refImagePaths.push_back(refImg["filePath"].get<std::string>());
                                        std::cout << "Found reference image path: " << refImg["filePath"].get<std::string>() << std::endl;
                                    }
                                }
                            }
                            else if (refImagesData.contains("filePath") && !refImagesData["filePath"].is_null())
                            {
                                refImagePaths.push_back(refImagesData["filePath"].get<std::string>());
                                std::cout << "Found single reference image path: " << refImagesData["filePath"].get<std::string>() << std::endl;
                            }
                        }

                        // --- Output Image ---
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

                        // --- Prompt ---
                        if (comp.contains("Prompt"))
                        {
                            nlohmann::json promptData = comp["Prompt"];
                            if (promptData.contains("posPrompt") && !promptData["posPrompt"].is_null())
                                posPrompt = promptData["posPrompt"].get<std::string>();
                            if (promptData.contains("negPrompt") && !promptData["negPrompt"].is_null())
                                negPrompt = promptData["negPrompt"].get<std::string>();
                        }

                        // --- ClipSkip ---
                        if (comp.contains("ClipSkip"))
                        {
                            nlohmann::json clipSkipData = comp["ClipSkip"];
                            if (clipSkipData.contains("clipSkip") && !clipSkipData["clipSkip"].is_null())
                                gen_params.clip_skip = clipSkipData["clipSkip"].get<int>();
                        }

                        // --- Sampler ---
                        if (comp.contains("Sampler"))
                        {
                            nlohmann::json samplerData = comp["Sampler"];

                            // Core generation params
                            if (samplerData.contains("seed") && !samplerData["seed"].is_null())
                                gen_params.seed = static_cast<int64_t>(samplerData["seed"].get<int>());
                            if (samplerData.contains("denoise") && !samplerData["denoise"].is_null())
                                gen_params.strength = samplerData["denoise"].get<float>();
                            if (samplerData.contains("batchSize") && !samplerData["batchSize"].is_null())
                                gen_params.batch_count = samplerData["batchSize"].get<int>();

                            // sample_params fields
                            if (samplerData.contains("steps") && !samplerData["steps"].is_null())
                                gen_params.sample_params.sample_steps = samplerData["steps"].get<int>();
                            if (samplerData.contains("eta") && !samplerData["eta"].is_null())
                                gen_params.sample_params.eta = samplerData["eta"].get<float>();
                            if (samplerData.contains("current_sample_method") && !samplerData["current_sample_method"].is_null())
                                gen_params.sample_params.sample_method = static_cast<sample_method_t>(samplerData["current_sample_method"].get<int>());
                            if (samplerData.contains("current_scheduler_method") && !samplerData["current_scheduler_method"].is_null())
                                gen_params.sample_params.scheduler = static_cast<scheduler_t>(samplerData["current_scheduler_method"].get<int>());
                            if (samplerData.contains("shifted_timestep") && !samplerData["shifted_timestep"].is_null())
                                gen_params.sample_params.shifted_timestep = samplerData["shifted_timestep"].get<int>();
                            if (samplerData.contains("extra_sample_args") && !samplerData["extra_sample_args"].is_null())
                                gen_params.sample_params.extra_sample_args = samplerData["extra_sample_args"].get<std::string>().c_str();

                            // Custom sigmas
                            if (samplerData.contains("custom_sigmas") && samplerData["custom_sigmas"].is_array())
                            {
                                auto sigmas = samplerData["custom_sigmas"].get<std::vector<float>>();
                                if (!sigmas.empty())
                                {
                                    float* sigma_data = new float[sigmas.size()];
                                    std::copy(sigmas.begin(), sigmas.end(), sigma_data);
                                    gen_params.sample_params.custom_sigmas = sigma_data;
                                    gen_params.sample_params.custom_sigmas_count = static_cast<int>(sigmas.size());
                                    // Note: We'll need to delete this later; for simplicity we don't free here,
                                    // but proper cleanup would be needed.
                                }
                            }

                        }

                        // --- Guidance ---
                        if (comp.contains("Guidance"))
                        {
                            nlohmann::json guidanceData = comp["Guidance"];
                            if (guidanceData.contains("txt_cfg") && !guidanceData["txt_cfg"].is_null())
                                gen_params.sample_params.guidance.txt_cfg = guidanceData["txt_cfg"].get<float>();
                            if (guidanceData.contains("img_cfg") && !guidanceData["img_cfg"].is_null())
                                gen_params.sample_params.guidance.img_cfg = guidanceData["img_cfg"].get<float>();
                            if (guidanceData.contains("distilled_guidance") && !guidanceData["distilled_guidance"].is_null())
                                gen_params.sample_params.guidance.distilled_guidance = guidanceData["distilled_guidance"].get<float>();
                        }

                        // --- SLG (LayerSkip) ---
                        if (comp.contains("SLG"))
                        {
                            nlohmann::json slgData = comp["SLG"];
                            if (slgData.contains("layer_start") && !slgData["layer_start"].is_null())
                                gen_params.sample_params.guidance.slg.layer_start = slgData["layer_start"].get<float>();
                            if (slgData.contains("layer_end") && !slgData["layer_end"].is_null())
                                gen_params.sample_params.guidance.slg.layer_end = slgData["layer_end"].get<float>();
                            if (slgData.contains("scale") && !slgData["scale"].is_null())
                                gen_params.sample_params.guidance.slg.scale = slgData["scale"].get<float>();

                            if (slgData.contains("layers") && slgData["layers"].is_array() &&
                                slgData.contains("layer_count") && !slgData["layer_count"].is_null())
                            {
                                size_t layer_count = slgData["layer_count"].get<size_t>();
                                if (layer_count > 0 && slgData["layers"].size() >= layer_count)
                                {
                                    slg_layers = new int[layer_count];
                                    gen_params.sample_params.guidance.slg.layers = slg_layers;
                                    gen_params.sample_params.guidance.slg.layer_count = layer_count;
                                    for (size_t i = 0; i < layer_count; i++)
                                    {
                                        gen_params.sample_params.guidance.slg.layers[i] = slgData["layers"][i].get<int>();
                                    }
                                }
                            }
                        }

                        // --- Latent ---
                        if (comp.contains("Latent"))
                        {
                            nlohmann::json latentData = comp["Latent"];
                            if (latentData.contains("latentWidth") && !latentData["latentWidth"].is_null())
                                gen_params.width = latentData["latentWidth"].get<int>();
                            if (latentData.contains("latentHeight") && !latentData["latentHeight"].is_null())
                                gen_params.height = latentData["latentHeight"].get<int>();
                            if (latentData.contains("batchSize") && !latentData["batchSize"].is_null())
                                gen_params.batch_count = latentData["batchSize"].get<int>();
                        }

                        // --- Vae ---
                        if (comp.contains("Vae"))
                        {
                            nlohmann::json vaeData = comp["Vae"];
                            if (vaeData.contains("isTiled") && !vaeData["isTiled"].is_null())
                                vae_tiling_params.enabled = vaeData["isTiled"].get<bool>();
                            if (vaeData.contains("tile_size_x") && !vaeData["tile_size_x"].is_null())
                                vae_tiling_params.tile_size_x = vaeData["tile_size_x"].get<int>();
                            if (vaeData.contains("tile_size_y") && !vaeData["tile_size_y"].is_null())
                                vae_tiling_params.tile_size_y = vaeData["tile_size_y"].get<int>();
                            if (vaeData.contains("target_overlap") && !vaeData["target_overlap"].is_null())
                                vae_tiling_params.target_overlap = vaeData["target_overlap"].get<float>();
                            if (vaeData.contains("rel_size_x") && !vaeData["rel_size_x"].is_null())
                                vae_tiling_params.rel_size_x = vaeData["rel_size_x"].get<float>();
                            if (vaeData.contains("rel_size_y") && !vaeData["rel_size_y"].is_null())
                                vae_tiling_params.rel_size_y = vaeData["rel_size_y"].get<float>();
                        }

                        // --- ControlNet ---
                        if (comp.contains("Controlnet"))
                        {
                            nlohmann::json controlData = comp["Controlnet"];
                            if (controlData.contains("cnStrength") && !controlData["cnStrength"].is_null())
                                control_strength = controlData["cnStrength"].get<float>();
                        }

                        // --- ControlNet Image ---
                        if (comp.contains("ControlNetImage"))
                        {
                            nlohmann::json controlImageData = comp["ControlNetImage"];
                            if (controlImageData.contains("filePath") && !controlImageData["filePath"].is_null() &&
                                !controlImageData["filePath"].get<std::string>().empty())
                            {
                                std::string controlImagePath = controlImageData["filePath"].get<std::string>();
                                control_image = LoadImageToSDImage(controlImagePath);
                                if (control_image.data) {
                                    imagesToCleanup.push_back(control_image);
                                }
                            }
                            if (controlImageData.contains("strength") && !controlImageData["strength"].is_null() && control_strength == 0.0f)
                            {
                                control_strength = controlImageData["strength"].get<float>();
                            }
                        }

                        // --- PhotoMaker ---
                        if (comp.contains("PhotoMaker"))
                        {
                            nlohmann::json pmData = comp["PhotoMaker"];
                            if (pmData.contains("style_strength") && !pmData["style_strength"].is_null())
                                pm_params.style_strength = pmData["style_strength"].get<float>();
                            if (pmData.contains("modelPath") && !pmData["modelPath"].is_null())
                            {
                                std::string pm_path = pmData["modelPath"].get<std::string>();
                                pm_params.id_embed_path = pm_path.c_str();
                            }
                        }

                        // --- PhotoMaker ID Images ---
                        if (comp.contains("PhotoMakerImage"))
                        {
                            nlohmann::json pmImageData = comp["PhotoMakerImage"];
                            if (pmImageData.contains("filePath") && !pmImageData["filePath"].is_null() &&
                                !pmImageData["filePath"].get<std::string>().empty())
                            {
                                std::string idImagePath = pmImageData["filePath"].get<std::string>();
                                sd_image_t id_image = LoadImageToSDImage(idImagePath);
                                if (id_image.data) {
                                    idImagesStorage.push_back(id_image);
                                    imagesToCleanup.push_back(id_image);
                                }
                            }
                        }

                        // --- EasyCache ---
                        if (comp.contains("EasyCache"))
                        {
                            nlohmann::json cacheData = comp["EasyCache"];
                            if (cacheData.contains("mode") && !cacheData["mode"].is_null())
                                cache_params.mode = static_cast<sd_cache_mode_t>(cacheData["mode"].get<int>());
                            if (cacheData.contains("reuse_threshold") && !cacheData["reuse_threshold"].is_null())
                                cache_params.reuse_threshold = cacheData["reuse_threshold"].get<float>();
                            if (cacheData.contains("start_percent") && !cacheData["start_percent"].is_null())
                                cache_params.start_percent = cacheData["start_percent"].get<float>();
                            if (cacheData.contains("end_percent") && !cacheData["end_percent"].is_null())
                                cache_params.end_percent = cacheData["end_percent"].get<float>();
                            // More cache fields can be parsed if needed
                        }

                        if (comp.contains("Chroma"))
                        {
                            nlohmann::json chromaData = comp["Chroma"];
                        }
                    }
                }

                gen_params.prompt = posPrompt.c_str();
                gen_params.negative_prompt = negPrompt.c_str();

                // Initialize empty mask and control image for edit operations
                gen_params.mask_image = { 0, 0, 0, nullptr };
                gen_params.control_image = control_image;
                gen_params.control_strength = control_strength;

                // Set VAE tiling parameters
                gen_params.vae_tiling_params = vae_tiling_params;

                // Set cache parameters
                gen_params.cache = cache_params;

                // Set up PhotoMaker parameters if available
                if (pm_params.id_embed_path != nullptr || !idImagesStorage.empty()) {
                    if (!idImagesStorage.empty()) {
                        pm_params.id_images_count = idImagesStorage.size();
                        pm_params.id_images = idImagesStorage.data();
                    }
                    gen_params.pm_params = pm_params;
                }

                // Validate parameters
                if (refImagePaths.empty())
                {
                    throw std::runtime_error("No reference images provided for edit operation!");
                }

                // Load reference images into sd_image_t vector
                for (const std::string& imagePath : refImagePaths)
                {
                    if (!std::filesystem::exists(imagePath))
                    {
                        throw std::runtime_error("Reference image file does not exist: " + imagePath);
                    }

                    int imgWidth, imgHeight, imgChannels;
                    std::cout << "Loading reference image from: " << imagePath << std::endl;

                    unsigned char* imageData = stbi_load(imagePath.c_str(), &imgWidth, &imgHeight, &imgChannels, 3);
                    if (!imageData)
                    {
                        std::string error = std::string("Failed to load reference image: ") + imagePath + " - " +
                            (stbi_failure_reason() ? stbi_failure_reason() : "unknown reason");
                        throw std::runtime_error(error);
                    }

                    imgChannels = 3;
                    std::cout << "Reference image loaded successfully: " << imgWidth << "x" << imgHeight
                        << " with " << imgChannels << " channels (forced RGB)" << std::endl;

                    if (imgWidth <= 0 || imgHeight <= 0)
                    {
                        stbi_image_free(imageData);
                        throw std::runtime_error("Invalid reference image dimensions: " + std::to_string(imgWidth) + "x" + std::to_string(imgHeight));
                    }

                    refImageData.push_back(imageData);

                    sd_image_t ref_img = {
                        static_cast<uint32_t>(imgWidth),
                        static_cast<uint32_t>(imgHeight),
                        3,
                        imageData
                    };
                    refImagesStorage.push_back(ref_img);
                }

                // Set the first reference image as init_image for edit operations
                if (!refImagesStorage.empty())
                {
                    gen_params.init_image = refImagesStorage[0];
                }

                // Set multiple reference images if available
                if (refImagesStorage.size() > 1)
                {
                    gen_params.ref_images = refImagesStorage.data();
                    gen_params.ref_images_count = static_cast<int>(refImagesStorage.size());
                }

                // Get SD context if not provided
                if (!contextProvided) {
                    sd_context = SDContextManager::GetOrCreateContext(metadata);
                }

                if (!sd_context)
                {
                    throw std::runtime_error("Failed to initialize Stable Diffusion context!");
                }

                // Ensure valid seed
                if (gen_params.seed < 0)
                {
                    gen_params.seed = static_cast<int64_t>(generateRandomSeed());
                    std::cout << "Generated random seed: " << gen_params.seed << std::endl;
                }

                std::cout << "Calling generate_image for edit with " << refImagesStorage.size() << " reference images..." << std::endl;
                std::cout << "Parameters: " << std::endl;
                std::cout << "  - Width: " << gen_params.width << std::endl;
                std::cout << "  - Height: " << gen_params.height << std::endl;
                std::cout << "  - Strength: " << gen_params.strength << std::endl;
                std::cout << "  - Seed: " << gen_params.seed << std::endl;
                std::cout << "  - Init image: " << (gen_params.init_image.data ? "Set" : "Not set") << std::endl;

                bool success = generate_image(sd_context, &gen_params, &result_images, &num_result_images);
                if (!success || num_result_images == 0 || !result_images) {
                    throw std::runtime_error("generate_image failed - no output image produced");
                }

                // Use the first image
                sd_image_t* first_image = &result_images[0];
                if (!first_image->data) {
                    throw std::runtime_error("generate_image produced invalid image data");
                }

                std::cout << "Edit successful: " << first_image->width << "x" << first_image->height
                    << "x" << first_image->channel << ", saving to: " << fullPath << std::endl;

                // Save the result image
                SaveImage(first_image->data, first_image->width, first_image->height,
                    first_image->channel, metadata, fullPath);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                // Cleanup
                CleanupResources(refImageData, result_images, num_result_images, slg_layers, imagesToCleanup);

                // Release context back to cache if we acquired it
                if (!contextProvided) {
                    SDContextManager::ReleaseContext(sd_context);
                }

                return true;
            }
            catch (const std::exception& e)
            {
                std::cerr << "Exception during edit: " << e.what() << std::endl;

                // Cleanup on error
                CleanupResources(refImageData, result_images, num_result_images, slg_layers, imagesToCleanup);

                // Release context back to cache if we acquired it
                if (!contextProvided && sd_context) {
                    SDContextManager::ReleaseContext(sd_context);
                }

                return false;
            }
        }

    private:
        static void CleanupResources(std::vector<unsigned char*>& refImageData,
            sd_image_t* result_images,
            int num_result_images,
            int* slg_layers,
            std::vector<sd_image_t>& imagesToCleanup) {

            // Cleanup reference images
            for (unsigned char* imageData : refImageData)
            {
                if (imageData)
                {
                    stbi_image_free(imageData);
                }
            }
            refImageData.clear();

            // Cleanup SLG layers array if it was allocated
            if (slg_layers != nullptr)
            {
                delete[] slg_layers;
            }

            // Cleanup additional loaded images (control, id images, etc.)
            for (auto& img : imagesToCleanup) {
                if (img.data) {
                    stbi_image_free(img.data);
                    img.data = nullptr;
                }
            }
            imagesToCleanup.clear();

            // Cleanup result images (array) using the library's dedicated function
            if (result_images)
            {
                free_sd_images(result_images, num_result_images);
                result_images = nullptr;
            }
        }

        // Helper function to load image from file to sd_image_t
        static sd_image_t LoadImageToSDImage(const std::string& filePath) {
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
    };
} // namespace Utils