#pragma once
#include "ViewTypes.hpp"
#include "pch.h"

namespace GUI {

class IViewList {
public:
    IViewList() = default;
    virtual ~IViewList() = default;
    virtual void Erase(const ViewID viewID) {}
    virtual void RenderViews() = 0;
};

template <typename T>
class ViewList : public IViewList {
public:
    ViewList() = default;
    ~ViewList() = default;

    void Insert(const T &view) {
        auto existingView = std::find_if(data.begin(), data.end(),
                                         [&](const std::shared_ptr<T> &v) { return v->GetID() == view.GetID(); });
        if (existingView == data.end()) {
            auto newView = std::make_shared<T>(view);
            data.push_back(newView);
            std::cout << "View added! ID: " << newView->GetID() << ", Type ID: " << ViewType<T>() << std::endl;
        } else {
            std::cout << "View already exists! ID: " << view.GetID() << std::endl;
        }
    }

    T &Get(const ViewID viewID) {
        auto view =
            std::find_if(data.begin(), data.end(), [&](const std::shared_ptr<T> &v) { return v->GetID() == viewID; });
        assert(view != data.end() && "View doesn't exist!");
        return *(*view);
    }

    void Erase(const ViewID viewID) override final {
        auto view =
            std::find_if(data.begin(), data.end(), [&](const std::shared_ptr<T> &v) { return v->GetID() == viewID; });
        if (view != data.end()) {
            data.erase(view);
            std::cout << "View erased! ID: " << viewID << ", Type ID: " << ViewType<T>() << std::endl;
        } else {
            std::cout << "No view found with ID: " << viewID << ", Type ID: " << ViewType<T>() << std::endl;
        }
    }

    void RenderViews() override {
        for (auto &view : data) {
            if (view) {
                view->Render();
            }
        }
    }

    // For Debugging
    /*void RenderViews() override {
        std::cout << "ViewList::RenderViews - Starting render, data size: " << data.size() << std::endl;
        for (auto &view : data) {
            if (view) {
                std::cout << "ViewList::RenderViews - Rendering view ID: " << view->GetID() << std::endl;
                view->Render();
                std::cout << "ViewList::RenderViews - Finished rendering view ID: " << view->GetID() << std::endl;
            } else {
                std::cout << "ViewList::RenderViews - Null view found in list" << std::endl;
            }
        }
    }*/

    std::vector<std::shared_ptr<T>> data;
};
} // namespace GUI