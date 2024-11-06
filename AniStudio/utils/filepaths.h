// FilePaths.hpp
#ifndef FILEPATHS_HPP
#define FILEPATHS_HPP

#include <string>

struct FilePaths {
    std::string virtualEnvPath = "./venv";
    std::string comfyuiRootPath = "./comfyui";
    std::string lastOpenProjectPath = "./projects"; 
    std::string defaultProjectPath = "./projects"; 
    std::string defaultModelRootPath = "./models"; 
    std::string assetsFolderPath = "./assets"; 

    std::string checkpointDir = "/checkpoints";
    std::string encoderDir = "/clip";
    std::string vaeDir = "/vae";
    std::string unetDir = "/unet";
    std::string loraDir = "/loras";
    std::string controlnetDir = "/controlnet";
    std::string upscaleDir = "/upscale_models";

    void SaveFilepathDefaults(){}
    void LoadFilePathDefaults(){}
    void SetByModelRoot() {}
};

#endif // FILEPATHS_HPP
