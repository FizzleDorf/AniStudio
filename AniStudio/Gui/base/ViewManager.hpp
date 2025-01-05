#pragma once
#include "BaseView.hpp"
#include "ViewList.hpp"
#include "ViewTypes.hpp"
#include "pch.h"

namespace GUI {
class ViewManager {
public:
    ViewManager() : viewCount(0) {
        for (ViewID view = 0u; view < MAX_VIEW_COUNT; view++) {
            availableViews.push(view);
        }
    }

    ~ViewManager() = default;

    void Render() {
        for (const auto &viewList : viewArrays) {
            viewList.second->RenderViews();
        }
    }

    const ViewID AddNewView() {
        const ViewID view = availableViews.front();
        AddViewSignature(view);
        availableViews.pop();
        viewCount++;
        return view;
    }

    void DestroyView(const ViewID view) {
        assert(view < MAX_VIEW_COUNT && "ViewID out of range!");
        viewSignatures.erase(view);

        for (auto &array : viewArrays) {
            array.second->Erase(view);
        }

        viewCount--;
        availableViews.push(view);
        std::cout << "Removed View: " << view << "\n";
    }

    template <typename T, typename... Args>
    void AddView(const ViewID view, Args &&...args) {
        assert(view < MAX_VIEW_COUNT && "ViewID out of range!");

        T viewComponent(std::forward<Args>(args)...);
        viewComponent.viewID = view;
        GetViewSignature(view)->insert(ViewType<T>());
        GetViewList<T>()->Insert(viewComponent);
    }

    template <typename T>
    void RemoveView(const ViewID view) {
        assert(view < MAX_VIEW_COUNT && "ViewID out of range!");
        const ViewTypeID viewType = ViewType<T>();
        viewSignatures.at(view).erase(viewType);
        GetViewList<T>()->Erase(view);
    }

    template <typename T>
    T &GetView(const ViewID view) {
        assert(view < MAX_VIEW_COUNT && "ViewID out of range!");
        const ViewTypeID viewType = ViewType<T>();
        return GetViewList<T>()->Get(view);
    }

    template <typename T>
    const bool HasView(const ViewID view) {
        assert(view < MAX_VIEW_COUNT && "ViewID out of range!");
        auto it = viewSignatures.find(view);
        if (it == viewSignatures.end()) {
            return false;
        }
        const ViewSignature &signature = *(it->second);
        const ViewTypeID viewType = ViewType<T>();
        return (signature.count(viewType) > 0);
    }

    void Reset() {
        for (auto &viewSignaturePair : viewSignatures) {
            DestroyView(viewSignaturePair.first);
        }
        viewSignatures.clear();

        while (!availableViews.empty()) {
            availableViews.pop();
        }
        for (ViewID view = 0u; view < MAX_VIEW_COUNT; ++view) {
            availableViews.push(view);
        }

        viewCount = 0;
    }

    std::vector<ViewID> GetAllViews() const {
        std::vector<ViewID> views;
        for (const auto &pair : viewSignatures) {
            views.push_back(pair.first);
        }
        return views;
    }

private:
    template <typename T>
    void AddViewList() {
        const ViewTypeID viewType = ViewType<T>();
        assert(viewArrays.find(viewType) == viewArrays.end() && "ViewList already registered!");
        viewArrays[viewType] = std::make_shared<ViewList<T>>();
    }

    template <typename T>
    std::shared_ptr<ViewList<T>> GetViewList() {
        const ViewTypeID viewType = ViewType<T>();
        if (viewArrays.count(viewType) == 0) {
            AddViewList<T>();
        }
        return std::static_pointer_cast<ViewList<T>>(viewArrays.at(viewType));
    }

    void AddViewSignature(const ViewID view) {
        assert(viewSignatures.find(view) == viewSignatures.end() && "Signature not found");
        viewSignatures[view] = std::make_shared<ViewSignature>();
    }

    std::shared_ptr<ViewSignature> GetViewSignature(const ViewID view) {
        assert(viewSignatures.find(view) != viewSignatures.end() && "Signature Not Found");
        return viewSignatures.at(view);
    }

private:
    ViewID viewCount;
    std::queue<ViewID> availableViews;
    std::map<ViewID, std::shared_ptr<ViewSignature>> viewSignatures;
    std::map<ViewTypeID, std::shared_ptr<IViewList>> viewArrays;
};

extern ViewManager viewMgr;
} // namespace GUI