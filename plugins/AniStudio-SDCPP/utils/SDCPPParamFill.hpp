// SDCPPParamFill.hpp
#pragma once

#include "stable-diffusion.h"
#include "DiffusionOptions.hpp"
#include "SDCPPUtils.hpp"
#include "ECS.h"
#include "SDCPPComponents.h"
#include "VideoUtils.hpp"
#include "ModelCacheSystem.hpp"

#include <string>
#include <vector>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}

namespace SDCPP {

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    inline std::vector<int> ParseIntList(const std::string& s) {
        std::vector<int> out;
        std::stringstream ss(s);
        std::string tok;
        while (std::getline(ss, tok, ',')) {
            size_t a = tok.find_first_not_of(" \t");
            size_t b = tok.find_last_not_of(" \t");
            if (a == std::string::npos) continue;
            try { out.push_back(std::stoi(tok.substr(a, b - a + 1))); }
            catch (...) {}
        }
        return out;
    }

    // Merge a user-provided `params_backend` string with per-module CPU pins
    // derived from the SamplerComponent booleans. sdcpp applies per-module
    // assignments as "last one wins", so we append our derived pins after the
    // user's existing string and let sdcpp resolve duplicates.
    inline std::string BuildParamsBackend(const std::string& existing,
        bool clipOnCpu,
        bool controlNetOnCpu,
        bool offloadAll)
    {
        std::string out = existing;
        auto append = [&](const std::string& s) {
            if (!out.empty()) out += ",";
            out += s;
            };
        if (clipOnCpu)       append("te=cpu");
        if (controlNetOnCpu) append("controlnet=cpu");
        if (offloadAll)      append("*=cpu");
        return out;
    }

