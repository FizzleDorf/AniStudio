#pragma once
#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "VideoComponent.hpp"
#include "AudioComponent.hpp"
#include "VideoSystem.hpp"
#include "AudioSystem.hpp"
#include <functional>
#include <future>
#include <mutex>

namespace ECS {

    class VideoAudioSystem : public BaseSystem {
    public:
        using LoadCallback = std::function<void(EntityID)>;

        VideoAudioSystem(EntityManager& entityMgr);
        ~VideoAudioSystem() override;

        void Start() override;
        void Update(float deltaT) override;
        void Destroy() override;

        void RegisterLoadCallback(const LoadCallback& cb);

        EntityID LoadVideoWithAudio(const std::string& filePath);

    private:
        std::vector<LoadCallback> m_callbacks;
        mutable std::mutex m_mutex;
    };

}