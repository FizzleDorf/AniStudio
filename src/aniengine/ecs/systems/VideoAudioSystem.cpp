#include "VideoAudioSystem.hpp"
#include "ThreadPoolSystem.hpp"
#include "Log.hpp"

namespace ECS {

    VideoAudioSystem::VideoAudioSystem(EntityManager& entityMgr)
        : BaseSystem(entityMgr) {
        sysName = "VideoAudioSystem";
    }

    VideoAudioSystem::~VideoAudioSystem() {
        Destroy();
    }

    void VideoAudioSystem::Start() {
        // Don't store pointers - get them when needed
    }

    void VideoAudioSystem::Update(float deltaT) {
    }

    void VideoAudioSystem::Destroy() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_callbacks.clear();
    }

    void VideoAudioSystem::RegisterLoadCallback(const LoadCallback& cb) {
        m_callbacks.push_back(cb);
    }

    EntityID VideoAudioSystem::LoadVideoWithAudio(const std::string& filePath) {
        auto videoSystem = mgr.GetSystem<VideoSystem>().get();
        auto audioSystem = mgr.GetSystem<AudioSystem>().get();

        if (!videoSystem || !audioSystem) {
            ANI_LOG_ERROR("[VideoAudioSystem] Required systems missing.");
            return 0;
        }

        EntityID entity = mgr.AddNewEntity();
        mgr.AddComponent<VideoComponent>(entity);

        auto audioLoad = AudioSystem::ExtractAudioFromVideoFile(filePath, entity);
        if (audioLoad.success) {
            mgr.AddComponent<AudioComponent>(entity);
            auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
            audioComp.pcmData = std::move(audioLoad.pcmData);
            audioComp.channels = audioLoad.channels;
            audioComp.sampleRate = audioLoad.sampleRate;
            audioComp.duration = audioLoad.duration;
            audioComp.fileName = audioLoad.fileName;
            audioComp.filePath = filePath;
            audioComp.isLoading = false;
            audioSystem->AddLoadedAudio(entity);
        }

        videoSystem->SetVideo(entity, filePath);

        for (const auto& cb : m_callbacks)
            cb(entity);

        return entity;
    }

}