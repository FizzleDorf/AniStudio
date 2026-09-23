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
        std::vector<float> pcmData;   // interleaved float PCM (frames * channels)
        int channels = 2;
        int sampleRate = 44100;
        double duration = 0.0;

        static AudioData FromInterleavedFloat(const float* data,
            uint64_t sampleCount,
            int channels,
            int sampleRate) {
            AudioData out;
            out.channels = channels > 0 ? channels : 1;
            out.sampleRate = sampleRate > 0 ? sampleRate : 44100;
            if (data && sampleCount > 0 && out.channels > 0) {
                size_t total = static_cast<size_t>(sampleCount) * out.channels;
                out.pcmData.assign(data, data + total);
                out.duration = static_cast<double>(sampleCount) / out.sampleRate;
            }
            return out;
        }
    };

    // Only the operations that VideoSystem/AVSystem actually need:
    // thumbnail extraction and full encode-to-file for saving.
    class VideoUtils {
    public:
        static unsigned char* LoadVideoFrame(const std::string& filePath,
            double timeInSeconds,
            int& width, int& height, int& channels,
            double* actualTime = nullptr);

        static bool GetVideoInfo(const std::string& filePath,
            int& width, int& height,
            double& duration, double& frameRate);

        static GLuint GenerateTextureFromVideoFrame(unsigned char* data,
            int width, int height);

        static void DeleteTexture(GLuint& textureID);

        static void FreeVideoFrameData(unsigned char* data);

        static bool SaveVideoFrameAsImage(const std::string& videoPath,
            const std::string& imagePath,
            double timeInSeconds);

        static bool EncodeFramesToVideo(const std::vector<VideoFrame>& frames,
            const std::string& outputPath,
            int fps = 24,
            const nlohmann::json& metadata = nlohmann::json(),
            const AudioData* audio = nullptr);
    };

} // namespace Utils