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

//#ifndef CONTROL_HPP
//#define CONTROL_HPP
//#include "Base/BaseView.hpp"
//#include <memory>
//#include <random>
//#include <type_traits>
//#include <vector>
//
//namespace GUI {
//
//enum class ControlMode { Fixed = 0, Increment, Decrement, Random };
//
//// Base class for polymorphic control behavior
//class ControlBase {
//public:
//    virtual ~ControlBase() = default;
//    virtual void activate() = 0;
//    virtual void renderUI() = 0;
//    virtual ControlMode getMode() const = 0;
//};
//
//template <typename T>
//class Control : public ControlBase {
//public:
//    Control(T &valueRef, ControlMode mode = ControlMode::Fixed, T newStep = 1, T min = 0,
//            T max = std::numeric_limits<int>::max())
//        : value(valueRef), mode(mode), step(newStep), randomRange(min, max) {
//        lastValue = value;
//    }
//
//    void setMode(ControlMode newMode) { mode = newMode; }
//    void setRandomRange(T min, T max) { randomRange = {min, max}; }
//    void setLastValue() {
//        value = lastValue;
//        mode = ControlMode::Fixed;
//    }
//    void activate() override {
//        lastValue = value;
//        switch (mode) {
//        case ControlMode::Fixed:
//            break;
//        case ControlMode::Increment:
//            value += step;
//            break;
//        case ControlMode::Decrement:
//            value -= step;
//            break;
//        case ControlMode::Random:
//            value = generateRandomValue();
//            break;
//        }
//    }
//
//    ControlMode getMode() const override { return mode; }
//
//    void renderUI() override {
//        const char *modeItems[] = {"Fixed", "Increment", "Decrement", "Random"};
//        const char *currentModeStr = modeItems[static_cast<int>(mode)];
//
//        // Combo box to select control mode
//        if (ImGui::BeginCombo("##Control Mode", currentModeStr)) {
//            for (int i = 0; i < IM_ARRAYSIZE(modeItems); i++) {
//                bool isSelected = (mode == static_cast<ControlMode>(i));
//                if (ImGui::Selectable(modeItems[i], isSelected)) {
//                    mode = static_cast<ControlMode>(i);
//                }
//                if (isSelected) {
//                    ImGui::SetItemDefaultFocus(); // Keep the selected item focused
//                }
//            }
//            ImGui::EndCombo();
//        }
//
//        if (ImGui::Button("Last Value##d0")) {
//            setLastValue();
//        }
//    }
//
//private:
//    T &value;
//    T lastValue;
//    ControlMode mode;
//    T step = 1;
//    std::pair<T, T> randomRange;
//
//    T generateRandomValue() const {
//        static std::random_device rd;
//        static std::mt19937 gen(rd());
//        if constexpr (std::is_integral_v<T>) {
//            std::uniform_int_distribution<T> dist(randomRange.first, randomRange.second);
//            return dist(gen);
//        } else if constexpr (std::is_floating_point_v<T>) {
//            std::uniform_real_distribution<T> dist(randomRange.first, randomRange.second);
//            return dist(gen);
//        }
//        return value;
//    }
//};
//
//// Helper function to render controls (you can expand this with more UI elements)
//template <typename T>
//void renderControlUI(Control<T> &control) {
//    control.renderUI();
//}
//} // namespace UI
//
//#endif // CONTROL_HPP