    // -------------------------------------------------------------------------
    // FillContextParams ? reads every model / settings component into
    // sd_ctx_params_t. All strings go into `res`.
    // -------------------------------------------------------------------------
    inline void FillContextParams(ECS::EntityManager& mgr,
        ECS::EntityID e,
        sd_ctx_params_t& ctx,
        ResourceManager& res)
    {
        sd_ctx_params_init(&ctx);

        auto set = [&](const std::string& s, const char*& field) {
            if (!s.empty()) field = res.storeString(s);
            };

        // ---- Model paths ----------------------------------------------------
        if (mgr.HasComponent<ECS::CheckpointComponent>(e))
            set(mgr.GetComponent<ECS::CheckpointComponent>(e).modelPath, ctx.model_path);

        if (mgr.HasComponent<ECS::DiffusionModelComponent>(e))
            set(mgr.GetComponent<ECS::DiffusionModelComponent>(e).modelPath, ctx.diffusion_model_path);

        if (mgr.HasComponent<ECS::HighNoiseDiffusionModelComponent>(e))
            set(mgr.GetComponent<ECS::HighNoiseDiffusionModelComponent>(e).modelPath, ctx.high_noise_diffusion_model_path);

        if (mgr.HasComponent<ECS::UncondDiffusionModelComponent>(e))
            set(mgr.GetComponent<ECS::UncondDiffusionModelComponent>(e).modelPath, ctx.uncond_diffusion_model_path);

        if (mgr.HasComponent<ECS::VaeComponent>(e)) {
            auto& v = mgr.GetComponent<ECS::VaeComponent>(e);
            set(v.modelPath, ctx.vae_path);
            ctx.vae_format = v.get_vae_format_enum();
        }

        if (mgr.HasComponent<ECS::ClipLComponent>(e))
            set(mgr.GetComponent<ECS::ClipLComponent>(e).modelPath, ctx.clip_l_path);

        if (mgr.HasComponent<ECS::ClipGComponent>(e))
            set(mgr.GetComponent<ECS::ClipGComponent>(e).modelPath, ctx.clip_g_path);

        if (mgr.HasComponent<ECS::ClipVisionComponent>(e))
            set(mgr.GetComponent<ECS::ClipVisionComponent>(e).modelPath, ctx.clip_vision_path);

        if (mgr.HasComponent<ECS::T5XXLComponent>(e))
            set(mgr.GetComponent<ECS::T5XXLComponent>(e).modelPath, ctx.t5xxl_path);

        if (mgr.HasComponent<ECS::LlmEncoderComponent>(e))
            set(mgr.GetComponent<ECS::LlmEncoderComponent>(e).modelPath, ctx.llm_path);

        if (mgr.HasComponent<ECS::LlmVisionComponent>(e))
            set(mgr.GetComponent<ECS::LlmVisionComponent>(e).modelPath, ctx.llm_vision_path);

        if (mgr.HasComponent<ECS::TokenizerComponent>(e))
            set(mgr.GetComponent<ECS::TokenizerComponent>(e).modelPath, ctx.tokenizer);

        if (mgr.HasComponent<ECS::AudioVaeComponent>(e))
            set(mgr.GetComponent<ECS::AudioVaeComponent>(e).modelPath, ctx.audio_vae_path);

        if (mgr.HasComponent<ECS::TaesdComponent>(e)) {
            auto& t = mgr.GetComponent<ECS::TaesdComponent>(e);
            set(t.modelPath, ctx.taesd_path);
            ctx.tae_preview_only = t.tae_preview_only;
        }

        if (mgr.HasComponent<ECS::ControlNetComponent>(e))
            set(mgr.GetComponent<ECS::ControlNetComponent>(e).modelPath, ctx.control_net_path);

        if (mgr.HasComponent<ECS::MotionModuleComponent>(e))
            set(mgr.GetComponent<ECS::MotionModuleComponent>(e).modelPath, ctx.motion_module_path);

        if (mgr.HasComponent<ECS::PhotoMakerComponent>(e))
            set(mgr.GetComponent<ECS::PhotoMakerComponent>(e).modelPath, ctx.photo_maker_path);

        if (mgr.HasComponent<ECS::PulidWeightsComponent>(e))
            set(mgr.GetComponent<ECS::PulidWeightsComponent>(e).modelPath, ctx.pulid_weights_path);

        // ---- Embeddings -----------------------------------------------------
        if (mgr.HasComponent<ECS::EmbeddingsComponent>(e)) {
            auto& emb = mgr.GetComponent<ECS::EmbeddingsComponent>(e);
            for (const auto& [name, path] : emb.embeddings) {
                sd_embedding_t s{};
                s.name = res.storeString(name);
                s.path = res.storeString(path);
                res.embeddingStorage.push_back(s);
            }
            if (!res.embeddingStorage.empty()) {
                ctx.embeddings = res.embeddingStorage.data();
                ctx.embedding_count = (uint32_t)res.embeddingStorage.size();
            }
        }

        // ---- Sampler / runtime ----------------------------------------------
        bool clipOnCpu = false;
        bool controlNetOnCpu = false;
        bool offloadAll = false;
        std::string userParamsBackend;

        if (mgr.HasComponent<ECS::SamplerComponent>(e)) {
            auto& s = mgr.GetComponent<ECS::SamplerComponent>(e);
            ctx.n_threads = s.n_threads;
            clipOnCpu = s.keep_clip_on_cpu;
            controlNetOnCpu = s.keep_control_net_on_cpu;
            offloadAll = s.offload_params_to_cpu;
        }

        if (mgr.HasComponent<ECS::LatentComponent>(e)) {
            auto& l = mgr.GetComponent<ECS::LatentComponent>(e);
            ctx.rng_type = (rng_type_t)rng_type_from_name(l.current_rng_type);
            ctx.sampler_rng_type = (rng_type_t)rng_type_from_name(l.sampler_rng_type);
        }

        if (mgr.HasComponent<ECS::LoraComponent>(e))
            ctx.lora_apply_mode = mgr.GetComponent<ECS::LoraComponent>(e).get_lora_apply_mode_enum();

        if (mgr.HasComponent<ECS::ConversionComponent>(e)) {
            auto& c = mgr.GetComponent<ECS::ConversionComponent>(e);
            set(c.tensorTypeRules, ctx.tensor_type_rules);
            ctx.wtype = c.get_output_type_enum();
        }

        // ---- Global SDCPP settings -----------------------------------------
        if (mgr.HasComponent<ECS::SDCPPSettingsComponent>(e)) {
            auto& g = mgr.GetComponent<ECS::SDCPPSettingsComponent>(e);
            ctx.enable_mmap = g.enable_mmap;
            ctx.flash_attn = g.flash_attn;
            ctx.eager_load = g.eager_load;
            ctx.auto_fit = g.auto_fit;
            ctx.diffusion_flash_attn = g.diffusion_flash_attn;
            ctx.diffusion_conv_direct = g.diffusion_conv_direct;
            ctx.vae_conv_direct = g.vae_conv_direct;
            ctx.force_sdxl_vae_conv_scale = g.force_sdxl_vae_conv_scale;
            ctx.disable_prefetch = g.disable_prefetch;
            ctx.disable_segmented_compute = g.disable_segmented_compute;
            ctx.linear_scale = g.linear_scale;
            ctx.attn_scale = g.attn_scale;
            ctx.sage_attn = g.sage_attn;
            set(g.max_vram, ctx.max_vram);
            set(g.backend, ctx.backend);
            set(g.split_mode, ctx.split_mode);
            set(g.rpc_servers, ctx.rpc_servers);
            set(g.model_args, ctx.model_args);
            userParamsBackend = g.params_backend;
        }

        // ---- params_backend: merge user string with CPU pins ----------------
        std::string merged = BuildParamsBackend(userParamsBackend,
            clipOnCpu,
            controlNetOnCpu,
            offloadAll);
        if (!merged.empty())
            ctx.params_backend = res.storeString(merged);

        if (ctx.n_threads == 0) ctx.n_threads = -1;
        if (ctx.wtype == 0)     ctx.wtype = SD_TYPE_F16;
    }

