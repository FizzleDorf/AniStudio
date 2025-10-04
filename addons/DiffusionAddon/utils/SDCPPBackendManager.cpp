#include "SDCPPBackendManager.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <curl/curl.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

// For ZIP extraction - using miniz (header-only)
#define MINIZ_HEADER_FILE_ONLY
#include "miniz.c"

namespace DiffusionAddon {

	static const std::string RELEASE_TAG = "master-309-35843c7";
	static const std::string BASE_URL = "https://github.com/leejet/stable-diffusion.cpp/releases/download/" + RELEASE_TAG + "/";

	SDCPPBackendManager::SDCPPBackendManager(const std::string& libsDirectory)
		: libsDir(libsDirectory) {
		std::filesystem::create_directories(libsDir);
		InitializeBackendInfo();
	}

	SDCPPBackendManager::~SDCPPBackendManager() {
		UnloadBackend();
	}

	void SDCPPBackendManager::InitializeBackendInfo() {
		// Define all available backends with their download info

#ifdef _WIN32
	// Windows backends
		backends[SDCPPBackend::CPU_NOAVX] = {
			SDCPPBackend::CPU_NOAVX,
			"CPU (No AVX)",
			BASE_URL + "sd-master-35843c7-bin-win-noavx-x64.zip",
			"dea6a698b10c91a9b190359974869e9a54597d9b39b54d474004bbdfa48f6061",
			"sd-master-35843c7-bin-win-noavx-x64.zip",
			{"stable-diffusion.dll", "ggml.dll"}
		};

		backends[SDCPPBackend::CPU_AVX] = {
			SDCPPBackend::CPU_AVX,
			"CPU (AVX)",
			BASE_URL + "sd-master-35843c7-bin-win-avx-x64.zip",
			"634940c594293223cd6e0f1b9041106db4875aac762209155136e762e6d38150",
			"sd-master-35843c7-bin-win-avx-x64.zip",
			{"stable-diffusion.dll", "ggml.dll"}
		};

		backends[SDCPPBackend::CPU_AVX2] = {
			SDCPPBackend::CPU_AVX2,
			"CPU (AVX2)",
			BASE_URL + "sd-master-35843c7-bin-win-avx2-x64.zip",
			"ccbab010f7edcd25184d39d2ca7e24e0dcb403cca4ebd3bee203cc9df3a6d14a",
			"sd-master-35843c7-bin-win-avx2-x64.zip",
			{"stable-diffusion.dll", "ggml.dll"}
		};

		backends[SDCPPBackend::CPU_AVX512] = {
			SDCPPBackend::CPU_AVX512,
			"CPU (AVX512)",
			BASE_URL + "sd-master-35843c7-bin-win-avx512-x64.zip",
			"a2a7cbb677ea4aa787954511d50558e09601c5789bbe7eb8c5afc94c81937e7f",
			"sd-master-35843c7-bin-win-avx512-x64.zip",
			{"stable-diffusion.dll", "ggml.dll"}
		};

		backends[SDCPPBackend::CUDA] = {
			SDCPPBackend::CUDA,
			"CUDA 12",
			BASE_URL + "sd-master-35843c7-bin-win-cuda12-x64.zip",
			"65073c082163cf21505311d344225c77f52dc7bc57adb778dbd0dad6325d7753",
			"sd-master-35843c7-bin-win-cuda12-x64.zip",
			{"stable-diffusion.dll", "ggml.dll"}
		};

		backends[SDCPPBackend::VULKAN] = {
			SDCPPBackend::VULKAN,
			"Vulkan",
			BASE_URL + "sd-master-35843c7-bin-win-vulkan-x64.zip",
			"89257f380302296490f59fd44ad1b994de16477845d9ff2cf9bfd91714806c6b",
			"sd-master-35843c7-bin-win-vulkan-x64.zip",
			{"stable-diffusion.dll", "ggml.dll"}
		};

#elif __linux__
	// Linux backend
		backends[SDCPPBackend::CPU_AVX2] = {
			SDCPPBackend::CPU_AVX2,
			"CPU (Linux)",
			BASE_URL + "sd-master--bin-Linux-Ubuntu-24.04-x86_64.zip",
			"2356808e1509fa018c14222709031f3fb382d6a649adb93aa7b20c09597ae096",
			"sd-master--bin-Linux-Ubuntu-24.04-x86_64.zip",
			{"libstable-diffusion.so", "libggml.so"}
		};

#elif __APPLE__
	// macOS backend
		backends[SDCPPBackend::METAL] = {
			SDCPPBackend::METAL,
			"Metal (macOS)",
			BASE_URL + "sd-master--bin-Darwin-macOS-15.6.1-arm64.zip",
			"80c574ee71652aecf01065fd064b0b819a6a882713c2603e5a4883b0c9233b74",
			"sd-master--bin-Darwin-macOS-15.6.1-arm64.zip",
			{"libstable-diffusion.dylib", "libggml.dylib"}
		};
#endif
	}

