#pragma once

#include "stable-diffusion.h"
#include "pch.h"
#include "ContextUtils.hpp"
#include "PngMetadataUtils.hpp"
#include "sdcpp_utils/SchedulerUtil.hpp"
#include "sdcpp_utils/SDGuidanceUtil.hpp"
#include "sdcpp_utils/SLGUtil.hpp"
#include "sdcpp_utils/SDImageUtil.hpp"
#include <stb_image.h>
#include <stb_image_write.h>
#include <iostream>
#include <filesystem>
#include <memory>
#include <vector>

namespace Utils
{
    uint64_t generateRandomSeed();
    void SaveImage(const unsigned char* data, int width, int height, int channels,
        const nlohmann::json& metadata, const std::string& fullPath);

    class Txt2Img
    {
    public:
        static bool RunInference(const nlohmann::json& metadata, const std::string& fullPath, sd_ctx_t* sd_context = nullptr)
        {
            bool contextProvided = (sd_context != nullptr);
            sd_image_t* result_images = nullptr;
            int num_result_images = 0;
            sd_img_gen_params_t gen_params;
            sd_img_gen_params_init(&gen_params);

            std::vector<std::unique_ptr<float[]>> floatArrayResources;
            std::vector<std::unique_ptr<int[]>> intArrayResources;
            std::vector<sd_image_t> imageResources;

            try
            {
                if (metadata.contains("components") && metadata["components"].is_array())
                {
                    std::vector<sd_image_t> idImagesStorage;
                    std::vector<sd_image_t> refImagesStorage;

                    for (const auto& comp : metadata["components"])
                    {
                        if (comp.contains("Prompt"))
                        {
                            nlohmann::json promptData = comp["Prompt"];
                            if (promptData.contains("posPrompt") && !promptData["posPrompt"].is_null())
                                gen_params.prompt = promptData["posPrompt"].get<std::string>().c_str();
                            if (promptData.contains("negPrompt") && !promptData["negPrompt"].is_null())
                                gen_params.negative_prompt = promptData["negPrompt"].get<std::string>().c_str();
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
                            if (samplerData.contains("batchSize") && !samplerData["batchSize"].is_null())
                                gen_params.batch_count = samplerData["batchSize"].get<int>();
                            if (samplerData.contains("denoise") && !samplerData["denoise"].is_null())
                                gen_params.strength = samplerData["denoise"].get<float>();
                            else
                                gen_params.strength = 1.0f;

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

                            if (samplerData.contains("custom_sigmas") && samplerData["custom_sigmas"].is_array())
                            {
                                auto sigmas = samplerData["custom_sigmas"].get<std::vector<float>>();
                                if (!sigmas.empty())
                                {
                                    auto sigma_data = std::make_unique<float[]>(sigmas.size());
                                    std::copy(sigmas.begin(), sigmas.end(), sigma_data.get());
                                    gen_params.sample_params.custom_sigmas = sigma_data.get();
                                    gen_params.sample_params.custom_sigmas_count = static_cast<int>(sigmas.size());
                                    floatArrayResources.push_back(std::move(sigma_data));
                                }
                            }
                        }

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
                                    auto layers = std::make_unique<int[]>(layer_count);
                                    for (size_t i = 0; i < layer_count; i++)
                                        layers[i] = slgData["layers"][i].get<int>();
                                    gen_params.sample_params.guidance.slg.layers = layers.get();
                                    gen_params.sample_params.guidance.slg.layer_count = layer_count;
                                    intArrayResources.push_back(std::move(layers));
                                }
                            }
                        }

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

                        if (comp.contains("Vae"))
                        {
                            nlohmann::json vaeData = comp["Vae"];
                            if (vaeData.contains("isTiled") && !vaeData["isTiled"].is_null())
                                gen_params.vae_tiling_params.enabled = vaeData["isTiled"].get<bool>();
                            if (vaeData.contains("tile_size_x") && !vaeData["tile_size_x"].is_null())
                                gen_params.vae_tiling_params.tile_size_x = vaeData["tile_size_x"].get<int>();
                            if (vaeData.contains("tile_size_y") && !vaeData["tile_size_y"].is_null())
                                gen_params.vae_tiling_params.tile_size_y = vaeData["tile_size_y"].get<int>();
                            if (vaeData.contains("target_overlap") && !vaeData["target_overlap"].is_null())
                                gen_params.vae_tiling_params.target_overlap = vaeData["target_overlap"].get<float>();
                            if (vaeData.contains("rel_size_x") && !vaeData["rel_size_x"].is_null())
                                gen_params.vae_tiling_params.rel_size_x = vaeData["rel_size_x"].get<float>();
                            if (vaeData.contains("rel_size_y") && !vaeData["rel_size_y"].is_null())
                                gen_params.vae_tiling_params.rel_size_y = vaeData["rel_size_y"].get<float>();
                        }

                        if (comp.contains("PhotoMaker"))
                        {
                            nlohmann::json pmData = comp["PhotoMaker"];
                            if (pmData.contains("modelPath") && !pmData["modelPath"].is_null())
                            {
                                std::string pm_path = pmData["modelPath"].get<std::string>();
                                gen_params.pm_params.id_embed_path = pm_path.c_str();
                            }
                            if (pmData.contains("style_strength") && !pmData["style_strength"].is_null())
                                gen_params.pm_params.style_strength = pmData["style_strength"].get<float>();
                        }

                        if (comp.contains("PhotoMakerImage"))
                        {
                            sd_image_t id_image = { 0, 0, 0, nullptr };
                            if (ParseImageComponent(comp, id_image, imageResources))
                            {
                                idImagesStorage.push_back(id_image);
                            }
                        }

                        if (comp.contains("ReferenceImage"))
                        {
                            sd_image_t ref_image = { 0, 0, 0, nullptr };
                            if (ParseImageComponent(comp, ref_image, imageResources))
                            {
                                refImagesStorage.push_back(ref_image);
                            }
                            nlohmann::json refImageData = comp["ReferenceImage"];
                        }

                        if (comp.contains("ControlNetImage"))
                        {
                            sd_image_t control_image = { 0, 0, 0, nullptr };
                            if (ParseImageComponent(comp, control_image, imageResources))
                            {
                                gen_params.control_image = control_image;
                            }
                            nlohmann::json controlImageData = comp["ControlNetImage"];
                            if (controlImageData.contains("strength") && !controlImageData["strength"].is_null())
                                gen_params.control_strength = controlImageData["strength"].get<float>();
                        }

                        if (comp.contains("Controlnet"))
                        {
                            nlohmann::json controlNetData = comp["Controlnet"];
                            if (controlNetData.contains("cnStrength") && !controlNetData["cnStrength"].is_null())
                                gen_params.control_strength = controlNetData["cnStrength"].get<float>();
                        }

                        if (comp.contains("MaskImage"))
                        {
                            sd_image_t mask_image = { 0, 0, 0, nullptr };
                            if (ParseImageComponent(comp, mask_image, imageResources))
                            {
                                gen_params.mask_image = mask_image;
                            }
                        }

                        if (comp.contains("InitImage"))
                        {
                            sd_image_t init_image = { 0, 0, 0, nullptr };
                            if (ParseImageComponent(comp, init_image, imageResources))
                            {
                                gen_params.init_image = init_image;
                            }
                        }

                        if (comp.contains("EasyCache"))
                        {
                            nlohmann::json cacheData = comp["EasyCache"];
                            if (cacheData.contains("mode") && !cacheData["mode"].is_null())
                                gen_params.cache.mode = static_cast<sd_cache_mode_t>(cacheData["mode"].get<int>());
                            if (cacheData.contains("reuse_threshold") && !cacheData["reuse_threshold"].is_null())
                                gen_params.cache.reuse_threshold = cacheData["reuse_threshold"].get<float>();
                            if (cacheData.contains("start_percent") && !cacheData["start_percent"].is_null())
                                gen_params.cache.start_percent = cacheData["start_percent"].get<float>();
                            if (cacheData.contains("end_percent") && !cacheData["end_percent"].is_null())
                                gen_params.cache.end_percent = cacheData["end_percent"].get<float>();
                        }

                        if (comp.contains("Hires"))
                        {
                            nlohmann::json hiresData = comp["Hires"];
                            if (hiresData.contains("enabled") && !hiresData["enabled"].is_null())
                                gen_params.hires.enabled = hiresData["enabled"].get<bool>();
                            if (hiresData.contains("upscaler") && !hiresData["upscaler"].is_null())
                                gen_params.hires.upscaler = static_cast<sd_hires_upscaler_t>(hiresData["upscaler"].get<int>());
                            if (hiresData.contains("modelPath") && !hiresData["modelPath"].is_null())
                                gen_params.hires.model_path = hiresData["modelPath"].get<std::string>().c_str();
                            if (hiresData.contains("scale") && !hiresData["scale"].is_null())
                                gen_params.hires.scale = hiresData["scale"].get<float>();
                            if (hiresData.contains("target_width") && !hiresData["target_width"].is_null())
                                gen_params.hires.target_width = hiresData["target_width"].get<int>();
                            if (hiresData.contains("target_height") && !hiresData["target_height"].is_null())
                                gen_params.hires.target_height = hiresData["target_height"].get<int>();
                            if (hiresData.contains("steps") && !hiresData["steps"].is_null())
                                gen_params.hires.steps = hiresData["steps"].get<int>();
                            if (hiresData.contains("denoising_strength") && !hiresData["denoising_strength"].is_null())
                                gen_params.hires.denoising_strength = hiresData["denoising_strength"].get<float>();
                            if (hiresData.contains("upscale_tile_size") && !hiresData["upscale_tile_size"].is_null())
                                gen_params.hires.upscale_tile_size = hiresData["upscale_tile_size"].get<int>();
                        }
                    }

