#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <imgui.h>
#include "OpenGLWrapper.hpp"
#include "Types.hpp"
#include "EntityManager.hpp"

namespace GUI {
    namespace DragDrop {

        static const char* PAYLOAD_ENTITY = "ANI_ENTITY";
        static const char* PAYLOAD_COMPONENT = "ANI_COMPONENT";
        static const char* PAYLOAD_FILE_PATH = "ANI_FILE_PATH";
        static const char* PAYLOAD_FILE_MULTIPLE = "ANI_FILE_MULTIPLE";

        void InitializeFileDrop(GLFWwindow* window);
        void ShutdownFileDrop(GLFWwindow* window);
        void GlfwDropCallback(GLFWwindow* window, int count, const char** paths);
        bool PollFileDrop(std::vector<std::string>& outFiles);

        bool IsExternalDragActive();
        const std::vector<std::string>& GetPendingDropFiles();
        void ClearPendingDropFiles();

        bool BeginDragSource(const char* payloadType, const nlohmann::json& payload, ImGuiID sourceID = 0);
        bool AcceptDragDrop(const char* payloadType, nlohmann::json& outPayload);
        bool AcceptFileDrop(std::vector<std::string>& outFiles);

        bool BeginEntityDrag(ECS::EntityID entity);
        bool BeginFilePathDrag(const std::string& filePath, const std::string& fileType = "");
        bool BeginMultipleFileDrag(const std::vector<std::string>& filePaths);

        bool AcceptEntityDrop(ECS::EntityID& outEntity);
        bool AcceptFilePathDrop(std::string& outFilePath, std::string& outFileType);
        bool AcceptMultipleFileDrop(std::vector<std::string>& outFilePaths);

        std::string GuessMediaType(const std::string& filePath);
        bool IsMediaFile(const std::string& filePath);
        bool IsImageFile(const std::string& filePath);
        bool IsVideoFile(const std::string& filePath);
        bool IsAudioFile(const std::string& filePath);
        bool IsModelFile(const std::string& filePath);

        bool StartExternalDragFile(const std::string& filePath, void* windowHandle);
        bool StartExternalDragFiles(const std::vector<std::string>& filePaths, void* windowHandle);

        void SetWindowHandle(void* handle);
        void* GetWindowHandle();

    }
}