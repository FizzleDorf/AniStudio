#pragma once

#include "BaseSystem.hpp"
#include "stable-diffusion.h"
#include "SDCPPUtils.hpp"
#include <unordered_map>
#include <deque>
#include <mutex>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <chrono>

namespace ECS {

    struct ContextDetail {
        std::string key;
        std::string displayName;
        std::string modelType;
        size_t memoryBytes;
        int activeCount;
        bool isInUse;
    };

    class ModelCacheSystem : public BaseSystem {
    public:
        ModelCacheSystem(EntityManager& mgr) : BaseSystem(mgr) {
            sysName = "ModelCacheSystem";
        }

        ~ModelCacheSystem() {
            std::cout << "[ModelCacheSystem] DESTRUCTOR CALLED - Cache size: " << m_cache.size() << std::endl;
        }

        sd_ctx_t* getOrCreateContext(const nlohmann::json& metadata);
        bool loadModelFromMetadata(const nlohmann::json& metadata);
        void reloadModel(const std::string& key);
        void UnloadModel(const std::string& key);
        void UnloadAllModels();
        void UnloadInactiveModels();
        void ForceUnloadIdleModels() { UnloadAllModels(); }

        std::vector<std::string> GetLoadedModels() const;
        std::vector<ContextDetail> GetContextDetails() const;
        void ListSDContexts() const;

        sd_ctx_t* acquireContext(const std::string& key);
        void releaseContext(const std::string& key);

        std::string computeKey(const nlohmann::json& metadata) const;

        void Destroy() override;

        size_t GetCacheSize() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_cache.size();
        }

    private:
        struct ContextInfo {
            sd_ctx_t* ctx;
            nlohmann::json metadata;
            size_t memoryBytes;
            int activeCount;
            std::string modelType;
            bool isInUse;
            std::chrono::steady_clock::time_point lastUsed;
        };

        std::unordered_map<std::string, ContextInfo> m_cache;
        std::deque<std::string> m_order;
        size_t m_maxCacheSize = 10;
        mutable std::mutex m_mutex;

        void promote(const std::string& key);
        void evictIfNeeded();
        size_t computeMemory(const nlohmann::json& metadata) const;
        std::string detectModelType(const nlohmann::json& metadata) const;
    };

}