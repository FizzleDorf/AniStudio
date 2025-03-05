//
// This header provides templated functions for
// numerical widgets
//
//
#ifndef IMJSCHEMA_WIDGETS_T_H
#define IMJSCHEMA_WIDGETS_T_H

#include <imgui.h>
#include <type_traits>
#include <cstdint>
#include <limits>
#include <imgui_internal.h>

namespace ImJSchema
{

/**
 * @brief toImGuiDataType
 * @return
 *
 * Given an arithmetic datatype, return the imgui type
 */
template<typename _T>
constexpr ImGuiDataType_ toImGuiDataType()
{
    using T = std::decay_t<_T>;
    static_assert( std::is_arithmetic_v<T>, "Must be an arithmetic type");

    if constexpr( std::is_same_v<T, float> )
    {
        return ImGuiDataType_Float;
    }
    else if constexpr( std::is_same_v<T, double> )
    {
        return ImGuiDataType_Double;
    }
    // signed ints
    else if constexpr( std::is_same_v<T, int64_t> )
    {
        return ImGuiDataType_S64;
    }
    else if constexpr( std::is_same_v<T, int32_t> )
    {
        return ImGuiDataType_S32;
    }
    else if constexpr( std::is_same_v<T, int16_t> )
    {
        return ImGuiDataType_S16;
    }
    else if constexpr( std::is_same_v<T, int8_t> )
    {
        return ImGuiDataType_S8;
    }
    // unsigned ints
    else if constexpr( std::is_same_v<T, uint64_t> )
    {
        return ImGuiDataType_U64;
    }
    else if constexpr( std::is_same_v<T, uint32_t> )
    {
        return ImGuiDataType_U32;
    }
    else if constexpr( std::is_same_v<T, uint16_t> )
    {
        return ImGuiDataType_U16;
    }
    else if constexpr( std::is_same_v<T, uint8_t> )
    {
        return ImGuiDataType_U8;
    }
}

/**
 * @brief InputScalar_T
 * @param label
 * @param v
 * @param step
 * @param step_fast
 * @param format_
 * @param flags
 * @return
 *
 * Templated function for numerical inputs.
 */
template<typename T>
bool InputScalar_T(const char* label, T* v, T step = std::numeric_limits<T>::max(), T step_fast = std::numeric_limits<T>::max(), const char* format_=nullptr, ImGuiInputTextFlags flags = 0)
{
    auto _step     = step < std::numeric_limits<T>::max() ? &step : nullptr;
    auto _fastStep = step < std::numeric_limits<T>::max() ? &step_fast : nullptr;

    static_assert( std::is_arithmetic_v<T>, "This function specifically requires arithmetic types (float, double, [u]int[8,16,32,64]");
    if constexpr( std::is_same_v<T, float> )
    {
        flags |= ImGuiInputTextFlags_CharsScientific;
        return ImGui::InputScalar(label, toImGuiDataType<T>(), static_cast<void*>(v), static_cast<void*>(_step), static_cast<void*>(_fastStep), format_, flags);
    }
    else if constexpr( std::is_same_v<T, double> )
    {
        return ImGui::InputScalar(label, toImGuiDataType<T>(), static_cast<void*>(v), static_cast<void*>(_step), static_cast<void*>(_fastStep), format_, flags);
    }
    // signed ints
    else if constexpr( std::is_same_v<T, int64_t> )
    {
        const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%016X" : "%ld";
        auto _fmt = format_ == nullptr ? format : format_;
        return ImGui::InputScalar(label, toImGuiDataType<T>(), static_cast<void*>(v), static_cast<void*>(_step), static_cast<void*>(_fastStep), _fmt, flags);
    }
    else if constexpr( std::is_same_v<T, int32_t> )
    {
        const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%08X" : "%d";
        auto _fmt = format_ == nullptr ? format : format_;
        return ImGui::InputScalar(label, toImGuiDataType<T>(), static_cast<void*>(v), static_cast<void*>(_step), static_cast<void*>(_fastStep), _fmt, flags);
    }
    else if constexpr( std::is_same_v<T, int16_t> )
    {
        const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%04X" : "%d";
        auto _fmt = format_ == nullptr ? format : format_;
        return ImGui::InputScalar(label, toImGuiDataType<T>(), static_cast<void*>(v), static_cast<void*>(_step), static_cast<void*>(_fastStep), _fmt, flags);
    }
    else if constexpr( std::is_same_v<T, int8_t> )
    {
        const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%02X" : "%d";
        auto _fmt = format_ == nullptr ? format : format_;
        return ImGui::InputScalar(label, toImGuiDataType<T>(), static_cast<void*>(v), static_cast<void*>(_step), static_cast<void*>(_fastStep), _fmt, flags);
    }
    // unsigned ints
    else if constexpr( std::is_same_v<T, uint64_t> )
    {
        const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%016X" : "%lu";
        auto _fmt = format_ == nullptr ? format : format_;
        return ImGui::InputScalar(label, toImGuiDataType<T>(), static_cast<void*>(v), static_cast<void*>(_step), static_cast<void*>(_fastStep), _fmt, flags);
    }
    else if constexpr( std::is_same_v<T, uint32_t> )
    {
        const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%08X" : "%d";
        auto _fmt = format_ == nullptr ? format : format_;
        return ImGui::InputScalar(label, toImGuiDataType<T>(), static_cast<void*>(v), static_cast<void*>(_step), static_cast<void*>(_fastStep), _fmt, flags);
    }
    else if constexpr( std::is_same_v<T, uint16_t> )
    {
        const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%04X" : "%d";
        auto _fmt = format_ == nullptr ? format : format_;
        return ImGui::InputScalar(label, toImGuiDataType<T>(), static_cast<void*>(v), static_cast<void*>(_step), static_cast<void*>(_fastStep), _fmt, flags);
    }
    else if constexpr( std::is_same_v<T, uint8_t> )
    {
        const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%02X" : "%d";
        auto _fmt = format_ == nullptr ? format : format_;
        return ImGui::InputScalar(label, toImGuiDataType<T>(), static_cast<void*>(v), static_cast<void*>(_step), static_cast<void*>(_fastStep), _fmt, flags);
    }
}

template<typename T>
bool DragScalar_T(const char* label, T* v, float speed, T _min = std::numeric_limits<T>::min(), T _max = std::numeric_limits<T>::max(), const char* format_=nullptr, ImGuiSliderFlags flags = 0)
{
    //auto _step     = step < std::numeric_limits<T>::max() ? &step : nullptr;
    //auto _fastStep = step < std::numeric_limits<T>::max() ? &step_fast : nullptr;
    static_assert( std::is_arithmetic_v<T>, "This function specifically requires arithmetic types (float, double, [u]int[8,16,32,64]");
    return ImGui::DragScalar(label, toImGuiDataType<T>(), v, speed, &_min, &_max, format_,  flags);
}

template<typename T>
bool SliderScalar_T(const char* label, T* v, T _min, T _max, const char* format_=nullptr, ImGuiSliderFlags flags = 0)
{
    //auto _step     = step < std::numeric_limits<T>::max() ? &step : nullptr;
    //auto _fastStep = step < std::numeric_limits<T>::max() ? &step_fast : nullptr;
    static_assert( std::is_arithmetic_v<T>, "This function specifically requires arithmetic types (float, double, [u]int[8,16,32,64]");
    return ImGui::SliderScalar(label, toImGuiDataType<T>(), v, &_min, &_max, format_,  flags);
}

template<typename T>
bool InputScalar_V(const char* label, T* firstElement, size_t count, size_t *itemIndexThatChanged=nullptr, T step = std::numeric_limits<T>::max(), T step_fast = std::numeric_limits<T>::max(), const char* format_=nullptr, ImGuiInputTextFlags flags = 0)
{
    auto p_data = firstElement;
    ImGuiContext& g = *GImGui;
    auto item_width = g.NextItemData.Width;
    bool value_changed = false;

    ImGui::PushID(label);

    for (size_t i = 0; i < count; i++)
    {
        ImGui::PushID(static_cast<int>(i));
        ImGui::SetNextItemWidth(item_width);
        auto changed = InputScalar_T(i == 0 ? label : "", p_data, step, step_fast, format_, flags);
        value_changed |= changed;
        if(changed && itemIndexThatChanged)
        {
            *itemIndexThatChanged = i;
        }
        ImGui::PopID();
        p_data += 1;
    }
    ImGui::PopID();

    return value_changed;
}


}

#endif
