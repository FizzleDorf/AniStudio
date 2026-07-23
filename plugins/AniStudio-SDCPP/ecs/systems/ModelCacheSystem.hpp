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

namespace ECS {

    class ModelCacheSystem : public BaseSystem {
    public:
        ModelCacheSystem(EntityManager& mgr) : BaseSystem(mgr) { sysName = "ModelCacheSystem"; }

        sd_ctx_t* getOrCreateContext(const nlohmann::json& metadata) {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::string key = computeKey(metadata);
            auto it = m_cache.find(key);
            if (it != m_cache.end()) {
                promote(key);
                return it->second;
            }
            sd_ctx_params_t ctxParams;
            sd_ctx_params_init(&ctxParams);
            SDCPP::ResourceManager res;
            if (!SDCPP::parseContextParams(metadata, ctxParams, res)) return nullptr;
            sd_ctx_t* ctx = new_sd_ctx(&ctxParams);
            if (ctx) {
                m_cache[key] = ctx;
                m_metadataMap[key] = metadata;
                m_order.push_back(key);
                evictIfNeeded();
            }
            return ctx;
        }

        bool loadModelFromMetadata(const nlohmann::json& metadata) {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::string key = computeKey(metadata);
            if (m_cache.find(key) != m_cache.end()) return true;
            sd_ctx_params_t ctxParams;
            sd_ctx_params_init(&ctxParams);
            SDCPP::ResourceManager res;
            if (!SDCPP::parseContextParams(metadata, ctxParams, res)) return false;
            sd_ctx_t* ctx = new_sd_ctx(&ctxParams);
            if (!ctx) return false;
            m_cache[key] = ctx;
            m_metadataMap[key] = metadata;
            m_order.push_back(key);
            evictIfNeeded();
            return true;
        }

        void reloadModel(const std::string& key) {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto metaIt = m_metadataMap.find(key);
            if (metaIt == m_metadataMap.end()) return;
            auto ctxIt = m_cache.find(key);
            if (ctxIt != m_cache.end()) {
                free_sd_ctx(ctxIt->second);
                m_cache.erase(ctxIt);
                auto orderIt = std::find(m_order.begin(), m_order.end(), key);
                if (orderIt != m_order.end()) m_order.erase(orderIt);
            }
            sd_ctx_params_t ctxParams;
            sd_ctx_params_init(&ctxParams);
            SDCPP::ResourceManager res;
            if (!SDCPP::parseContextParams(metaIt->second, ctxParams, res)) return;
            sd_ctx_t* ctx = new_sd_ctx(&ctxParams);
            if (ctx) {
                m_cache[key] = ctx;
                m_order.push_back(key);
            }
        }

        std::vector<std::string> GetLoadedModels() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::vector<std::string> result;
            result.reserve(m_cache.size());
            for (const auto& pair : m_cache) result.push_back(pair.first);
            return result;
        }

        void UnloadModel(const std::string& key) {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_cache.find(key);
            if (it != m_cache.end()) {
                free_sd_ctx(it->second);
                m_cache.erase(it);
                m_metadataMap.erase(key);
                auto orderIt = std::find(m_order.begin(), m_order.end(), key);
                if (orderIt != m_order.end()) m_order.erase(orderIt);
            }
        }

        void UnloadAllModels() {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (auto& pair : m_cache) free_sd_ctx(pair.second);
            m_cache.clear();
            m_metadataMap.clear();
            m_order.clear();
        }

        void ForceUnloadIdleModels() { UnloadAllModels(); }

        void ListSDContexts() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::cout << "Cached contexts (" << m_cache.size() << "):\n";
            for (const auto& pair : m_cache) std::cout << "  " << pair.first << std::endl;
        }

        void Destroy() override { UnloadAllModels(); BaseSystem::Destroy(); }

    private:
        std::unordered_map<std::string, sd_ctx_t*> m_cache;
        std::unordered_map<std::string, nlohmann::json> m_metadataMap;
        std::deque<std::string> m_order;
        size_t m_maxCacheSize = 10;
        mutable std::mutex m_mutex;

        void promote(const std::string& key) {
            auto it = std::find(m_order.begin(), m_order.end(), key);
            if (it != m_order.end()) {
                m_order.erase(it);
                m_order.push_back(key);
            }
        }

        void evictIfNeeded() {
            while (m_cache.size() > m_maxCacheSize) {
                std::string evictKey = m_order.front();
                m_order.pop_front();
                auto it = m_cache.find(evictKey);
                if (it != m_cache.end()) {
                    free_sd_ctx(it->second);
                    m_cache.erase(it);
                    m_metadataMap.erase(evictKey);
                }
            }
        }

        std::string computeKey(const nlohmann::json& metadata) {
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
    };

} // namespace ECS