                    if (!idImagesStorage.empty())
                    {
                        gen_params.pm_params.id_images = idImagesStorage.data();
                        gen_params.pm_params.id_images_count = static_cast<int>(idImagesStorage.size());
                        for (auto& img : idImagesStorage)
                            imageResources.push_back(img);
                        idImagesStorage.clear();
                    }
                    if (!refImagesStorage.empty())
                    {
                        gen_params.ref_images = refImagesStorage.data();
                        gen_params.ref_images_count = static_cast<int>(refImagesStorage.size());
                        for (auto& img : refImagesStorage)
                            imageResources.push_back(img);
                        refImagesStorage.clear();
                    }
                }

                if (!contextProvided) {
                    sd_context = SDContextManager::GetOrCreateContext(metadata);
                }
                if (!sd_context)
                {
                    throw std::runtime_error("Failed to initialize Stable Diffusion context!");
                }

                if (gen_params.seed < 0)
                {
                    gen_params.seed = static_cast<int64_t>(generateRandomSeed());
                    std::cout << "Generated random seed: " << gen_params.seed << std::endl;
                }

                std::cout << "Generating image: " << gen_params.width << "x" << gen_params.height
                    << ", steps=" << gen_params.sample_params.sample_steps
                    << ", seed=" << gen_params.seed << std::endl;

