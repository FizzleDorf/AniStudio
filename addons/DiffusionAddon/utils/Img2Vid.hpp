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
#include <iostream>
#include <filesystem>

namespace Utils
{
    uint64_t generateRandomSeed();
    void SaveImage(const unsigned char* data, int width, int height, int channels,
        const nlohmann::json& metadata, const std::string& fullPath);

    class Img2Vid
    {
    public:
        static bool RunImg2Vid(const nlohmann::json& metadata, const std::string& fullPath, sd_ctx_t* sd_context = nullptr)
        {
            bool contextProvided = (sd_context != nullptr);
            unsigned char* inputData = nullptr;
            unsigned char* endInputData = nullptr;
            sd_image_t* result_images = nullptr;
            int num_frames_out = 0;
            sd_vid_gen_params_t vid_params;
            sd_vid_gen_params_init(&vid_params);

            // Additional structures for advanced features
            std::vector<sd_image_t> imagesToCleanup;
            std::vector<sd_image_t> idImagesStorage;
            std::vector<sd_image_t> controlFramesStorage;
            int* slg_layers = nullptr;

            try
            {
                std::string inputImagePath = "";
                std::string endImagePath = "";
                std::string outputPath = Utils::FilePathService::GetPath("DefaultProject");
                std::string outputFilename = "img2vid_output";
                std::string posPrompt = "";
                std::string negPrompt = "";
                int latentWidth = 0;
                int latentHeight = 0;

                // VAE tiling parameters
                sd_tiling_params_t vae_tiling_params = { false, 64, 64, 0.0f, 64.0f, 64.0f };

                // PhotoMaker parameters
                sd_pm_params_t pm_params = { nullptr, 0, nullptr, 0.0f };

                // Cache parameters
                sd_cache_params_t cache_params;
                sd_cache_params_init(&cache_params);

                // Video-specific parameters
                int video_frames = 25;
                float vace_strength = 0.0f;
                int motion_bucket_id = 127;
                int fps = 6;
                float augmentation_level = 0.0f;
                float min_cfg = 1.0f;
                float moe_boundary = 0.0f;
                float flow_shift = 3.0f;

                // High noise sampler (Wan 2.2 dual-model)
                sd_sample_params_t high_noise_sample_params;
                sd_sample_params_init(&high_noise_sample_params);

                if (metadata.contains("components") && metadata["components"].is_array())
                {
                    for (const auto& comp : metadata["components"])
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
                                video_frames = videoData["video_frames"].get<int>();
                            if (videoData.contains("vace_strength") && !videoData["vace_strength"].is_null())
                                vace_strength = videoData["vace_strength"].get<float>();
                            if (videoData.contains("motion_bucket_id") && !videoData["motion_bucket_id"].is_null())
                                motion_bucket_id = videoData["motion_bucket_id"].get<int>();
                            if (videoData.contains("fps") && !videoData["fps"].is_null())
                                fps = videoData["fps"].get<int>();
                            if (videoData.contains("augmentation_level") && !videoData["augmentation_level"].is_null())
                                augmentation_level = videoData["augmentation_level"].get<float>();
                            if (videoData.contains("min_cfg") && !videoData["min_cfg"].is_null())
                                min_cfg = videoData["min_cfg"].get<float>();
                            if (videoData.contains("moe_boundary") && !videoData["moe_boundary"].is_null())
                                moe_boundary = videoData["moe_boundary"].get<float>();
                            if (videoData.contains("flow_shift") && !videoData["flow_shift"].is_null())
                                flow_shift = videoData["flow_shift"].get<float>();
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
                                high_noise_sample_params.sample_steps = samplerData["steps"].get<int>();
                            }
                            if (samplerData.contains("eta") && !samplerData["eta"].is_null())
                            {
                                vid_params.sample_params.eta = samplerData["eta"].get<float>();
                                high_noise_sample_params.eta = samplerData["eta"].get<float>();
                            }
                            if (samplerData.contains("current_sample_method") && !samplerData["current_sample_method"].is_null())
                            {
                                vid_params.sample_params.sample_method = static_cast<sample_method_t>(samplerData["current_sample_method"].get<int>());
                                high_noise_sample_params.sample_method = vid_params.sample_params.sample_method;
                            }
                            if (samplerData.contains("current_scheduler_method") && !samplerData["current_scheduler_method"].is_null())
                            {
                                vid_params.sample_params.scheduler = static_cast<scheduler_t>(samplerData["current_scheduler_method"].get<int>());
                                high_noise_sample_params.scheduler = vid_params.sample_params.scheduler;
                            }
                            if (samplerData.contains("shifted_timestep") && !samplerData["shifted_timestep"].is_null())
                            {
                                vid_params.sample_params.shifted_timestep = samplerData["shifted_timestep"].get<int>();
                                high_noise_sample_params.shifted_timestep = samplerData["shifted_timestep"].get<int>();
                            }
                            if (samplerData.contains("extra_sample_args") && !samplerData["extra_sample_args"].is_null())
                            {
                                vid_params.sample_params.extra_sample_args = samplerData["extra_sample_args"].get<std::string>().c_str();
                                high_noise_sample_params.extra_sample_args = vid_params.sample_params.extra_sample_args;
                            }
                            if (samplerData.contains("custom_sigmas") && samplerData["custom_sigmas"].is_array())
                            {
                                auto sigmas = samplerData["custom_sigmas"].get<std::vector<float>>();
                                if (!sigmas.empty())
                                {
                                    float* sigma_data = new float[sigmas.size()];
                                    std::copy(sigmas.begin(), sigmas.end(), sigma_data);
                                    vid_params.sample_params.custom_sigmas = sigma_data;
                                    vid_params.sample_params.custom_sigmas_count = static_cast<int>(sigmas.size());
                                    // Also set for high_noise? Usually they share, but we can set separately if needed
                                }
                            }
                        }

                        if (comp.contains("HighNoiseSampler"))
                        {
                            nlohmann::json highNoiseData = comp["HighNoiseSampler"];
                            if (highNoiseData.contains("high_noise_sample_method") && !highNoiseData["high_noise_sample_method"].is_null())
                                high_noise_sample_params.sample_method = static_cast<sample_method_t>(highNoiseData["high_noise_sample_method"].get<int>());
                            if (highNoiseData.contains("high_noise_scheduler_method") && !highNoiseData["high_noise_scheduler_method"].is_null())
                                high_noise_sample_params.scheduler = static_cast<scheduler_t>(highNoiseData["high_noise_scheduler_method"].get<int>());
                            if (highNoiseData.contains("high_noise_steps") && !highNoiseData["high_noise_steps"].is_null())
                                high_noise_sample_params.sample_steps = highNoiseData["high_noise_steps"].get<int>();
                            if (highNoiseData.contains("high_noise_eta") && !highNoiseData["high_noise_eta"].is_null())
                                high_noise_sample_params.eta = highNoiseData["high_noise_eta"].get<float>();
                            // high_noise_cfg is not part of sample_params; it's guidance, we'll set it below if needed
                        }

                        if (comp.contains("Guidance"))
                        {
                            nlohmann::json guidanceData = comp["Guidance"];
                            if (guidanceData.contains("txt_cfg") && !guidanceData["txt_cfg"].is_null())
                            {
                                vid_params.sample_params.guidance.txt_cfg = guidanceData["txt_cfg"].get<float>();
                                high_noise_sample_params.guidance.txt_cfg = guidanceData["txt_cfg"].get<float>();
                            }
                            if (guidanceData.contains("img_cfg") && !guidanceData["img_cfg"].is_null())
                            {
                                vid_params.sample_params.guidance.img_cfg = guidanceData["img_cfg"].get<float>();
                                high_noise_sample_params.guidance.img_cfg = guidanceData["img_cfg"].get<float>();
                            }
                            if (guidanceData.contains("distilled_guidance") && !guidanceData["distilled_guidance"].is_null())
                            {
                                vid_params.sample_params.guidance.distilled_guidance = guidanceData["distilled_guidance"].get<float>();
                                high_noise_sample_params.guidance.distilled_guidance = guidanceData["distilled_guidance"].get<float>();
                            }
                        }

                        if (comp.contains("SLG"))
                        {
                            nlohmann::json slgData = comp["SLG"];
                            if (slgData.contains("layer_start") && !slgData["layer_start"].is_null())
                            {
                                vid_params.sample_params.guidance.slg.layer_start = slgData["layer_start"].get<float>();
                                high_noise_sample_params.guidance.slg.layer_start = slgData["layer_start"].get<float>();
                            }
                            if (slgData.contains("layer_end") && !slgData["layer_end"].is_null())
                            {
                                vid_params.sample_params.guidance.slg.layer_end = slgData["layer_end"].get<float>();
                                high_noise_sample_params.guidance.slg.layer_end = slgData["layer_end"].get<float>();
                            }
                            if (slgData.contains("scale") && !slgData["scale"].is_null())
                            {
                                vid_params.sample_params.guidance.slg.scale = slgData["scale"].get<float>();
                                high_noise_sample_params.guidance.slg.scale = slgData["scale"].get<float>();
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

                        if (comp.contains("PhotoMaker"))
                        {
                            nlohmann::json pmData = comp["PhotoMaker"];
                            if (pmData.contains("modelPath") && !pmData["modelPath"].is_null())
                            {
                                std::string pm_path = pmData["modelPath"].get<std::string>();
                                pm_params.id_embed_path = pm_path.c_str();
                            }
                            if (pmData.contains("style_strength") && !pmData["style_strength"].is_null())
                            {
                                pm_params.style_strength = pmData["style_strength"].get<float>();
                            }
                        }

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

                        if (comp.contains("ControlFrames"))
                        {
                            nlohmann::json controlFramesData = comp["ControlFrames"];
                            if (controlFramesData.is_array())
                            {
                                for (const auto& frameData : controlFramesData)
                                {
                                    if (frameData.contains("filePath") && !frameData["filePath"].is_null())
                                    {
                                        std::string framePath = frameData["filePath"].get<std::string>();
                                        sd_image_t control_frame = LoadImageToSDImage(framePath);
                                        if (control_frame.data) {
                                            controlFramesStorage.push_back(control_frame);
                                            imagesToCleanup.push_back(control_frame);
                                        }
                                    }
                                }
                            }
                        }

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
                        }
                    }
                }

