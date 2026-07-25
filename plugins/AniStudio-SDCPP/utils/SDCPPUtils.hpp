#pragma once

#include "stable-diffusion.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace SDCPP {

    class ResourceManager {
    public:
        std::vector<std::unique_ptr<float[]>> floatArrays;
        std::vector<std::unique_ptr<int[]>> intArrays;
        std::vector<sd_image_t> images;
        std::vector<sd_image_t> controlFrames;
        std::vector<std::string> strings;
        std::vector<sd_lora_t> loraStorage;
        std::vector<sd_embedding_t> embeddingStorage;

        const char* storeString(const std::string& s) {
            strings.push_back(s);
            return strings.back().c_str();
        }

        float* storeFloats(const std::vector<float>& data) {
            if (data.empty()) return nullptr;
            auto ptr = std::make_unique<float[]>(data.size());
            std::copy(data.begin(), data.end(), ptr.get());
            floatArrays.push_back(std::move(ptr));
            return floatArrays.back().get();
        }

        int* storeInts(const std::vector<int>& data) {
            if (data.empty()) return nullptr;
            auto ptr = std::make_unique<int[]>(data.size());
            std::copy(data.begin(), data.end(), ptr.get());
            intArrays.push_back(std::move(ptr));
            return intArrays.back().get();
        }

        void storeImage(const sd_image_t& img) {
            images.push_back(img);
        }

        void storeControlFrame(const sd_image_t& img) {
            controlFrames.push_back(img);
        }
    };

    inline bool parseSampleParams(const nlohmann::json& comp, sd_sample_params_t& params, ResourceManager& res) {
        try {
            if (comp.contains("steps")) params.sample_steps = comp["steps"].get<int>();
            if (comp.contains("current_sample_method")) {
                params.sample_method = static_cast<enum sample_method_t>(comp["current_sample_method"].get<int>());
            }
            if (comp.contains("current_scheduler_method")) {
                params.scheduler = static_cast<enum scheduler_t>(comp["current_scheduler_method"].get<int>());
            }
            if (comp.contains("extra_sample_args") && !comp["extra_sample_args"].is_null()) {
                std::string s = comp["extra_sample_args"].get<std::string>();
                if (!s.empty()) params.extra_sample_args = res.storeString(s);
            }
            return true;
        }
        catch (...) { return false; }
    }

    inline bool parseGuidanceParams(const nlohmann::json& comp, sd_sample_params_t& params, ResourceManager& res) {
        try {
            if (comp.contains("txt_cfg")) params.guidance.txt_cfg = comp["txt_cfg"].get<float>();
            if (comp.contains("img_cfg")) params.guidance.img_cfg = comp["img_cfg"].get<float>();
            if (comp.contains("distilled_guidance")) params.guidance.distilled_guidance = comp["distilled_guidance"].get<float>();
            if (comp.contains("eta")) params.eta = comp["eta"].get<float>();
            if (comp.contains("shifted_timestep")) params.shifted_timestep = comp["shifted_timestep"].get<int>();
            if (comp.contains("flow_shift")) params.flow_shift = comp["flow_shift"].get<float>();
            if (comp.contains("custom_sigmas") && comp["custom_sigmas"].is_array()) {
                auto sigmas = comp["custom_sigmas"].get<std::vector<float>>();
                params.custom_sigmas = res.storeFloats(sigmas);
                params.custom_sigmas_count = static_cast<int>(sigmas.size());
            }
            return true;
        }
        catch (...) { return false; }
    }

    inline bool parseSLGParams(const nlohmann::json& comp, sd_slg_params_t& slg, ResourceManager& res) {
        try {
            if (comp.contains("slg_scale")) slg.scale = comp["slg_scale"].get<float>();
            if (comp.contains("slg_layer_start")) slg.layer_start = comp["slg_layer_start"].get<float>();
            if (comp.contains("slg_layer_end")) slg.layer_end = comp["slg_layer_end"].get<float>();
            if (comp.contains("slg_layers") && !comp["slg_layers"].is_null()) {
                std::string layersStr = comp["slg_layers"].get<std::string>();
                std::vector<int> layers;
                size_t pos = 0;
                while (pos < layersStr.size()) {
                    size_t next = layersStr.find(',', pos);
                    std::string token = layersStr.substr(pos, next - pos);
                    if (!token.empty()) {
                        try { layers.push_back(std::stoi(token)); }
                        catch (...) {}
                    }
                    if (next == std::string::npos) break;
                    pos = next + 1;
                }
                if (!layers.empty()) {
                    slg.layers = res.storeInts(layers);
                    slg.layer_count = layers.size();
                }
            }
            return true;
        }
        catch (...) { return false; }
    }

    inline bool parseCacheParams(const nlohmann::json& comp, sd_cache_params_t& cache) {
        try {
            if (comp.contains("mode")) cache.mode = static_cast<enum sd_cache_mode_t>(comp["mode"].get<int>());
            if (comp.contains("reuse_threshold")) cache.reuse_threshold = comp["reuse_threshold"].get<float>();
            if (comp.contains("start_percent")) cache.start_percent = comp["start_percent"].get<float>();
            if (comp.contains("end_percent")) cache.end_percent = comp["end_percent"].get<float>();
            if (comp.contains("error_decay_rate")) cache.error_decay_rate = comp["error_decay_rate"].get<float>();
            if (comp.contains("use_relative_threshold")) cache.use_relative_threshold = comp["use_relative_threshold"].get<bool>();
            if (comp.contains("reset_error_on_compute")) cache.reset_error_on_compute = comp["reset_error_on_compute"].get<bool>();
            if (comp.contains("Fn_compute_blocks")) cache.Fn_compute_blocks = comp["Fn_compute_blocks"].get<int>();
            if (comp.contains("Bn_compute_blocks")) cache.Bn_compute_blocks = comp["Bn_compute_blocks"].get<int>();
            if (comp.contains("residual_diff_threshold")) cache.residual_diff_threshold = comp["residual_diff_threshold"].get<float>();
            if (comp.contains("max_warmup_steps")) cache.max_warmup_steps = comp["max_warmup_steps"].get<int>();
            if (comp.contains("max_cached_steps")) cache.max_cached_steps = comp["max_cached_steps"].get<int>();
            if (comp.contains("max_continuous_cached_steps")) cache.max_continuous_cached_steps = comp["max_continuous_cached_steps"].get<int>();
            if (comp.contains("taylorseer_n_derivatives")) cache.taylorseer_n_derivatives = comp["taylorseer_n_derivatives"].get<int>();
            if (comp.contains("taylorseer_skip_interval")) cache.taylorseer_skip_interval = comp["taylorseer_skip_interval"].get<int>();
            if (comp.contains("scm_policy_dynamic")) cache.scm_policy_dynamic = comp["scm_policy_dynamic"].get<bool>();
            if (comp.contains("spectrum_w")) cache.spectrum_w = comp["spectrum_w"].get<float>();
            if (comp.contains("spectrum_m")) cache.spectrum_m = comp["spectrum_m"].get<int>();
            if (comp.contains("spectrum_lam")) cache.spectrum_lam = comp["spectrum_lam"].get<float>();
            if (comp.contains("spectrum_window_size")) cache.spectrum_window_size = comp["spectrum_window_size"].get<int>();
            if (comp.contains("spectrum_flex_window")) cache.spectrum_flex_window = comp["spectrum_flex_window"].get<float>();
            if (comp.contains("spectrum_warmup_steps")) cache.spectrum_warmup_steps = comp["spectrum_warmup_steps"].get<int>();
            if (comp.contains("spectrum_stop_percent")) cache.spectrum_stop_percent = comp["spectrum_stop_percent"].get<float>();
            return true;
        }
        catch (...) { return false; }
    }

    inline bool parseTilingParams(const nlohmann::json& comp, sd_tiling_params_t& tiling) {
        try {
            if (comp.contains("isTiled")) tiling.enabled = comp["isTiled"].get<bool>();
            if (comp.contains("temporal_tiling")) tiling.temporal_tiling = comp["temporal_tiling"].get<bool>();
            if (comp.contains("tile_size_x")) tiling.tile_size_x = comp["tile_size_x"].get<int>();
            if (comp.contains("tile_size_y")) tiling.tile_size_y = comp["tile_size_y"].get<int>();
            if (comp.contains("target_overlap")) tiling.target_overlap = comp["target_overlap"].get<float>();
            if (comp.contains("rel_size_x")) tiling.rel_size_x = comp["rel_size_x"].get<float>();
            if (comp.contains("rel_size_y")) tiling.rel_size_y = comp["rel_size_y"].get<float>();
            return true;
        }
        catch (...) { return false; }
    }

    inline bool parseHiresParams(const nlohmann::json& comp, sd_hires_params_t& hires, ResourceManager& res) {
        try {
            if (comp.contains("enabled")) hires.enabled = comp["enabled"].get<bool>();
            if (comp.contains("upscaler")) hires.upscaler = static_cast<enum sd_hires_upscaler_t>(comp["upscaler"].get<int>());
            if (comp.contains("model_path") && !comp["model_path"].is_null()) {
                std::string val = comp["model_path"].get<std::string>();
                if (!val.empty()) hires.model_path = res.storeString(val);
            }
            if (comp.contains("scale")) hires.scale = comp["scale"].get<float>();
            if (comp.contains("target_width")) hires.target_width = comp["target_width"].get<int>();
            if (comp.contains("target_height")) hires.target_height = comp["target_height"].get<int>();
            if (comp.contains("steps")) hires.steps = comp["steps"].get<int>();
            if (comp.contains("denoising_strength")) hires.denoising_strength = comp["denoising_strength"].get<float>();
            if (comp.contains("upscale_tile_size")) hires.upscale_tile_size = comp["upscale_tile_size"].get<int>();
            if (comp.contains("custom_sigmas") && comp["custom_sigmas"].is_array()) {
                auto sigmas = comp["custom_sigmas"].get<std::vector<float>>();
                hires.custom_sigmas = res.storeFloats(sigmas);
                hires.custom_sigmas_count = static_cast<int>(sigmas.size());
            }
            return true;
        }
        catch (...) { return false; }
    }

    inline bool parsePMParams(const nlohmann::json& comp, sd_pm_params_t& pm, ResourceManager& res) {
        try {
            if (comp.contains("modelPath") && !comp["modelPath"].is_null()) {
                std::string val = comp["modelPath"].get<std::string>();
                if (!val.empty()) pm.id_embed_path = res.storeString(val);
            }
            if (comp.contains("styleStrength")) pm.style_strength = comp["styleStrength"].get<float>();
            return true;
        }
        catch (...) { return false; }
    }

    inline bool parsePulidParams(const nlohmann::json& comp, sd_pulid_params_t& pulid, ResourceManager& res) {
        try {
            if (comp.contains("modelPath") && !comp["modelPath"].is_null()) {
                std::string val = comp["modelPath"].get<std::string>();
                if (!val.empty()) pulid.id_embedding_path = res.storeString(val);
            }
            if (comp.contains("id_weight")) pulid.id_weight = comp["id_weight"].get<float>();
            return true;
        }
        catch (...) { return false; }
    }

    inline bool parseLoras(const nlohmann::json& comp, std::vector<sd_lora_t>& loras, ResourceManager& res) {
        try {
            nlohmann::json loraArray;
            if (comp.is_array()) {
                loraArray = comp;
            }
            else if (comp.contains("loras") && comp["loras"].is_array()) {
                loraArray = comp["loras"];
            }
            else {
                return false;
            }
            for (const auto& item : loraArray) {
                sd_lora_t lora{};
                if (item.contains("path") && !item["path"].is_null()) {
                    std::string val = item["path"].get<std::string>();
                    if (!val.empty()) lora.path = res.storeString(val);
                }
                if (item.contains("multiplier")) lora.multiplier = item["multiplier"].get<float>();
                if (item.contains("is_high_noise")) lora.is_high_noise = item["is_high_noise"].get<bool>();
                loras.push_back(lora);
            }
            return true;
        }
        catch (...) { return false; }
    }

    inline bool loadImageFromPath(const std::string& filePath, sd_image_t& out, ResourceManager& res) {
        if (filePath.empty()) return false;
        int w, h, c;
        unsigned char* data = stbi_load(filePath.c_str(), &w, &h, &c, 0);
        if (!data) return false;
        out.width = w;
        out.height = h;
        out.channel = c;
        out.data = data;
        res.storeImage(out);
        return true;
    }

    inline bool parseImageGenParams(const nlohmann::json& metadata, sd_img_gen_params_t& params, ResourceManager& res) {
        params.loras = nullptr;
        params.lora_count = 0;
        params.prompt = nullptr;
        params.negative_prompt = nullptr;
        params.init_image = { 0,0,0,nullptr };
        params.ref_images = nullptr;
        params.ref_images_count = 0;
        params.mask_image = { 0,0,0,nullptr };
        params.control_image = { 0,0,0,nullptr };
        params.pm_params.id_images = nullptr;
        params.pm_params.id_images_count = 0;
        params.pm_params.id_embed_path = nullptr;
        params.pm_params.style_strength = 0.0f;
        params.pulid_params.id_embedding_path = nullptr;
        params.pulid_params.id_weight = 0.0f;
        params.vae_tiling_params = { false, false, 64, 64, 0.0f, 64.0f, 64.0f, nullptr };
        sd_cache_params_init(&params.cache);
        sd_hires_params_init(&params.hires);

        std::vector<sd_lora_t> loras;
        std::vector<sd_image_t> idImages;
        std::vector<sd_image_t> refImages;

        if (!metadata.contains("components") || !metadata["components"].is_array()) {
            return false;
        }

        for (const auto& comp : metadata["components"]) {
            if (comp.contains("Prompt")) {
                const auto& p = comp["Prompt"];
                if (p.contains("posPrompt") && !p["posPrompt"].is_null()) {
                    std::string val = p["posPrompt"].get<std::string>();
                    if (!val.empty()) params.prompt = res.storeString(val);
                }
                if (p.contains("negPrompt") && !p["negPrompt"].is_null()) {
                    std::string val = p["negPrompt"].get<std::string>();
                    if (!val.empty()) params.negative_prompt = res.storeString(val);
                }
            }
            if (comp.contains("ClipSkip")) {
                const auto& cs = comp["ClipSkip"];
                if (cs.contains("clipSkip")) params.clip_skip = cs["clipSkip"].get<int>();
            }
            if (comp.contains("Latent")) {
                const auto& lat = comp["Latent"];
                if (lat.contains("latentWidth")) params.width = lat["latentWidth"].get<int>();
                if (lat.contains("latentHeight")) params.height = lat["latentHeight"].get<int>();
                if (lat.contains("batchSize")) params.batch_count = lat["batchSize"].get<int>();
                if (lat.contains("seed")) params.seed = static_cast<int64_t>(lat["seed"].get<int>());
            }
            if (comp.contains("Vae")) {
                parseTilingParams(comp["Vae"], params.vae_tiling_params);
            }
            if (comp.contains("Guidance")) {
                parseGuidanceParams(comp["Guidance"], params.sample_params, res);
                if (comp["Guidance"].contains("enable_slg") && comp["Guidance"]["enable_slg"].get<bool>()) {
                    parseSLGParams(comp["Guidance"], params.sample_params.guidance.slg, res);
                }
            }
            if (comp.contains("Sampler")) {
                const auto& samp = comp["Sampler"];
                if (samp.contains("seed")) params.seed = static_cast<int64_t>(samp["seed"].get<int>());
                if (samp.contains("denoise")) params.strength = samp["denoise"].get<float>();
                if (samp.contains("batchCount")) params.batch_count = samp["batchCount"].get<int>();
                parseSampleParams(samp, params.sample_params, res);
            }
            if (comp.contains("EasyCache")) {
                parseCacheParams(comp["EasyCache"], params.cache);
            }
            if (comp.contains("Hires")) {
                parseHiresParams(comp["Hires"], params.hires, res);
            }
            if (comp.contains("PhotoMaker")) {
                parsePMParams(comp["PhotoMaker"], params.pm_params, res);
            }
            if (comp.contains("PulidWeights")) {
                parsePulidParams(comp["PulidWeights"], params.pulid_params, res);
            }
            if (comp.contains("Lora")) {
                parseLoras(comp["Lora"], loras, res);
            }
            if (comp.contains("InputImage") || comp.contains("InitImage")) {
                const auto& imgData = comp.contains("InputImage") ? comp["InputImage"] : comp["InitImage"];
                if (imgData.contains("filePath") && !imgData["filePath"].is_null()) {
                    std::string path = imgData["filePath"].get<std::string>();
                    loadImageFromPath(path, params.init_image, res);
                }
            }
            if (comp.contains("MaskImage")) {
                const auto& m = comp["MaskImage"];
                if (m.contains("filePath") && !m["filePath"].is_null()) {
                    std::string path = m["filePath"].get<std::string>();
                    loadImageFromPath(path, params.mask_image, res);
                }
            }
            if (comp.contains("ControlNetImage")) {
                const auto& ctrl = comp["ControlNetImage"];
                if (ctrl.contains("filePath") && !ctrl["filePath"].is_null()) {
                    std::string path = ctrl["filePath"].get<std::string>();
                    loadImageFromPath(path, params.control_image, res);
                }
                if (ctrl.contains("strength")) params.control_strength = ctrl["strength"].get<float>();
                else if (comp.contains("ControlNet") && comp["ControlNet"].contains("cnStrength")) {
                    params.control_strength = comp["ControlNet"]["cnStrength"].get<float>();
                }
            }
            if (comp.contains("PhotoMakerImage")) {
                const auto& pmImg = comp["PhotoMakerImage"];
                if (pmImg.contains("filePath") && !pmImg["filePath"].is_null()) {
                    sd_image_t img{ 0,0,0,nullptr };
                    std::string path = pmImg["filePath"].get<std::string>();
                    if (loadImageFromPath(path, img, res)) {
                        idImages.push_back(img);
                    }
                }
                if (pmImg.contains("styleStrength")) {
                    params.pm_params.style_strength = pmImg["styleStrength"].get<float>();
                }
            }
            if (comp.contains("RefImages")) {
                const auto& refs = comp["RefImages"];
                nlohmann::json paths;
                if (refs.is_array()) {
                    paths = refs;
                }
                else if (refs.contains("filePaths") && refs["filePaths"].is_array()) {
                    paths = refs["filePaths"];
                }
                else if (refs.contains("filePath")) {
                    paths = nlohmann::json::array({ refs });
                }
                if (paths.is_array()) {
                    for (const auto& item : paths) {
                        std::string path;
                        if (item.is_string()) path = item.get<std::string>();
                        else if (item.contains("filePath")) path = item["filePath"].get<std::string>();
                        if (!path.empty()) {
                            sd_image_t img{ 0,0,0,nullptr };
                            if (loadImageFromPath(path, img, res)) {
                                refImages.push_back(img);
                            }
                        }
                    }
                }
            }
        }

        if (!loras.empty()) {
            res.loraStorage = std::move(loras);
            params.loras = res.loraStorage.data();
            params.lora_count = static_cast<uint32_t>(res.loraStorage.size());
        }

        if (!idImages.empty()) {
            params.pm_params.id_images = idImages.data();
            params.pm_params.id_images_count = static_cast<int>(idImages.size());
            for (auto& img : idImages) {
                res.storeImage(img);
            }
        }

        if (!refImages.empty()) {
            params.ref_images = refImages.data();
            params.ref_images_count = static_cast<int>(refImages.size());
            for (auto& img : refImages) {
                res.storeImage(img);
            }
        }

        if (params.width == 0) params.width = 512;
        if (params.height == 0) params.height = 512;
        if (params.batch_count == 0) params.batch_count = 1;
        if (params.seed < 0) params.seed = -1;

        return true;
    }

    inline bool parseContextParams(const nlohmann::json& metadata, sd_ctx_params_t& ctx, ResourceManager& res) {
        sd_ctx_params_init(&ctx);

        for (const auto& comp : metadata["components"]) {
            if (comp.contains("Checkpoint")) {
                const auto& m = comp["Checkpoint"];
                if (m.contains("modelPath") && !m["modelPath"].is_null()) {
                    std::string val = m["modelPath"].get<std::string>();
                    if (!val.empty()) ctx.model_path = res.storeString(val);
                }
            }
            if (comp.contains("Vae")) {
                const auto& v = comp["Vae"];
                if (v.contains("modelPath") && !v["modelPath"].is_null()) {
                    std::string val = v["modelPath"].get<std::string>();
                    if (!val.empty()) ctx.vae_path = res.storeString(val);
                }
                if (v.contains("vae_format")) {
                    int idx = v["vae_format"].get<int>();
                    if (idx == 0) {
                        ctx.vae_format = SD_VAE_FORMAT_AUTO;
                    }
                    else {
                        ctx.vae_format = static_cast<enum sd_vae_format_t>(idx - 1);
                    }
                }
            }
            if (comp.contains("ClipL")) {
                const auto& c = comp["ClipL"];
                if (c.contains("modelPath") && !c["modelPath"].is_null()) {
                    std::string val = c["modelPath"].get<std::string>();
                    if (!val.empty()) ctx.clip_l_path = res.storeString(val);
                }
            }
            if (comp.contains("ClipG")) {
                const auto& c = comp["ClipG"];
                if (c.contains("modelPath") && !c["modelPath"].is_null()) {
                    std::string val = c["modelPath"].get<std::string>();
                    if (!val.empty()) ctx.clip_g_path = res.storeString(val);
                }
            }
            if (comp.contains("ClipVision")) {
                const auto& c = comp["ClipVision"];
                if (c.contains("modelPath") && !c["modelPath"].is_null()) {
                    std::string val = c["modelPath"].get<std::string>();
                    if (!val.empty()) ctx.clip_vision_path = res.storeString(val);
                }
            }
            if (comp.contains("T5XXL")) {
                const auto& t = comp["T5XXL"];
                if (t.contains("modelPath") && !t["modelPath"].is_null()) {
                    std::string val = t["modelPath"].get<std::string>();
                    if (!val.empty()) ctx.t5xxl_path = res.storeString(val);
                }
            }
            if (comp.contains("LlmEncoder")) {
                const auto& l = comp["LlmEncoder"];
                if (l.contains("modelPath") && !l["modelPath"].is_null()) {
                    std::string val = l["modelPath"].get<std::string>();
                    if (!val.empty()) ctx.llm_path = res.storeString(val);
                }
            }
            if (comp.contains("LlmVision")) {
                const auto& l = comp["LlmVision"];
                if (l.contains("modelPath") && !l["modelPath"].is_null()) {
                    std::string val = l["modelPath"].get<std::string>();
                    if (!val.empty()) ctx.llm_vision_path = res.storeString(val);
                }
            }
            if (comp.contains("DiffusionModel")) {
                const auto& d = comp["DiffusionModel"];
                if (d.contains("modelPath") && !d["modelPath"].is_null()) {
                    std::string val = d["modelPath"].get<std::string>();
                    if (!val.empty()) ctx.diffusion_model_path = res.storeString(val);
                }
            }
            if (comp.contains("HighNoiseDiffusionModel")) {
                const auto& h = comp["HighNoiseDiffusionModel"];
                if (h.contains("modelPath") && !h["modelPath"].is_null()) {
                    std::string val = h["modelPath"].get<std::string>();
                    if (!val.empty()) ctx.high_noise_diffusion_model_path = res.storeString(val);
                }
            }
            if (comp.contains("UncondDiffusionModel")) {
                const auto& u = comp["UncondDiffusionModel"];
                if (u.contains("modelPath") && !u["modelPath"].is_null()) {
                    std::string val = u["modelPath"].get<std::string>();
                    if (!val.empty()) ctx.uncond_diffusion_model_path = res.storeString(val);
                }
            }
            if (comp.contains("Embedding")) {
                const auto& e = comp["Embedding"];
                if (e.contains("modelPath") && !e["modelPath"].is_null()) {
                    std::string val = e["modelPath"].get<std::string>();
                    if (!val.empty()) ctx.embeddings_connectors_path = res.storeString(val);
                }
            }
            if (comp.contains("AudioVAE")) {
                const auto& a = comp["AudioVAE"];
                if (a.contains("modelPath") && !a["modelPath"].is_null()) {
                    std::string val = a["modelPath"].get<std::string>();
                    if (!val.empty()) ctx.audio_vae_path = res.storeString(val);
                }
            }
            if (comp.contains("Taesd")) {
                const auto& t = comp["Taesd"];
                if (t.contains("modelPath") && !t["modelPath"].is_null()) {
                    std::string val = t["modelPath"].get<std::string>();
                    if (!val.empty()) ctx.taesd_path = res.storeString(val);
                }
            }
            if (comp.contains("ControlNet")) {
                const auto& c = comp["ControlNet"];
                if (c.contains("modelPath") && !c["modelPath"].is_null()) {
                    std::string val = c["modelPath"].get<std::string>();
                    if (!val.empty()) ctx.control_net_path = res.storeString(val);
                }
            }
            if (comp.contains("MotionModule")) {
                const auto& m = comp["MotionModule"];
                if (m.contains("modelPath") && !m["modelPath"].is_null()) {
                    std::string val = m["modelPath"].get<std::string>();
                    if (!val.empty()) ctx.motion_module_path = res.storeString(val);
                }
            }
            if (comp.contains("PhotoMaker")) {
                const auto& p = comp["PhotoMaker"];
                if (p.contains("modelPath") && !p["modelPath"].is_null()) {
                    std::string val = p["modelPath"].get<std::string>();
                    if (!val.empty()) ctx.photo_maker_path = res.storeString(val);
                }
            }
            if (comp.contains("PulidWeights")) {
                const auto& p = comp["PulidWeights"];
                if (p.contains("modelPath") && !p["modelPath"].is_null()) {
                    std::string val = p["modelPath"].get<std::string>();
                    if (!val.empty()) ctx.pulid_weights_path = res.storeString(val);
                }
            }
            if (comp.contains("Sampler")) {
                const auto& s = comp["Sampler"];
                if (s.contains("n_threads")) ctx.n_threads = s["n_threads"].get<int>();
                if (s.contains("current_type_method")) ctx.wtype = static_cast<enum sd_type_t>(s["current_type_method"].get<int>());
                if (s.contains("current_rng_type")) ctx.rng_type = static_cast<enum rng_type_t>(s["current_rng_type"].get<int>());
                if (s.contains("sampler_rng_type")) ctx.sampler_rng_type = static_cast<enum rng_type_t>(s["sampler_rng_type"].get<int>());
                if (s.contains("current_prediction_type")) ctx.prediction = static_cast<enum prediction_t>(s["current_prediction_type"].get<int>());
                if (s.contains("lora_apply_mode")) ctx.lora_apply_mode = static_cast<enum lora_apply_mode_t>(s["lora_apply_mode"].get<int>());
                if (s.contains("enable_mmap")) ctx.enable_mmap = s["enable_mmap"].get<bool>();
                if (s.contains("flash_attn")) ctx.flash_attn = s["flash_attn"].get<bool>();
                if (s.contains("diffusion_flash_attn")) ctx.diffusion_flash_attn = s["diffusion_flash_attn"].get<bool>();
                if (s.contains("tae_preview_only")) ctx.tae_preview_only = s["tae_preview_only"].get<bool>();
                if (s.contains("diffusion_conv_direct")) ctx.diffusion_conv_direct = s["diffusion_conv_direct"].get<bool>();
                if (s.contains("vae_conv_direct")) ctx.vae_conv_direct = s["vae_conv_direct"].get<bool>();
                if (s.contains("force_sdxl_vae_conv_scale")) ctx.force_sdxl_vae_conv_scale = s["force_sdxl_vae_conv_scale"].get<bool>();
                if (s.contains("max_vram") && !s["max_vram"].is_null()) {
                    std::string val = s["max_vram"].get<std::string>();
                    if (!val.empty()) ctx.max_vram = res.storeString(val);
                }
                if (s.contains("stream_layers")) ctx.stream_layers = s["stream_layers"].get<bool>();
                if (s.contains("eager_load")) ctx.eager_load = s["eager_load"].get<bool>();
                if (s.contains("backend") && !s["backend"].is_null()) {
                    std::string val = s["backend"].get<std::string>();
                    if (!val.empty()) ctx.backend = res.storeString(val);
                }
                if (s.contains("params_backend") && !s["params_backend"].is_null()) {
                    std::string val = s["params_backend"].get<std::string>();
                    if (!val.empty()) ctx.params_backend = res.storeString(val);
                }
                if (s.contains("split_mode") && !s["split_mode"].is_null()) {
                    std::string val = s["split_mode"].get<std::string>();
                    if (!val.empty()) ctx.split_mode = res.storeString(val);
                }
                if (s.contains("auto_fit")) ctx.auto_fit = s["auto_fit"].get<bool>();
                if (s.contains("rpc_servers") && !s["rpc_servers"].is_null()) {
                    std::string val = s["rpc_servers"].get<std::string>();
                    if (!val.empty()) ctx.rpc_servers = res.storeString(val);
                }
                if (s.contains("model_args") && !s["model_args"].is_null()) {
                    std::string val = s["model_args"].get<std::string>();
                    if (!val.empty()) ctx.model_args = res.storeString(val);
                }
            }
            if (comp.contains("Embeddings") && comp["Embeddings"].is_array()) {
                const auto& embeds = comp["Embeddings"];
                for (const auto& e : embeds) {
                    if (e.contains("name") && e.contains("path")) {
                        sd_embedding_t emb;
                        std::string name = e["name"].get<std::string>();
                        std::string path = e["path"].get<std::string>();
                        if (!name.empty() && !path.empty()) {
                            emb.name = res.storeString(name);
                            emb.path = res.storeString(path);
                            res.embeddingStorage.push_back(emb);
                        }
                    }
                }
                if (!res.embeddingStorage.empty()) {
                    ctx.embeddings = res.embeddingStorage.data();
                    ctx.embedding_count = static_cast<uint32_t>(res.embeddingStorage.size());
                }
            }
            if (comp.contains("Conversion")) {
                const auto& conv = comp["Conversion"];
                if (conv.contains("tensorTypeRules") && !conv["tensorTypeRules"].is_null()) {
                    std::string val = conv["tensorTypeRules"].get<std::string>();
                    if (!val.empty()) ctx.tensor_type_rules = res.storeString(val);
                }
            }
        }

        if (ctx.n_threads == 0) ctx.n_threads = -1;
        if (ctx.wtype == 0) ctx.wtype = SD_TYPE_F16;

        return true;
    }

    inline bool parseVideoGenParams(const nlohmann::json& metadata, sd_vid_gen_params_t& params, ResourceManager& res) {
        sd_vid_gen_params_init(&params);

        std::vector<sd_lora_t> loras;
        std::vector<sd_image_t> controlFramesVec;

        for (const auto& comp : metadata["components"]) {
            if (comp.contains("VideoParams")) {
                const auto& vp = comp["VideoParams"];
                if (vp.contains("video_frames")) params.video_frames = vp["video_frames"].get<int>();
                if (vp.contains("fps")) params.fps = vp["fps"].get<int>();
                if (vp.contains("vace_strength")) params.vace_strength = vp["vace_strength"].get<float>();
                if (vp.contains("moe_boundary")) params.moe_boundary = vp["moe_boundary"].get<float>();
            }
            if (comp.contains("Prompt")) {
                const auto& p = comp["Prompt"];
                if (p.contains("posPrompt") && !p["posPrompt"].is_null()) {
                    std::string val = p["posPrompt"].get<std::string>();
                    if (!val.empty()) params.prompt = res.storeString(val);
                }
                if (p.contains("negPrompt") && !p["negPrompt"].is_null()) {
                    std::string val = p["negPrompt"].get<std::string>();
                    if (!val.empty()) params.negative_prompt = res.storeString(val);
                }
            }
            if (comp.contains("ClipSkip")) {
                const auto& cs = comp["ClipSkip"];
                if (cs.contains("clipSkip")) params.clip_skip = cs["clipSkip"].get<int>();
            }
            if (comp.contains("Latent")) {
                const auto& lat = comp["Latent"];
                if (lat.contains("latentWidth")) params.width = lat["latentWidth"].get<int>();
                if (lat.contains("latentHeight")) params.height = lat["latentHeight"].get<int>();
                if (lat.contains("seed")) params.seed = static_cast<int64_t>(lat["seed"].get<int>());
            }
            if (comp.contains("Vae")) {
                parseTilingParams(comp["Vae"], params.vae_tiling_params);
            }
            if (comp.contains("Guidance")) {
                parseGuidanceParams(comp["Guidance"], params.sample_params, res);
                if (comp["Guidance"].contains("enable_slg") && comp["Guidance"]["enable_slg"].get<bool>()) {
                    parseSLGParams(comp["Guidance"], params.sample_params.guidance.slg, res);
                }
            }
            if (comp.contains("Sampler")) {
                const auto& samp = comp["Sampler"];
                if (samp.contains("seed")) params.seed = static_cast<int64_t>(samp["seed"].get<int>());
                if (samp.contains("denoise")) params.strength = samp["denoise"].get<float>();
                parseSampleParams(samp, params.sample_params, res);
            }
            if (comp.contains("HighNoiseSampler")) {
                const auto& hn = comp["HighNoiseSampler"];
                parseSampleParams(hn, params.high_noise_sample_params, res);
            }
            if (comp.contains("EasyCache")) {
                parseCacheParams(comp["EasyCache"], params.cache);
            }
            if (comp.contains("Hires")) {
                parseHiresParams(comp["Hires"], params.hires, res);
            }
            if (comp.contains("Lora")) {
                parseLoras(comp["Lora"], loras, res);
            }
            if (comp.contains("InputImage") || comp.contains("InitImage")) {
                const auto& imgData = comp.contains("InputImage") ? comp["InputImage"] : comp["InitImage"];
                if (imgData.contains("filePath") && !imgData["filePath"].is_null()) {
                    std::string path = imgData["filePath"].get<std::string>();
                    loadImageFromPath(path, params.init_image, res);
                }
            }
            if (comp.contains("EndImage")) {
                const auto& end = comp["EndImage"];
                if (end.contains("filePath") && !end["filePath"].is_null()) {
                    std::string path = end["filePath"].get<std::string>();
                    loadImageFromPath(path, params.end_image, res);
                }
            }
            if (comp.contains("ControlFrames")) {
                const auto& cf = comp["ControlFrames"];
                nlohmann::json paths;
                if (cf.is_array()) {
                    paths = cf;
                }
                else if (cf.contains("filePaths") && cf["filePaths"].is_array()) {
                    paths = cf["filePaths"];
                }
                if (paths.is_array()) {
                    for (const auto& item : paths) {
                        std::string path;
                        if (item.is_string()) path = item.get<std::string>();
                        else if (item.contains("filePath")) path = item["filePath"].get<std::string>();
                        if (!path.empty()) {
                            sd_image_t img{ 0,0,0,nullptr };
                            if (loadImageFromPath(path, img, res)) {
                                controlFramesVec.push_back(img);
                            }
                        }
                    }
                }
            }
        }

        if (!loras.empty()) {
            res.loraStorage = std::move(loras);
            params.loras = res.loraStorage.data();
            params.lora_count = static_cast<uint32_t>(res.loraStorage.size());
        }

        if (!controlFramesVec.empty()) {
            params.control_frames = controlFramesVec.data();
            params.control_frames_size = static_cast<int>(controlFramesVec.size());
            for (auto& img : controlFramesVec) {
                res.storeControlFrame(img);
            }
        }

        if (params.width == 0) params.width = 512;
        if (params.height == 0) params.height = 512;
        if (params.seed < 0) params.seed = -1;

        return true;
    }

} // namespace SDCPP