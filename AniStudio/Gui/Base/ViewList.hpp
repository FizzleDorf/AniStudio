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

#pragma once
#include "ViewTypes.hpp"
#include "pch.h"

namespace GUI {

class IViewList {
public:
    IViewList() = default;
    virtual ~IViewList() = default;
    virtual void Erase(const ViewListID viewID) {}
    virtual void UpdateViews(const float deltaT) {}
    virtual void RenderViews() = 0;
};

template <typename T>
class ViewList : public IViewList {
public:
    ViewList() = default;
    ~ViewList() = default;

    void Insert(T &&view) {
        // Print ID before any operations
        std::cout << "Attempting to add view with ID: " << view.GetID() << std::endl;

        // First create the shared_ptr to avoid losing the view if find_if throws
        auto newView = std::make_shared<T>(std::forward<T>(view));

        // Check for existing view after creating the new one
        auto existingView =
            std::find_if(data.begin(), data.end(),
                         [id = newView->GetID()](const std::shared_ptr<T> &v) { return v && v->GetID() == id; });

        if (existingView == data.end()) {
            data.push_back(newView);
            std::cout << "View added! ID: " << newView->GetID() << ", Type ID: " << ViewType<T>()
                      << ", Total Views: " << data.size() << std::endl;
        } else {
            std::cout << "View already exists with ID: " << newView->GetID() << std::endl;
        }
    }

    T &Get(const ViewListID viewID) {
        auto view =
            std::find_if(data.begin(), data.end(), [&](const std::shared_ptr<T> &v) { return v->GetID() == viewID; });
        assert(view != data.end() && "View doesn't exist!");
        return *(*view);
    }

    void Erase(const ViewListID viewID) override final {
        auto view =
            std::find_if(data.begin(), data.end(), [&](const std::shared_ptr<T> &v) { return v->GetID() == viewID; });
        if (view != data.end()) {
            data.erase(view);
            std::cout << "View erased! ID: " << viewID << ", Type ID: " << ViewType<T>() << std::endl;
        } else {
            std::cout << "No view found with ID: " << viewID << ", Type ID: " << ViewType<T>() << std::endl;
        }
    }

    void UpdateViews(const float deltaT) override {
        for (auto& view : data) {
            if (view) {
                view->Update(deltaT);
            }
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