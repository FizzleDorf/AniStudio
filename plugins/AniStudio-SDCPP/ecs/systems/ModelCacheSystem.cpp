// ModelCacheSystem.cpp
#include "ModelCacheSystem.hpp"
#include "SDContextHandle.hpp"
#include <filesystem>

namespace ECS {

    ModelCacheSystem::~ModelCacheSystem() {
        UnloadAllModels();
    }

    // -------------------------------------------------------------------
    // Memory accounting (used only for the GUI's memoryBytes display;
    // no pre-flight gate)
    // -------------------------------------------------------------------
    size_t ModelCacheSystem::computeMemory(const sd_ctx_params_t& p) const {
        size_t total = 0;
        auto add = [&](const char* path) {
            if (!path || !*path) return;
            std::error_code ec;
            auto sz = std::filesystem::file_size(path, ec);
            if (!ec) total += (size_t)sz;
            };
        add(p.model_path);
        add(p.diffusion_model_path);
        add(p.high_noise_diffusion_model_path);
        add(p.uncond_diffusion_model_path);
        add(p.vae_path);
        add(p.audio_vae_path);
        add(p.taesd_path);
        add(p.control_net_path);
        add(p.motion_module_path);
        add(p.photo_maker_path);
        add(p.pulid_weights_path);
        add(p.clip_l_path);
        add(p.clip_g_path);
        add(p.clip_vision_path);
        add(p.t5xxl_path);
        add(p.llm_path);
        add(p.llm_vision_path);
        return total;
    }

    // -------------------------------------------------------------------
    // Key generation
    // -------------------------------------------------------------------
    std::string ModelCacheSystem::computeKey(const sd_ctx_params_t& p) const {
        std::string key;
        auto add = [&](const char* name, const char* v) {
            if (v && *v) { key += name; key += '='; key += v; key += '|'; }
            };
        add("model", p.model_path);
        add("diffusion", p.diffusion_model_path);
        add("high_noise", p.high_noise_diffusion_model_path);
        add("uncond", p.uncond_diffusion_model_path);
        add("vae", p.vae_path);
        add("audio_vae", p.audio_vae_path);
        add("taesd", p.taesd_path);
        add("controlnet", p.control_net_path);
        add("motion", p.motion_module_path);
        add("photo_maker", p.photo_maker_path);
        add("pulid", p.pulid_weights_path);
        add("clip_l", p.clip_l_path);
        add("clip_g", p.clip_g_path);
        add("clip_vision", p.clip_vision_path);
        add("t5xxl", p.t5xxl_path);
        add("llm", p.llm_path);
        add("llm_vision", p.llm_vision_path);

        if (p.vae_format != SD_VAE_FORMAT_AUTO) {
            key += "vae_fmt=";
            key += std::to_string((int)p.vae_format);
            key += '|';
        }
        if (p.lora_apply_mode != LORA_APPLY_AUTO) {
            key += "lora_mode=";
            key += std::to_string((int)p.lora_apply_mode);
            key += '|';
        }
        return key.empty() ? "default" : key;
    }

    std::string ModelCacheSystem::computeUpscalerKey(const sd_ctx_params_t& p) const {
        std::string key;
        if (p.control_net_path && *p.control_net_path) {
            key += "model=";
            key += p.control_net_path;
            key += '|';
        }
        return key.empty() ? "default" : key;
    }

    // -------------------------------------------------------------------
    // Acquisition
    // -------------------------------------------------------------------
    std::optional<SDCPP::SDContextHandle>
        ModelCacheSystem::acquireOrCreateContext(
            const sd_ctx_params_t& params,
            std::shared_ptr<SDCPP::ResourceManager>& ctxRes)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::string key = computeKey(params);

        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            it->second.activeCount++;
            it->second.lastUsed = std::chrono::steady_clock::now();
            promote(key);

            // Hand the caller the entry's own ResourceManager. This is what
            // keeps the strings params.* point at alive for the caller too.
            ctxRes = it->second.ctxRes;

