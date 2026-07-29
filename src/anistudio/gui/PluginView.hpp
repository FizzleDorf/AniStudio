// PluginView.hpp
#pragma once

#include "BaseView.hpp"
#include "StudioPluginManager.hpp"
#include "OpenGLWrapper.hpp"
#include <imgui.h>
#include <string>
#include <vector>
#include <set>
#include <filesystem>
#include <unordered_map>

namespace GUI {

    struct AvailablePluginInfo {
        std::string name;
        std::filesystem::path directory;
        bool isLoaded;
        bool isEnabled;
        uint32_t currentVersion;
        std::string sourceDirectory;
        bool isProjectScope;
        GLuint logoTexture;

        AvailablePluginInfo(const std::string& n, const std::filesystem::path& dir, const std::string& src)
            : name(n), directory(dir), isLoaded(false), isEnabled(false), currentVersion(0), sourceDirectory(src), isProjectScope(false), logoTexture(0) {
        }

        ~AvailablePluginInfo() {
            if (logoTexture != 0) {
                glDeleteTextures(1, &logoTexture);
                logoTexture = 0;
            }
        }
    };

    class PluginView : public BaseView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
                "displayName": "Plugin Manager",
                "category": "Tools",
                "description": "Manage and configure plugins with versioned hot reload support"
            })";
        }

        PluginView(ECS::EntityManager& mgr,
            ViewManager& vm,
            Plugins::StudioPluginManager& pluginMgr)
            : BaseView(mgr, vm),
            m_pluginManager(pluginMgr) {
            viewName = "PluginView";
        }
        ~PluginView();

        void Init() override;
        void Update(float deltaT) override;
        void Render() override;

    private:
        Plugins::StudioPluginManager& m_pluginManager;

        std::string m_statusMessage;
        float m_statusTimer = 0.0f;

        bool m_hotReloadEnabled = true;
        bool m_hotReloadWasEnabled = false;

        std::vector<AvailablePluginInfo> m_availablePlugins;
        std::string m_selectedPluginForDetails;

        std::unordered_map<std::string, std::vector<uint32_t>> m_pluginVersions;

        void RefreshAvailablePlugins();
        void DiscoverPluginsInDirectory(const std::filesystem::path& searchDir);

        GLuint LoadLogoTexture(const std::string& pluginDir);
        void LoadPlugin(const std::string& pluginName);
        void EnablePlugin(const std::string& pluginName);
        void DisablePlugin(const std::string& pluginName);
        void ReloadPlugin(const std::string& pluginName);
        void UnloadPlugin(const std::string& pluginName);
        void SetPluginScope(const std::string& pluginName, bool isProjectScope);
        void SwitchToVersion(const std::string& pluginName, uint32_t version);

        void SavePluginScopeConfig();
        void LoadPluginScopeConfig();

        void RenderMainContent();
        void RenderPluginLists();
        void RenderSelectedPluginDetails();
        void RenderStatusBar();

        void ShowStatus(const std::string& message, float duration = 3.0f);
        const char* GetPluginStateText(const Plugins::PluginInfo& plugin) const;
        ImVec4 GetPluginStateColor(const Plugins::PluginInfo& plugin) const;
        AvailablePluginInfo* FindAvailablePlugin(const std::string& pluginName);
        void LoadVersionList(const std::string& pluginName);
    };

} // namespace GUI