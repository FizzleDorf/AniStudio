#include "ModelCacheSystem.hpp"
#include "SDContextHandle.hpp"
#include "Log.hpp"

#include <filesystem>

namespace ECS {

    ModelCacheSystem::~ModelCacheSystem() {
        ANI_LOG_INFO("ModelCacheSystem destructor - unloading all models");
        UnloadAllModels();
    }

    // Memory accounting (used only for the GUI's memoryBytes display;
    // no pre-flight gate)
    size_t ModelCacheSystem::computeMemory(const sd_ctx_params_t& p) const {
        size_t total = 0;
        auto add = [&](const char* path) {
            if (!path || !*path) return;
            std::error_code ec;
            auto sz = std::filesystem::file_size(path, ec);
            if (!ec) {
                total += (size_t)sz;
            }
            else {
                ANI_LOG_TRACE("computeMemory: cannot stat '%s': %s",
                    path, ec.message().c_str());
            }
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

            ANI_LOG_TRACE("Cache hit for context key, active count now %d",
                it->second.activeCount);

            return SDCPP::SDContextHandle(this, key, it->second.ctx);
        }

        // No pre-flight memory gate. sdcpp handles placement/streaming itself.
        // If the model genuinely cannot be created, new_sd_ctx returns null.
        ANI_LOG_DEBUG("Cache miss, creating new sd_ctx (current cache size: %zu)",
            m_cache.size());

        sd_ctx_t* ctx = new_sd_ctx(&params);
        if (!ctx) {
            m_lastError = "new_sd_ctx returned null for: " + key;
            ANI_LOG_ERROR("%s", m_lastError.c_str());
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

        ANI_LOG_INFO("Created context (type: %s, est. memory: %zu bytes)",
            inserted->second.modelType.c_str(),
            inserted->second.memoryBytes);

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
            ANI_LOG_WARN("%s", m_lastError.c_str());
            return std::nullopt;
        }

        auto it = m_upscalers.find(key);
        if (it != m_upscalers.end()) {
            it->second.activeCount++;
            it->second.lastUsed = std::chrono::steady_clock::now();
            ctxRes = it->second.ctxRes;

            ANI_LOG_TRACE("Cache hit for upscaler key, active count now %d",
                it->second.activeCount);

            return SDCPP::UpscalerHandle(this, key, it->second.ctx);
        }

        ANI_LOG_DEBUG("Upscaler cache miss, creating new upscaler_ctx");

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
            ANI_LOG_ERROR("%s", m_lastError.c_str());
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

        ANI_LOG_INFO("Created upscaler '%s' (est. memory: %zu bytes)",
            key.c_str(), required);

        return SDCPP::UpscalerHandle(this, key, ctx);
    }

    void ModelCacheSystem::releaseContext(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cache.find(key);
        if (it == m_cache.end()) {
            ANI_LOG_TRACE("releaseContext: key not in cache: %s", key.c_str());
            return;
        }
        if (it->second.activeCount > 0) {
            it->second.activeCount--;
            ANI_LOG_TRACE("Released context, active count now %d",
                it->second.activeCount);
        }
    }

    void ModelCacheSystem::releaseUpscaler(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_upscalers.find(key);
        if (it == m_upscalers.end()) {
            ANI_LOG_TRACE("releaseUpscaler: key not in cache: %s", key.c_str());
            return;
        }
        if (it->second.activeCount > 0) {
            it->second.activeCount--;
            ANI_LOG_TRACE("Released upscaler, active count now %d",
                it->second.activeCount);
        }
    }

    void ModelCacheSystem::UnloadModel(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cache.find(key);
        if (it == m_cache.end()) {
            ANI_LOG_TRACE("UnloadModel: key not in cache: %s", key.c_str());
            return;
        }
        if (it->second.activeCount > 0) {
            m_lastError = "Cannot unload model in use (active=" +
                std::to_string(it->second.activeCount) + "): " + key;
            ANI_LOG_WARN("%s", m_lastError.c_str());
            return;
        }
        free_sd_ctx(it->second.ctx);
        m_cache.erase(it);
        removeFromOrder(key);
        m_lastError.clear();

        ANI_LOG_DEBUG("Unloaded model: %s", key.c_str());
    }

