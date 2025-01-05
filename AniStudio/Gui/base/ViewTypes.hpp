#pragma once
#include <set>

namespace GUI {
class BaseView;

// Constants
const size_t MAX_VIEW_COUNT = 100;

// Custom Types
using ViewID = size_t;
using ViewTypeID = size_t;
using ViewSignature = std::set<ViewTypeID>;

// Return view runtime type id
inline static const ViewTypeID GetRuntimeViewTypeID() {
    static ViewTypeID typeID = 0u;
    return typeID++;
}

// Attach type id to view class and return it
template <typename T>
inline static const ViewTypeID ViewType() noexcept {
    static_assert((std::is_base_of<BaseView, T>::value && !std::is_same<BaseView, T>::value), "INVALID VIEW TYPE");
    static const ViewTypeID typeID = GetRuntimeViewTypeID();
    return typeID;
}
} // namespace UI