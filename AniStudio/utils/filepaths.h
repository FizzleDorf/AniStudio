// FilePaths.hpp
#ifndef FILEPATHS_HPP
#define FILEPATHS_HPP

#include <string>

struct FilePaths {
    std::string virtualEnvPath;          // Path to the virtual environment
    std::string comfyuiRootPath;         // Path to the ComfyUI root folder
    std::string lastOpenProjectPath;     // Path to the last open project folder
    std::string defaultProjectPath;      // Path to the default project folder
    std::string defaultModelRootPath;    // Path to the default model root folder
    std::string assetsFolderPath;        // Path to the assets folder

    // Constructor for initializing the struct with default paths
    FilePaths(
        const std::string& virtualEnv = "",
        const std::string& comfyuiRoot = "",
        const std::string& lastOpenProject = "",
        const std::string& defaultProject = "",
        const std::string& defaultModelRoot = "",
        const std::string& assetsFolder = ""
    )
        : virtualEnvPath(virtualEnv)
        , comfyuiRootPath(comfyuiRoot)
        , lastOpenProjectPath(lastOpenProject)
        , defaultProjectPath(defaultProject)
        , defaultModelRootPath(defaultModelRoot)
        , assetsFolderPath(assetsFolder)
    {}
};

#endif // FILEPATHS_HPP
