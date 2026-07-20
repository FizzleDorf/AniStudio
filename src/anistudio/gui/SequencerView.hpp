// SequencerView.hpp
#ifndef SEQUENCER_VIEW_HPP
#define SEQUENCER_VIEW_HPP

#include "GUI.h"
#include "pch.h"
#include <ImSequencer.h>

namespace GUI {

    struct Track {
        std::string name;
        ECS::EntityID entity;
        int startFrame;
        int endFrame;
        bool expanded;
    };

    class SequencerView : public ImSequencer::SequenceInterface, public BaseView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Sequencer View",
            "category": "Tools",
            "description": "A Simple Sequencer."
        })";
        }

        SequencerView(ECS::EntityManager& entityMgr);
        ~SequencerView() = default;

        void Play();
        void Pause();
        void Stop();

        void Render();

        void AddTrack(const std::string& name, ECS::EntityID entity);

        int GetFrameMin() const override;
        int GetFrameMax() const override;
        int GetItemCount() const override;
        void Get(int index, int** start, int** end, int* type, unsigned int* color) override;
        const char* GetItemLabel(int index) const override;

    private:
        std::vector<Track> tracks;
        int currentFrame;
        bool playing;
        float playbackSpeed;
        double lastTime;
    };

}

#endif // SEQUENCER_VIEW_HPP