    // -------------------------------------------------------------------------
    // FillSampleParamsFromComponents ? shared by image and video paths.
    // -------------------------------------------------------------------------
    inline void FillSampleParamsFromComponents(
        const ECS::SamplerComponent* samp,
        const ECS::GuidanceComponent* guid,
        const ECS::SLGComponent* slg,
        const ECS::CustomSigmasComponent* sig,
        sd_sample_params_t& out,
        ResourceManager& res)
    {
        sd_sample_params_init(&out);

        if (samp) {
            out.sample_steps = samp->steps;
            out.sample_method = (sample_method_t)sample_method_from_name(samp->current_sample_method);
            out.scheduler = (scheduler_t)scheduler_from_name(samp->current_scheduler_method);
            if (!samp->extra_sample_args.empty())
                out.extra_sample_args = res.storeString(samp->extra_sample_args);
        }
        if (guid) {
            out.guidance.txt_cfg = guid->txt_cfg;
            out.guidance.img_cfg = guid->img_cfg;
            out.guidance.distilled_guidance = guid->distilled_guidance;
            out.eta = guid->eta;
            out.shifted_timestep = guid->shifted_timestep;
            out.flow_shift = guid->flow_shift;
        }
        if (slg && slg->enable_slg) {
            out.guidance.slg.scale = slg->slg_scale;
            out.guidance.slg.layer_start = slg->slg_layer_start;
            out.guidance.slg.layer_end = slg->slg_layer_end;
            auto layers = ParseIntList(slg->slg_layers);
            if (!layers.empty()) {
                out.guidance.slg.layers = res.storeInts(layers);
                out.guidance.slg.layer_count = layers.size();
            }
        }
        if (sig && !sig->custom_sigmas.empty()) {
            out.custom_sigmas = res.storeFloats(sig->custom_sigmas);
            out.custom_sigmas_count = (int)sig->custom_sigmas.size();
        }
    }

    // -------------------------------------------------------------------------
    // FillCacheParams ? writes into res.cacheStorage and patches pointers.
    // -------------------------------------------------------------------------
    inline void FillCacheParams(const ECS::EasyCacheComponent& c, ResourceManager& res) {
        res.cacheStorage.mode = (sd_cache_mode_t)cache_mode_from_name(c.mode);
        res.cacheStorage.reuse_threshold = c.reuse_threshold;
        res.cacheStorage.start_percent = c.start_percent;
        res.cacheStorage.end_percent = c.end_percent;
        res.cacheStorage.error_decay_rate = c.error_decay_rate;
        res.cacheStorage.use_relative_threshold = c.use_relative_threshold;
        res.cacheStorage.reset_error_on_compute = c.reset_error_on_compute;
        res.cacheStorage.Fn_compute_blocks = c.Fn_compute_blocks;
        res.cacheStorage.Bn_compute_blocks = c.Bn_compute_blocks;
        res.cacheStorage.residual_diff_threshold = c.residual_diff_threshold;
        res.cacheStorage.max_warmup_steps = c.max_warmup_steps;
        res.cacheStorage.max_cached_steps = c.max_cached_steps;
        res.cacheStorage.max_continuous_cached_steps = c.max_continuous_cached_steps;
        res.cacheStorage.taylorseer_n_derivatives = c.taylorseer_n_derivatives;
        res.cacheStorage.taylorseer_skip_interval = c.taylorseer_skip_interval;
        if (!c.scm_mask.empty()) {
            res.cache_scm_mask = c.scm_mask;
            res.cacheStorage.scm_mask = res.cache_scm_mask.c_str();
        }
        res.cacheStorage.scm_policy_dynamic = c.scm_policy_dynamic;
        res.cacheStorage.spectrum_w = c.spectrum_w;
        res.cacheStorage.spectrum_m = c.spectrum_m;
        res.cacheStorage.spectrum_lam = c.spectrum_lam;
        res.cacheStorage.spectrum_window_size = c.spectrum_window_size;
        res.cacheStorage.spectrum_flex_window = c.spectrum_flex_window;
        res.cacheStorage.spectrum_warmup_steps = c.spectrum_warmup_steps;
        res.cacheStorage.spectrum_stop_percent = c.spectrum_stop_percent;
    }

