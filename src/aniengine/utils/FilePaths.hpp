#ifndef FILEPATHS_HPP
#define FILEPATHS_HPP

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>
#include <unordered_map>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#include <climits>
#elif defined(__APPLE__)
#include <unistd.h>
#include <climits>
#include <mach-o/dyld.h>
#endif

namespace Utils
{
	class FilePaths
	{
	private:
		std::unordered_map<std::string, std::string> m_pathMap;  // CHANGED: Use std::string as key
		std::string m_executableDir;
		std::string m_dataPath;
		bool m_initialized = false;

		// Get the directory where the executable is located
		std::string GetExecutableDirectory() {
			if (!m_executableDir.empty()) {
				return m_executableDir;
			}

			std::cout << "[FilePaths] Detecting executable directory..." << std::endl;

			try {
				std::filesystem::path exePath;

#ifdef _WIN32
				char buffer[MAX_PATH];
				DWORD length = GetModuleFileNameA(NULL, buffer, MAX_PATH);
				if (length > 0 && length < MAX_PATH) {
					exePath = std::filesystem::path(buffer);
				}
				else {
					std::cerr << "[FilePaths] GetModuleFileName failed" << std::endl;
				}
#elif defined(__linux__)
				char buffer[PATH_MAX];
				ssize_t length = readlink("/proc/self/exe", buffer, PATH_MAX - 1);
				if (length > 0) {
					buffer[length] = '\0';
					exePath = std::filesystem::path(buffer);
				}
				else {
					std::cerr << "[FilePaths] readlink failed" << std::endl;
				}
#elif defined(__APPLE__)
				char buffer[PATH_MAX];
				uint32_t size = sizeof(buffer);
				if (_NSGetExecutablePath(buffer, &size) == 0) {
					char* resolved = realpath(buffer, nullptr);
					if (resolved) {
						exePath = std::filesystem::path(resolved);
						free(resolved);
					}
					else {
						std::cerr << "[FilePaths] realpath failed" << std::endl;
					}
				}
				else {
					std::cerr << "[FilePaths] _NSGetExecutablePath failed" << std::endl;
				}
#endif

				if (!exePath.empty()) {
					m_executableDir = exePath.parent_path().string();
					std::cout << "[FilePaths] Executable directory: " << m_executableDir << std::endl;
				}
				else {
					m_executableDir = std::filesystem::current_path().string();
					std::cout << "[FilePaths] Using CWD: " << m_executableDir << std::endl;
				}
			}
			catch (const std::exception& e) {
				std::cerr << "[FilePaths] Exception in GetExecutableDirectory: " << e.what() << std::endl;
				m_executableDir = std::filesystem::current_path().string();
			}

			return m_executableDir;
		}

		// Safe directory creation with error handling
		bool SafeCreateDirectories(const std::string& path) {
			try {
				if (path.empty()) {
					return false;
				}

				std::error_code ec;
				bool created = std::filesystem::create_directories(path, ec);

				if (ec) {
					std::cerr << "[FilePaths] Failed to create directory '" << path << "': " << ec.message() << std::endl;
					return false;
				}

				return true;
			}
			catch (const std::exception& e) {
				std::cerr << "[FilePaths] Exception creating directory: " << e.what() << std::endl;
				return false;
			}
		}

		// Get path from map with fallback
		std::string GetMapPath(const std::string& key) const {
			auto it = m_pathMap.find(key);
			if (it != m_pathMap.end() && !it->second.empty()) {
				return it->second;
			}
			return "";
		}

		// Set path in map
		void SetMapPath(const std::string& key, const std::string& path) {
			m_pathMap[key] = path;
		}

		// Check if path exists in map
		bool HasMapPath(const std::string& key) const {
			auto it = m_pathMap.find(key);
			return it != m_pathMap.end() && !it->second.empty();
		}

	public:
		FilePaths() = default;

		FilePaths(const std::string& customDataPath) : m_dataPath(customDataPath) {
		}

		~FilePaths() = default;

		FilePaths(const FilePaths&) = delete;
		FilePaths& operator=(const FilePaths&) = delete;

		FilePaths(FilePaths&& other) = default;
		FilePaths& operator=(FilePaths&& other) = default;

