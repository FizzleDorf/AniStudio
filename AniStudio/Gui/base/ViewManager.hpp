#pragma once
#include "BaseView.hpp"
#include "ViewList.hpp"
#include "ViewTypes.hpp"
#include "pch.h"

using json = nlohmann::json;

namespace GUI {

	class ViewManager {
	public:
		ViewManager() : viewListCount(0) {
			// Initialize available view IDs
			for (ViewListID view = 0u; view < MAX_VIEW_COUNT; view++) {
				availableViews.push(view);
			}
		}

		~ViewManager() = default;

		void Init() {}

		// Update all view lists
		void Update(const float deltaT) {
			for (const auto& viewList : viewArrays) {
				viewList.second->UpdateViews(deltaT);
			}
		}

		// Render all views
		void Render() {
			for (const auto &viewList : viewArrays) {
				viewList.second->RenderViews();
			}
		}

		// Adds a viewlist
		const ViewListID CreateView() {
			const ViewListID viewList = availableViews.front();
			AddViewSignature(viewList);
			availableViews.pop();
			viewListCount++;
			return viewList;
		}

		// Removes a viewlist and all of its views
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

		// Adds a view to the viewlist by template class
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
			auto viewListPtr = GetViewList<T>();
			std::cout << "Adding view - ID: " << viewList << ", Type: " << typeid(T).name() << std::endl;
			viewListPtr->Insert(std::forward<T>(view));

			// Note: We don't initialize the view here - it will be initialized separately
		}

		// Removes a view from the viewlist by template class
		template <typename T>
		void RemoveView(const ViewListID viewList) {
			assert(viewList < MAX_VIEW_COUNT && "ViewListID out of range!");

			// Remove from signatures
			const ViewTypeID viewType = ViewType<T>();
			GetViewSignature(viewList)->erase(viewType);

			// Remove from view list
			GetViewList<T>()->Erase(viewList);
		}

		// Returns a designated view type by template class
		template <typename T>
		T &GetView(const ViewListID viewList) {
			assert(viewList < MAX_VIEW_COUNT && "ViewListID out of range!");
			return GetViewList<T>()->Get(viewList);
		}

		// Returns true if view is in a view list
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

		// Registers a view by string name
		template <typename T>
		void RegisterViewType(const std::string &name) {
			ViewTypeID typeId = ViewType<T>();
			registeredViews[name] = typeId;
			std::cout << "Registered view type: " << name << " with ID: " << typeId << std::endl;
		}

		// Returns a designated view type by string name
		ViewTypeID GetViewType(const std::string &name) const {
			auto it = registeredViews.find(name);
			if (it != registeredViews.end()) {
				return it->second;
			}
			throw std::runtime_error("View type not registered: " + name);
		}

		// State Management

		// Resets to init state
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
			Init();
		}

		// Return all view IDs
		std::vector<ViewListID> GetAllViews() const {
			std::vector<ViewListID> views;
			for (const auto &pair : viewSignatures) {
				views.push_back(pair.first);
			}
			return views;
		}

		// Returns all registered view names and types
		const std::unordered_map<std::string, ViewTypeID> &GetRegisteredViews() const { return registeredViews; }

		// Returns all view list IDs and view signatures
		const std::map<ViewListID, std::shared_ptr<ViewSignature>> &GetViewSignatures() const { return viewSignatures; }

		// Adds a view in the view list by type
		void AddViewByType(const ViewListID viewList, const ViewTypeID viewType) {
			assert(viewList < MAX_VIEW_COUNT && "ViewListID out of range!");
			GetViewSignature(viewList)->insert(viewType);
		}

		// Removes a view in the view list by type
		void RemoveViewByType(const ViewListID viewList, const ViewTypeID viewType) {
			assert(viewList < MAX_VIEW_COUNT && "ViewListID out of range!");
			GetViewSignature(viewList)->erase(viewType);
		}

		// Serialization

		// Serialize JSON into view lists
		json SerializeViewLists() const {
			json viewListsJson;
			for (const auto &[viewListID, signature] : viewSignatures) {
				json viewListJson;
				viewListJson["ViewListID"] = viewListID;

				json viewsJson;
				for (const auto &viewTypeID : *signature) {
					json viewJson;
					viewJson["ViewTypeID"] = viewTypeID;
					viewsJson.push_back(viewJson);
				}
				viewListJson["Views"] = viewsJson;

				viewListsJson.push_back(viewListJson);
			}
			return viewListsJson;
		}

		// Deserialize JSON into view lists
		void DeserializeViewLists(const json &viewListsJson) {
			for (const auto &viewListJson : viewListsJson) {
				ViewListID viewListID = viewListJson["ViewListID"];
				AddViewSignature(viewListID); // Create a new ViewList

				for (const auto &viewJson : viewListJson["Views"]) {
					ViewTypeID viewTypeID = viewJson["ViewTypeID"];
					AddViewByType(viewListID, viewTypeID); // Add the view to the ViewList
				}
			}
		}

	private:
		// Adds a new view list
		template <typename T>
		void AddViewList() {
			const ViewTypeID viewType = ViewType<T>();
			assert(viewArrays.find(viewType) == viewArrays.end() && "ViewList already registered!");
			viewArrays[viewType] = std::make_shared<ViewList<T>>();
		}

		// Returns all view lists
		template <typename T>
		std::shared_ptr<ViewList<T>> GetViewList() {
			const ViewTypeID viewType = ViewType<T>();
			if (viewArrays.count(viewType) == 0) {
				AddViewList<T>();
			}
			return std::static_pointer_cast<ViewList<T>>(viewArrays.at(viewType));
		}

		// Adds a new view signature if it doesn't exist
		void AddViewSignature(const ViewListID viewList) {
			assert(viewSignatures.find(viewList) == viewSignatures.end() && "Signature already exists");
			viewSignatures[viewList] = std::make_shared<ViewSignature>();
		}

		// Returns view signatures
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