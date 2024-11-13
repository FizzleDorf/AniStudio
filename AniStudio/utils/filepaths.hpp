#ifndef FILEPATHS_HPP
#define FILEPATHS_HPP

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

struct FilePaths {
    std::string virtualEnvPath = "../venv";
    std::string comfyuiRootPath = "../comfyui";
    std::string lastOpenProjectPath = "../projects";
    std::string defaultProjectPath = "../projects";
    std::string defaultModelRootPath = "../models";
    std::string assetsFolderPath = "../assets";

    // These paths can be set to default model paths
    std::string checkpointDir = "/checkpoints";
    std::string encoderDir = "/clip";
    std::string clipLPath = "/clip";
    std::string clipGPath = "/clip";
    std::string t5xxlPath = "/clip";
    std::string embedDir = "/clip";
    std::string vaeDir = "/vae";
    std::string unetDir = "/unet";
    std::string loraDir = "/loras";
    std::string controlnetDir = "/controlnet";
    std::string upscaleDir = "/upscale_models";

    void SaveFilepathDefaults() {
        // Create the data directory if it does not exist
        std::filesystem::create_directories("./data/defaults");

        // Create a JSON object to store paths
        nlohmann::json json;
        json["virtualEnvPath"] = virtualEnvPath;
        json["comfyuiRootPath"] = comfyuiRootPath;
        json["lastOpenProjectPath"] = lastOpenProjectPath;
        json["defaultProjectPath"] = defaultProjectPath;
        json["defaultModelRootPath"] = defaultModelRootPath;
        json["assetsFolderPath"] = assetsFolderPath;
        json["checkpointDir"] = checkpointDir;
        json["encoderDir"] = encoderDir;
        json["vaeDir"] = vaeDir;
        json["unetDir"] = unetDir;
        json["loraDir"] = loraDir;
        json["controlnetDir"] = controlnetDir;
        json["upscaleDir"] = upscaleDir;

        // Write JSON to file
        std::ofstream file("./data/defaults/paths.json");
        if (file.is_open()) {
            file << json.dump(4); // Pretty print with 4 spaces
            file.close();
        }
    }

    void LoadFilePathDefaults() {
        // Open JSON file
        std::ifstream file("./data/defaults/paths.json");
        if (file.is_open()) {
            // Parse the JSON file
            nlohmann::json json;
            file >> json;
            file.close();

            // Update paths from JSON
            if (json.contains("virtualEnvPath"))
                virtualEnvPath = json["virtualEnvPath"];
            if (json.contains("comfyuiRootPath"))
                comfyuiRootPath = json["comfyuiRootPath"];
            if (json.contains("lastOpenProjectPath"))
                lastOpenProjectPath = json["lastOpenProjectPath"];
            if (json.contains("defaultProjectPath"))
                defaultProjectPath = json["defaultProjectPath"];
            if (json.contains("defaultModelRootPath"))
                defaultModelRootPath = json["defaultModelRootPath"];
            if (json.contains("assetsFolderPath"))
                assetsFolderPath = json["assetsFolderPath"];
            if (json.contains("checkpointDir"))
                checkpointDir = json["checkpointDir"];
            if (json.contains("encoderDir"))
                encoderDir = json["encoderDir"];
            if (json.contains("vaeDir"))
                vaeDir = json["vaeDir"];
            if (json.contains("unetDir"))
                unetDir = json["unetDir"];
            if (json.contains("loraDir"))
                loraDir = json["loraDir"];
            if (json.contains("controlnetDir"))
                controlnetDir = json["controlnetDir"];
            if (json.contains("upscaleDir"))
                upscaleDir = json["upscaleDir"];
        }
    }

    void SetByModelRoot() {
        checkpointDir = defaultModelRootPath + "/checkpoints";
        encoderDir = defaultModelRootPath + "/clip";
        vaeDir = defaultModelRootPath + "/vae";
        unetDir = defaultModelRootPath + "/unet";
        loraDir = defaultModelRootPath + "/loras";
        controlnetDir = defaultModelRootPath + "/controlnet";
        upscaleDir = defaultModelRootPath + "/upscale_models";
    }
};

#endif // FILEPATHS_HPP
