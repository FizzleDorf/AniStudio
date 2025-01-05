#pragma once
#include "ViewTypes.hpp"
#include "pch.h"

namespace GUI {

class IViewList {
public:
    IViewList() = default;
    virtual ~IViewList() = default;
    virtual void Erase(const ViewID viewID) {}
    virtual void RenderViews() const = 0;
};

template <typename T>
class ViewList : public IViewList {
public:
    ViewList() = default;
    ~ViewList() = default;

    void Insert(const T &view) {
        auto existingView =
            std::find_if(data.begin(), data.end(), [&](const T &v) { return v.GetID() == view.GetID(); });
        if (existingView == data.end()) {
            data.push_back(view);
            std::cout << "View added! ID: " << view.GetID() << ", Type ID: " << ViewType<T>() << std::endl;
        } else {
            std::cout << "View already exists! ID: " << view.GetID() << std::endl;
        }
    }

    T &Get(const ViewID viewID) {
        auto view = std::find_if(data.begin(), data.end(), [&](const T &v) { return v.GetID() == viewID; });
        assert(view != data.end() && "View doesn't exist!");
        return *view;
    }

    void Erase(const ViewID viewID) override final {
        auto view = std::find_if(data.begin(), data.end(), [&](const T &v) { return v.GetID() == viewID; });
        if (view != data.end()) {
            data.erase(view);
            std::cout << "View erased! ID: " << viewID << ", Type ID: " << ViewType<T>() << std::endl;
        } else {
            std::cout << "No view found with ID: " << viewID << ", Type ID: " << ViewType<T>() << std::endl;
        }
    }

    void RenderViews() const override {
        for (const auto &view : data) {
            view.Render();
        }
    }

    std::vector<T> data;
};
} // namespace GUI