	bool SDCPPBackendManager::Initialize() {
		std::cout << "[SDCPPBackendManager] Initializing..." << std::endl;

		// Check which backends are already available
		for (auto&[type, info] : backends) {
			info.available = IsBackendAvailable(type);
			if (info.available) {
				std::cout << "[SDCPPBackendManager] Found: " << info.name << std::endl;
			}
		}

		// Try to load best available backend
		SDCPPBackend best = SelectBestBackend();

		if (best == SDCPPBackend::UNKNOWN) {
			std::cout << "[SDCPPBackendManager] No backends available, downloading default..." << std::endl;

			// Download default backend (CPU AVX2 for Windows/Linux, Metal for macOS)
#ifdef __APPLE__
			best = SDCPPBackend::METAL;
#else
			best = SDCPPBackend::CPU_AVX2;
#endif

			if (!DownloadBackend(best)) {
				std::cerr << "[SDCPPBackendManager] Failed to download default backend" << std::endl;
				return false;
			}
		}

		return LoadBackend(best);
	}

	SDCPPBackend SDCPPBackendManager::SelectBestBackend() {
		// Priority order: CUDA > Vulkan > Metal > AVX512 > AVX2 > AVX > NoAVX

		std::vector<SDCPPBackend> priority = {
			SDCPPBackend::CUDA,
			SDCPPBackend::VULKAN,
			SDCPPBackend::METAL,
			SDCPPBackend::CPU_AVX512,
			SDCPPBackend::CPU_AVX2,
			SDCPPBackend::CPU_AVX,
			SDCPPBackend::CPU_NOAVX
		};

		for (SDCPPBackend backend : priority) {
			if (backends.count(backend) && backends[backend].available) {
				return backend;
			}
		}

		return SDCPPBackend::UNKNOWN;
	}

	bool SDCPPBackendManager::IsBackendAvailable(SDCPPBackend backend) const {
		auto it = backends.find(backend);
		if (it == backends.end()) return false;

		const BackendInfo& info = it->second;
		std::string backendDir = libsDir + "/" + info.name;

		// Check if all required files exist
		for (const auto& file : info.requiredFiles) {
			std::string filePath = backendDir + "/" + file;
			if (!std::filesystem::exists(filePath)) {
				return false;
			}
		}

		return true;
	}

	bool SDCPPBackendManager::DownloadBackend(SDCPPBackend backend, std::function<void(int, int)> progressCallback) {
		auto it = backends.find(backend);
		if (it == backends.end()) {
			std::cerr << "[SDCPPBackendManager] Unknown backend" << std::endl;
			return false;
		}

		BackendInfo& info = it->second;
		std::cout << "[SDCPPBackendManager] Downloading " << info.name << "..." << std::endl;

		std::string zipPath = libsDir + "/" + info.zipFilename;

		// Download ZIP
		if (!DownloadFile(info.downloadUrl, zipPath, progressCallback)) {
			std::cerr << "[SDCPPBackendManager] Download failed" << std::endl;
			return false;
		}

		// Verify SHA256
		std::cout << "[SDCPPBackendManager] Verifying checksum..." << std::endl;
		if (!VerifySHA256(zipPath, info.sha256)) {
			std::cerr << "[SDCPPBackendManager] Checksum verification failed!" << std::endl;
			std::filesystem::remove(zipPath);
			return false;
		}

		// Extract ZIP
		std::string extractPath = libsDir + "/" + info.name;
		std::cout << "[SDCPPBackendManager] Extracting to: " << extractPath << std::endl;

		if (!ExtractZip(zipPath, extractPath)) {
			std::cerr << "[SDCPPBackendManager] Extraction failed" << std::endl;
			return false;
		}

		// Clean up ZIP
		std::filesystem::remove(zipPath);

		info.available = true;
		info.downloaded = true;

		std::cout << "[SDCPPBackendManager] Successfully downloaded " << info.name << std::endl;
		return true;
	}

