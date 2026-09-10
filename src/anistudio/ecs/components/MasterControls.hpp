#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {

    struct MasterControlsComponent : public BaseComponent {
        MasterControlsComponent() {
            compName = "MasterControls";
            compCategory = "Playback";
            setupBaseSchema();
        }

        // === Master Transport ===
        enum class TransportState {
            Stopped,
            Playing,
            Paused,
            Scrubbing
        };
        TransportState transportState = TransportState::Stopped;
        double masterTime = 0.0;
        double masterDuration = 0.0;

        // === Master Audio ===
        float masterVolume = 1.0f;
        float masterPan = 0.0f;
        bool masterMute = false;

        // === Master Video ===
        float masterSpeed = 1.0f;
        bool masterLoop = false;

        // === Playhead ===
        double playheadPosition = 0.0;
        double playheadFrame = 0.0;
        double fps = 30.0;

        // === Selection ===
        double selectionStart = 0.0;
        double selectionEnd = 0.0;
        bool hasSelection = false;

        // === Sync ===
        bool isSyncEnabled = true;
        double syncReferenceTime = 0.0;

        virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            std::unordered_map<std::string, UISchema::PropertyVariant> properties;
            properties["transportState"] = &transportState;
            properties["masterTime"] = &masterTime;
            properties["masterDuration"] = &masterDuration;
            properties["masterVolume"] = &masterVolume;
            properties["masterPan"] = &masterPan;
            properties["masterMute"] = &masterMute;
            properties["masterSpeed"] = &masterSpeed;
            properties["masterLoop"] = &masterLoop;
            properties["playheadPosition"] = &playheadPosition;
            properties["fps"] = &fps;
            properties["isSyncEnabled"] = &isSyncEnabled;
            return properties;
        }

        virtual nlohmann::json Serialize() const override {
            nlohmann::json j;
            j["compName"] = compName;
            j[compName] = {
                {"transportState", static_cast<int>(transportState)},
                {"masterTime", masterTime},
                {"masterDuration", masterDuration},
                {"masterVolume", masterVolume},
                {"masterPan", masterPan},
                {"masterMute", masterMute},
                {"masterSpeed", masterSpeed},
                {"masterLoop", masterLoop},
                {"playheadPosition", playheadPosition},
                {"fps", fps},
                {"isSyncEnabled", isSyncEnabled}
            };
            return j;
        }

        virtual void Deserialize(const nlohmann::json& j) override {
            BaseComponent::Deserialize(j);
            nlohmann::json componentData;
            if (j.contains(compName)) componentData = j.at(compName);
            else componentData = j;

            if (componentData.contains("transportState")) transportState = static_cast<TransportState>(componentData["transportState"]);
            if (componentData.contains("masterTime")) masterTime = componentData["masterTime"];
            if (componentData.contains("masterDuration")) masterDuration = componentData["masterDuration"];
            if (componentData.contains("masterVolume")) masterVolume = componentData["masterVolume"];
            if (componentData.contains("masterPan")) masterPan = componentData["masterPan"];
            if (componentData.contains("masterMute")) masterMute = componentData["masterMute"];
            if (componentData.contains("masterSpeed")) masterSpeed = componentData["masterSpeed"];
            if (componentData.contains("masterLoop")) masterLoop = componentData["masterLoop"];
            if (componentData.contains("playheadPosition")) playheadPosition = componentData["playheadPosition"];
            if (componentData.contains("fps")) fps = componentData["fps"];
            if (componentData.contains("isSyncEnabled")) isSyncEnabled = componentData["isSyncEnabled"];
        }

    protected:
        void setupBaseSchema() {
            schema = {
                {"title", "Master Controls"},
                {"type", "object"},
                {"properties", {
                    {"transportState", {{"type", "integer"}, {"title", "Transport State"}}},
                    {"masterTime", {{"type", "number"}, {"title", "Master Time"}}},
                    {"masterDuration", {{"type", "number"}, {"title", "Master Duration"}}},
                    {"masterVolume", {{"type", "number"}, {"title", "Master Volume"}}},
                    {"masterPan", {{"type", "number"}, {"title", "Master Pan"}}},
                    {"masterMute", {{"type", "boolean"}, {"title", "Master Mute"}}},
                    {"masterSpeed", {{"type", "number"}, {"title", "Master Speed"}}},
                    {"masterLoop", {{"type", "boolean"}, {"title", "Master Loop"}}},
                    {"playheadPosition", {{"type", "number"}, {"title", "Playhead Position"}}},
                    {"fps", {{"type", "number"}, {"title", "FPS"}}},
                    {"isSyncEnabled", {{"type", "boolean"}, {"title", "Sync Enabled"}}}
                }}
            };
        }
    };

}