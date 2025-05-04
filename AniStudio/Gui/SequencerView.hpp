/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license iformation, please contact legal@kframe.ai.
 */

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
    SequencerView(ECS::EntityManager &entityMgr);

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