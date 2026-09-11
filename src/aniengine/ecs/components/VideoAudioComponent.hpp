#pragma once

#include "BaseComponent.hpp"
#include "FileFormats.hpp"
#include <string>

namespace ECS {

    struct VideoAudioComponent : BaseComponent {
        EntityID videoEntityID = 0;
        EntityID audioEntityID = 0;
        bool hasAudio = false;
        double duration = 0.0;
        double currentTime = 0.0;
        float volume = 1.0f;
        float playbackSpeed = 1.0f;
        bool isPlaying = false;
        bool isPaused = false;
        bool looping = false;
        bool audioEnabled = true;
        float lastVolume = 1.0f;

        VideoAudioComponent() = default;

        const char* GetCompName() const override { return "VideoAudioComponent"; }
        const char* GetCompCategory() const override { return "Media"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "Video Audio"},
                {"type", "object"},
                {"properties", {
                    {"videoEntityID", {{"type", "integer"}, {"title", "Video Entity ID"}}},
                    {"audioEntityID", {{"type", "integer"}, {"title", "Audio Entity ID"}}},
                    {"hasAudio", {{"type", "boolean"}, {"title", "Has Audio Track"}}},
                    {"duration", {{"type", "number"}, {"title", "Duration (seconds)"}}},
                    {"currentTime", {{"type", "number"}, {"title", "Current Time (seconds)"}}},
                    {"volume", {{"type", "number"}, {"title", "Volume"}, {"minimum", 0.0}, {"maximum", 1.0}}},
                    {"playbackSpeed", {{"type", "number"}, {"title", "Playback Speed"}}},
                    {"isPlaying", {{"type", "boolean"}, {"title", "Is Playing"}}},
                    {"isPaused", {{"type", "boolean"}, {"title", "Is Paused"}}},
                    {"looping", {{"type", "boolean"}, {"title", "Looping"}}},
                    {"audioEnabled", {{"type", "boolean"}, {"title", "Audio Enabled"}}}
                }}
            };
            return j;
        }

        VideoAudioComponent(const VideoAudioComponent& other)
            : BaseComponent(other)
            , videoEntityID(other.videoEntityID)
            , audioEntityID(other.audioEntityID)
            , hasAudio(other.hasAudio)
            , duration(other.duration)
            , currentTime(other.currentTime)
            , volume(other.volume)
            , playbackSpeed(other.playbackSpeed)
            , isPlaying(other.isPlaying)
            , isPaused(other.isPaused)
            , looping(other.looping)
            , audioEnabled(other.audioEnabled)
            , lastVolume(other.lastVolume) {
        }

        VideoAudioComponent& operator=(const VideoAudioComponent& other) {
            if (this != &other) {
                videoEntityID = other.videoEntityID;
                audioEntityID = other.audioEntityID;
                hasAudio = other.hasAudio;
                duration = other.duration;
                currentTime = other.currentTime;
                volume = other.volume;
                playbackSpeed = other.playbackSpeed;
                isPlaying = other.isPlaying;
                isPaused = other.isPaused;
                looping = other.looping;
                audioEnabled = other.audioEnabled;
                lastVolume = other.lastVolume;
            }
            return *this;
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"videoEntityID", &videoEntityID},
                {"audioEntityID", &audioEntityID},
                {"hasAudio", &hasAudio},
                {"duration", &duration},
                {"currentTime", &currentTime},
                {"volume", &volume},
                {"playbackSpeed", &playbackSpeed},
                {"isPlaying", &isPlaying},
                {"isPaused", &isPaused},
                {"looping", &looping},
                {"audioEnabled", &audioEnabled}
            };
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"videoEntityID", videoEntityID},
                {"audioEntityID", audioEntityID},
                {"hasAudio", hasAudio},
                {"duration", duration},
                {"currentTime", currentTime},
                {"volume", volume},
                {"playbackSpeed", playbackSpeed},
                {"isPlaying", isPlaying},
                {"isPaused", isPaused},
                {"looping", looping},
                {"audioEnabled", audioEnabled},
                {"lastVolume", lastVolume}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();
            nlohmann::json componentData;
            if (j.contains(key))
                componentData = j.at(key);
            else
                componentData = j;

            if (componentData.contains("videoEntityID")) videoEntityID = componentData["videoEntityID"];
            if (componentData.contains("audioEntityID")) audioEntityID = componentData["audioEntityID"];
            if (componentData.contains("hasAudio")) hasAudio = componentData["hasAudio"];
            if (componentData.contains("duration")) duration = componentData["duration"];
            if (componentData.contains("currentTime")) currentTime = componentData["currentTime"];
            if (componentData.contains("volume")) volume = componentData["volume"];
            if (componentData.contains("playbackSpeed")) playbackSpeed = componentData["playbackSpeed"];
            if (componentData.contains("isPlaying")) isPlaying = componentData["isPlaying"];
            if (componentData.contains("isPaused")) isPaused = componentData["isPaused"];
            if (componentData.contains("looping")) looping = componentData["looping"];
            if (componentData.contains("audioEnabled")) audioEnabled = componentData["audioEnabled"];
            if (componentData.contains("lastVolume")) lastVolume = componentData["lastVolume"];
        }
    };

}