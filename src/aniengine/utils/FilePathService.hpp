#pragma once
#include "FilePaths.hpp"
#include <memory>
#include <mutex>
#include <string>

namespace Utils {

	class FilePathService {
	private:
		static inline std::shared_ptr<FilePaths> filePathsInstance = nullptr;
		static inline std::mutex instanceMutex;

		FilePathService() = delete;
		~FilePathService() = delete;

	public:
		static void SetInstance(std::shared_ptr<FilePaths> filePaths) {
			std::lock_guard<std::mutex> lock(instanceMutex);
			filePathsInstance = filePaths;
		}

		static std::shared_ptr<FilePaths> GetInstance() {
			std::lock_guard<std::mutex> lock(instanceMutex);
			return filePathsInstance;
		}

		static bool IsInitialized() {
			std::lock_guard<std::mutex> lock(instanceMutex);
			return filePathsInstance != nullptr && filePathsInstance->IsInitialized();
		}

		static std::string GetPath(const std::string& key) {
			auto instance = GetInstance();
			if (instance && instance->IsInitialized()) {
				const char* path = instance->GetPath(key.c_str());
				return path ? std::string(path) : "";
			}
			return "";
		}

		static void SetPath(const std::string& key, const std::string& path) {
			auto instance = GetInstance();
			if (instance) {
				instance->SetPath(key.c_str(), path.c_str());
			}
		}

		static bool HasPath(const std::string& key) {
			auto instance = GetInstance();
			if (instance) {
				const char* path = instance->GetPath(key.c_str());
				return path && path[0] != '\0';
			}
			return false;
		}

		static void Shutdown() {
			std::lock_guard<std::mutex> lock(instanceMutex);
			filePathsInstance.reset();
		}

		static void Init() {
			auto instance = GetInstance();
			if (instance) {
				instance->Init();
			}
		}

		static void SaveFilepathDefaults() {
			auto instance = GetInstance();
			if (instance) {
				instance->SaveFilepathDefaults();
			}
		}

		static void LoadFilePathDefaults() {
			auto instance = GetInstance();
			if (instance) {
				instance->LoadFilePathDefaults();
			}
		}

		static void SetByModelRoot() {
			auto instance = GetInstance();
			if (instance) {
				instance->SetByModelRoot();
			}
		}

		static bool IsProjectPath(const std::string& path) {
			auto instance = GetInstance();
			if (instance) {
				return instance->IsProjectPath(path.c_str());
			}
			return false;
		}

		static std::string GetProjectName(const std::string& projectPath) {
			auto instance = GetInstance();
			if (instance) {
				const char* name = instance->GetProjectName(projectPath.c_str());
				return name ? std::string(name) : "";
			}
			return "";
		}

		static void PrintCurrentPaths() {
			auto instance = GetInstance();
			if (instance) {
				instance->PrintCurrentPaths();
			}
		}

		static std::string GetExecutableDir() {
			auto instance = GetInstance();
			if (instance) {
				const char* dir = instance->GetExecutableDir();
				return dir ? std::string(dir) : "";
			}
			return "";
		}

		static std::string GetDataPath() {
			auto instance = GetInstance();
			if (instance) {
				const char* path = instance->GetDataPath();
				return path ? std::string(path) : "";
			}
			return "";
		}
	};

}