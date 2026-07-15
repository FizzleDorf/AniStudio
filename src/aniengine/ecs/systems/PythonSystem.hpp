#pragma once

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "ThreadPool.hpp"
#include "PythonComponent.hpp"
#include <string>
#include <mutex>
#include <memory>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <array>
#include <fstream> 

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

namespace ECS {
	class PythonSystem : public BaseSystem {
	public:
		PythonSystem(EntityManager& entityMgr) : BaseSystem(entityMgr) {
			sysName = "PythonSystem";
			AddComponentSignature<PythonComponent>();
		}

		~PythonSystem() override {
			// Cleanup handled automatically
		}

		void Update(float deltaTime) override {
			for (EntityID entity : entities) {
				if (mgr.HasComponent<PythonComponent>(entity)) {
					auto& pythonComp = mgr.GetComponent<PythonComponent>(entity);
					if (pythonComp.execute) {
						pythonComp.execute = false;

						if (pythonComp.isFile) {
							ExecuteScriptFile(pythonComp.filePath, pythonComp);
						}
						else {
							ExecuteScript(pythonComp.script, pythonComp);
						}
					}
				}
			}
		}

	private:
		std::mutex pythonMutex;

		std::string GetPythonExecutable(const PythonComponent& comp) {
			if (comp.useVirtualEnv && !comp.pythonExecutable.empty()) {
				// Convert relative path to absolute path for Windows compatibility
				std::filesystem::path pythonPath(comp.pythonExecutable);
				if (pythonPath.is_relative()) {
					pythonPath = std::filesystem::absolute(pythonPath);
				}

				if (std::filesystem::exists(pythonPath)) {
					return pythonPath.string();
				}
			}
			return "python"; // Fallback to system python
		}

		std::pair<std::string, std::string> RunCommand(const std::string& command) {
			std::string output;
			std::string error;

#ifdef _WIN32
			// Windows implementation
			SECURITY_ATTRIBUTES sa;
			sa.nLength = sizeof(SECURITY_ATTRIBUTES);
			sa.lpSecurityDescriptor = NULL;
			sa.bInheritHandle = TRUE;

			HANDLE hStdoutRead, hStdoutWrite;
			HANDLE hStderrRead, hStderrWrite;

			if (!CreatePipe(&hStdoutRead, &hStdoutWrite, &sa, 0) ||
				!CreatePipe(&hStderrRead, &hStderrWrite, &sa, 0)) {
				return { "", "Failed to create pipes" };
			}

			STARTUPINFOA si;
			PROCESS_INFORMATION pi;
			ZeroMemory(&si, sizeof(si));
			si.cb = sizeof(si);
			si.hStdError = hStderrWrite;
			si.hStdOutput = hStdoutWrite;
			si.dwFlags |= STARTF_USESTDHANDLES;
			ZeroMemory(&pi, sizeof(pi));

			// Use the command directly without cmd /c wrapper since we're already formatting it properly
			std::string cmdLine = command;
			if (CreateProcessA(NULL, const_cast<char*>(cmdLine.c_str()), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
				CloseHandle(hStdoutWrite);
				CloseHandle(hStderrWrite);

				// Read stdout
				DWORD dwRead;
				char buffer[4096];
				while (ReadFile(hStdoutRead, buffer, sizeof(buffer), &dwRead, NULL) && dwRead != 0) {
					output.append(buffer, dwRead);
				}

				// Read stderr
				while (ReadFile(hStderrRead, buffer, sizeof(buffer), &dwRead, NULL) && dwRead != 0) {
					error.append(buffer, dwRead);
				}

				WaitForSingleObject(pi.hProcess, INFINITE);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
			}
			else {
				error = "Failed to start process";
			}

			CloseHandle(hStdoutRead);
			CloseHandle(hStderrRead);

#else
			// Unix/Linux implementation using popen
			FILE* pipe = popen((command + " 2>&1").c_str(), "r");
			if (pipe) {
				char buffer[128];
				while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
					output += buffer;
				}
				int result = pclose(pipe);
				if (result != 0) {
					error = "Process exited with code: " + std::to_string(result);
				}
			}
			else {
				error = "Failed to start process";
			}
#endif

			return { output, error };
		}

		std::string CreateTempScriptFile(const std::string& script) {
			
			std::filesystem::path tempDir = std::filesystem::temp_directory_path();
			std::filesystem::path tempFile = tempDir / "anistudio_temp_script.py";
			return tempFile.string();
		}

		void ExecuteScript(const std::string& script, PythonComponent& comp) {
			std::lock_guard<std::mutex> lock(pythonMutex);

			try {
				comp.error.clear();
				comp.output.clear();

				std::string pythonExe = GetPythonExecutable(comp);

				// Create a temporary file for the script
				std::string tempFile = CreateTempScriptFile(script);

				std::ofstream scriptFile(tempFile);
				if (!scriptFile.is_open()) {
					comp.error = "Failed to create temporary script file: " + tempFile;
					return;
				}
				scriptFile << script;
				scriptFile.close();

				std::ostringstream commandStream;
				commandStream << "\"" << pythonExe << "\" \"" << tempFile << "\"";
				std::string command = commandStream.str();

				std::cout << "Executing command: " << command << std::endl;

				auto[output, error] = RunCommand(command);

				comp.output = output;
				if (!error.empty()) {
					comp.error = error;
				}

				// Clean up temp file
				try {
					std::filesystem::remove(tempFile);
				}
				catch (const std::exception& e) {
					std::cout << "Warning: Failed to remove temporary file: " << e.what() << std::endl;
				}

				std::cout << "Python script executed successfully" << std::endl;
				if (!comp.output.empty()) {
					std::cout << "Output: " << comp.output << std::endl;
				}
				if (!comp.error.empty()) {
					std::cerr << "Error: " << comp.error << std::endl;
				}

			}
			catch (const std::exception& e) {
				comp.error = "C++ Exception: " + std::string(e.what());
				std::cerr << "Error executing Python script: " << comp.error << std::endl;
			}
		}

		void ExecuteScriptFile(const std::string& filePath, PythonComponent& comp) {
			std::lock_guard<std::mutex> lock(pythonMutex);

			try {
				comp.error.clear();
				comp.output.clear();

				// Check if file exists
				if (!std::filesystem::exists(filePath)) {
					comp.error = "File not found: " + filePath;
					return;
				}

				std::string pythonExe = GetPythonExecutable(comp);
				std::ostringstream commandStream;
				commandStream << "\"" << pythonExe << "\" \"" << filePath << "\"";
				std::string command = commandStream.str();

				std::cout << "Executing file command: " << command << std::endl;

				auto[output, error] = RunCommand(command);

				comp.output = output;
				if (!error.empty()) {
					comp.error = error;
				}

				std::cout << "Python file executed successfully" << std::endl;
				if (!comp.output.empty()) {
					std::cout << "Output: " << comp.output << std::endl;
				}
				if (!comp.error.empty()) {
					std::cerr << "Error: " << comp.error << std::endl;
				}

			}
			catch (const std::exception& e) {
				comp.error = "C++ Exception: " + std::string(e.what());
				std::cerr << "Error executing Python file: " << comp.error << std::endl;
			}
		}
	};
}