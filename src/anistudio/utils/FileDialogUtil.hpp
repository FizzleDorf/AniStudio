#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include "FileDialogFilters.hpp"

namespace FileDialog {

    bool OpenFile(const std::string& title, const std::string& filter, std::string& outPath, const std::string& defaultPath = std::filesystem::current_path().string());
    bool OpenFiles(const std::string& title, const std::string& filter, std::vector<std::string>& outPaths, const std::string& defaultPath = std::filesystem::current_path().string());
    bool SaveFile(const std::string& title, const std::string& filter, const std::string& defaultName, std::string& outPath, const std::string& defaultPath = std::filesystem::current_path().string());

    bool OpenFile(const std::string& title, FilterType type, std::string& outPath, const std::string& defaultPath = std::filesystem::current_path().string());
    bool OpenFiles(const std::string& title, FilterType type, std::vector<std::string>& outPaths, const std::string& defaultPath = std::filesystem::current_path().string());
    bool SaveFile(const std::string& title, FilterType type, const std::string& defaultName, std::string& outPath, const std::string& defaultPath = std::filesystem::current_path().string());

    bool SelectFolder(const std::string& title, std::string& outPath, const std::string& defaultPath = std::filesystem::current_path().string());

}