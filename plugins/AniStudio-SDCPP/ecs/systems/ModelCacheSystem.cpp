#include "ModelCacheSystem.hpp"
#include <filesystem>

namespace ECS {

    sd_ctx_t* ModelCacheSystem::getOrCreateContext(const nlohmann::json& metadata) {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::string key = computeKey(metadata);
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            promote(key);
            return it->second.ctx;
        }

        sd_ctx_params_t ctxParams;
        sd_ctx_params_init(&ctxParams);
        SDCPP::ResourceManager res;
        if (!SDCPP::parseContextParams(metadata, ctxParams, res)) return nullptr;
        sd_ctx_t* ctx = new_sd_ctx(&ctxParams);
        if (!ctx) return nullptr;

        ContextInfo info;
        info.ctx = ctx;
        info.metadata = metadata;
        info.memoryBytes = computeMemory(metadata);
        info.activeCount = 0;
        info.modelType = detectModelType(metadata);
        info.isInUse = false;
        m_cache[key] = info;
        m_order.push_back(key);
        evictIfNeeded();
        return ctx;
    }

    bool ModelCacheSystem::loadModelFromMetadata(const nlohmann::json& metadata) {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::string key = computeKey(metadata);
        if (m_cache.find(key) != m_cache.end()) return true;

        sd_ctx_params_t ctxParams;
        sd_ctx_params_init(&ctxParams);
        SDCPP::ResourceManager res;
        if (!SDCPP::parseContextParams(metadata, ctxParams, res)) return false;
        sd_ctx_t* ctx = new_sd_ctx(&ctxParams);
        if (!ctx) return false;

        ContextInfo info;
        info.ctx = ctx;
        info.metadata = metadata;
        info.memoryBytes = computeMemory(metadata);
        info.activeCount = 0;
        info.modelType = detectModelType(metadata);
        info.isInUse = false;
        m_cache[key] = info;
        m_order.push_back(key);
        evictIfNeeded();
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
        m_cache[key] = info;
        m_order.push_back(key);
    }

    void ModelCacheSystem::UnloadModel(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
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
        for (auto& pair : m_cache) {
            free_sd_ctx(pair.second.ctx);
        }
        m_cache.clear();
        m_order.clear();
    }

    void ModelCacheSystem::UnloadInactiveModels() {
        std::lock_guard<std::mutex> lock(m_mutex);
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
        auto it = m_cache.find(key);
        if (it == m_cache.end()) return nullptr;
        it->second.activeCount++;
        it->second.isInUse = true;
        return it->second.ctx;
    }

    void ModelCacheSystem::releaseContext(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cache.find(key);
        if (it == m_cache.end()) return;
        if (it->second.activeCount > 0) {
            it->second.activeCount--;
            if (it->second.activeCount == 0) {
                it->second.isInUse = false;
            }
        }
    }

    std::string ModelCacheSystem::computeKey(const nlohmann::json& metadata) const {
        std::string key;
        if (metadata.contains("components") && metadata["components"].is_array()) {
            for (const auto& comp : metadata["components"]) {
                if (comp.contains("Checkpoint") && comp["Checkpoint"].contains("modelPath"))
                    key += comp["Checkpoint"]["modelPath"].get<std::string>() + "|";
                if (comp.contains("Vae") && comp["Vae"].contains("modelPath"))
                    key += comp["Vae"]["modelPath"].get<std::string>() + "|";
                if (comp.contains("DiffusionModel") && comp["DiffusionModel"].contains("modelPath"))
                    key += comp["DiffusionModel"]["modelPath"].get<std::string>() + "|";
                if (comp.contains("ClipL") && comp["ClipL"].contains("modelPath"))
                    key += comp["ClipL"]["modelPath"].get<std::string>() + "|";
                if (comp.contains("ClipG") && comp["ClipG"].contains("modelPath"))
                    key += comp["ClipG"]["modelPath"].get<std::string>() + "|";
                if (comp.contains("T5XXL") && comp["T5XXL"].contains("modelPath"))
                    key += comp["T5XXL"]["modelPath"].get<std::string>() + "|";
                if (comp.contains("ClipVision") && comp["ClipVision"].contains("modelPath"))
                    key += comp["ClipVision"]["modelPath"].get<std::string>() + "|";
                if (comp.contains("LlmEncoder") && comp["LlmEncoder"].contains("modelPath"))
                    key += comp["LlmEncoder"]["modelPath"].get<std::string>() + "|";
                if (comp.contains("LlmVision") && comp["LlmVision"].contains("modelPath"))
                    key += comp["LlmVision"]["modelPath"].get<std::string>() + "|";
                if (comp.contains("ControlNet") && comp["ControlNet"].contains("modelPath"))
                    key += comp["ControlNet"]["modelPath"].get<std::string>() + "|";
            }
        }
        if (key.empty()) key = "default";
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
            }
        }
        if (combined.empty()) return "Unknown";
        std::string lower = combined;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("flux") != std::string::npos) return "Flux";
        if (lower.find("sdxl") != std::string::npos) return "SDXL";
        if (lower.find("sd3") != std::string::npos) return "SD3";
        if (lower.find("sd1.5") != std::string::npos || lower.find("sd1_5") != std::string::npos) return "SD1.5";
        if (lower.find("sd2") != std::string::npos) return "SD2";
        if (lower.find("wan") != std::string::npos) return "Wan";
        if (lower.find("ltx") != std::string::npos) return "LTX";
        return "Diffusion";
    }

} // namespace ECS