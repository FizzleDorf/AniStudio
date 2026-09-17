// SDContextHandle.cpp
#include "SDContextHandle.hpp"
#include "ModelCacheSystem.hpp"

namespace SDCPP {

    void SDContextHandle::reset() {
        if (m_cache && m_ctx) {
            m_cache->releaseContext(m_key);
        }
        m_cache = nullptr;
        m_ctx = nullptr;
    }

    void UpscalerHandle::reset() {
        if (m_cache && m_ctx) {
            m_cache->releaseUpscaler(m_key);
        }
        m_cache = nullptr;
        m_ctx = nullptr;
    }

} // namespace SDCPP