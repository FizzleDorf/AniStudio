#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <GL/glew.h>
#include <nlohmann/json.hpp>

namespace Utils {

    struct VideoFrame {
        int width = 0;
        int height = 0;
        int channels = 4;
        const unsigned char* data = nullptr;
    };

    struct AudioData {
        std::vector<float> pcmData;
        int channels = 2;
        int sampleRate = 44100;
        double duration = 0.0;
    };

    class VideoUtils {
    public:
        static unsigned char* LoadVideoFrame(const std::string& filePath, double timeInSeconds,
            int& width, int& height, int& channels,
            double* actualTime = nullptr);

        static bool GetVideoInfo(const std::string& filePath, int& width, int& height,
            double& duration, double& frameRate);

        static GLuint GenerateTextureFromVideoFrame(unsigned char* data, int width, int height);

        static void DeleteTexture(GLuint& textureID);

        static void FreeVideoFrameData(unsigned char* data);

        static bool SaveVideoFrameAsImage(const std::string& videoPath, const std::string& imagePath,
            double timeInSeconds);

        // Updated to include audio support
        static bool EncodeFramesToVideo(const std::vector<VideoFrame>& frames,
            const std::string& outputPath,
            int fps = 24,
            const nlohmann::json& metadata = nlohmann::json(),
            const AudioData* audio = nullptr);

        static bool HasExifMetadata(const std::string& filePath);
        static bool HasLSBMetadata(const std::string& filePath);
        static int GetMetadataStatus(const std::string& filePath);
    };

}