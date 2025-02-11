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

        // Remove this view's signature
        viewSignatures.erase(viewList);

        // Remove this view from all view arrays
        for (auto &array : viewArrays) {
            array.second->Erase(viewList);
        }

        // Return the ID to the available pool
        viewListCount--;
        availableViews.push(viewList);
    }

    template <typename T>
    void AddView(const ViewListID viewList, T &&view) {
        assert(viewList < MAX_VIEW_COUNT && "ViewListID out of range!");
        assert(GetViewSignature(viewList)->size() < MAX_VIEW_COUNT && "View count limit reached!");

        // Check if view already exists
        if (HasView<T>(viewList)) {
            std::cerr << "View with ID " << viewList << " already exists! Skipping AddView." << std::endl;
            return;
        }

        // Set the view's ID and add it to signatures
        view.viewID = viewList;
        GetViewSignature(viewList)->insert(ViewType<T>());

        // Add to appropriate view list
        auto &viewListPtr = GetViewList<T>();
        std::cout << "Adding view - ID: " << viewList << ", Type: " << typeid(T).name() << std::endl;
        viewListPtr->Insert(std::move(view));

        // Update view count
        viewListCount++;
    }

    template <typename T>
    void RemoveView(const ViewListID viewList) {
        assert(viewList < MAX_VIEW_COUNT && "ViewListID out of range!");

        // Remove from signatures
        const ViewTypeID viewType = ViewType<T>();
        GetViewSignature(viewList)->erase(viewType);

        // Remove from view list
        GetViewList<T>()->Erase(viewList);
    }

    template <typename T>
    T &GetView(const ViewListID viewList) {
        assert(viewList < MAX_VIEW_COUNT && "ViewListID out of range!");
        return GetViewList<T>()->Get(viewList);
    }

    template <typename T>
    const bool HasView(const ViewListID viewList) {
        assert(viewList < MAX_VIEW_COUNT && "ViewListID out of range!");

        auto it = viewSignatures.find(viewList);
        if (it == viewSignatures.end()) {
            return false;
        }

        const ViewSignature &signature = *(it->second);
        const ViewTypeID viewType = ViewType<T>();
        return (signature.count(viewType) > 0);
    }

    // View Type Registration
    template <typename T>
    void RegisterViewType(const std::string &name) {
        ViewTypeID typeId = ViewType<T>();
        registeredViews[name] = typeId;
        std::cout << "Registered view type: " << name << " with ID: " << typeId << std::endl;
    }

    ViewTypeID GetViewType(const std::string &name) const {
        auto it = registeredViews.find(name);
        if (it != registeredViews.end()) {
            return it->second;
        }
        throw std::runtime_error("View type not registered: " + name);
    }

    // State Management
    void Reset() {
        // Destroy all views
        for (auto &viewSignaturePair : viewSignatures) {
            DestroyView(viewSignaturePair.first);
        }
        viewSignatures.clear();

        // Reset available views queue
        while (!availableViews.empty()) {
            availableViews.pop();
        }
        for (ViewListID view = 0u; view < MAX_VIEW_COUNT; ++view) {
            availableViews.push(view);
        }

        viewListCount = 0;
        viewArrays.clear();
        registeredViews.clear();
    }

    // Serialization
    void SaveState(const std::string &filepath) {
        nlohmann::json j;
        j["viewCount"] = viewListCount;

        // Save view signatures
        j["signatures"] = nlohmann::json::array();
        for (const auto &[id, signature] : viewSignatures) {
            nlohmann::json sigJson;
            sigJson["id"] = id;
            sigJson["types"] = nlohmann::json::array();
            for (const auto &type : *signature) {
                sigJson["types"].push_back(type);
            }
            j["signatures"].push_back(sigJson);
        }

        // Write to file
        std::ofstream file(filepath);
        if (file.is_open()) {
            file << j.dump(4);
        }
    }

    void LoadState(const std::string &filepath) {
        std::ifstream file(filepath);
        if (!file.is_open())
            return;

        try {
            nlohmann::json j;
            file >> j;

            Reset();

            viewListCount = j["viewCount"];

            // Restore signatures
            for (const auto &sigJson : j["signatures"]) {
                ViewListID id = sigJson["id"].get<ViewListID>();
                AddViewSignature(id);

                // Load types into a vector first
                std::vector<ViewTypeID> typeIds = sigJson["types"].get<std::vector<ViewTypeID>>();

                // Then insert each type ID into the signature
                for (const auto &typeId : typeIds) {
                    GetViewSignature(id)->insert(typeId);
                }
            }

        } catch (const std::exception &e) {
            std::cerr << "Error loading view state: " << e.what() << std::endl;
        }
    }

    // Render all views
    void Render() {
        for (const auto &viewList : viewArrays) {
            viewList.second->RenderViews();
        }
    }

    // Get all view IDs
    std::vector<ViewListID> GetAllViews() const {
        std::vector<ViewListID> views;
        for (const auto &pair : viewSignatures) {
            views.push_back(pair.first);
        }
        return views;
    }

    const std::unordered_map<std::string, ViewTypeID> &GetRegisteredViews() const { return registeredViews; }

    const std::map<ViewListID, std::shared_ptr<ViewSignature>> &GetViewSignatures() const { return viewSignatures; }

    void AddViewByType(const ViewListID viewList, const ViewTypeID viewType) {
        assert(viewList < MAX_VIEW_COUNT && "ViewListID out of range!");
        GetViewSignature(viewList)->insert(viewType);
    }

    void RemoveViewByType(const ViewListID viewList, const ViewTypeID viewType) {
        assert(viewList < MAX_VIEW_COUNT && "ViewListID out of range!");
        GetViewSignature(viewList)->erase(viewType);
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
        assert(viewSignatures.find(viewList) == viewSignatures.end() && "Signature already exists");
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