                // Apply parsed values
                vid_params.prompt = posPrompt.c_str();
                vid_params.negative_prompt = negPrompt.c_str();
                vid_params.video_frames = video_frames;
                vid_params.fps = fps;
                vid_params.vace_strength = vace_strength;
                vid_params.moe_boundary = moe_boundary;
                vid_params.vae_tiling_params = vae_tiling_params;
                vid_params.cache = cache_params;

                // Flow shift and other sample params
                vid_params.sample_params.flow_shift = flow_shift;
                high_noise_sample_params.flow_shift = flow_shift;

                // Set high noise sample params
                vid_params.high_noise_sample_params = high_noise_sample_params;

                // Set PhotoMaker
                if (!idImagesStorage.empty()) {
                    pm_params.id_images_count = idImagesStorage.size();
                    pm_params.id_images = idImagesStorage.data();
                    // vid_params currently doesn't have pm_params; if needed, we can add, but for now ignore
                }

                // Set control frames
                if (!controlFramesStorage.empty()) {
                    vid_params.control_frames = controlFramesStorage.data();
                    vid_params.control_frames_size = static_cast<int>(controlFramesStorage.size());
                }

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

                std::cout << "Final settings:" << std::endl;
                std::cout << "  - Input image: " << inputWidth << "x" << inputHeight << std::endl;
                std::cout << "  - Target size: " << vid_params.width << "x" << vid_params.height << std::endl;
                std::cout << "  - Strength: " << vid_params.strength << std::endl;
                std::cout << "  - Video frames: " << vid_params.video_frames << std::endl;
                std::cout << "  - Control frames: " << controlFramesStorage.size() << std::endl;
                std::cout << "=====================" << std::endl;

