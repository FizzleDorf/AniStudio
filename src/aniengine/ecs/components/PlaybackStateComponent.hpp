#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {

    enum class PlaybackMode {
        Cached,
        Streaming
    };

    enum class PlaybackState {
        Stopped = 0,
        Playing = 1,
        Paused = 2,
        EndOfStream = 3,
        Error = 4
    };

    enum class TrackType {
        Video,
        Audio,
        Both
    };

    struct PlaybackStateComponent : public BaseComponent {
        PlaybackStateComponent() = default;

        const char* GetCompName() const override { return "PlaybackState"; }
        const char* GetCompCategory() const override { return "Playback"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "Playback State"},
                {"type", "object"},
                {"properties", {
                    {"trackType", {{"type", "integer"}, {"title", "Track Type"}}},
                    {"trackIndex", {{"type", "integer"}, {"title", "Track Index"}}},
                    {"trackName", {{"type", "string"}, {"title", "Track Name"}}},
                    {"isMuted", {{"type", "boolean"}, {"title", "Muted"}}},
                    {"isSolo", {{"type", "boolean"}, {"title", "Solo"}}},
                    {"mode", {{"type", "integer"}, {"title", "Mode"}}},
                    {"state", {{"type", "integer"}, {"title", "State"}}},
                    {"isPaused", {{"type", "boolean"}, {"title", "Paused"}}},
                    {"isLoaded", {{"type", "boolean"}, {"title", "Loaded"}}},
                    {"looping", {{"type", "boolean"}, {"title", "Looping"}}},
                    {"currentTime", {{"type", "number"}, {"title", "Current Time"}}},
                    {"duration", {{"type", "number"}, {"title", "Duration"}}},
                    {"speed", {{"type", "number"}, {"title", "Speed"}}},
                    {"volume", {{"type", "number"}, {"title", "Volume"}}},
                    {"pan", {{"type", "number"}, {"title", "Pan"}}},
                    {"filePath", {{"type", "string"}, {"title", "File Path"}}},
                    {"currentFrame", {{"type", "integer"}, {"title", "Current Frame"}}},
                    {"totalFrames", {{"type", "integer"}, {"title", "Total Frames"}}},
                    {"fps", {{"type", "number"}, {"title", "FPS"}}},
                    {"syncOffset", {{"type", "number"}, {"title", "Sync Offset"}}}
                }}
            };
            return j;
        }

        TrackType trackType = TrackType::Video;
        int trackIndex = -1;
        std::string trackName;
        bool isMuted = false;
        bool isSolo = false;

        PlaybackMode mode = PlaybackMode::Cached;
        PlaybackState state = PlaybackState::Stopped;
        bool isPaused = false;
        bool isLoaded = false;
        bool looping = false;
        double currentTime = 0.0;
        double duration = 0.0;
        float speed = 1.0f;
        float volume = 1.0f;
        float pan = 0.0f;
        std::string filePath;

        long long currentFrame = 0;
        long long totalFrames = 0;
        double fps = 30.0;

        double syncOffset = 0.0;

        EntityID videoComponentID = 0;
        EntityID audioComponentID = 0;

        bool needsTextureUpdate = false;
        std::vector<uint8_t> frameDataRGBA;

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"trackType", &trackType},
                {"trackIndex", &trackIndex},
                {"trackName", &trackName},
                {"isMuted", &isMuted},
                {"isSolo", &isSolo},
                {"mode", &mode},
                {"state", &state},
                {"isPaused", &isPaused},
                {"isLoaded", &isLoaded},
                {"looping", &looping},
                {"currentTime", &currentTime},
                {"duration", &duration},
                {"speed", &speed},
                {"volume", &volume},
                {"pan", &pan},
                {"filePath", &filePath},
                {"currentFrame", &currentFrame},
                {"totalFrames", &totalFrames},
                {"fps", &fps},
                {"syncOffset", &syncOffset}
            };
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"trackType", static_cast<int>(trackType)},
                {"trackIndex", trackIndex},
                {"trackName", trackName},
                {"isMuted", isMuted},
                {"isSolo", isSolo},
                {"mode", static_cast<int>(mode)},
                {"state", static_cast<int>(state)},
                {"isPaused", isPaused},
                {"isLoaded", isLoaded},
                {"looping", looping},
                {"currentTime", currentTime},
                {"duration", duration},
                {"speed", speed},
                {"volume", volume},
                {"pan", pan},
                {"filePath", filePath},
                {"currentFrame", currentFrame},
                {"totalFrames", totalFrames},
                {"fps", fps},
                {"syncOffset", syncOffset}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();
            nlohmann::json componentData;
            if (j.contains(key)) componentData = j.at(key);
            else componentData = j;

            if (componentData.contains("trackType")) trackType = static_cast<TrackType>(componentData["trackType"]);
            if (componentData.contains("trackIndex")) trackIndex = componentData["trackIndex"];
            if (componentData.contains("trackName")) trackName = componentData["trackName"];
            if (componentData.contains("isMuted")) isMuted = componentData["isMuted"];
            if (componentData.contains("isSolo")) isSolo = componentData["isSolo"];
            if (componentData.contains("mode")) mode = static_cast<PlaybackMode>(componentData["mode"]);
            if (componentData.contains("state")) state = static_cast<PlaybackState>(componentData["state"]);
            if (componentData.contains("isPaused")) isPaused = componentData["isPaused"];
            if (componentData.contains("isLoaded")) isLoaded = componentData["isLoaded"];
            if (componentData.contains("looping")) looping = componentData["looping"];
            if (componentData.contains("currentTime")) currentTime = componentData["currentTime"];
            if (componentData.contains("duration")) duration = componentData["duration"];
            if (componentData.contains("speed")) speed = componentData["speed"];
            if (componentData.contains("volume")) volume = componentData["volume"];
            if (componentData.contains("pan")) pan = componentData["pan"];
            if (componentData.contains("filePath")) filePath = componentData["filePath"];
            if (componentData.contains("currentFrame")) currentFrame = componentData["currentFrame"];
            if (componentData.contains("totalFrames")) totalFrames = componentData["totalFrames"];
            if (componentData.contains("fps")) fps = componentData["fps"];
            if (componentData.contains("syncOffset")) syncOffset = componentData["syncOffset"];
        }
    };

}