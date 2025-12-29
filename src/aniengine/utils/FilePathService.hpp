#pragma once
#include "FilePaths.hpp"
#include <memory>
#include <mutex>
#include <string>

namespace Utils {

	class FilePathService {
	private:
		static std::unique_ptr<FilePaths> filePathsInstance;
		static std::mutex instanceMutex;

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
	};

} // namespace Utils