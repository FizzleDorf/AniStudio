// TypesManager.hpp
#pragma once
#include "DLLDefines.hpp"
#include <atomic>
#include <mutex>

namespace ECS {
ANI_API ComponentTypeID AllocateComponentTypeID();
ANI_API SystemTypeID AllocateSystemTypeID();
} // namespace ECS

namespace GUI {
// Core vs Plugin view management
enum class ViewOwner { Core = 0, Plugin = 1 };

class ANI_API TypesManager {
public:
    static TypesManager &Get() {
        static TypesManager instance;
        return instance;
    }

    // View type ID allocation with ownership tracking
    ViewTypeID AllocateViewTypeID(ViewOwner owner) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (owner == ViewOwner::Core) {
            return coreViewCounter_++;
        } else {
            return PLUGIN_VIEW_OFFSET + pluginViewCounter_++;
        }
    }

    // Get current counts (mainly for debugging)
    size_t GetCoreViewCount() const { return coreViewCounter_; }
    size_t GetPluginViewCount() const { return pluginViewCounter_; }

private:
    TypesManager() = default;
    static constexpr ViewTypeID PLUGIN_VIEW_OFFSET = 1000; // Start plugin views at 1000

    std::atomic<ViewTypeID> coreViewCounter_{0};
    std::atomic<ViewTypeID> pluginViewCounter_{0};
    std::mutex mutex_;
};

} // namespace GUI