    // -------------------------------------------------------------------------
    // FillHiresParams
    // -------------------------------------------------------------------------
    inline void FillHiresParams(const ECS::HiresComponent& h, ResourceManager& res) {
        res.hiresStorage.enabled = h.enabled;
        res.hiresStorage.upscaler = h.upscaler;
        if (!h.model_path.empty()) {
            res.hires_model_path = h.model_path;
            res.hiresStorage.model_path = res.hires_model_path.c_str();
        }
        res.hiresStorage.scale = h.scale;
        res.hiresStorage.target_width = h.target_width;
        res.hiresStorage.target_height = h.target_height;
        res.hiresStorage.steps = h.steps;
        res.hiresStorage.denoising_strength = h.denoising_strength;
        res.hiresStorage.upscale_tile_size = h.upscale_tile_size;
        if (!h.custom_sigmas.empty()) {
            res.hires_custom_sigmas = h.custom_sigmas;
            res.hiresStorage.custom_sigmas = res.hires_custom_sigmas.data();
            res.hiresStorage.custom_sigmas_count = (int)res.hires_custom_sigmas.size();
        }
    }

    // -------------------------------------------------------------------------
    // FillTilingParams
    // -------------------------------------------------------------------------
    inline void FillTilingParams(const ECS::VaeTilingComponent& t,
        sd_tiling_params_t& out,
        ResourceManager& res)
    {
        out.enabled = t.isTiled;
        out.temporal_tiling = t.temporal_tiling;
        out.tile_size_x = t.tile_size_x;
        out.tile_size_y = t.tile_size_y;
        out.target_overlap = t.target_overlap;
        out.rel_size_x = t.rel_size_x;
        out.rel_size_y = t.rel_size_y;
        if (!t.extra_tiling_args.empty())
            out.extra_tiling_args = res.storeString(t.extra_tiling_args);
    }

    // -------------------------------------------------------------------------
    // FillLoras
    // -------------------------------------------------------------------------
    inline void FillLoras(const ECS::LoraComponent& l,
        const sd_lora_t*& outLoras,
        uint32_t& outCount,
        ResourceManager& res)
    {
        for (const auto& entry : l.loras) {
            sd_lora_t s{};
            s.path = res.storeString(entry.path);
            s.multiplier = entry.multiplier;
            s.is_high_noise = entry.is_high_noise;
            res.loraStorage.push_back(s);
        }
        if (!res.loraStorage.empty()) {
            outLoras = res.loraStorage.data();
            outCount = (uint32_t)res.loraStorage.size();
        }
    }

    // -------------------------------------------------------------------------
    // DecodeAudioToFloat ? libavformat -> interleaved float PCM.
    // -------------------------------------------------------------------------
    inline bool DecodeAudioToFloat(const std::string& path,
        sd_audio_t& out,
        ResourceManager& res)
    {
        if (path.empty()) return false;

        AVFormatContext* fmt = nullptr;
        if (avformat_open_input(&fmt, path.c_str(), nullptr, nullptr) < 0) return false;
        if (avformat_find_stream_info(fmt, nullptr) < 0) {
            avformat_close_input(&fmt); return false;
        }
        int streamIdx = -1;
        for (unsigned i = 0; i < fmt->nb_streams; ++i) {
            if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                streamIdx = i; break;
            }
        }
        if (streamIdx < 0) { avformat_close_input(&fmt); return false; }

        const AVCodec* codec = avcodec_find_decoder(fmt->streams[streamIdx]->codecpar->codec_id);
        if (!codec) { avformat_close_input(&fmt); return false; }

        AVCodecContext* cctx = avcodec_alloc_context3(codec);
        if (!cctx) { avformat_close_input(&fmt); return false; }
        if (avcodec_parameters_to_context(cctx, fmt->streams[streamIdx]->codecpar) < 0) {
            avcodec_free_context(&cctx); avformat_close_input(&fmt); return false;
        }
        if (avcodec_open2(cctx, codec, nullptr) < 0) {
            avcodec_free_context(&cctx); avformat_close_input(&fmt); return false;
        }

        int outChannels = cctx->ch_layout.nb_channels > 0 ? cctx->ch_layout.nb_channels : 1;
        int outSampleRate = cctx->sample_rate > 0 ? cctx->sample_rate : 44100;

