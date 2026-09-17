// ModelCacheSystem.hpp
#pragma once

#include "BaseSystem.hpp"
#include "stable-diffusion.h"
#include "SDCPPUtils.hpp"
#include "SDContextHandle.hpp"

#include <unordered_map>
#include <deque>
#include <mutex>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <optional>

namespace ECS {

    struct ContextDetail {
        std::string key;
        std::string displayName;
        std::string modelType;
        size_t memoryBytes;
        int activeCount;
        bool isInUse;
    };

    struct UpscalerDetail {
        std::string key;
        std::string displayName;
        size_t memoryBytes;
        int activeCount;
        bool isInUse;
    };

    class ModelCacheSystem : public BaseSystem {
    public:
        explicit ModelCacheSystem(EntityManager& mgr)
            : BaseSystem(mgr) {
            sysName = "ModelCacheSystem";
        }

        ~ModelCacheSystem() override;

        // ----------------------------------------------------------------
        // Acquisition
        //
        // ctxRes is passed by reference. On a cache hit the entry's own
        // ResourceManager is assigned back into ctxRes, so the caller gets
        // a shared_ptr to whichever manager actually owns the strings the
        // cached sd_ctx_params_t points at. On a miss, ctxRes is moved into
        // the new entry.
        //
        // No pre-flight memory gate. sdcpp's memory manager decides
        // placement, segmented execution, and eviction; if the model can't
        // fit at all, new_sd_ctx returns null and we report that.
        // ----------------------------------------------------------------
        std::optional<SDCPP::SDContextHandle>
            acquireOrCreateContext(const sd_ctx_params_t& params,
                std::shared_ptr<SDCPP::ResourceManager>& ctxRes);

        std::optional<SDCPP::UpscalerHandle>
            acquireOrCreateUpscaler(const sd_ctx_params_t& params,
                std::shared_ptr<SDCPP::ResourceManager>& ctxRes);

        // ----------------------------------------------------------------
        // Release (called by handles)
        // ----------------------------------------------------------------
        void releaseContext(const std::string& key);
        void releaseUpscaler(const std::string& key);

        // ----------------------------------------------------------------
        // GUI / management
        //
        // Unload refuses to touch entries with activeCount > 0. Callers
        // that want to unload an in-use entry must first cancel every task
        // referencing it (SDCPPSystem::ClearAllTasks / CancelCurrentTask),
        // which drops the handles and brings activeCount to zero.
        // ----------------------------------------------------------------
        void UnloadModel(const std::string& key);
        void UnloadAllModels();
        void UnloadInactiveModels();

        void UnloadUpscaler(const std::string& key);
        void UnloadAllUpscalers();

        bool reloadModel(const std::string& key);
        bool reloadUpscaler(const std::string& key);

        std::vector<ContextDetail> GetContextDetails() const;
        std::vector<UpscalerDetail> GetUpscalerDetails() const;

        size_t GetCacheSize() const;
        size_t GetUpscalerCacheSize() const;

        std::string getLastError() const;

        std::string computeKey(const sd_ctx_params_t& params) const;
        std::string computeUpscalerKey(const sd_ctx_params_t& params) const;

        void Destroy() override;

    private:
        struct ContextInfo {
            sd_ctx_t* ctx = nullptr;
            sd_ctx_params_t params{};
            std::shared_ptr<SDCPP::ResourceManager> ctxRes;
            std::string key;
            size_t memoryBytes = 0;
            int activeCount = 0;
            std::string modelType;
            std::chrono::steady_clock::time_point lastUsed;
        };

        struct UpscalerInfo {
            upscaler_ctx_t* ctx = nullptr;
            sd_ctx_params_t params{};
            std::shared_ptr<SDCPP::ResourceManager> ctxRes;
            std::string key;
            size_t memoryBytes = 0;
            int activeCount = 0;
            std::chrono::steady_clock::time_point lastUsed;
        };

        std::unordered_map<std::string, ContextInfo> m_cache;
        std::deque<std::string> m_order;

        std::unordered_map<std::string, UpscalerInfo> m_upscalers;
        std::deque<std::string> m_upscalerOrder;

        size_t m_maxCacheSize = 10;
        mutable std::mutex m_mutex;
        mutable std::string m_lastError;

        void promote(const std::string& key);
        void evictIfNeeded();
        void removeFromOrder(const std::string& key);

        size_t computeMemory(const sd_ctx_params_t& params) const;
        std::string detectModelType(const sd_ctx_params_t& params) const;
    };

} // namespace ECS