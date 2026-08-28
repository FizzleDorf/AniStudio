#include "ModelCacheSystem.hpp"
#include <filesystem>

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/sysinfo.h>
#endif

namespace ECS {

    size_t ModelCacheSystem::getAvailableMemory() const {
#if defined(_WIN32)
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(status);
        if (GlobalMemoryStatusEx(&status)) {
            return static_cast<size_t>(status.ullAvailPhys);
        }
        return 0;
#else
        struct sysinfo info;
        if (sysinfo(&info) == 0) {
            return static_cast<size_t>(info.freeram) * info.mem_unit;
        }
        return 0;
#endif
    }

    bool ModelCacheSystem::hasEnoughMemory(size_t requiredBytes) const {
        size_t available = getAvailableMemory();
        // Use a safety margin of 10% to avoid filling memory completely
        return requiredBytes < static_cast<size_t>(available * 0.9);
    }

    sd_ctx_t* ModelCacheSystem::getOrCreateContext(const nlohmann::json& metadata) {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::string key = computeKey(metadata);

        std::cout << "[ModelCacheSystem] getOrCreateContext - key: " << key << std::endl;
        std::cout << "[ModelCacheSystem] Cache size before: " << m_cache.size() << std::endl;

        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            std::cout << "[ModelCacheSystem] CACHE HIT!" << std::endl;
            it->second.lastUsed = std::chrono::steady_clock::now();
            promote(key);
            return it->second.ctx;
        }

        std::cout << "[ModelCacheSystem] CACHE MISS - loading model..." << std::endl;

        size_t requiredMemory = computeMemory(metadata);
        if (!hasEnoughMemory(requiredMemory)) {
            m_lastError = "Insufficient available memory to load model. Required: " + std::to_string(requiredMemory) +
                " bytes, Available: " + std::to_string(getAvailableMemory()) + " bytes.";
            std::cerr << "[ModelCacheSystem] " << m_lastError << std::endl;
            return nullptr;
        }

        sd_ctx_params_t ctxParams;
        sd_ctx_params_init(&ctxParams);
        SDCPP::ResourceManager res;
        if (!SDCPP::parseContextParams(metadata, ctxParams, res)) {
            m_lastError = "Failed to parse context parameters.";
            return nullptr;
        }
        sd_ctx_t* ctx = new_sd_ctx(&ctxParams);
        if (!ctx) {
            m_lastError = "new_sd_ctx returned null.";
            return nullptr;
        }

        ContextInfo info;
        info.ctx = ctx;
        info.metadata = metadata;
        info.memoryBytes = requiredMemory;
        info.activeCount = 0;
        info.modelType = detectModelType(metadata);
        info.isInUse = false;
        info.lastUsed = std::chrono::steady_clock::now();
        m_cache[key] = info;
        m_order.push_back(key);
        evictIfNeeded();

