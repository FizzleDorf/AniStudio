// PluginManager.hpp
#pragma once

#include "PluginInterface.hpp"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <filesystem>

// Forward declarations 
namespace ECS {
	class EntityManager;
}

namespace GUI {
	class ViewManager;
}

#ifdef _WIN32
#include <Windows.h>
#define PLUGIN_EXTENSION ".dll"
typedef HMODULE LibraryHandle;
#else
#include <dlfcn.h>
#define PLUGIN_EXTENSION ".so"
typedef void* LibraryHandle;
#endif

namespace Plugin {
	class PluginManager {
	public:
		PluginManager(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr);
		~PluginManager();

		void Init();
		void Update(float deltaTime);
		bool LoadPlugin(const std::string& path);
		bool UnloadPlugin(const std::string& name);
		void ScanForPlugins(const std::string& pluginDir = "../plugins");
		std::vector<std::string> GetLoadedPlugins() const;
		Plugin::IPlugin* GetPlugin(const std::string& name);
		std::string GetPluginPath(const std::string& name) const;
		void HotReloadPlugins();

	private:
		struct PluginInfo {
			std::string path;
			LibraryHandle handle = nullptr;
			Plugin::IPlugin* instance = nullptr;
			Plugin::CreatePluginFn createFn = nullptr;
			Plugin::DestroyPluginFn destroyFn = nullptr;
			std::filesystem::file_time_type lastModified;
			bool needsReload = false;
		};

		LibraryHandle OpenLibrary(const std::string& path);
		void CloseLibrary(LibraryHandle handle);
		void* GetSymbol(LibraryHandle handle, const std::string& name);
		bool CheckPluginUpdates();

		ECS::EntityManager& mgr;
		GUI::ViewManager& viewMgr;
		std::unordered_map<std::string, PluginInfo> loadedPlugins;
		std::string pluginsDirectory;
	};
}