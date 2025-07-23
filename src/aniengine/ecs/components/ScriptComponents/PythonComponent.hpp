/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license iformation, please contact legal@kframe.ai.
 */

#pragma once

#include "BaseComponent.hpp"
#include "FilePaths.hpp"
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
		std::string virtualEnvPath = Utils::FilePaths::virtualEnvPath;  // Use FilePaths default
		std::string pythonExecutable;
		std::string sitePackagesPath;

		// Results
		std::string error;
		std::string output;

		PythonComponent() {
			compName = "Python";
			UpdateVirtualEnvPaths();
		}

		virtual ~PythonComponent() = default;

		void UpdateVirtualEnvPaths() {
			if (virtualEnvPath.empty()) {
				virtualEnvPath = Utils::FilePaths::virtualEnvPath;
			}

			std::filesystem::path venvPath(virtualEnvPath);

#ifdef _WIN32
			pythonExecutable = (venvPath / "Scripts" / "python.exe").string();
			sitePackagesPath = (venvPath / "Lib" / "site-packages").string();
#else
			pythonExecutable = (venvPath / "bin" / "python").string();
			sitePackagesPath = (venvPath / "lib" / "python3.11" / "site-packages").string();
#endif
		}

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