		// Application initialization - should be called at startup
		void Init()
		{
			if (m_initialized) {
				std::cout << "[FilePaths] Already initialized, skipping..." << std::endl;
				return;
			}

			std::cout << "[FilePaths] ==========================" << std::endl;
			std::cout << "[FilePaths] Initializing application paths..." << std::endl;

			try {
				// First, determine where the executable is
				std::string exeDir = GetExecutableDirectory();
				if (exeDir.empty()) {
					std::cerr << "[FilePaths] FATAL: Could not determine executable directory!" << std::endl;
					return;
				}

				// For build/bin/AniStudio.exe, we want build/ as our base
				std::filesystem::path basePath = std::filesystem::path(exeDir).parent_path();
				std::cout << "[FilePaths] Base directory: " << basePath.string() << std::endl;

				// Set data path if not already set
				if (m_dataPath.empty()) {
					m_dataPath = (basePath / "data" / "defaults").string();
				}
				std::cout << "[FilePaths] Data path: " << m_dataPath << std::endl;

				// Try to load saved paths FIRST
				std::cout << "[FilePaths] Loading saved paths..." << std::endl;
				LoadFilePathDefaults();

				// **CRITICAL: ALWAYS SET THESE PATHS, NO MATTER WHAT**
				std::cout << "[FilePaths] Setting essential paths..." << std::endl;

				// Application-level paths
				SetMapPath("ImguiState", (basePath / "data" / "defaults" / "imgui.ini").string());
				SetMapPath("VirtualEnv", (basePath / "venv").string());
				SetMapPath("Scripts", (basePath / "scripts").string());
				SetMapPath("Plugins", (basePath / "plugins").string());

				// Project-level paths - THESE ARE THE ONES THAT MATTER
				std::string defaultProjectPath = (basePath / "projects").string();
				// Only set DefaultProject if it's not already loaded from JSON
				if (!HasMapPath("DefaultProject")) {
					SetMapPath("DefaultProject", defaultProjectPath);
					std::cout << "[FilePaths] SET DefaultProject path to: " << defaultProjectPath << std::endl;
				}
				else {
					std::cout << "[FilePaths] Using saved DefaultProject: " << GetMapPath("DefaultProject") << std::endl;
				}

				// Only set LastOpenProject if it's not already loaded from JSON
				if (!HasMapPath("LastOpenProject")) {
					SetMapPath("LastOpenProject", "");
				}

				// Only set AssetsFolder and OutputFolder if not loaded from JSON
				if (!HasMapPath("AssetsFolder")) {
					SetMapPath("AssetsFolder", "");
				}
				if (!HasMapPath("OutputFolder")) {
					SetMapPath("OutputFolder", "");
				}
				if (!HasMapPath("ProjectDataPath")) {
					SetMapPath("ProjectDataPath", "");
				}

				// Model paths - only set ModelRoot if not already loaded from JSON
				if (!HasMapPath("ModelRoot")) {
					std::string modelRoot = (basePath / "models").string();
					SetMapPath("ModelRoot", modelRoot);
				}

				// Set up model subdirectories if ModelRoot exists
				std::string modelRoot = GetMapPath("ModelRoot");
				if (!modelRoot.empty()) {
					std::cout << "[FilePaths] Setting up model directories from ModelRoot: " << modelRoot << std::endl;
					SetByModelRoot();
				}
				else {
					std::cout << "[FilePaths] ModelRoot not set, skipping model directories setup" << std::endl;
				}

				// Create essential directories
				std::cout << "[FilePaths] Creating essential directories..." << std::endl;
				SafeCreateDirectories(m_dataPath);
				SafeCreateDirectories(GetMapPath("DefaultProject"));
				SafeCreateDirectories(GetMapPath("ModelRoot"));
				SafeCreateDirectories(GetMapPath("Scripts"));
				SafeCreateDirectories(GetMapPath("Plugins"));

				// Save initialized paths
				std::cout << "[FilePaths] Saving paths to file..." << std::endl;
				SaveFilepathDefaults();

				m_initialized = true;
				std::cout << "[FilePaths] Initialization complete!" << std::endl;
				std::cout << "[FilePaths] DefaultProject: " << GetMapPath("DefaultProject") << std::endl;
				std::cout << "[FilePaths] ModelRoot: " << GetMapPath("ModelRoot") << std::endl;
				std::cout << "[FilePaths] ==========================" << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "[FilePaths] FATAL EXCEPTION during initialization: " << e.what() << std::endl;
			}
		}

		// Save all current paths to JSON
		void SaveFilepathDefaults()
		{
			try {
				std::cout << "[FilePaths] Saving paths..." << std::endl;

				if (!SafeCreateDirectories(m_dataPath)) {
					std::cerr << "[FilePaths] Failed to create data directory" << std::endl;
					return;
				}

				nlohmann::json json;

				// Save all paths from the map
				for (const auto&[key, path] : m_pathMap) {
					json[key] = path;
				}

				json["ExecutableDir"] = m_executableDir;
				json["DataPath"] = m_dataPath;

				std::string pathsFile = m_dataPath + "/paths.json";
				std::ofstream file(pathsFile);
				if (file.is_open())
				{
					file << json.dump(4);
					file.close();
					std::cout << "[FilePaths] Saved paths to: " << pathsFile << std::endl;
				}
				else {
					std::cerr << "[FilePaths] Failed to open paths file: " << pathsFile << std::endl;
				}
			}
			catch (const std::exception& e) {
				std::cerr << "[FilePaths] Exception saving paths: " << e.what() << std::endl;
			}
		}

