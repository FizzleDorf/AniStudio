#ifndef SEQUENCER_VIEW_HPP
#define SEQUENCER_VIEW_HPP

#include "Base/BaseView.hpp"
#include "pch.h"
#include <ImSequencer.h>
#include "ImGuiFileDialog.h"

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
    SequencerView();

    // Sequencer control
    void Play();
    void Pause();
    void Stop();

    // Render the view
    void Render();

    // Add a track
    void AddTrack(const std::string &name, ECS::EntityID entity);

    // Implement SequenceInterface
    int GetFrameMin() const override;
    int GetFrameMax() const override;
    int GetItemCount() const override;
    void Get(int index, int **start, int **end, int *type, unsigned int *color) override;
    const char *GetItemLabel(int index) const override;

private:
    std::vector<Track> tracks; // Stores the tracks
    int currentFrame;          // Current playback frame
    bool playing;              // Is the sequencer playing
    float playbackSpeed;       // Playback speed
    double lastTime;           // Last time for frame calculation
};

#endif // SEQUENCER_VIEW_HPP
}