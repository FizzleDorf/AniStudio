// RepeatButtonUtils.cpp
#include "RepeatButtonUtils.hpp"

namespace GUI {

    bool RepeatButtonHandler::dummy = false;

    bool RepeatButtonHandler::Process(ImGuiID id, double initialDelay, double repeatRate, bool& outHeld) {
        auto it = m_states.find(id);
        if (it == m_states.end()) {
            it = m_states.emplace(id, RepeatButtonState{}).first;
        }
        auto& state = it->second;

        bool triggered = false;

        if (ImGui::IsItemActive()) {
            double now = ImGui::GetTime();
            if (state.repeatCount == 0) {
                state.lastActionTime = now;
                state.repeatCount = 1;
                triggered = true;
            }
            else {
                double delay = (state.repeatCount == 1) ? initialDelay : repeatRate;
                if (state.repeatCount > 5) delay = repeatRate * 0.5;
                if (state.repeatCount > 10) delay = repeatRate * 0.25;

                if (now - state.lastActionTime >= delay) {
                    state.lastActionTime = now;
                    state.repeatCount++;
                    triggered = true;
                }
            }
            outHeld = true;
        }
        else {
            state.repeatCount = 0;
            outHeld = false;
        }

        return triggered;
    }

    void RepeatButtonHandler::Reset(ImGuiID id) {
        auto it = m_states.find(id);
        if (it != m_states.end()) {
            it->second.repeatCount = 0;
            it->second.lastActionTime = 0.0;
        }
    }

}