		// Load paths from JSON
		void LoadFilePathDefaults()
		{
			try {
				std::string pathsFile = m_dataPath + "/paths.json";

				if (!std::filesystem::exists(pathsFile)) {
					std::cout << "[FilePaths] No existing paths file, will create defaults" << std::endl;
					return;
				}

				std::ifstream file(pathsFile);
				if (!file.is_open())
				{
					std::cout << "[FilePaths] Could not open paths file" << std::endl;
					return;
				}

				nlohmann::json json;
				file >> json;
				file.close();

				std::cout << "[FilePaths] Loading saved paths from: " << pathsFile << std::endl;

				// Clear existing map first to avoid duplicates
				m_pathMap.clear();

				for (auto&[key, value] : json.items()) {
					if (value.is_string()) {
						std::string val = value.get<std::string>();
						if (key == "ExecutableDir") {
							m_executableDir = val;
							std::cout << "[FilePaths] Loaded ExecutableDir: " << val << std::endl;
						}
						else if (key == "DataPath") {
							m_dataPath = val;
							std::cout << "[FilePaths] Loaded DataPath: " << val << std::endl;
						}
						else {
							m_pathMap[key] = val;
							std::cout << "[FilePaths] Loaded " << key << ": " << val << std::endl;
						}
					}
				}

				std::cout << "[FilePaths] Loaded " << m_pathMap.size() << " saved paths" << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "[FilePaths] Exception loading paths: " << e.what() << std::endl;
			}
		}

		// Set up model subdirectories based on root path
		void SetByModelRoot() {
			std::string modelRoot = GetMapPath("ModelRoot");
			if (modelRoot.empty()) {
				std::cerr << "[FilePaths] ModelRoot path is empty, cannot set up model directories" << std::endl;
				return;
			}

			try {
				std::cout << "[FilePaths] Setting up model subdirectories from ModelRoot: " << modelRoot << std::endl;

				// Create base directories
				std::filesystem::path rootPath(modelRoot);

				// Model types from ModelComponents.hpp
				std::string checkpointDir = (rootPath / "checkpoints").string();
				SafeCreateDirectories(checkpointDir);
				SetMapPath("Checkpoint", checkpointDir);

				std::string unetDir = (rootPath / "unet").string();
				SafeCreateDirectories(unetDir);
				SetMapPath("Unet", unetDir);
				SetMapPath("DiffusionModel", unetDir);  // For DiffusionModelComponent
				SetMapPath("HighNoiseDiffusionModel", unetDir);  // For HighNoiseDiffusionModelComponent

				// Encoders from EncoderModelComponents.hpp
				std::string encoderDir = (rootPath / "clip").string();
				SafeCreateDirectories(encoderDir);
				SetMapPath("Encoder", encoderDir);
				SetMapPath("ClipL", encoderDir);  // Directory, not file!
				SetMapPath("ClipG", encoderDir);  // Directory, not file!
				SetMapPath("T5XXL", encoderDir);  // Directory, not file!
				SetMapPath("LLM", encoderDir);    // For LlmComponent
				SetMapPath("Llm", encoderDir);    // Alternative

				// Vision encoders from VisionModelComponents.hpp
				SetMapPath("ClipVision", encoderDir);  // Uses same encoder directory
				SetMapPath("LlmVision", encoderDir);   // Uses same encoder directory

				// AutoEncoders from AutoEncoderComponent.hpp
				std::string vaeDir = (rootPath / "vae").string();
				SafeCreateDirectories(vaeDir);
				SetMapPath("Vae", vaeDir);
				SetMapPath("Taesd", vaeDir);  // For TaesdComponent

				// Other models from OtherModelComponents.hpp
				std::string loraDir = (rootPath / "loras").string();
				SafeCreateDirectories(loraDir);
				SetMapPath("Lora", loraDir);

				std::string controlnetDir = (rootPath / "controlnet").string();
				SafeCreateDirectories(controlnetDir);
				SetMapPath("ControlNet", controlnetDir);
				SetMapPath("Controlnet", controlnetDir);  // Lowercase for ControlNetComponent

				std::string upscaleDir = (rootPath / "upscale_models").string();
				SafeCreateDirectories(upscaleDir);
				SetMapPath("Upscale", upscaleDir);
				SetMapPath("Esrgan", upscaleDir);  // For EsrganComponent

				std::string embedDir = (rootPath / "embeddings").string();
				SafeCreateDirectories(embedDir);
				SetMapPath("Embed", embedDir);
				SetMapPath("Embedding", embedDir);  // For EmbeddingComponent

				// PhotoMaker from OtherModelComponents.hpp
				std::string photoMakerDir = (rootPath / "photomaker").string();
				SafeCreateDirectories(photoMakerDir);
				SetMapPath("PhotoMaker", photoMakerDir);

				std::cout << "[FilePaths] Model directories set up" << std::endl;
				std::cout << "[FilePaths] Registered paths:" << std::endl;
				std::cout << "[FilePaths]   Checkpoint: " << checkpointDir << std::endl;
				std::cout << "[FilePaths]   Unet/DiffusionModel: " << unetDir << std::endl;
				std::cout << "[FilePaths]   Encoder: " << encoderDir << std::endl;
				std::cout << "[FilePaths]   Vae: " << vaeDir << std::endl;
				std::cout << "[FilePaths]   ControlNet: " << controlnetDir << std::endl;
				std::cout << "[FilePaths]   Upscale/Esrgan: " << upscaleDir << std::endl;
				std::cout << "[FilePaths]   Embed/Embedding: " << embedDir << std::endl;
				std::cout << "[FilePaths]   PhotoMaker: " << photoMakerDir << std::endl;
				std::cout << "[FilePaths]   Lora: " << loraDir << std::endl;

				// Save after setting up model directories
				SaveFilepathDefaults();
			}
			catch (const std::exception& e) {
				std::cerr << "[FilePaths] Exception setting up model directories: " << e.what() << std::endl;
			}
		}

