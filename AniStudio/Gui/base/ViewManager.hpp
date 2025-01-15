#pragma once
#include "BaseView.hpp"
#include "ViewList.hpp"
#include "ViewTypes.hpp"
#include "pch.h"

namespace GUI {

class ViewManager {
public:
    ViewManager() : viewListCount(0) {
        for (ViewListID viewList = 0u; viewList < MAX_VIEW_COUNT; viewList++) {
            availableViews.push(viewList);
        }
    }

    ~ViewManager() = default;

    void Init() {}

    const ViewListID CreateView() {
        const ViewListID viewList = availableViews.front();
        AddViewSignature(viewList);
        availableViews.pop();
        viewListCount++;
        return viewList;
    }

    void DestroyView(const ViewListID viewList) {
        assert(viewList < MAX_VIEW_COUNT && "ViewListID out of range!");
        viewSignatures.erase(viewList);

        for (auto &array : viewArrays) {
            array.second->Erase(viewList);
        }

        viewListCount--;
        availableViews.push(viewList);

    }

    template <typename T>
    void AddView(const ViewListID viewList, T &&view) {
        assert(viewList < MAX_VIEW_COUNT && "ViewListID out of range!");

        if (HasView<T>(viewList)) {
            std::cerr << "View with ID " << viewList << " already exists! Skipping AddView." << std::endl;
            return;
        }

        view.viewID = viewList;
        GetViewSignature(viewList)->insert(ViewType<T>());
        auto &viewListPtr = GetViewList<T>();

        std::cout << "Adding view - ID: " << viewList << ", Type: " << typeid(T).name() << std::endl;
        viewListPtr->Insert(std::move(view));
    }

    template <typename T>
    void RemoveView(const ViewListID viewList) {
        assert(viewList < MAX_VIEW_COUNT && "ViewListID out of range!");
        const ViewTypeID viewType = ViewType<T>();
        viewSignatures.at(viewList).erase(viewType);
        GetViewList<T>()->Erase(viewList);
    }

    template <typename T>
    T &GetView(const ViewListID viewList) {
        assert(view < MAX_VIEW_COUNT && "ViewListID out of range!");
        return GetViewList<T>()->Get(viewList);
    }

    template <typename T>
    const bool HasView(const ViewListID viewList) {
        assert(view < MAX_VIEW_COUNT && "ViewListID out of range!");
        auto it = viewSignatures.find(viewList);
        if (it == viewSignatures.end()) {
            return false;
        }
        const ViewSignature &signature = *(it->second);
        const ViewTypeID viewType = ViewType<T>();
        return (signature.count(viewType) > 0);
    }


    // View Rendering
    void Render() {
        for (const auto &viewList : viewArrays) {
            viewList.second->RenderViews();
        }
    }

    // State Management
    void Reset() {
        for (auto &viewSignaturePair : viewSignatures) {
            DestroyView(viewSignaturePair.first);
        }
        viewSignatures.clear();

        while (!availableViews.empty()) {
            availableViews.pop();
        }
        for (ViewListID view = 0u; view < MAX_VIEW_COUNT; ++view) {
            availableViews.push(view);
        }

        viewListCount = 0;
    }

    std::vector<ViewListID> GetAllViews() const {
        std::vector<ViewListID> views;
        for (const auto &pair : viewSignatures) {
            views.push_back(pair.first);
        }
        return views;
    }

    // View Type Registration
    template <typename T>
    void RegisterView(const ViewListID viewList, T &view) {
        assert(view < MAX_VIEW_COUNT && "ViewListID out of range!");
        component.viewID = viewList;
        GetViewSignature(viewList)->insert(ViewType<T>());
        GetViewList<T>()->Insert(view);
    }

    // Get registered view type by name
    ViewTypeID GetViewType(const std::string &name) const {
        auto it = registeredViews.find(name);
        if (it != registeredViews.end()) {
            return it->second;
        }
        throw std::runtime_error("View type not registered: " + name);
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

    void AddViewSignature(const ViewListID viewList) {
        assert(viewSignatures.find(viewList) == viewSignatures.end() && "Signature not found");
        viewSignatures[viewList] = std::make_shared<ViewSignature>();
    }

    std::shared_ptr<ViewSignature> GetViewSignature(const ViewListID viewList) {
        assert(viewSignatures.find(viewList) != viewSignatures.end() && "Signature Not Found");
        return viewSignatures.at(viewList);
    }

private:
    ViewListID viewListCount;
    std::queue<ViewListID> availableViews;
    std::map<ViewListID, std::shared_ptr<ViewSignature>> viewSignatures;
    std::map<ViewTypeID, std::shared_ptr<IViewList>> viewArrays;
    std::unordered_map<std::string, ViewTypeID> registeredViews;
};

} // namespace GUI