        SwrContext* swr = nullptr;
        AVChannelLayout outLayout;
        av_channel_layout_default(&outLayout, outChannels);
        swr_alloc_set_opts2(&swr,
            &outLayout, AV_SAMPLE_FMT_FLT, outSampleRate,
            &cctx->ch_layout, cctx->sample_fmt, cctx->sample_rate,
            0, nullptr);
        if (!swr || swr_init(swr) < 0) {
            if (swr) swr_free(&swr);
            avcodec_free_context(&cctx); avformat_close_input(&fmt);
            return false;
        }

        AVFrame* frame = av_frame_alloc();
        AVPacket* pkt = av_packet_alloc();
        std::vector<float> samples;
        samples.reserve(outSampleRate * outChannels * 10);

        auto drainFrame = [&]() {
            int maxOut = swr_get_out_samples(swr, frame->nb_samples) + 256;
            size_t base = samples.size();
            samples.resize(base + (size_t)maxOut * outChannels);
            uint8_t* outBuf = reinterpret_cast<uint8_t*>(samples.data() + base);
            int got = swr_convert(swr, &outBuf, maxOut,
                (const uint8_t**)frame->extended_data,
                frame->nb_samples);
            if (got > 0) {
                samples.resize(base + (size_t)got * outChannels);
            }
            else {
                samples.resize(base);
            }
            };

        while (av_read_frame(fmt, pkt) >= 0) {
            if (pkt->stream_index == streamIdx) {
                if (avcodec_send_packet(cctx, pkt) == 0) {
                    while (avcodec_receive_frame(cctx, frame) == 0) {
                        drainFrame();
                    }
                }
            }
            av_packet_unref(pkt);
        }
        avcodec_send_packet(cctx, nullptr);
        while (avcodec_receive_frame(cctx, frame) == 0) {
            drainFrame();
        }

        av_packet_free(&pkt);
        av_frame_free(&frame);
        swr_free(&swr);
        avcodec_free_context(&cctx);
        avformat_close_input(&fmt);

        if (samples.empty()) return false;

        float* stable = res.storeFloats(samples);
        if (!stable) return false;