                bool success = generate_image(sd_context, &gen_params, &result_images, &num_result_images);
                if (!success || num_result_images == 0 || !result_images || !result_images[0].data)
                {
                    throw std::runtime_error("generate_image failed - no output image produced");
                }

                SaveImage(result_images[0].data, result_images[0].width, result_images[0].height,
                    result_images[0].channel, metadata, fullPath);

                if (result_images)
                {
                    free_sd_images(result_images, num_result_images);
                    result_images = nullptr;
                }

                for (auto& img : imageResources)
                {
                    if (img.data)
                        stbi_image_free(img.data);
                }
                imageResources.clear();

                if (!contextProvided) {
                    SDContextManager::ReleaseContext(sd_context);
                }

                return true;
            }
            catch (const std::exception& e)
            {
                std::cerr << "Exception during txt2img: " << e.what() << std::endl;

                if (result_images)
                {
                    free_sd_images(result_images, num_result_images);
                }

                for (auto& img : imageResources)
                {
                    if (img.data)
                        stbi_image_free(img.data);
                }
                imageResources.clear();

                if (!contextProvided && sd_context) {
                    SDContextManager::ReleaseContext(sd_context);
                }

                return false;
            }
        }

    private:
        static bool ParseImageComponent(const nlohmann::json& comp, sd_image_t& out_image,
            std::vector<sd_image_t>& imageResources)
        {
            nlohmann::json imageData;
            if (comp.contains("filePath") && !comp["filePath"].is_null())
            {
                imageData = comp;
            }
            else
            {
                std::string compName;
                for (auto it = comp.begin(); it != comp.end(); ++it)
                {
                    if (it.value().contains("filePath") || it.value().contains("modelPath"))
                    {
                        compName = it.key();
                        imageData = it.value();
                        break;
                    }
                }
                if (imageData.empty())
                    return false;
            }

            std::string filePath;
            if (imageData.contains("filePath") && !imageData["filePath"].is_null())
                filePath = imageData["filePath"].get<std::string>();
            else if (imageData.contains("modelPath") && !imageData["modelPath"].is_null())
                filePath = imageData["modelPath"].get<std::string>();
            else
                return false;

            if (filePath.empty() || !std::filesystem::exists(filePath))
                return false;

            int width, height, channels;
            unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
            if (!data)
                return false;

            out_image.width = static_cast<uint32_t>(width);
            out_image.height = static_cast<uint32_t>(height);
            out_image.channel = static_cast<uint32_t>(channels);
            out_image.data = data;

            imageResources.push_back(out_image);
            return true;
        }
    };
}