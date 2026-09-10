// RepeatButtonUtils.hpp
#pragma once
#include <imgui.h>
#include <unordered_map>

namespace GUI {

    struct RepeatButtonState {
        double lastActionTime = 0.0;
        int repeatCount = 0;
    };

    class RepeatButtonHandler {
    public:
        bool Process(ImGuiID id, double initialDelay = 0.4, double repeatRate = 0.1, bool& outHeld = dummy);

        void Reset(ImGuiID id);

        const std::unordered_map<ImGuiID, RepeatButtonState>& GetAllStates() const { return m_states; }

    private:
        static bool dummy;
        std::unordered_map<ImGuiID, RepeatButtonState> m_states;
    };

}