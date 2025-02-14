#pragma once
#include <set>

namespace GUI {
class BaseView;

const size_t MAX_VIEW_COUNT = 100;

using ViewListID = size_t;
using ViewTypeID = size_t;
using ViewSignature = std::set<ViewTypeID>;

// Non-template function - implemented in TypesImpl.cpp
static ViewTypeID GetRuntimeViewTypeID() {
    static ViewTypeID typeID = 0u;
    return typeID++;
}

// Template function - must be implemented in header
template <typename T>
inline static const ViewTypeID ViewType() noexcept {
    static_assert((std::is_base_of<BaseView, T>::value && !std::is_same<BaseView, T>::value), "INVALID VIEW TYPE");
    static const ViewTypeID typeID = GetRuntimeViewTypeID();
    return typeID;
}
} // namespace GUI