        std::cout << "[ModelCacheSystem] Cached. Cache size now: " << m_cache.size() << std::endl;
        m_lastError.clear();
        return ctx;
    }

    bool ModelCacheSystem::loadModelFromMetadata(const nlohmann::json& metadata) {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::string key = computeKey(metadata);
        if (m_cache.find(key) != m_cache.end()) {
            m_cache[key].lastUsed = std::chrono::steady_clock::now();
            m_lastError.clear();
            return true;
        }

        size_t requiredMemory = computeMemory(metadata);
        if (!hasEnoughMemory(requiredMemory)) {
            m_lastError = "Insufficient available memory to load model. Required: " + std::to_string(requiredMemory) +
                " bytes, Available: " + std::to_string(getAvailableMemory()) + " bytes.";
            std::cerr << "[ModelCacheSystem] " << m_lastError << std::endl;
            return false;
        }

        sd_ctx_params_t ctxParams;
        sd_ctx_params_init(&ctxParams);
        SDCPP::ResourceManager res;
        if (!SDCPP::parseContextParams(metadata, ctxParams, res)) {
            m_lastError = "Failed to parse context parameters.";
            return false;
        }
        sd_ctx_t* ctx = new_sd_ctx(&ctxParams);
        if (!ctx) {
            m_lastError = "new_sd_ctx returned null.";
            return false;
        }

        ContextInfo info;
        info.ctx = ctx;
        info.metadata = metadata;
        info.memoryBytes = requiredMemory;
        info.activeCount = 0;
        info.modelType = detectModelType(metadata);
        info.isInUse = false;
        info.lastUsed = std::chrono::steady_clock::now();
        m_cache[key] = info;
        m_order.push_back(key);
        evictIfNeeded();
        m_lastError.clear();
        return true;
    }

    void ModelCacheSystem::reloadModel(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cache.find(key);
        if (it == m_cache.end()) return;

        if (it->second.activeCount > 0) {
            std::cerr << "Cannot reload active model: " << key << std::endl;
            return;
        }

        nlohmann::json metaCopy = it->second.metadata;
        free_sd_ctx(it->second.ctx);
        m_cache.erase(it);
        auto orderIt = std::find(m_order.begin(), m_order.end(), key);
        if (orderIt != m_order.end()) m_order.erase(orderIt);

        sd_ctx_params_t ctxParams;
        sd_ctx_params_init(&ctxParams);
        SDCPP::ResourceManager res;
        if (!SDCPP::parseContextParams(metaCopy, ctxParams, res)) return;
        sd_ctx_t* ctx = new_sd_ctx(&ctxParams);
        if (!ctx) return;

        ContextInfo info;
        info.ctx = ctx;
        info.metadata = metaCopy;
        info.memoryBytes = computeMemory(metaCopy);
        info.activeCount = 0;
        info.modelType = detectModelType(metaCopy);
        info.isInUse = false;
        info.lastUsed = std::chrono::steady_clock::now();
        m_cache[key] = info;
        m_order.push_back(key);
    }

    void ModelCacheSystem::UnloadModel(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cout << "[ModelCacheSystem] UnloadModel: " << key << std::endl;
        auto it = m_cache.find(key);
        if (it == m_cache.end()) return;
        if (it->second.activeCount > 0) {
            std::cerr << "Cannot unload active model: " << key << std::endl;
            return;
        }
        free_sd_ctx(it->second.ctx);
        m_cache.erase(it);
        auto orderIt = std::find(m_order.begin(), m_order.end(), key);
        if (orderIt != m_order.end()) m_order.erase(orderIt);
    }

    void ModelCacheSystem::UnloadAllModels() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cout << "[ModelCacheSystem] UnloadAllModels called! Cache size before: " << m_cache.size() << std::endl;
        for (auto& pair : m_cache) {
            free_sd_ctx(pair.second.ctx);
        }
        m_cache.clear();
        m_order.clear();
        std::cout << "[ModelCacheSystem] UnloadAllModels done. Cache size now: " << m_cache.size() << std::endl;
    }

    void ModelCacheSystem::UnloadInactiveModels() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cout << "[ModelCacheSystem] UnloadInactiveModels called! Cache size before: " << m_cache.size() << std::endl;
        std::vector<std::string> toRemove;
        for (const auto& pair : m_cache) {
            if (pair.second.activeCount == 0) {
                toRemove.push_back(pair.first);
            }
        }
        for (const auto& key : toRemove) {
            auto it = m_cache.find(key);
            if (it != m_cache.end()) {
                free_sd_ctx(it->second.ctx);
                m_cache.erase(it);
                auto orderIt = std::find(m_order.begin(), m_order.end(), key);
                if (orderIt != m_order.end()) m_order.erase(orderIt);
            }
        }
        std::cout << "[ModelCacheSystem] UnloadInactiveModels done. Cache size now: " << m_cache.size() << std::endl;
    }

    std::vector<std::string> ModelCacheSystem::GetLoadedModels() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<std::string> result;
        result.reserve(m_cache.size());
        for (const auto& pair : m_cache) {
            result.push_back(pair.first);
        }
        return result;
    }

    std::vector<ContextDetail> ModelCacheSystem::GetContextDetails() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<ContextDetail> details;
        details.reserve(m_cache.size());
        for (const auto& pair : m_cache) {
            const auto& info = pair.second;
            ContextDetail d;
            d.key = pair.first;
            d.displayName = std::filesystem::path(pair.first).filename().string();
            d.modelType = info.modelType;
            d.memoryBytes = info.memoryBytes;
            d.activeCount = info.activeCount;
            d.isInUse = (info.activeCount > 0);
            details.push_back(d);
        }
        return details;
    }

    void ModelCacheSystem::ListSDContexts() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cout << "Cached contexts (" << m_cache.size() << "):\n";
        for (const auto& pair : m_cache) {
            std::cout << "  " << pair.first << " (active: " << pair.second.activeCount << ")\n";
        }
    }

    void ModelCacheSystem::Destroy() {
        UnloadAllModels();
        BaseSystem::Destroy();
    }

    sd_ctx_t* ModelCacheSystem::acquireContext(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cout << "[ModelCacheSystem] acquireContext: " << key << " cache size: " << m_cache.size() << std::endl;
        auto it = m_cache.find(key);
        if (it == m_cache.end()) {
            std::cerr << "[ModelCacheSystem] Context not found: " << key << std::endl;
            return nullptr;
        }
        it->second.activeCount++;
        it->second.isInUse = true;
        it->second.lastUsed = std::chrono::steady_clock::now();
        std::cout << "[ModelCacheSystem] acquireContext: activeCount now " << it->second.activeCount << std::endl;
        return it->second.ctx;
    }

    void ModelCacheSystem::releaseContext(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cout << "[ModelCacheSystem] releaseContext: " << key << " cache size: " << m_cache.size() << std::endl;
        auto it = m_cache.find(key);
        if (it == m_cache.end()) {
            std::cerr << "[ModelCacheSystem] Context not found for release: " << key << std::endl;
            return;
        }
        if (it->second.activeCount > 0) {
            it->second.activeCount--;
            std::cout << "[ModelCacheSystem] releaseContext: activeCount now " << it->second.activeCount << std::endl;
            if (it->second.activeCount == 0) {
                it->second.isInUse = false;
            }
        }
    }

    std::string ModelCacheSystem::computeKey(const nlohmann::json& metadata) const {
        std::string key;
        if (!metadata.contains("components") || !metadata["components"].is_array()) {
            return "default";
        }

        std::string diffusionPath;
        std::string highNoiseDiffusionPath;
        std::string uncondDiffusionPath;
        std::string vaePath;
        std::string vaeFormat;
        std::string vaeDecodeOnly;
        std::string llmPath;
        std::string checkpointPath;
        std::string clipLPath;
        std::string clipGPath;
        std::string t5xxlPath;
        std::string clipVisionPath;
        std::string llmVisionPath;
        std::string controlNetPath;
        std::string audioVaePath;
        std::string motionPath;
        std::string photoMakerPath;
        std::string pulidPath;
        std::string taesdPath;
        std::string embeddingsPath;

        for (const auto& comp : metadata["components"]) {
            if (comp.contains("DiffusionModel") && comp["DiffusionModel"].contains("modelPath")) {
                diffusionPath = comp["DiffusionModel"]["modelPath"].get<std::string>();
            }
            if (comp.contains("HighNoiseDiffusionModel") && comp["HighNoiseDiffusionModel"].contains("modelPath")) {
                highNoiseDiffusionPath = comp["HighNoiseDiffusionModel"]["modelPath"].get<std::string>();
            }
            if (comp.contains("UncondDiffusionModel") && comp["UncondDiffusionModel"].contains("modelPath")) {
                uncondDiffusionPath = comp["UncondDiffusionModel"]["modelPath"].get<std::string>();
            }
            if (comp.contains("Vae") && comp["Vae"].contains("modelPath")) {
                vaePath = comp["Vae"]["modelPath"].get<std::string>();
                if (comp["Vae"].contains("vae_format")) {
                    const auto& val = comp["Vae"]["vae_format"];
                    if (val.is_string()) {
                        vaeFormat = val.get<std::string>();
                    }
                    else if (val.is_number()) {
                        int vaeFormatInt = val.get<int>();
                        for (const auto& [name, id] : get_vae_format_map()) {
                            if (id == vaeFormatInt) {
                                vaeFormat = name;
                                break;
                            }
                        }
                    }
                }
                if (comp["Vae"].contains("vae_decode_only")) {
                    vaeDecodeOnly = std::to_string(comp["Vae"]["vae_decode_only"].get<bool>());
                }
            }
            if (comp.contains("LlmEncoder") && comp["LlmEncoder"].contains("modelPath")) {
                llmPath = comp["LlmEncoder"]["modelPath"].get<std::string>();
            }
            if (comp.contains("Checkpoint") && comp["Checkpoint"].contains("modelPath")) {
                checkpointPath = comp["Checkpoint"]["modelPath"].get<std::string>();
            }
            if (comp.contains("ClipL") && comp["ClipL"].contains("modelPath")) {
                clipLPath = comp["ClipL"]["modelPath"].get<std::string>();
            }
            if (comp.contains("ClipG") && comp["ClipG"].contains("modelPath")) {
                clipGPath = comp["ClipG"]["modelPath"].get<std::string>();
            }
            if (comp.contains("T5XXL") && comp["T5XXL"].contains("modelPath")) {
                t5xxlPath = comp["T5XXL"]["modelPath"].get<std::string>();
            }
            if (comp.contains("ClipVision") && comp["ClipVision"].contains("modelPath")) {
                clipVisionPath = comp["ClipVision"]["modelPath"].get<std::string>();
            }
            if (comp.contains("LlmVision") && comp["LlmVision"].contains("modelPath")) {
                llmVisionPath = comp["LlmVision"]["modelPath"].get<std::string>();
            }
            if (comp.contains("ControlNet") && comp["ControlNet"].contains("modelPath")) {
                controlNetPath = comp["ControlNet"]["modelPath"].get<std::string>();
            }
            if (comp.contains("AudioVae") && comp["AudioVae"].contains("modelPath")) {
                audioVaePath = comp["AudioVae"]["modelPath"].get<std::string>();
            }
            if (comp.contains("MotionModule") && comp["MotionModule"].contains("modelPath")) {
                motionPath = comp["MotionModule"]["modelPath"].get<std::string>();
            }
            if (comp.contains("PhotoMaker") && comp["PhotoMaker"].contains("modelPath")) {
                photoMakerPath = comp["PhotoMaker"]["modelPath"].get<std::string>();
            }
            if (comp.contains("PulidWeights") && comp["PulidWeights"].contains("modelPath")) {
                pulidPath = comp["PulidWeights"]["modelPath"].get<std::string>();
            }
            if (comp.contains("Taesd") && comp["Taesd"].contains("modelPath")) {
                taesdPath = comp["Taesd"]["modelPath"].get<std::string>();
            }
            if (comp.contains("Embeddings") && comp["Embeddings"].contains("modelPath")) {
                embeddingsPath = comp["Embeddings"]["modelPath"].get<std::string>();
            }
        }

        if (!diffusionPath.empty()) {
            key += "diffusion=" + diffusionPath + "|";
        }
        if (!highNoiseDiffusionPath.empty()) {
            key += "high_noise_diffusion=" + highNoiseDiffusionPath + "|";
        }
        if (!uncondDiffusionPath.empty()) {
            key += "uncond_diffusion=" + uncondDiffusionPath + "|";
        }
        if (!vaePath.empty()) {
            key += "vae=" + vaePath + "|";
        }
        if (!vaeFormat.empty()) {
            key += "vae_format=" + vaeFormat + "|";
        }
        if (!vaeDecodeOnly.empty()) {
            key += "vae_decode_only=" + vaeDecodeOnly + "|";
        }
        if (!llmPath.empty()) {
            key += "llm=" + llmPath + "|";
        }
        if (!checkpointPath.empty()) {
            key += "checkpoint=" + checkpointPath + "|";
        }
        if (!clipLPath.empty()) {
            key += "clip_l=" + clipLPath + "|";
        }
        if (!clipGPath.empty()) {
            key += "clip_g=" + clipGPath + "|";
        }
        if (!t5xxlPath.empty()) {
            key += "t5xxl=" + t5xxlPath + "|";
        }
        if (!clipVisionPath.empty()) {
            key += "clip_vision=" + clipVisionPath + "|";
        }
        if (!llmVisionPath.empty()) {
            key += "llm_vision=" + llmVisionPath + "|";
        }
        if (!controlNetPath.empty()) {
            key += "controlnet=" + controlNetPath + "|";
        }
        if (!audioVaePath.empty()) {
            key += "audio_vae=" + audioVaePath + "|";
        }
        if (!motionPath.empty()) {
            key += "motion=" + motionPath + "|";
        }
        if (!photoMakerPath.empty()) {
            key += "photo_maker=" + photoMakerPath + "|";
        }
        if (!pulidPath.empty()) {
            key += "pulid=" + pulidPath + "|";
        }
        if (!taesdPath.empty()) {
            key += "taesd=" + taesdPath + "|";
        }
        if (!embeddingsPath.empty()) {
            key += "embeddings=" + embeddingsPath + "|";
        }

        if (key.empty()) {
            key = "default";
        }

        return key;
    }

    void ModelCacheSystem::promote(const std::string& key) {
        auto it = std::find(m_order.begin(), m_order.end(), key);
        if (it != m_order.end()) {
            m_order.erase(it);
            m_order.push_back(key);
        }
    }

    void ModelCacheSystem::evictIfNeeded() {
        while (m_cache.size() > m_maxCacheSize) {
            std::string evictKey = m_order.front();
            auto it = m_cache.find(evictKey);
            if (it != m_cache.end() && it->second.activeCount > 0) {
                m_order.pop_front();
                m_order.push_back(evictKey);
                continue;
            }
            m_order.pop_front();
            if (it != m_cache.end()) {
                free_sd_ctx(it->second.ctx);
                m_cache.erase(it);
            }
            if (m_cache.size() == m_order.size()) break;
        }
    }

    size_t ModelCacheSystem::computeMemory(const nlohmann::json& metadata) const {
        size_t total = 0;
        if (!metadata.contains("components") || !metadata["components"].is_array())
            return total;
        for (const auto& comp : metadata["components"]) {
            std::vector<std::string> pathKeys = {
                "modelPath", "vae_path", "diffusion_model_path", "high_noise_diffusion_model_path",
                "uncond_diffusion_model_path", "embeddings_connectors_path", "audio_vae_path",
                "taesd_path", "control_net_path", "ip_adapter_path", "motion_module_path",
                "photo_maker_path", "pulid_weights_path", "clip_l_path", "clip_g_path",
                "clip_vision_path", "t5xxl_path", "llm_path", "llm_vision_path"
            };
            for (const auto& key : pathKeys) {
                if (comp.contains(key) && comp[key].is_string()) {
                    std::string path = comp[key].get<std::string>();
                    if (!path.empty() && std::filesystem::exists(path)) {
                        try {
                            total += std::filesystem::file_size(path);
                        }
                        catch (...) {}
                    }
                }
            }
            for (const auto& item : comp.items()) {
                if (item.value().is_object() && item.value().contains("modelPath")) {
                    std::string path = item.value()["modelPath"].get<std::string>();
                    if (!path.empty() && std::filesystem::exists(path)) {
                        try {
                            total += std::filesystem::file_size(path);
                        }
                        catch (...) {}
                    }
                }
            }
        }
        return total;
    }

    std::string ModelCacheSystem::detectModelType(const nlohmann::json& metadata) const {
        std::string combined;
        if (metadata.contains("components") && metadata["components"].is_array()) {
            for (const auto& comp : metadata["components"]) {
                if (comp.contains("Checkpoint") && comp["Checkpoint"].contains("modelPath")) {
                    std::string path = comp["Checkpoint"]["modelPath"].get<std::string>();
                    combined += path + " ";
                }
                if (comp.contains("DiffusionModel") && comp["DiffusionModel"].contains("modelPath")) {
                    std::string path = comp["DiffusionModel"]["modelPath"].get<std::string>();
                    combined += path + " ";
                }
                if (comp.contains("LlmEncoder") && comp["LlmEncoder"].contains("modelPath")) {
                    std::string path = comp["LlmEncoder"]["modelPath"].get<std::string>();
                    combined += path + " ";
                }
            }
        }
        if (combined.empty()) return "Unknown";
        std::string lower = combined;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("minimax") != std::string::npos) return "MiniMax";
        if (lower.find("flux") != std::string::npos) return "Flux";
        if (lower.find("sdxl") != std::string::npos) return "SDXL";
        if (lower.find("sd3") != std::string::npos) return "SD3";
        if (lower.find("sd1.5") != std::string::npos || lower.find("sd1_5") != std::string::npos) return "SD1.5";
        if (lower.find("sd2") != std::string::npos) return "SD2";
        if (lower.find("wan") != std::string::npos) return "Wan";
        if (lower.find("ltx") != std::string::npos) return "LTX";
        if (lower.find("qwen") != std::string::npos) return "Qwen";
        return "Diffusion";
    }

}