                // Get SD context if not provided
                if (!contextProvided) {
                    sd_context = SDContextManager::GetOrCreateContext(metadata);
                }

                if (!sd_context) {
                    throw std::runtime_error("Failed to initialize Stable Diffusion context!");
                }

                if (vid_params.seed < 0) {
                    vid_params.seed = static_cast<int64_t>(generateRandomSeed());
                }

                bool success = generate_video(sd_context, &vid_params, &result_images, &num_frames_out, nullptr);
                if (!success || !result_images || num_frames_out == 0) {
                    throw std::runtime_error("generate_video failed");
                }

                std::cout << "Generated " << num_frames_out << " frames" << std::endl;

                // Ensure output directory exists
                std::filesystem::path frameDir(outputPath);
                if (outputPath.empty() || outputPath[0] == '\0' || !std::filesystem::exists(frameDir)) {
                    std::string outputFolder = Utils::FilePathService::GetPath("OutputFolder");
                    if (!outputFolder.empty() && outputFolder[0] != '\0' && std::filesystem::exists(outputFolder)) {
                        frameDir = outputFolder;
                    }
                }
                std::filesystem::create_directories(frameDir);

                // Save individual frames
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

                // Cleanup
                CleanupResources(inputData, endInputData, result_images, num_frames_out,
                    slg_layers, imagesToCleanup);

                // Release context back to cache if we acquired it
                if (!contextProvided) {
                    SDContextManager::ReleaseContext(sd_context);
                }

                return true;
            }
            catch (const std::exception& e)
            {
                std::cerr << "Exception during img2vid: " << e.what() << std::endl;

                CleanupResources(inputData, endInputData, result_images, num_frames_out,
                    slg_layers, imagesToCleanup);

                if (!contextProvided && sd_context) {
                    SDContextManager::ReleaseContext(sd_context);
                }

                return false;
            }
        }

    private:
        static void CleanupResources(unsigned char* inputData,
            unsigned char* endInputData,
            sd_image_t* result_images,
            int num_frames_out,
            int* slg_layers,
            std::vector<sd_image_t>& imagesToCleanup) {

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

            if (slg_layers != nullptr)
            {
                delete[] slg_layers;
            }

            for (auto& img : imagesToCleanup) {
                FreeSDImage(img);
            }
        }

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

        static void FreeSDImage(sd_image_t& img) {
            if (img.data) {
                stbi_image_free(img.data);
                img.data = nullptr;
            }
        }
    };
}