    void ModelCacheSystem::UnloadAllModels() {
        std::lock_guard<std::mutex> lock(m_mutex);

        size_t freed = 0;
        for (auto it = m_cache.begin(); it != m_cache.end(); ) {
            if (it->second.activeCount == 0) {
                free_sd_ctx(it->second.ctx);
                removeFromOrder(it->first);
                it = m_cache.erase(it);
                freed++;
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

        ANI_LOG_INFO("UnloadAllModels: freed %zu model(s)", freed);
    }

    void ModelCacheSystem::UnloadInactiveModels() {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t freed = 0;
        for (auto it = m_cache.begin(); it != m_cache.end(); ) {
            if (it->second.activeCount == 0) {
                free_sd_ctx(it->second.ctx);
                removeFromOrder(it->first);
                it = m_cache.erase(it);
                freed++;
            }
            else ++it;
        }
        for (auto it = m_upscalers.begin(); it != m_upscalers.end(); ) {
            if (it->second.activeCount == 0) {
                free_upscaler_ctx(it->second.ctx);
                auto oit = std::find(m_upscalerOrder.begin(), m_upscalerOrder.end(), it->first);
                if (oit != m_upscalerOrder.end()) m_upscalerOrder.erase(oit);
                it = m_upscalers.erase(it);
                freed++;
            }
            else ++it;
        }

        if (freed > 0) {
            ANI_LOG_DEBUG("UnloadInactiveModels: freed %zu inactive entry(s)", freed);
        }
    }

    void ModelCacheSystem::UnloadUpscaler(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_upscalers.find(key);
        if (it == m_upscalers.end()) {
            ANI_LOG_TRACE("UnloadUpscaler: key not in cache: %s", key.c_str());
            return;
        }
        if (it->second.activeCount > 0) {
            m_lastError = "Cannot unload upscaler in use: " + key;
            ANI_LOG_WARN("%s", m_lastError.c_str());
            return;
        }
        free_upscaler_ctx(it->second.ctx);
        m_upscalers.erase(it);
        auto oit = std::find(m_upscalerOrder.begin(), m_upscalerOrder.end(), key);
        if (oit != m_upscalerOrder.end()) m_upscalerOrder.erase(oit);
        m_lastError.clear();

        ANI_LOG_DEBUG("Unloaded upscaler: %s", key.c_str());
    }

    void ModelCacheSystem::UnloadAllUpscalers() {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t freed = 0;
        for (auto it = m_upscalers.begin(); it != m_upscalers.end(); ) {
            if (it->second.activeCount == 0) {
                free_upscaler_ctx(it->second.ctx);
                it = m_upscalers.erase(it);
                freed++;
            }
            else {
                ++it;
            }
        }
        m_upscalerOrder.clear();

        ANI_LOG_INFO("UnloadAllUpscalers: freed %zu upscaler(s)", freed);
    }

    bool ModelCacheSystem::reloadModel(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cache.find(key);
        if (it == m_cache.end()) {
            ANI_LOG_TRACE("reloadModel: key not in cache: %s", key.c_str());
            return false;
        }
        if (it->second.activeCount > 0) {
            m_lastError = "Cannot reload model in use: " + key;
            ANI_LOG_WARN("%s", m_lastError.c_str());
            return false;
        }

        free_sd_ctx(it->second.ctx);
        it->second.ctx = new_sd_ctx(&it->second.params);
        if (!it->second.ctx) {
            m_lastError = "reload failed for: " + key;
            ANI_LOG_ERROR("%s", m_lastError.c_str());
            m_cache.erase(it);
            removeFromOrder(key);
            return false;
        }
        it->second.lastUsed = std::chrono::steady_clock::now();
        m_lastError.clear();

        ANI_LOG_DEBUG("Reloaded model: %s", key.c_str());
        return true;
    }

    bool ModelCacheSystem::reloadUpscaler(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_upscalers.find(key);
        if (it == m_upscalers.end()) {
            ANI_LOG_TRACE("reloadUpscaler: key not in cache: %s", key.c_str());
            return false;
        }
        if (it->second.activeCount > 0) {
            m_lastError = "Cannot reload upscaler in use: " + key;
            ANI_LOG_WARN("%s", m_lastError.c_str());
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
            ANI_LOG_ERROR("%s", m_lastError.c_str());
            m_upscalers.erase(it);
            auto oit = std::find(m_upscalerOrder.begin(), m_upscalerOrder.end(), key);
            if (oit != m_upscalerOrder.end()) m_upscalerOrder.erase(oit);
            return false;
        }
        it->second.lastUsed = std::chrono::steady_clock::now();
        m_lastError.clear();

        ANI_LOG_DEBUG("Reloaded upscaler: %s", key.c_str());
        return true;
    }

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
        ANI_LOG_INFO("ModelCacheSystem destroying");
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

        ANI_LOG_DEBUG("Cache size %zu exceeds max %zu, evicting LRU entries",
            m_cache.size(), m_maxCacheSize);

        size_t scanned = 0;
        size_t evicted = 0;
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
                evicted++;
            }
            else {
                m_order.push_back(key);
            }
            scanned++;
        }

        if (evicted > 0) {
            ANI_LOG_DEBUG("Evicted %zu LRU entry(s)", evicted);
        }

        if (m_cache.size() > m_maxCacheSize) {
            ANI_LOG_WARN("Cannot evict: all %zu entries in use (max %zu)",
                m_cache.size(), m_maxCacheSize);
        }
    }

    std::string ModelCacheSystem::detectModelType(const sd_ctx_params_t& p) const {
        std::string combined;
        if (p.model_path)           combined += std::string(p.model_path) + " ";
        if (p.diffusion_model_path) combined += std::string(p.diffusion_model_path) + " ";
        if (p.llm_path)             combined += std::string(p.llm_path) + " ";
        if (combined.empty()) {
            ANI_LOG_TRACE("detectModelType: no model paths set");
            return "Unknown";
        }

        std::string lower = combined;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        std::string type;
        if (lower.find("minimax") != std::string::npos) type = "MiniMax";
        else if (lower.find("flux") != std::string::npos) type = "Flux";
        else if (lower.find("sdxl") != std::string::npos) type = "SDXL";
        else if (lower.find("sd3") != std::string::npos) type = "SD3";
        else if (lower.find("sd1.5") != std::string::npos ||
            lower.find("sd1_5") != std::string::npos) type = "SD1.5";
        else if (lower.find("sd2") != std::string::npos) type = "SD2";
        else if (lower.find("wan") != std::string::npos) type = "Wan";
        else if (lower.find("ltx") != std::string::npos) type = "LTX";
        else if (lower.find("qwen") != std::string::npos) type = "Qwen";
        else type = "Diffusion";

        ANI_LOG_TRACE("detectModelType: %s", type.c_str());
        return type;
    }

} // namespace ECS