	bool SDCPPBackendManager::LoadBackend(SDCPPBackend backend) {
		if (currentBackend != SDCPPBackend::UNKNOWN) {
			UnloadBackend();
		}

		auto it = backends.find(backend);
		if (it == backends.end()) {
			std::cerr << "[SDCPPBackendManager] Unknown backend" << std::endl;
			return false;
		}

		const BackendInfo& info = it->second;

		if (!info.available) {
			std::cerr << "[SDCPPBackendManager] Backend not available: " << info.name << std::endl;
			return false;
		}

		std::string backendDir = libsDir + "/" + info.name;
		std::string libPath = backendDir + "/" + info.requiredFiles[0];  // Load main DLL

		std::cout << "[SDCPPBackendManager] Loading backend: " << info.name << std::endl;
		std::cout << "[SDCPPBackendManager] Library path: " << libPath << std::endl;

		libraryHandle = LoadDynamicLibrary(libPath);
		if (!libraryHandle) {
			std::cerr << "[SDCPPBackendManager] Failed to load library" << std::endl;
			return false;
		}

		currentBackend = backend;
		std::cout << "[SDCPPBackendManager] Successfully loaded " << info.name << std::endl;
		return true;
	}

	void SDCPPBackendManager::UnloadBackend() {
		if (libraryHandle) {
			UnloadDynamicLibrary(libraryHandle);
			libraryHandle = nullptr;
			currentBackend = SDCPPBackend::UNKNOWN;
			std::cout << "[SDCPPBackendManager] Backend unloaded" << std::endl;
		}
	}

	void* SDCPPBackendManager::GetFunction(const std::string& name) {
		if (!libraryHandle) return nullptr;
		return GetSymbol(libraryHandle, name);
	}

	// Helper: Download file with CURL
	static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
		std::ofstream* file = static_cast<std::ofstream*>(userp);
		file->write(static_cast<char*>(contents), size * nmemb);
		return size * nmemb;
	}

	static int ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
		auto callback = static_cast<std::function<void(int, int)>*>(clientp);
		if (callback && dltotal > 0) {
			(*callback)(static_cast<int>(dlnow), static_cast<int>(dltotal));
		}
		return 0;
	}

	bool SDCPPBackendManager::DownloadFile(const std::string& url, const std::string& outputPath,
		std::function<void(int, int)> progressCallback) {
		CURL* curl = curl_easy_init();
		if (!curl) return false;

		std::ofstream file(outputPath, std::ios::binary);
		if (!file.is_open()) {
			curl_easy_cleanup(curl);
			return false;
		}

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

		if (progressCallback) {
			curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
			curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progressCallback);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		}

		CURLcode res = curl_easy_perform(curl);
		file.close();
		curl_easy_cleanup(curl);

		return (res == CURLE_OK);
	}

	bool SDCPPBackendManager::ExtractZip(const std::string& zipPath, const std::string& extractPath) {
		mz_zip_archive zip;
		memset(&zip, 0, sizeof(zip));

		if (!mz_zip_reader_init_file(&zip, zipPath.c_str(), 0)) {
			std::cerr << "[SDCPPBackendManager] Failed to open ZIP" << std::endl;
			return false;
		}

		std::filesystem::create_directories(extractPath);

		int fileCount = mz_zip_reader_get_num_files(&zip);
		for (int i = 0; i < fileCount; i++) {
			mz_zip_archive_file_stat fileStat;
			if (!mz_zip_reader_file_stat(&zip, i, &fileStat)) continue;

			if (mz_zip_reader_is_file_a_directory(&zip, i)) continue;

			std::string filename = fileStat.m_filename;
			std::string outPath = extractPath + "/" + std::filesystem::path(filename).filename().string();

			if (!mz_zip_reader_extract_to_file(&zip, i, outPath.c_str(), 0)) {
				std::cerr << "[SDCPPBackendManager] Failed to extract: " << filename << std::endl;
			}
		}

		mz_zip_reader_end(&zip);
		return true;
	}

	bool SDCPPBackendManager::VerifySHA256(const std::string& filePath, const std::string& expectedHash) {
		// TODO: Implement SHA256 verification using OpenSSL or similar
		// For now, just return true
		std::cout << "[SDCPPBackendManager] Warning: SHA256 verification not implemented" << std::endl;
		return true;
	}

	void* SDCPPBackendManager::LoadDynamicLibrary(const std::string& path) {
#ifdef _WIN32
		return LoadLibraryA(path.c_str());
#else
		return dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
#endif
	}

	void SDCPPBackendManager::UnloadDynamicLibrary(void* handle) {
#ifdef _WIN32
		FreeLibrary((HMODULE)handle);
#else
		dlclose(handle);
#endif
	}

	void* SDCPPBackendManager::GetSymbol(void* handle, const std::string& name) {
#ifdef _WIN32
		return (void*)GetProcAddress((HMODULE)handle, name.c_str());
#else
		return dlsym(handle, name.c_str());
#endif
	}

	std::vector<BackendInfo> SDCPPBackendManager::GetAvailableBackends() const {
		std::vector<BackendInfo> available;
		for (const auto&[type, info] : backends) {
			if (info.available) {
				available.push_back(info);
			}
		}
		return available;
	}

} // namespace DiffusionAddon