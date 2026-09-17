// SDContextHandle.hpp
#pragma once

#include "stable-diffusion.h"
#include <string>

namespace ECS { class ModelCacheSystem; }

namespace SDCPP {

    // RAII handle for a cached sd_ctx_t. On destruction, calls
    // ModelCacheSystem::releaseContext with the stored key. Move-only.
    class SDContextHandle {
    public:
        SDContextHandle() = default;

        SDContextHandle(ECS::ModelCacheSystem* cache, std::string key, sd_ctx_t* ctx)
            : m_cache(cache), m_key(std::move(key)), m_ctx(ctx) {
        }

        ~SDContextHandle() { reset(); }

        SDContextHandle(const SDContextHandle&) = delete;
        SDContextHandle& operator=(const SDContextHandle&) = delete;

        SDContextHandle(SDContextHandle&& o) noexcept
            : m_cache(o.m_cache), m_key(std::move(o.m_key)), m_ctx(o.m_ctx) {
            o.m_cache = nullptr;
            o.m_ctx = nullptr;
        }

        SDContextHandle& operator=(SDContextHandle&& o) noexcept {
            if (this != &o) {
                reset();
                m_cache = o.m_cache;
                m_key = std::move(o.m_key);
                m_ctx = o.m_ctx;
                o.m_cache = nullptr;
                o.m_ctx = nullptr;
            }
            return *this;
        }

        sd_ctx_t* get() const { return m_ctx; }
        explicit operator bool() const { return m_ctx != nullptr; }
        const std::string& key() const { return m_key; }

    private:
        void reset();

        ECS::ModelCacheSystem* m_cache = nullptr;
        std::string m_key;
        sd_ctx_t* m_ctx = nullptr;
    };

    // RAII handle for a cached upscaler_ctx_t. On destruction, calls
    // ModelCacheSystem::releaseUpscaler with the stored key. Move-only.
    class UpscalerHandle {
    public:
        UpscalerHandle() = default;

        UpscalerHandle(ECS::ModelCacheSystem* cache, std::string key, upscaler_ctx_t* ctx)
            : m_cache(cache), m_key(std::move(key)), m_ctx(ctx) {
        }

        ~UpscalerHandle() { reset(); }

        UpscalerHandle(const UpscalerHandle&) = delete;
        UpscalerHandle& operator=(const UpscalerHandle&) = delete;

        UpscalerHandle(UpscalerHandle&& o) noexcept
            : m_cache(o.m_cache), m_key(std::move(o.m_key)), m_ctx(o.m_ctx) {
            o.m_cache = nullptr;
            o.m_ctx = nullptr;
        }

        UpscalerHandle& operator=(UpscalerHandle&& o) noexcept {
            if (this != &o) {
                reset();
                m_cache = o.m_cache;
                m_key = std::move(o.m_key);
                m_ctx = o.m_ctx;
                o.m_cache = nullptr;
                o.m_ctx = nullptr;
            }
            return *this;
        }

        upscaler_ctx_t* get() const { return m_ctx; }
        explicit operator bool() const { return m_ctx != nullptr; }
        const std::string& key() const { return m_key; }

    private:
        void reset();

        ECS::ModelCacheSystem* m_cache = nullptr;
        std::string m_key;
        upscaler_ctx_t* m_ctx = nullptr;
    };

} // namespace SDCPP