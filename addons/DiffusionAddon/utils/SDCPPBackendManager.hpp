#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace DiffusionAddon {

	enum class SDCPPBackend {
		CPU_NOAVX,
		CPU_AVX,
		CPU_AVX2,
		CPU_AVX512,
		CUDA,
		VULKAN,
		METAL,
		UNKNOWN
	};

	struct BackendInfo {
		SDCPPBackend type;
		std::string name;
		std::string downloadUrl;
		std::string sha256;
		std::string zipFilename;
		std::vector<std::string> requiredFiles;  // DLLs/SOs needed
		bool available = false;
		bool downloaded = false;
	};

	class SDCPPBackendManager {
	public:
		SDCPPBackendManager(const std::string& libsDirectory);
		~SDCPPBackendManager();

		// Initialize - detect available backends, download if needed
		bool Initialize();

		// Load a specific backend
		bool LoadBackend(SDCPPBackend backend);

		// Get current loaded backend
		SDCPPBackend GetCurrentBackend() const { return currentBackend; }

		// Get list of available backends
		std::vector<BackendInfo> GetAvailableBackends() const;

		// Download and extract a backend
		bool DownloadBackend(SDCPPBackend backend, std::function<void(int, int)> progressCallback = nullptr);

		// Check if backend libraries exist
		bool IsBackendAvailable(SDCPPBackend backend) const;

		// Unload current backend
		void UnloadBackend();

		// Get function pointer from loaded library
		void* GetFunction(const std::string& name);

	private:
		std::string libsDir;
		SDCPPBackend currentBackend = SDCPPBackend::UNKNOWN;
		void* libraryHandle = nullptr;

		// Backend definitions with download URLs
		std::map<SDCPPBackend, BackendInfo> backends;

		void InitializeBackendInfo();
		bool DetectSystemCapabilities();
		SDCPPBackend SelectBestBackend();

		bool DownloadFile(const std::string& url, const std::string& outputPath,
			std::function<void(int, int)> progressCallback);
		bool ExtractZip(const std::string& zipPath, const std::string& extractPath);
		bool VerifySHA256(const std::string& filePath, const std::string& expectedHash);

		void* LoadDynamicLibrary(const std::string& path);
		void UnloadDynamicLibrary(void* handle);
		void* GetSymbol(void* handle, const std::string& name);

		std::string GetPlatformLibraryName(SDCPPBackend backend);
		std::string GetBackendPath(SDCPPBackend backend);
	};

} // namespace DiffusionAddon