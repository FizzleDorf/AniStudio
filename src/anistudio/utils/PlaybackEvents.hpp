#pragma once

#include "EntityManager.hpp"
#include "PlaybackStateComponent.hpp"
#include <string>
#include <any>
#include <unordered_map>

namespace ANI {
    namespace PlaybackEvents {

        // Event names - views send these with just entity ID
        constexpr const char* EVENT_PLAYBACK_LOAD = "PlaybackLoad";
        constexpr const char* EVENT_PLAYBACK_PLAY = "PlaybackPlay";
        constexpr const char* EVENT_PLAYBACK_PAUSE = "PlaybackPause";
        constexpr const char* EVENT_PLAYBACK_STOP = "PlaybackStop";
        constexpr const char* EVENT_PLAYBACK_SEEK = "PlaybackSeek";
        constexpr const char* EVENT_PLAYBACK_SET_SPEED = "PlaybackSetSpeed";
        constexpr const char* EVENT_PLAYBACK_SET_VOLUME = "PlaybackSetVolume";
        constexpr const char* EVENT_PLAYBACK_REMOVE = "PlaybackRemove";
        constexpr const char* EVENT_PLAYBACK_SET_MODE = "PlaybackSetMode";

        // ONLY ONE HELPER - entity ID only!
        inline std::unordered_map<std::string, std::any> MakeEntityEvent(ECS::EntityID entity) {
            std::unordered_map<std::string, std::any> data;
            data["entityID"] = entity;
            return data;
        }

    }
}