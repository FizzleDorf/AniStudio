#pragma once
#include "FilePaths.hpp"
#include <memory>
#include <mutex>
#include <string>

namespace Utils {

	class FilePathService {
	private:
		static inline std::unique_ptr<FilePaths> filePathsInstance = nullptr;
		static inline std::mutex instanceMutex;

		// Private constructor to prevent instantiation
		FilePathService() = delete;
		~FilePathService() = delete;

	public:
		// Initialize with a FilePaths instance (call this once at startup)
		static void Initialize(std::shared_ptr<FilePaths> filePaths) {
			std::lock_guard<std::mutex> lock(instanceMutex);
			if (filePaths) {
				filePathsInstance.reset(new FilePaths(std::move(*filePaths)));
			}
		}

		// Initialize with a new FilePaths instance
		static void Initialize() {
			std::lock_guard<std::mutex> lock(instanceMutex);
			filePathsInstance = std::make_unique<FilePaths>();
			filePathsInstance->Init();
		}

		// Get a path by key
		static std::string GetPath(const std::string& key) {
			std::lock_guard<std::mutex> lock(instanceMutex);
			if (filePathsInstance && filePathsInstance->IsInitialized()) {
				const char* path = filePathsInstance->GetPath(key.c_str());
				return path ? std::string(path) : "";
			}
			return "";
		}

		// Set a path by key
		static void SetPath(const std::string& key, const std::string& path) {
			std::lock_guard<std::mutex> lock(instanceMutex);
			if (filePathsInstance) {
				filePathsInstance->SetPath(key.c_str(), path.c_str());
			}
		}

		// Check if a path exists
		static bool HasPath(const std::string& key) {
			std::lock_guard<std::mutex> lock(instanceMutex);
			if (filePathsInstance) {
				const char* path = filePathsInstance->GetPath(key.c_str());
				return path && path[0] != '\0';
			}
			return false;
		}

		// Check if service is initialized
		static bool IsInitialized() {
			std::lock_guard<std::mutex> lock(instanceMutex);
			return filePathsInstance != nullptr && filePathsInstance->IsInitialized();
		}

		// Get the raw FilePaths pointer (for advanced use only)
		static FilePaths* GetInstance() {
			std::lock_guard<std::mutex> lock(instanceMutex);
			return filePathsInstance.get();
		}

		// Clean up (call on shutdown)
		static void Shutdown() {
			std::lock_guard<std::mutex> lock(instanceMutex);
			filePathsInstance.reset();
		}

		// ===== NEW METHODS - Exposing FilePaths public methods =====

		// Application initialization - should be called at startup
		static void Init() {
			std::lock_guard<std::mutex> lock(instanceMutex);
			if (filePathsInstance) {
				filePathsInstance->Init();
			}
		}

		// Save all current paths to JSON
		static void SaveFilepathDefaults() {
			std::lock_guard<std::mutex> lock(instanceMutex);
			if (filePathsInstance) {
				filePathsInstance->SaveFilepathDefaults();
			}
		}

		// Load paths from JSON
		static void LoadFilePathDefaults() {
			std::lock_guard<std::mutex> lock(instanceMutex);
			if (filePathsInstance) {
				filePathsInstance->LoadFilePathDefaults();
			}
		}

		// Set up model subdirectories based on root path
		static void SetByModelRoot() {
			std::lock_guard<std::mutex> lock(instanceMutex);
			if (filePathsInstance) {
				filePathsInstance->SetByModelRoot();
			}
		}

		// Utility functions
		static bool IsProjectPath(const std::string& path) {
			std::lock_guard<std::mutex> lock(instanceMutex);
			if (filePathsInstance) {
				return filePathsInstance->IsProjectPath(path.c_str());
			}
			return false;
		}

		static std::string GetProjectName(const std::string& projectPath) {
			std::lock_guard<std::mutex> lock(instanceMutex);
			if (filePathsInstance) {
				const char* name = filePathsInstance->GetProjectName(projectPath.c_str());
				return name ? std::string(name) : "";
			}
			return "";
		}

		// Debug output
		static void PrintCurrentPaths() {
			std::lock_guard<std::mutex> lock(instanceMutex);
			if (filePathsInstance) {
				filePathsInstance->PrintCurrentPaths();
			}
		}

		// Getters for internal state
		static std::string GetExecutableDir() {
			std::lock_guard<std::mutex> lock(instanceMutex);
			if (filePathsInstance) {
				const char* dir = filePathsInstance->GetExecutableDir();
				return dir ? std::string(dir) : "";
			}
			return "";
		}

		static std::string GetDataPath() {
			std::lock_guard<std::mutex> lock(instanceMutex);
			if (filePathsInstance) {
				const char* path = filePathsInstance->GetDataPath();
				return path ? std::string(path) : "";
			}
			return "";
		}
	};

} // namespace Utils