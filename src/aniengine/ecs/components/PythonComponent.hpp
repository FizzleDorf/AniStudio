#pragma once

#include "BaseComponent.hpp"
#include "FilePathService.hpp"  // Changed from FilePaths.hpp to FilePathService.hpp
#include <string>
#include <filesystem>

namespace ECS {
	struct PythonComponent : public BaseComponent {
		std::string script;
		std::string filePath;
		bool execute = false;
		bool isFile = false;

		// Virtual environment support
		bool useVirtualEnv = false;
		std::string virtualEnvPath = "";  // Initialize empty - will use service
		std::string pythonExecutable;
		std::string sitePackagesPath;

		// Results
		std::string error;
		std::string output;

		PythonComponent() {
			compName = "Python";

			// Initialize virtual environment path from service
			if (Utils::FilePathService::IsInitialized()) {
				std::string venvPath = Utils::FilePathService::GetPath("VirtualEnv");
				if (!venvPath.empty()) {
					virtualEnvPath = venvPath;
				}
			}

			UpdateVirtualEnvPaths();
		}

		virtual ~PythonComponent() = default;

		void UpdateVirtualEnvPaths() {
			// If virtualEnvPath is empty, try to get it from the service
			if (virtualEnvPath.empty() && Utils::FilePathService::IsInitialized()) {
				std::string venvPath = Utils::FilePathService::GetPath("VirtualEnv");
				if (!venvPath.empty()) {
					virtualEnvPath = venvPath;
				}
			}

			// Only set up paths if we have a valid virtual environment path
			if (!virtualEnvPath.empty()) {
				std::filesystem::path venvPath(virtualEnvPath);

#ifdef _WIN32
				pythonExecutable = (venvPath / "Scripts" / "python.exe").string();
				sitePackagesPath = (venvPath / "Lib" / "site-packages").string();
#else
				pythonExecutable = (venvPath / "bin" / "python").string();
				sitePackagesPath = (venvPath / "lib" / "python3.11" / "site-packages").string();
#endif
			}
			else {
				// Clear paths if no virtual environment
				pythonExecutable.clear();
				sitePackagesPath.clear();
			}
		}

		// Get property map for UI rendering
		virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			std::unordered_map<std::string, UISchema::PropertyVariant> properties;
			properties["script"] = &script;
			properties["filePath"] = &filePath;
			properties["execute"] = &execute;
			properties["isFile"] = &isFile;
			properties["useVirtualEnv"] = &useVirtualEnv;
			properties["virtualEnvPath"] = &virtualEnvPath;
			properties["error"] = &error;
			properties["output"] = &output;
			return properties;
		}

		// Serialize the component to JSON
		virtual nlohmann::json Serialize() const override {
			nlohmann::json j;
			j["compName"] = compName;
			j[compName] = {
				{"script", script},
				{"filePath", filePath},
				{"execute", execute},
				{"isFile", isFile},
				{"useVirtualEnv", useVirtualEnv},
				{"virtualEnvPath", virtualEnvPath},
				{"error", error},
				{"output", output}
			};
			return j;
		}

		// Deserialize the component from JSON
		virtual void Deserialize(const nlohmann::json& j) override {
			BaseComponent::Deserialize(j);

			nlohmann::json componentData;
			if (j.contains(compName)) {
				componentData = j.at(compName);
			}
			else {
				componentData = j;
			}

			if (componentData.contains("script"))
				script = componentData["script"];
			if (componentData.contains("filePath"))
				filePath = componentData["filePath"];
			if (componentData.contains("execute"))
				execute = componentData["execute"];
			if (componentData.contains("isFile"))
				isFile = componentData["isFile"];
			if (componentData.contains("useVirtualEnv"))
				useVirtualEnv = componentData["useVirtualEnv"];
			if (componentData.contains("virtualEnvPath")) {
				virtualEnvPath = componentData["virtualEnvPath"];
				UpdateVirtualEnvPaths();
			}
			if (componentData.contains("error"))
				error = componentData["error"];
			if (componentData.contains("output"))
				output = componentData["output"];
		}
	};
}