        out.sample_rate = (uint32_t)outSampleRate;
        out.channels = (uint32_t)outChannels;
        out.sample_count = (uint64_t)(samples.size() / outChannels);
        out.data = stable;
        return true;
    }

    // -------------------------------------------------------------------------
    // FillImageParams
    // -------------------------------------------------------------------------
    inline void FillImageParams(ECS::EntityManager& mgr,
        ECS::EntityID e,
        sd_img_gen_params_t& img,
        ResourceManager& res)
    {
        sd_img_gen_params_init(&img);

        if (mgr.HasComponent<ECS::PromptComponent>(e)) {
            auto& p = mgr.GetComponent<ECS::PromptComponent>(e);
            if (!p.posPrompt.empty()) img.prompt = res.storeString(p.posPrompt);
            if (!p.negPrompt.empty()) img.negative_prompt = res.storeString(p.negPrompt);
        }

        if (mgr.HasComponent<ECS::LatentComponent>(e)) {
            auto& l = mgr.GetComponent<ECS::LatentComponent>(e);
            img.width = l.latentWidth;
            img.height = l.latentHeight;
            img.circular_x = l.circular_x;
            img.circular_y = l.circular_y;
        }

        const ECS::SamplerComponent* samp = nullptr;
        const ECS::GuidanceComponent* guid = nullptr;
        const ECS::SLGComponent* slg = nullptr;
        const ECS::CustomSigmasComponent* sig = nullptr;
        if (mgr.HasComponent<ECS::SamplerComponent>(e))      samp = &mgr.GetComponent<ECS::SamplerComponent>(e);
        if (mgr.HasComponent<ECS::GuidanceComponent>(e))     guid = &mgr.GetComponent<ECS::GuidanceComponent>(e);
        if (mgr.HasComponent<ECS::SLGComponent>(e))          slg = &mgr.GetComponent<ECS::SLGComponent>(e);
        if (mgr.HasComponent<ECS::CustomSigmasComponent>(e)) sig = &mgr.GetComponent<ECS::CustomSigmasComponent>(e);

        FillSampleParamsFromComponents(samp, guid, slg, sig, img.sample_params, res);
        if (samp) {
            img.seed = samp->seed;
            img.strength = samp->denoise;
        }

        if (mgr.HasComponent<ECS::EasyCacheComponent>(e)) {
            FillCacheParams(mgr.GetComponent<ECS::EasyCacheComponent>(e), res);
            img.cache = res.cacheStorage;
        }
        if (mgr.HasComponent<ECS::HiresComponent>(e)) {
            FillHiresParams(mgr.GetComponent<ECS::HiresComponent>(e), res);
            img.hires = res.hiresStorage;
        }
        if (mgr.HasComponent<ECS::VaeTilingComponent>(e))
            FillTilingParams(mgr.GetComponent<ECS::VaeTilingComponent>(e),
                img.vae_tiling_params, res);
        if (mgr.HasComponent<ECS::LoraComponent>(e))
            FillLoras(mgr.GetComponent<ECS::LoraComponent>(e),
                img.loras, img.lora_count, res);

        if (mgr.HasComponent<ECS::InputImageComponent>(e)) {
            auto& in = mgr.GetComponent<ECS::InputImageComponent>(e);
            if (!in.filePath.empty())
                loadImageFromPath(in.filePath, img.init_image, res);
        }
        if (mgr.HasComponent<ECS::MaskImageComponent>(e)) {
            auto& m = mgr.GetComponent<ECS::MaskImageComponent>(e);
            std::string path = !m.maskFilePath.empty() ? m.maskFilePath : m.filePath;
            if (!path.empty())
                loadImageFromPath(path, img.mask_image, res);
        }
        if (mgr.HasComponent<ECS::ControlNetImageComponent>(e)) {
            auto& c = mgr.GetComponent<ECS::ControlNetImageComponent>(e);
            if (!c.filePath.empty())
                loadImageFromPath(c.filePath, img.control_image, res);
            img.control_strength = c.strength;
            if (mgr.HasComponent<ECS::ControlNetComponent>(e)) {
                auto& cn = mgr.GetComponent<ECS::ControlNetComponent>(e);
                if (c.strength == 1.0f && cn.cnStrength != 1.0f)
                    img.control_strength = cn.cnStrength;
            }
        }
        if (mgr.HasComponent<ECS::PhotoMakerImageComponent>(e)) {
            auto& p = mgr.GetComponent<ECS::PhotoMakerImageComponent>(e);
            if (!p.filePath.empty()) {
                sd_image_t idImg{};
                if (loadImageFromPath(p.filePath, idImg, res)) {
                    img.pm_params.id_images = &res.images.back();
                    img.pm_params.id_images_count = 1;
                    img.pm_params.style_strength = p.styleStrength;
                }
            }
            if (mgr.HasComponent<ECS::PhotoMakerComponent>(e)) {
                auto& pm = mgr.GetComponent<ECS::PhotoMakerComponent>(e);
                if (!pm.modelPath.empty())
                    img.pm_params.id_embed_path = res.storeString(pm.modelPath);
            }
        }
        if (mgr.HasComponent<ECS::PulidWeightsComponent>(e)) {
            auto& pw = mgr.GetComponent<ECS::PulidWeightsComponent>(e);
            if (!pw.modelPath.empty())
                img.pulid_params.id_embedding_path = res.storeString(pw.modelPath);
        }
        if (mgr.HasComponent<ECS::RefImagesComponent>(e)) {
            auto& r = mgr.GetComponent<ECS::RefImagesComponent>(e);
            size_t start = res.images.size();
            for (const auto& path : r.ref_image_paths) {
                sd_image_t ref{};
                loadImageFromPath(path, ref, res);
            }
            size_t loaded = res.images.size() - start;
            if (loaded > 0) {
                img.ref_images = &res.images[start];
                img.ref_images_count = (int)loaded;
            }
            if (!r.ref_image_args.empty())
                img.ref_image_args = res.storeString(r.ref_image_args);
        }

        if (img.width == 0) img.width = 512;
        if (img.height == 0) img.height = 512;
        if (img.batch_count == 0) img.batch_count = 1;
    }

    // -------------------------------------------------------------------------
    // FillVideoParams
    // -------------------------------------------------------------------------
    inline void FillVideoParams(ECS::EntityManager& mgr,
        ECS::EntityID e,
        sd_vid_gen_params_t& vid,
        ResourceManager& res)
    {
        sd_vid_gen_params_init(&vid);

        if (mgr.HasComponent<ECS::PromptComponent>(e)) {
            auto& p = mgr.GetComponent<ECS::PromptComponent>(e);
            if (!p.posPrompt.empty()) vid.prompt = res.storeString(p.posPrompt);
            if (!p.negPrompt.empty()) vid.negative_prompt = res.storeString(p.negPrompt);
        }

        if (mgr.HasComponent<ECS::LatentComponent>(e)) {
            auto& l = mgr.GetComponent<ECS::LatentComponent>(e);
            vid.width = l.latentWidth;
            vid.height = l.latentHeight;
            vid.circular_x = l.circular_x;
            vid.circular_y = l.circular_y;
        }

        const ECS::SamplerComponent* samp = nullptr;
        const ECS::GuidanceComponent* guid = nullptr;
        const ECS::SLGComponent* slg = nullptr;
        const ECS::CustomSigmasComponent* sig = nullptr;
        if (mgr.HasComponent<ECS::SamplerComponent>(e))      samp = &mgr.GetComponent<ECS::SamplerComponent>(e);
        if (mgr.HasComponent<ECS::GuidanceComponent>(e))     guid = &mgr.GetComponent<ECS::GuidanceComponent>(e);
        if (mgr.HasComponent<ECS::SLGComponent>(e))          slg = &mgr.GetComponent<ECS::SLGComponent>(e);
        if (mgr.HasComponent<ECS::CustomSigmasComponent>(e)) sig = &mgr.GetComponent<ECS::CustomSigmasComponent>(e);
        FillSampleParamsFromComponents(samp, guid, slg, sig, vid.sample_params, res);
        if (samp) {
            vid.seed = samp->seed;
            vid.strength = samp->denoise;
            vid.vace_strength = samp->vace_strength;
            vid.moe_boundary = samp->moe_boundary;
        }

        if (mgr.HasComponent<ECS::HighNoiseSamplerComponent>(e)) {
            auto& hn = mgr.GetComponent<ECS::HighNoiseSamplerComponent>(e);
            vid.high_noise_sample_params.sample_method =
                (sample_method_t)sample_method_from_name(hn.high_noise_sample_method);
            vid.high_noise_sample_params.scheduler =
                (scheduler_t)scheduler_from_name(hn.high_noise_scheduler_method);
            vid.high_noise_sample_params.sample_steps = hn.high_noise_steps;
            vid.high_noise_sample_params.eta = hn.high_noise_eta;
            vid.high_noise_sample_params.guidance.txt_cfg = hn.high_noise_cfg;
        }

        if (mgr.HasComponent<ECS::EasyCacheComponent>(e)) {
            FillCacheParams(mgr.GetComponent<ECS::EasyCacheComponent>(e), res);
            vid.cache = res.cacheStorage;
        }
        if (mgr.HasComponent<ECS::HiresComponent>(e)) {
            FillHiresParams(mgr.GetComponent<ECS::HiresComponent>(e), res);
            vid.hires = res.hiresStorage;
        }
        if (mgr.HasComponent<ECS::VaeTilingComponent>(e))
            FillTilingParams(mgr.GetComponent<ECS::VaeTilingComponent>(e),
                vid.vae_tiling_params, res);
        if (mgr.HasComponent<ECS::LoraComponent>(e))
            FillLoras(mgr.GetComponent<ECS::LoraComponent>(e),
                vid.loras, vid.lora_count, res);

        if (mgr.HasComponent<ECS::InputImageComponent>(e)) {
            auto& in = mgr.GetComponent<ECS::InputImageComponent>(e);
            if (!in.filePath.empty())
                loadImageFromPath(in.filePath, vid.init_image, res);
        }

        if (mgr.HasComponent<ECS::EndImageComponent>(e)) {
            auto& en = mgr.GetComponent<ECS::EndImageComponent>(e);
            if (!en.filePath.empty())
                loadImageFromPath(en.filePath, vid.end_image, res);
        }

        if (mgr.HasComponent<ECS::ControlFramesComponent>(e)) {
            auto& cf = mgr.GetComponent<ECS::ControlFramesComponent>(e);
            size_t start = res.images.size();
            for (const auto& path : cf.filePaths) {
                sd_image_t f{};
                loadImageFromPath(path, f, res);
            }
            size_t loaded = res.images.size() - start;
            if (loaded > 0) {
                vid.control_frames = &res.images[start];
                vid.control_frames_size = (int)loaded;
            }
        }

        if (mgr.HasComponent<ECS::RefImagesComponent>(e)) {
            auto& r = mgr.GetComponent<ECS::RefImagesComponent>(e);
            size_t start = res.images.size();
            for (const auto& path : r.ref_image_paths) {
                sd_image_t ref{};
                loadImageFromPath(path, ref, res);
            }
            size_t loaded = res.images.size() - start;
            if (loaded > 0) {
                vid.ref_images = &res.images[start];
                vid.ref_images_count = (int)loaded;
            }
        }

        if (mgr.HasComponent<ECS::RefVideoComponent>(e)) {
            auto& rv = mgr.GetComponent<ECS::RefVideoComponent>(e);
            for (const auto& dir : rv.videoPaths) {
                if (!std::filesystem::is_directory(dir)) continue;
                std::vector<std::string> files;
                for (const auto& entry : std::filesystem::directory_iterator(dir))
                    if (entry.is_regular_file()) files.push_back(entry.path().string());
                std::sort(files.begin(), files.end());
                if (files.empty()) continue;

                sd_ref_video_t ref{};
                ref.fps = 24;
                ref.frame_count = (int)files.size();
                size_t start = res.images.size();
                for (const auto& f : files) {
                    sd_image_t frame{};
                    loadImageFromPath(f, frame, res);
                }
                ref.frames = &res.images[start];
                res.refVideoStorage.push_back(ref);
            }
        }

        if (mgr.HasComponent<ECS::InputVideoComponent>(e)) {
            auto& iv = mgr.GetComponent<ECS::InputVideoComponent>(e);
            if (!iv.filePath.empty()) {
                int w = 0, h = 0;
                double duration = 0.0, fps = 0.0;
                if (Utils::VideoUtils::GetVideoInfo(iv.filePath, w, h, duration, fps) && fps > 0.0) {
                    int frameCount = (int)(duration * fps);
                    if (frameCount <= 0) frameCount = 1;

                    sd_ref_video_t ref{};
                    ref.fps = (int)(fps + 0.5);
                    ref.frame_count = frameCount;
                    size_t start = res.images.size();

                    for (int i = 0; i < frameCount; ++i) {
                        double t = (double)i / fps;
                        int fw = 0, fh = 0, fc = 0;
                        unsigned char* raw = Utils::VideoUtils::LoadVideoFrame(
                            iv.filePath, t, fw, fh, fc, nullptr);
                        if (!raw) continue;
                        sd_image_t frame{};
                        frame.width = (uint32_t)fw;
                        frame.height = (uint32_t)fh;
                        frame.channel = (uint32_t)fc;
                        frame.data = raw;
                        res.storeImage(frame);
                    }
                    size_t loaded = res.images.size() - start;
                    if (loaded > 0) {
                        ref.frames = &res.images[start];
                        ref.frame_count = (int)loaded;
                        res.refVideoStorage.push_back(ref);
                    }
                }
            }
        }

        if (mgr.HasComponent<ECS::RefAudioComponent>(e)) {
            auto& ra = mgr.GetComponent<ECS::RefAudioComponent>(e);
            for (const auto& path : ra.audioPaths) {
                sd_audio_t audio{};
                if (DecodeAudioToFloat(path, audio, res))
                    res.refAudioStorage.push_back(audio);
            }
        }

        if (mgr.HasComponent<ECS::RefVideoAudioComponent>(e)) {
            auto& rva = mgr.GetComponent<ECS::RefVideoAudioComponent>(e);
            size_t n = std::min(rva.audioPaths.size(), res.refVideoStorage.size());
            for (size_t i = 0; i < n; ++i) {
                sd_audio_t audio{};
                if (DecodeAudioToFloat(rva.audioPaths[i], audio, res))
                    res.refVideoStorage[i].audio = audio;
            }
        }

        if (!res.refVideoStorage.empty()) {
            vid.ref_videos = res.refVideoStorage.data();
            vid.ref_videos_count = (int)res.refVideoStorage.size();
        }
        if (!res.refAudioStorage.empty()) {
            vid.ref_audios = res.refAudioStorage.data();
            vid.ref_audios_count = (int)res.refAudioStorage.size();
        }

        if (mgr.HasComponent<ECS::OutputVideoComponent>(e)) {
            auto& v = mgr.GetComponent<ECS::OutputVideoComponent>(e);
            vid.video_frames = v.video_frames;
            vid.fps = v.output_fps;
        }

        if (vid.width == 0) vid.width = 512;
        if (vid.height == 0) vid.height = 512;
    }

    inline std::string ComputeKeyForEntity(ECS::EntityManager& mgr,
        ECS::EntityID e)
    {
        auto res = std::make_shared<ResourceManager>();
        sd_ctx_params_t ctx{};
        FillContextParams(mgr, e, ctx, *res);

        auto cache = mgr.GetSystem<ECS::ModelCacheSystem>();
        if (!cache) return "default";
        return cache->computeKey(ctx);
    }

    inline bool PreloadEntity(ECS::EntityManager& mgr,
        ECS::EntityID e,
        ECS::ModelCacheSystem& cache)
    {
        auto res = std::make_shared<ResourceManager>();
        sd_ctx_params_t ctx{};
        FillContextParams(mgr, e, ctx, *res);

        auto handle = cache.acquireOrCreateContext(ctx, res);
        if (!handle) return false;
        return true;
    }

} // namespace SDCPP