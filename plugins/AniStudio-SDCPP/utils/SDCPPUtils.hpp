// SDCPPUtils.hpp
#pragma once

#include "stable-diffusion.h"
#include "DiffusionOptions.hpp"
#include <nlohmann/json.hpp>
#include <stb_image.h>
#include <string>
#include <vector>
#include <deque>
#include <memory>
#include <unordered_map>
#include <filesystem>
#include <algorithm>

namespace SDCPP {

    // Owns every string / float array / int array / image buffer that the
    // C API structs (sd_ctx_params_t, sd_img_gen_params_t, sd_vid_gen_params_t)
    // point at. The caller must keep the ResourceManager alive for as long as
    // the params structs are in use.
    //
    // strings / images / controlFrames use deque so that push_back never
    // invalidates previously handed-out c_str() pointers or element addresses.
    class ResourceManager {
    public:
        std::vector<std::unique_ptr<float[]>> floatArrays;
        std::vector<std::unique_ptr<int[]>>   intArrays;
        std::deque<std::string> strings;
        std::deque<sd_image_t>  images;
        std::deque<sd_image_t>  controlFrames;

        std::vector<sd_lora_t>      loraStorage;
        std::vector<sd_embedding_t> embeddingStorage;
        std::vector<sd_ref_video_t> refVideoStorage;
        std::vector<sd_audio_t>     refAudioStorage;

        // Side-storage for POD structs that hold pointers. We copy the struct
        // by value into these, patch the pointer fields to point at strings /
        // arrays owned by this manager, then point the params struct at the
        // stable copy.
        sd_cache_params_t cacheStorage{};
        sd_hires_params_t hiresStorage{};
        std::string cache_scm_mask;
        std::string hires_model_path;
        std::vector<float> hires_custom_sigmas;

        // Merged params_backend string (user value + CPU pin overrides).
        std::string params_backend_override;

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

        void storeImage(const sd_image_t& img) { images.push_back(img); }
        void storeControlFrame(const sd_image_t& img) { controlFrames.push_back(img); }
    };

    // Decode an image file into a stable sd_image_t owned by `res`.
    // Returns false on failure. The stb-allocated pixel buffer is stored
    // inside `res.images`; do not free it yourself.
    inline bool loadImageFromPath(const std::string& filePath,
        sd_image_t& out,
        ResourceManager& res)
    {
        if (filePath.empty()) return false;
        int w = 0, h = 0, c = 0;
        unsigned char* data = stbi_load(filePath.c_str(), &w, &h, &c, 0);
        if (!data) return false;
        out.width = (uint32_t)w;
        out.height = (uint32_t)h;
        out.channel = (uint32_t)c;
        out.data = data;
        res.storeImage(out);
        return true;
    }

} // namespace SDCPP