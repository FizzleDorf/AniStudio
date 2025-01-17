#ifndef FILEPATHS_HPP
#define FILEPATHS_HPP

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

struct FilePaths {
    std::string dataPath = "../data/defaults";
    std::string ImguiStatePath = "../data/defaults/imgui.ini";
    std::string virtualEnvPath = "";
    std::string comfyuiRootPath = "";
    std::string lastOpenProjectPath = "";
    std::string defaultProjectPath = "";
    std::string defaultModelRootPath = "";
    std::string assetsFolderPath = "";
    std::string pluginPath = "../plugins";

    // These paths can be set to default model paths
    std::string checkpointDir = "";
    std::string encoderDir = "";
    std::string clipLPath = "";
    std::string clipGPath = "";
    std::string t5xxlPath = "";
    std::string embedDir = "";
    std::string vaeDir = "";
    std::string unetDir = "";
    std::string loraDir = "";
    std::string controlnetDir = "";
    std::string upscaleDir = "";

    void Init() {
        LoadFilePathDefaults();
        if (lastOpenProjectPath.empty()) {
            std::filesystem::path newProjectPath = "../projects/new_project";
            std::filesystem::create_directories(newProjectPath);
            defaultProjectPath = std::filesystem::absolute(newProjectPath).string();
            SaveFilepathDefaults();
        }
        if (defaultModelRootPath.empty()) {
            std::filesystem::path newModelPath = "../models";
            std::filesystem::create_directories(newModelPath);
            defaultModelRootPath = std::filesystem::absolute(newModelPath).string();
            SetByModelRoot();
            SaveFilepathDefaults();
        }
    }

    void SaveFilepathDefaults() {
        // Create the data directory if it does not exist
        std::filesystem::create_directories("..\\data\\defaults");

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
        std::ofstream file("..\\data\\defaults\\paths.json");
        if (file.is_open()) {
            file << json.dump(4); // Pretty print with 4 spaces
            file.close();
        }
    }

    void LoadFilePathDefaults() {
        // Open JSON file
        std::ifstream file("..\\data\\defaults\\paths.json");
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
        std::filesystem::path newModelPath = defaultModelRootPath + "\\checkpoints";
        std::filesystem::create_directories(newModelPath);
        checkpointDir = newModelPath.string();

        newModelPath = defaultModelRootPath + "\\clip";
        std::filesystem::create_directories(newModelPath);
        encoderDir = newModelPath.string();

        newModelPath = defaultModelRootPath + "\\vae";
        std::filesystem::create_directories(newModelPath);
        vaeDir = newModelPath.string();

        newModelPath = defaultModelRootPath + "\\unet";
        std::filesystem::create_directories(newModelPath);
        unetDir = newModelPath.string();

        newModelPath = defaultModelRootPath + "\\loras";
        std::filesystem::create_directories(newModelPath);
        loraDir = newModelPath.string();

        newModelPath = defaultModelRootPath + "\\controlnet";
        std::filesystem::create_directories(newModelPath);
        controlnetDir = newModelPath.string();

        newModelPath = defaultModelRootPath + "\\upscale_models";
        std::filesystem::create_directories(newModelPath);
        upscaleDir = newModelPath.string();
    }
};

extern FilePaths filePaths;

#endif // FILEPATHS_HPP
