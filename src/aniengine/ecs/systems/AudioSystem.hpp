#pragma once

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "AudioComponent.hpp"
#include "ThreadPoolSystem.hpp"
#include "AudioUtils.hpp"
#include <memory>
#include <functional>
#include <vector>
#include <queue>
#include <mutex>
#include <future>
#include <chrono>
#include <filesystem>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

namespace ECS {

    class AudioSystem : public BaseSystem {
    public:
        using AudioCallback = std::function<void(EntityID)>;
        using AudioDataCallback = std::function<void(EntityID, const float*, size_t, int, int)>;

        struct LoadResult {
            bool success = false;
            EntityID entityID = 0;
            std::string filePath;
            std::string fileName;
            double duration = 0.0;
            int channels = 0;
            int sampleRate = 0;
            int64_t totalSamples = 0;
            std::vector<float> pcmData;
            bool hasExif = false;
            bool hasLSB = false;
            bool hasAniStudio = false;
        };

        struct LoadingTask {
            EntityID entityID;
            std::string filePath;
            std::future<LoadResult> future;

            LoadingTask() = default;
            LoadingTask(LoadingTask&& other) noexcept;
            LoadingTask& operator=(LoadingTask&& other) noexcept;
            LoadingTask(const LoadingTask&) = delete;
            LoadingTask& operator=(const LoadingTask&) = delete;
        };

        AudioSystem(EntityManager& entityMgr);
        ~AudioSystem() override;

        void Start() override;
        void Update(float deltaT) override;
        void Destroy() override;

        void RegisterAudioAddedCallback(const AudioCallback& callback);
        void RegisterAudioRemovedCallback(const AudioCallback& callback);
        void RegisterAudioDataCallback(const AudioDataCallback& callback);

        void SetAudio(EntityID entity, const std::string& filePath);
        void RemoveAudio(EntityID entity);
        std::vector<EntityID> GetAllAudioEntities() const;

        const float* GetAudioData(EntityID entity, size_t& outSize, int& outChannels) const;

    private:
        std::vector<AudioCallback> audioAddedCallbacks;
        std::vector<AudioCallback> audioRemovedCallbacks;
        std::vector<AudioDataCallback> audioDataCallbacks;
        std::vector<LoadingTask> pendingLoads;
        mutable std::mutex loadMutex;

        void LoadAudioAsync(EntityID entity, const std::string& filePath);
        void ProcessCompletedLoads();
        void NotifyAudioAdded(EntityID entity);
        void NotifyAudioRemoved(EntityID entity);
        void NotifyAudioData(EntityID entity, const float* data, size_t size, int channels, int sampleRate);

        static LoadResult DecodeAudioFile(const std::string& filePath, EntityID entity, int targetSampleRate, int targetChannels);
    };

} // namespace ECS