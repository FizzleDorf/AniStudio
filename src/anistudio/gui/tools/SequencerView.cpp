// SequencerView.cpp
#include "SequencerView.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include "Events.hpp"

namespace GUI {
    SequencerView::SequencerView(ECS::EntityManager& entityMgr) : BaseView(entityMgr), currentFrame(0), playing(false), playbackSpeed(30.0f), lastTime(0.0) {
        viewName = "SequencerView";
    }

    void SequencerView::AddTrack(const std::string& name, ECS::EntityID entity) {
        tracks.push_back({ name, entity, 0, 100, false });
    }

    void SequencerView::Play() {
        playing = true;
        lastTime = glfwGetTime();
    }

    void SequencerView::Pause() { playing = false; }

    void SequencerView::Stop() {
        playing = false;
        currentFrame = 0;
    }

    int SequencerView::GetFrameMin() const { return 0; }

    int SequencerView::GetFrameMax() const {
        return 1000;
    }

    int SequencerView::GetItemCount() const { return static_cast<int>(tracks.size()); }

    void SequencerView::Get(int index, int** start, int** end, int* type, unsigned int* color) {
        if (index < 0 || index >= static_cast<int>(tracks.size()))
            return;

        Track& track = tracks[index];
        *start = &track.startFrame;
        *end = &track.endFrame;
        *type = 0;
        *color = 0xFFAA0000;
    }

    const char* SequencerView::GetItemLabel(int index) const {
        if (index < 0 || index >= static_cast<int>(tracks.size()))
            return nullptr;

        return tracks[index].name.c_str();
    }

    void SequencerView::Render() {
        if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen)) {

            if (!windowOpen) {
                std::unordered_map<std::string, std::any> eventData;
                eventData["workspaceID"] = GetID();
                eventData["viewTypeName"] = viewName;
                ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
                ImGui::End();
                return;
            }

            if (ImGui::Button("Play"))
                Play();
            ImGui::SameLine();
            if (ImGui::Button("Pause"))
                Pause();
            ImGui::SameLine();
            if (ImGui::Button("Stop"))
                Stop();

            ImGui::SliderFloat("Playback Speed", &playbackSpeed, 1.0f, 120.0f);

            if (playing) {
                double now = glfwGetTime();
                double deltaTime = now - lastTime;
                lastTime = now;
                currentFrame += static_cast<int>(deltaTime * playbackSpeed);
            }

            int selectedEntry = -1;
            bool expanded = true;
            int firstFrame = 0;
            ImSequencer::Sequencer(this, &currentFrame, &expanded, &selectedEntry, &firstFrame,
                ImSequencer::SEQUENCER_EDIT_ALL);
        }
        ImGui::End();
    }
} // namespace GUI