// BasePlugin.hpp - FIXED VERSION
// Removes duplicate HostData definition that conflicts with CRWrapper.hpp
#pragma once

#include <string>
#include <memory>

// Forward declarations
namespace ECS { class EntityManager; }
namespace GUI { class ViewManager; }
namespace HotReload { struct HostData; } // Use the one from CRWrapper.hpp

namespace Plugin {

	class BasePlugin {
	public:
		BasePlugin() = default;
		virtual ~BasePlugin() = default;

		// Plugin lifecycle - MUST be implemented by plugins
		virtual bool Initialize(HotReload::HostData* hostData) = 0;
		virtual void Update(float deltaTime) = 0;
		virtual void Render() = 0;
		virtual void Shutdown() = 0;

		// Plugin information
		virtual const char* GetName() const = 0;
		virtual const char* GetVersion() const = 0;
		virtual const char* GetDescription() const { return "No description"; }

		// Hot reload support
		virtual void OnReload() {}
		virtual void OnUnload() {}

		// Access to host data
		HotReload::HostData* GetHostData() const { return hostData; }
		ECS::EntityManager* GetEntityManager() const {
			return hostData ? hostData->entityManager : nullptr;
		}
		GUI::ViewManager* GetViewManager() const {
			return hostData ? hostData->viewManager : nullptr;
		}

	protected:
		HotReload::HostData* hostData = nullptr;

		// Helper methods for common operations
		void SetImGuiContext();
		void UpdateImGuiFrame();
		void RenderImGuiFrame();

		// ImGui rendering state helpers
		bool CreateImGuiDeviceObjects();
		void InvalidateImGuiDeviceObjects();
		bool CreateImGuiFontsTexture();
		void RenderImGuiDrawLists(void* draw_data);
	};

	// Plugin factory function type
	using CreatePluginFunc = BasePlugin * (*)();
	using DestroyPluginFunc = void(*)(BasePlugin*);

} // namespace Plugin

// Macro to simplify plugin export for CR
#define DECLARE_CR_PLUGIN(ClassName) \
    static ClassName* g_plugin = nullptr; \
    extern "C" int cr_main(struct cr_plugin* ctx, enum cr_op operation) { \
        auto* hostData = static_cast<HotReload::HostData*>(ctx->userdata); \
        switch (operation) { \
            case CR_LOAD: \
                if (!g_plugin) g_plugin = new ClassName(); \
                return g_plugin->Initialize(hostData) ? 0 : -1; \
            case CR_STEP: \
                if (g_plugin) { \
                    g_plugin->Update(0.016f); \
                    g_plugin->Render(); \
                } \
                return 0; \
            case CR_UNLOAD: \
                if (g_plugin) g_plugin->OnUnload(); \
                return 0; \
            case CR_CLOSE: \
                if (g_plugin) { \
                    g_plugin->Shutdown(); \
                    delete g_plugin; \
                    g_plugin = nullptr; \
                } \
                return 0; \
        } \
        return 0; \
    }