            return SDCPP::SDContextHandle(this, key, it->second.ctx);
        }

        // No pre-flight memory gate. sdcpp handles placement/streaming itself.
        // If the model genuinely cannot be created, new_sd_ctx returns null.
        sd_ctx_t* ctx = new_sd_ctx(&params);
        if (!ctx) {
            m_lastError = "new_sd_ctx returned null for: " + key;
            std::cerr << "[ModelCacheSystem] " << m_lastError << std::endl;
            return std::nullopt;
        }

        ContextInfo info;
        info.ctx = ctx;
        info.params = params;
        info.ctxRes = std::move(ctxRes);   // cache takes ownership
        info.key = key;
        info.memoryBytes = computeMemory(params);
        info.activeCount = 1;
        info.modelType = detectModelType(params);
        info.lastUsed = std::chrono::steady_clock::now();

        auto [inserted, ok] = m_cache.emplace(key, std::move(info));
        m_order.push_back(key);

        // Give the caller a shared reference back. On the miss path we moved
        // ctxRes into the entry, so read it out of the inserted entry.
        ctxRes = inserted->second.ctxRes;

        evictIfNeeded();
        m_lastError.clear();
        return SDCPP::SDContextHandle(this, key, ctx);
    }

    std::optional<SDCPP::UpscalerHandle>
        ModelCacheSystem::acquireOrCreateUpscaler(
            const sd_ctx_params_t& params,
            std::shared_ptr<SDCPP::ResourceManager>& ctxRes)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::string key = computeUpscalerKey(params);

        if (key == "default" || !params.control_net_path || !*params.control_net_path) {
            m_lastError = "Upscaler requires control_net_path (ESRGAN model path)";
            return std::nullopt;
        }

        auto it = m_upscalers.find(key);
        if (it != m_upscalers.end()) {
            it->second.activeCount++;
            it->second.lastUsed = std::chrono::steady_clock::now();
            ctxRes = it->second.ctxRes;
            return SDCPP::UpscalerHandle(this, key, it->second.ctx);
        }

        int n_threads = params.n_threads > 0 ? params.n_threads : 4;
        int tile_size = 64;
        bool direct = false;
        upscaler_ctx_t* ctx = new_upscaler_ctx(
            params.control_net_path,
            direct,
            n_threads,
            tile_size,
            params.backend,
            params.params_backend);
        if (!ctx) {
            m_lastError = "new_upscaler_ctx returned null";
            return std::nullopt;
        }

        size_t required = 0;
        {
            std::error_code ec;
            auto sz = std::filesystem::file_size(params.control_net_path, ec);
            if (!ec) required = (size_t)sz;
        }

        UpscalerInfo info;
        info.ctx = ctx;
        info.params = params;
        info.ctxRes = std::move(ctxRes);
        info.key = key;
        info.memoryBytes = required;
        info.activeCount = 1;
        info.lastUsed = std::chrono::steady_clock::now();

        auto [inserted, ok] = m_upscalers.emplace(key, std::move(info));
        m_upscalerOrder.push_back(key);

        ctxRes = inserted->second.ctxRes;

        return SDCPP::UpscalerHandle(this, key, ctx);
    }

    // -------------------------------------------------------------------
    // Release
    // -------------------------------------------------------------------
    void ModelCacheSystem::releaseContext(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cache.find(key);
        if (it == m_cache.end()) return;
        if (it->second.activeCount > 0)
            it->second.activeCount--;
    }

    void ModelCacheSystem::releaseUpscaler(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_upscalers.find(key);
        if (it == m_upscalers.end()) return;
        if (it->second.activeCount > 0)
            it->second.activeCount--;
    }

    // -------------------------------------------------------------------
    // Unload
    // -------------------------------------------------------------------
    void ModelCacheSystem::UnloadModel(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cache.find(key);
        if (it == m_cache.end()) return;
        if (it->second.activeCount > 0) {
            m_lastError = "Cannot unload model in use (active=" +
                std::to_string(it->second.activeCount) + "): " + key;
            std::cerr << "[ModelCacheSystem] " << m_lastError << std::endl;
            return;
        }
        free_sd_ctx(it->second.ctx);
        m_cache.erase(it);
        removeFromOrder(key);
        m_lastError.clear();
    }

    void ModelCacheSystem::UnloadAllModels() {
        std::lock_guard<std::mutex> lock(m_mutex);

        for (auto it = m_cache.begin(); it != m_cache.end(); ) {
            if (it->second.activeCount == 0) {
                free_sd_ctx(it->second.ctx);
                removeFromOrder(it->first);
                it = m_cache.erase(it);
            }
            else {
                ++it;
            }
        }

        for (auto it = m_upscalers.begin(); it != m_upscalers.end(); ) {
            if (it->second.activeCount == 0) {
                free_upscaler_ctx(it->second.ctx);
                auto oit = std::find(m_upscalerOrder.begin(), m_upscalerOrder.end(), it->first);
                if (oit != m_upscalerOrder.end()) m_upscalerOrder.erase(oit);
                it = m_upscalers.erase(it);
            }
            else {
                ++it;
            }
        }
        m_order.clear();
        m_upscalerOrder.clear();
    }

    void ModelCacheSystem::UnloadInactiveModels() {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto it = m_cache.begin(); it != m_cache.end(); ) {
            if (it->second.activeCount == 0) {
                free_sd_ctx(it->second.ctx);
                removeFromOrder(it->first);
                it = m_cache.erase(it);
            }
            else ++it;
        }
        for (auto it = m_upscalers.begin(); it != m_upscalers.end(); ) {
            if (it->second.activeCount == 0) {
                free_upscaler_ctx(it->second.ctx);
                auto oit = std::find(m_upscalerOrder.begin(), m_upscalerOrder.end(), it->first);
                if (oit != m_upscalerOrder.end()) m_upscalerOrder.erase(oit);
                it = m_upscalers.erase(it);
            }
            else ++it;
        }
    }

    void ModelCacheSystem::UnloadUpscaler(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_upscalers.find(key);
        if (it == m_upscalers.end()) return;
        if (it->second.activeCount > 0) {
            m_lastError = "Cannot unload upscaler in use: " + key;
            std::cerr << "[ModelCacheSystem] " << m_lastError << std::endl;
            return;
        }
        free_upscaler_ctx(it->second.ctx);
        m_upscalers.erase(it);
        auto oit = std::find(m_upscalerOrder.begin(), m_upscalerOrder.end(), key);
        if (oit != m_upscalerOrder.end()) m_upscalerOrder.erase(oit);
        m_lastError.clear();
    }

    void ModelCacheSystem::UnloadAllUpscalers() {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto it = m_upscalers.begin(); it != m_upscalers.end(); ) {
            if (it->second.activeCount == 0) {
                free_upscaler_ctx(it->second.ctx);
                it = m_upscalers.erase(it);
            }
            else {
                ++it;
            }
        }
        m_upscalerOrder.clear();
    }

    // -------------------------------------------------------------------
    // Reload
    // -------------------------------------------------------------------
    bool ModelCacheSystem::reloadModel(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cache.find(key);
        if (it == m_cache.end()) return false;
        if (it->second.activeCount > 0) {
            m_lastError = "Cannot reload model in use: " + key;
            return false;
        }

        free_sd_ctx(it->second.ctx);
        it->second.ctx = new_sd_ctx(&it->second.params);
        if (!it->second.ctx) {
            m_lastError = "reload failed for: " + key;
            m_cache.erase(it);
            removeFromOrder(key);
            return false;
        }
        it->second.lastUsed = std::chrono::steady_clock::now();
        m_lastError.clear();
        return true;
    }

    bool ModelCacheSystem::reloadUpscaler(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_upscalers.find(key);
        if (it == m_upscalers.end()) return false;
        if (it->second.activeCount > 0) {
            m_lastError = "Cannot reload upscaler in use: " + key;
            return false;
        }

        free_upscaler_ctx(it->second.ctx);

        int n_threads = it->second.params.n_threads > 0 ? it->second.params.n_threads : 4;
        int tile_size = 64;
        bool direct = false;
        it->second.ctx = new_upscaler_ctx(
            it->second.params.control_net_path,
            direct,
            n_threads,
            tile_size,
            it->second.params.backend,
            it->second.params.params_backend);
        if (!it->second.ctx) {
            m_lastError = "reload failed for upscaler: " + key;
            m_upscalers.erase(it);
            auto oit = std::find(m_upscalerOrder.begin(), m_upscalerOrder.end(), key);
            if (oit != m_upscalerOrder.end()) m_upscalerOrder.erase(oit);
            return false;
        }
        it->second.lastUsed = std::chrono::steady_clock::now();
        m_lastError.clear();
        return true;
    }

    // -------------------------------------------------------------------
    // Details / introspection
    // -------------------------------------------------------------------
    std::vector<ContextDetail> ModelCacheSystem::GetContextDetails() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<ContextDetail> out;
        out.reserve(m_cache.size());
        for (const auto& [k, info] : m_cache) {
            ContextDetail d;
            d.key = k;
            d.displayName = std::filesystem::path(k).filename().string();
            d.modelType = info.modelType;
            d.memoryBytes = info.memoryBytes;
            d.activeCount = info.activeCount;
            d.isInUse = info.activeCount > 0;
            out.push_back(std::move(d));
        }
        return out;
    }

    std::vector<UpscalerDetail> ModelCacheSystem::GetUpscalerDetails() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<UpscalerDetail> out;
        out.reserve(m_upscalers.size());
        for (const auto& [k, info] : m_upscalers) {
            UpscalerDetail d;
            d.key = k;
            d.displayName = std::filesystem::path(k).filename().string();
            d.memoryBytes = info.memoryBytes;
            d.activeCount = info.activeCount;
            d.isInUse = info.activeCount > 0;
            out.push_back(std::move(d));
        }
        return out;
    }

    size_t ModelCacheSystem::GetCacheSize() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_cache.size();
    }

    size_t ModelCacheSystem::GetUpscalerCacheSize() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_upscalers.size();
    }

    std::string ModelCacheSystem::getLastError() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_lastError;
    }

    void ModelCacheSystem::Destroy() {
        UnloadAllModels();
        BaseSystem::Destroy();
    }

    // -------------------------------------------------------------------
    // LRU / eviction
    // -------------------------------------------------------------------
    void ModelCacheSystem::promote(const std::string& key) {
        auto it = std::find(m_order.begin(), m_order.end(), key);
        if (it != m_order.end()) {
            m_order.erase(it);
            m_order.push_back(key);
        }
    }

    void ModelCacheSystem::removeFromOrder(const std::string& key) {
        auto it = std::find(m_order.begin(), m_order.end(), key);
        if (it != m_order.end()) m_order.erase(it);
    }

    void ModelCacheSystem::evictIfNeeded() {
        if (m_cache.size() <= m_maxCacheSize) return;

        size_t scanned = 0;
        while (m_cache.size() > m_maxCacheSize && scanned < m_order.size()) {
            std::string key = m_order.front();
            m_order.pop_front();
            auto it = m_cache.find(key);
            if (it == m_cache.end()) {
                scanned++;
                continue;
            }
            if (it->second.activeCount == 0) {
                free_sd_ctx(it->second.ctx);
                m_cache.erase(it);
            }
            else {
                m_order.push_back(key);
            }
            scanned++;
        }

        if (m_cache.size() > m_maxCacheSize) {
            std::cerr << "[ModelCacheSystem] Cannot evict: all "
                << m_cache.size() << " entries in use (max "
                << m_maxCacheSize << ")\n";
        }
    }

    // -------------------------------------------------------------------
    // Detection
    // -------------------------------------------------------------------
    std::string ModelCacheSystem::detectModelType(const sd_ctx_params_t& p) const {
        std::string combined;
        if (p.model_path)           combined += std::string(p.model_path) + " ";
        if (p.diffusion_model_path) combined += std::string(p.diffusion_model_path) + " ";
        if (p.llm_path)             combined += std::string(p.llm_path) + " ";
        if (combined.empty()) return "Unknown";

        std::string lower = combined;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        if (lower.find("minimax") != std::string::npos) return "MiniMax";
        if (lower.find("flux") != std::string::npos) return "Flux";
        if (lower.find("sdxl") != std::string::npos) return "SDXL";
        if (lower.find("sd3") != std::string::npos) return "SD3";
        if (lower.find("sd1.5") != std::string::npos ||
            lower.find("sd1_5") != std::string::npos) return "SD1.5";
        if (lower.find("sd2") != std::string::npos) return "SD2";
        if (lower.find("wan") != std::string::npos) return "Wan";
        if (lower.find("ltx") != std::string::npos) return "LTX";
        if (lower.find("qwen") != std::string::npos) return "Qwen";
        return "Diffusion";
    }

} // namespace ECS