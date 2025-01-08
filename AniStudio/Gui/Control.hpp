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

