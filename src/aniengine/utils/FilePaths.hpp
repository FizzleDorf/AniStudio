#ifndef FILEPATHS_HPP
#define FILEPATHS_HPP

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>

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
	struct FilePaths
	{
		// Application-level paths
		inline static std::string executableDir = "";
		inline static std::string dataPath = "";
		inline static std::string ImguiStatePath = "";
		inline static std::string virtualEnvPath = "";
		inline static std::string defaultScriptsPath = "";
		inline static std::string pluginPath = "";

		// Project-level paths
		inline static std::string lastOpenProjectPath = "";
		inline static std::string defaultProjectPath = "";
		inline static std::string assetsFolderPath = "";
		inline static std::string outputFolderPath = "";  // NEW: Project output folder

		// Model paths - global defaults
		inline static std::string defaultModelRootPath = "";
		inline static std::string checkpointDir = "";
		inline static std::string encoderDir = "";
		inline static std::string clipLPath = "";
		inline static std::string clipGPath = "";
		inline static std::string t5xxlPath = "";
		inline static std::string embedDir = "";
		inline static std::string vaeDir = "";
		inline static std::string unetDir = "";
		inline static std::string loraDir = "";
		inline static std::string controlnetDir = "";
		inline static std::string upscaleDir = "";

		// Flag to track if we've been initialized
		inline static bool initialized = false;

		// Get the directory where the executable is located
		static std::string GetExecutableDirectory() {
			if (!executableDir.empty()) {
				return executableDir;
			}

			std::cout << "[FilePaths] Detecting executable directory..." << std::endl;

			try {
				std::filesystem::path exePath;

#ifdef _WIN32
				char buffer[MAX_PATH];
				DWORD length = GetModuleFileNameA(NULL, buffer, MAX_PATH);
				if (length > 0 && length < MAX_PATH) {
					exePath = std::filesystem::path(buffer);
					std::cout << "[FilePaths] Windows exe path: " << buffer << std::endl;
				}
				else {
					std::cerr << "[FilePaths] GetModuleFileName failed or path too long" << std::endl;
				}
#elif defined(__linux__)
				char buffer[PATH_MAX];
				ssize_t length = readlink("/proc/self/exe", buffer, PATH_MAX - 1);
				if (length > 0) {
					buffer[length] = '\0';
					exePath = std::filesystem::path(buffer);
					std::cout << "[FilePaths] Linux exe path: " << buffer << std::endl;
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
						std::cout << "[FilePaths] macOS exe path: " << resolved << std::endl;
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
					executableDir = exePath.parent_path().string();
					std::cout << "[FilePaths] Executable directory: " << executableDir << std::endl;
				}
				else {
					// Fallback to current working directory
					executableDir = std::filesystem::current_path().string();
					std::cout << "[FilePaths] WARNING: Could not determine executable directory, using CWD: " << executableDir << std::endl;
				}
			}
			catch (const std::exception& e) {
				std::cerr << "[FilePaths] Exception in GetExecutableDirectory: " << e.what() << std::endl;
				executableDir = std::filesystem::current_path().string();
				std::cout << "[FilePaths] Using fallback CWD: " << executableDir << std::endl;
			}

			return executableDir;
		}

		// Safe directory creation with error handling
		static bool SafeCreateDirectories(const std::string& path) {
			try {
				if (path.empty()) {
					std::cerr << "[FilePaths] Cannot create directory: path is empty" << std::endl;
					return false;
				}

				std::error_code ec;
				bool created = std::filesystem::create_directories(path, ec);

				if (ec) {
					std::cerr << "[FilePaths] Failed to create directory '" << path << "': " << ec.message() << std::endl;
					return false;
				}

				if (created) {
					std::cout << "[FilePaths] Created directory: " << path << std::endl;
				}
				else {
					std::cout << "[FilePaths] Directory already exists: " << path << std::endl;
				}

				return true;
			}
			catch (const std::exception& e) {
				std::cerr << "[FilePaths] Exception creating directory '" << path << "': " << e.what() << std::endl;
				return false;
			}
		}

		// Application initialization - should be called at startup
		static void Init()
		{
			if (initialized) {
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

				// Set default paths relative to build directory
				dataPath = (basePath / "data" / "defaults").string();
				ImguiStatePath = (basePath / "data" / "defaults" / "imgui.ini").string();
				virtualEnvPath = (basePath / "venv").string();
				defaultScriptsPath = (basePath / "scripts").string();
				pluginPath = (basePath / "plugins").string();

				std::cout << "[FilePaths] Set data path: " << dataPath << std::endl;

				// Try to load saved paths (this might override some defaults)
				std::cout << "[FilePaths] Loading saved paths..." << std::endl;
				LoadFilePathDefaults();

				// Initialize default project directory if not set
				if (defaultProjectPath.empty())
				{
					std::filesystem::path newProjectPath = basePath / "projects";
					defaultProjectPath = std::filesystem::absolute(newProjectPath).string();
					std::cout << "[FilePaths] Set default project path: " << defaultProjectPath << std::endl;
				}

				// Initialize model directory if not set
				if (defaultModelRootPath.empty())
				{
					std::filesystem::path newModelPath = basePath / "models";
					defaultModelRootPath = std::filesystem::absolute(newModelPath).string();
					std::cout << "[FilePaths] Set default model root: " << defaultModelRootPath << std::endl;
				}

				// Create essential directories
				std::cout << "[FilePaths] Creating essential directories..." << std::endl;
				SafeCreateDirectories(dataPath);
				SafeCreateDirectories(defaultProjectPath);
				SafeCreateDirectories(defaultModelRootPath);
				SafeCreateDirectories(defaultScriptsPath);
				SafeCreateDirectories(pluginPath);

				// Set up model subdirectories
				std::cout << "[FilePaths] Setting up model directories..." << std::endl;
				SetByModelRoot();

				// Save initialized paths
				std::cout << "[FilePaths] Saving initialized paths..." << std::endl;
				SaveFilepathDefaults();

				initialized = true;
				std::cout << "[FilePaths] Initialization complete successfully!" << std::endl;
				std::cout << "[FilePaths] ==========================" << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "[FilePaths] FATAL EXCEPTION during initialization: " << e.what() << std::endl;
				std::cerr << "[FilePaths] ==========================" << std::endl;
			}
		}

		// Save all current paths to JSON
		static void SaveFilepathDefaults()
		{
			try {
				std::cout << "[FilePaths] Saving paths..." << std::endl;

				// Create the data directory if it does not exist
				if (!SafeCreateDirectories(dataPath)) {
					std::cerr << "[FilePaths] Failed to create data directory, cannot save paths" << std::endl;
					return;
				}

				// Create a JSON object to store paths
				nlohmann::json json;

				// Application paths
				json["executableDir"] = executableDir;
				json["virtualEnvPath"] = virtualEnvPath;
				json["defaultScriptsPath"] = defaultScriptsPath;
				json["pluginPath"] = pluginPath;

				// Project paths
				json["lastOpenProjectPath"] = lastOpenProjectPath;
				json["defaultProjectPath"] = defaultProjectPath;
				json["assetsFolderPath"] = assetsFolderPath;
				json["outputFolderPath"] = outputFolderPath;  // NEW: Save output folder path

				// Model paths
				json["defaultModelRootPath"] = defaultModelRootPath;
				json["checkpointDir"] = checkpointDir;
				json["encoderDir"] = encoderDir;
				json["clipLPath"] = clipLPath;
				json["clipGPath"] = clipGPath;
				json["t5xxlPath"] = t5xxlPath;
				json["embedDir"] = embedDir;
				json["vaeDir"] = vaeDir;
				json["unetDir"] = unetDir;
				json["loraDir"] = loraDir;
				json["controlnetDir"] = controlnetDir;
				json["upscaleDir"] = upscaleDir;

				// Write JSON to file
				std::string pathsFile = dataPath + "/paths.json";
				std::ofstream file(pathsFile);
				if (file.is_open())
				{
					file << json.dump(4); // Pretty print with 4 spaces
					file.close();
					std::cout << "[FilePaths] Saved paths to: " << pathsFile << std::endl;
				}
				else {
					std::cerr << "[FilePaths] Failed to open paths file for writing: " << pathsFile << std::endl;
				}
			}
			catch (const std::exception& e) {
				std::cerr << "[FilePaths] Exception saving paths: " << e.what() << std::endl;
			}
		}

		// Load paths from JSON
		static void LoadFilePathDefaults()
		{
			try {
				// Open JSON file
				std::string pathsFile = dataPath + "/paths.json";

				if (!std::filesystem::exists(pathsFile)) {
					std::cout << "[FilePaths] No existing paths file found: " << pathsFile << std::endl;
					return;
				}

				std::ifstream file(pathsFile);
				if (!file.is_open())
				{
					std::cout << "[FilePaths] Could not open paths file: " << pathsFile << std::endl;
					return;
				}

				// Parse the JSON file
				nlohmann::json json;
				file >> json;
				file.close();

				std::cout << "[FilePaths] Loading saved paths from: " << pathsFile << std::endl;

				// Update paths from JSON (but preserve executableDir)
				if (json.contains("virtualEnvPath") && json["virtualEnvPath"].is_string())
					virtualEnvPath = json["virtualEnvPath"];
				if (json.contains("defaultScriptsPath") && json["defaultScriptsPath"].is_string())
					defaultScriptsPath = json["defaultScriptsPath"];
				if (json.contains("pluginPath") && json["pluginPath"].is_string())
					pluginPath = json["pluginPath"];
				if (json.contains("lastOpenProjectPath") && json["lastOpenProjectPath"].is_string())
					lastOpenProjectPath = json["lastOpenProjectPath"];
				if (json.contains("defaultProjectPath") && json["defaultProjectPath"].is_string())
					defaultProjectPath = json["defaultProjectPath"];
				if (json.contains("defaultModelRootPath") && json["defaultModelRootPath"].is_string())
					defaultModelRootPath = json["defaultModelRootPath"];
				if (json.contains("assetsFolderPath") && json["assetsFolderPath"].is_string())
					assetsFolderPath = json["assetsFolderPath"];
				if (json.contains("outputFolderPath") && json["outputFolderPath"].is_string())  // NEW: Load output folder path
					outputFolderPath = json["outputFolderPath"];
				if (json.contains("checkpointDir") && json["checkpointDir"].is_string())
					checkpointDir = json["checkpointDir"];
				if (json.contains("encoderDir") && json["encoderDir"].is_string())
					encoderDir = json["encoderDir"];
				if (json.contains("clipLPath") && json["clipLPath"].is_string())
					clipLPath = json["clipLPath"];
				if (json.contains("clipGPath") && json["clipGPath"].is_string())
					clipGPath = json["clipGPath"];
				if (json.contains("t5xxlPath") && json["t5xxlPath"].is_string())
					t5xxlPath = json["t5xxlPath"];
				if (json.contains("embedDir") && json["embedDir"].is_string())
					embedDir = json["embedDir"];
				if (json.contains("vaeDir") && json["vaeDir"].is_string())
					vaeDir = json["vaeDir"];
				if (json.contains("unetDir") && json["unetDir"].is_string())
					unetDir = json["unetDir"];
				if (json.contains("loraDir") && json["loraDir"].is_string())
					loraDir = json["loraDir"];
				if (json.contains("controlnetDir") && json["controlnetDir"].is_string())
					controlnetDir = json["controlnetDir"];
				if (json.contains("upscaleDir") && json["upscaleDir"].is_string())
					upscaleDir = json["upscaleDir"];

				std::cout << "[FilePaths] Successfully loaded saved paths" << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "[FilePaths] Exception loading paths: " << e.what() << std::endl;
			}
		}

		// Set up model subdirectories based on root path
		static void SetByModelRoot()
		{
			if (defaultModelRootPath.empty()) {
				std::cerr << "[FilePaths] Cannot set model paths: root path is empty" << std::endl;
				return;
			}

			try {
				std::cout << "[FilePaths] Setting up model subdirectories..." << std::endl;

				// Create all model subdirectories
				checkpointDir = (std::filesystem::path(defaultModelRootPath) / "checkpoints").string();
				SafeCreateDirectories(checkpointDir);

				encoderDir = (std::filesystem::path(defaultModelRootPath) / "clip").string();
				SafeCreateDirectories(encoderDir);

				vaeDir = (std::filesystem::path(defaultModelRootPath) / "vae").string();
				SafeCreateDirectories(vaeDir);

				unetDir = (std::filesystem::path(defaultModelRootPath) / "unet").string();
				SafeCreateDirectories(unetDir);

				loraDir = (std::filesystem::path(defaultModelRootPath) / "loras").string();
				SafeCreateDirectories(loraDir);

				controlnetDir = (std::filesystem::path(defaultModelRootPath) / "controlnet").string();
				SafeCreateDirectories(controlnetDir);

				upscaleDir = (std::filesystem::path(defaultModelRootPath) / "upscale_models").string();
				SafeCreateDirectories(upscaleDir);

				embedDir = (std::filesystem::path(defaultModelRootPath) / "embeddings").string();
				SafeCreateDirectories(embedDir);

				// Set up CLIP specific paths (these are file paths, not directories)
				clipLPath = (std::filesystem::path(encoderDir) / "clip_l.safetensors").string();
				clipGPath = (std::filesystem::path(encoderDir) / "clip_g.safetensors").string();
				t5xxlPath = (std::filesystem::path(encoderDir) / "t5xxl.safetensors").string();

				std::cout << "[FilePaths] Model directories set up successfully" << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "[FilePaths] Exception setting up model directories: " << e.what() << std::endl;
			}
		}

		// Utility functions (same as before but with safer error handling)
		static bool IsProjectPath(const std::string& path) {
			try {
				return !path.empty() && std::filesystem::exists(path + "/project.ani");
			}
			catch (const std::exception& e) {
				std::cerr << "[FilePaths] Exception checking project path: " << e.what() << std::endl;
				return false;
			}
		}

		static std::string GetProjectName(const std::string& projectPath) {
			try {
				std::string projectFile = projectPath + "/project.ani";
				if (!std::filesystem::exists(projectFile)) return "";

				std::ifstream file(projectFile);
				if (!file.is_open()) return "";

				nlohmann::json projectData;
				file >> projectData;
				file.close();

				if (projectData.contains("settings") &&
					projectData["settings"].contains("projectName")) {
					return projectData["settings"]["projectName"];
				}
			}
			catch (const std::exception& e) {
				std::cerr << "[FilePaths] Exception reading project name: " << e.what() << std::endl;
			}
			return "";
		}

		// Debug output
		static void PrintCurrentPaths() {
			std::cout << "[FilePaths] ==================== Current Configuration ====================" << std::endl;
			std::cout << "[FilePaths] Initialized: " << (initialized ? "YES" : "NO") << std::endl;
			std::cout << "[FilePaths] Executable Dir: " << executableDir << std::endl;
			std::cout << "[FilePaths] Data Path: " << dataPath << std::endl;
			std::cout << "[FilePaths] Last Open Project: " << lastOpenProjectPath << std::endl;
			std::cout << "[FilePaths] Default Project: " << defaultProjectPath << std::endl;
			std::cout << "[FilePaths] Assets Folder: " << assetsFolderPath << std::endl;
			std::cout << "[FilePaths] Output Folder: " << outputFolderPath << std::endl;  // NEW: Show output folder
			std::cout << "[FilePaths] Model Root: " << defaultModelRootPath << std::endl;
			std::cout << "[FilePaths] Scripts: " << defaultScriptsPath << std::endl;
			std::cout << "[FilePaths] Plugins: " << pluginPath << std::endl;
			std::cout << "[FilePaths] Virtual Env: " << virtualEnvPath << std::endl;
			std::cout << "[FilePaths] =============================================================" << std::endl;
		}
	};
} // namespace Utils
#endif // FILEPATHS_HPP