		// Force update model directories from current ModelRoot
		void UpdateModelDirectories() {
			std::cout << "[FilePaths] Updating model directories..." << std::endl;
			SetByModelRoot();
		}

		// Get path by name
		const char* GetPath(const char* key) const {
			if (!key) return "";

			std::string keyStr(key);
			auto it = m_pathMap.find(keyStr);
			if (it != m_pathMap.end()) {
				return it->second.c_str();
			}
			return "";
		}

		// Set path by name
		void SetPath(const char* key, const char* path) {
			if (!key || !path) return;

			std::string keyStr(key);
			std::string pathStr(path);

			// UPDATE THE MEMORY MAP DIRECTLY
			m_pathMap[keyStr] = pathStr;

			// If setting ModelRoot, update all model directories
			if (keyStr == "ModelRoot") {
				UpdateModelDirectories();
			}

			// Auto-save when paths are changed (if initialized)
			if (m_initialized) {
				SaveFilepathDefaults();
				std::cout << "[FilePaths] Path updated and saved: " << keyStr << " = " << pathStr << std::endl;
			}
		}

		// Utility functions
		bool IsProjectPath(const char* path) {
			try {
				return path && path[0] != '\0' && std::filesystem::exists(std::string(path) + "/project.ani");
			}
			catch (const std::exception& e) {
				std::cerr << "[FilePaths] Exception checking project path: " << e.what() << std::endl;
				return false;
			}
		}

		const char* GetProjectName(const char* projectPath) {
			try {
				std::string projectFile = std::string(projectPath) + "/project.ani";
				if (!std::filesystem::exists(projectFile)) return "";

				std::ifstream file(projectFile);
				if (!file.is_open()) return "";

				nlohmann::json projectData;
				file >> projectData;
				file.close();

				if (projectData.contains("settings") &&
					projectData["settings"].contains("projectName")) {
					static std::string result;
					result = projectData["settings"]["projectName"];
					return result.c_str();
				}
			}
			catch (const std::exception& e) {
				std::cerr << "[FilePaths] Exception reading project name: " << e.what() << std::endl;
			}
			return "";
		}

		// Debug output
		void PrintCurrentPaths() {
			std::cout << "[FilePaths] ==================== Current Configuration ====================" << std::endl;
			std::cout << "[FilePaths] Initialized: " << (m_initialized ? "YES" : "NO") << std::endl;
			std::cout << "[FilePaths] Executable Dir: " << m_executableDir << std::endl;
			std::cout << "[FilePaths] Data Path: " << m_dataPath << std::endl;

			for (const auto&[key, path] : m_pathMap) {
				std::cout << "[FilePaths] " << key << ": " << path << std::endl;
			}

			std::cout << "[FilePaths] =============================================================" << std::endl;
		}

		// Getters for internal state
		bool IsInitialized() const { return m_initialized; }
		const char* GetExecutableDir() const { return m_executableDir.c_str(); }
		const char* GetDataPath() const { return m_dataPath.c_str(); }

		// Check if a specific key exists
		bool HasPathKey(const std::string& key) const {
			return m_pathMap.find(key) != m_pathMap.end() && !m_pathMap.at(key).empty();
		}
	};
} // namespace Utils

#endif // FILEPATHS_HPP