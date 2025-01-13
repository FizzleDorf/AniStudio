#pragma once
#include <iostream>
#include <set>
#include <type_traits>
#include <typeinfo>
#include "DLLDefines.hpp"

namespace GUI {
class BaseView;

// Constants
const size_t MAX_VIEW_COUNT = 100;

// Custom Types
using ViewID = size_t;
using ViewTypeID = size_t;
using ViewSignature = std::set<ViewTypeID>;

// Global counter for ViewType IDs
ANI_API ViewTypeID GetRuntimeViewTypeID();

// Attach type ID to view class and return it
template <typename T>
inline static const ViewTypeID ViewType() noexcept {
    static_assert((std::is_base_of<BaseView, T>::value && !std::is_same<BaseView, T>::value), "INVALID VIEW TYPE");
    static const ViewTypeID typeID = GetRuntimeViewTypeID();
    std::cout << "Generated ViewTypeID for " << typeid(T).name() << ": " << typeID << std::endl;
    return typeID;
}
} // namespace GUI