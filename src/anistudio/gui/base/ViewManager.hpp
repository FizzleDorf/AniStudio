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
#include "BaseView.hpp"
#include "ViewList.hpp"
#include "pch.h"

using json = nlohmann::json;

namespace GUI {

	// Factory function type
	using ViewCreationCallback = std::function<std::unique_ptr<BaseView>(ECS::EntityManager&)>;

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
			// Update generic views
			UpdateGenericViews(deltaT);
		}

		// Render all views
		void Render() {
			for (const auto &viewList : viewArrays) {
				viewList.second->RenderViews();
			}
			// Render generic views
			RenderGenericViews();
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

			// Remove from generic views if it's there
			genericViews.erase(viewList);

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

			// Initialize the view before adding it
			view.Init();

			// Add to appropriate view list
			auto viewListPtr = GetViewList<T>();
			std::cout << "Adding and initializing view - ID: " << viewList << ", Type: " << typeid(T).name() << std::endl;
			viewListPtr->Insert(std::forward<T>(view));
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

		// Registers a view by string name WITH FACTORY AND METADATA
		template <typename T>
		void RegisterView(const std::string &name, const std::string& source = "Core") {
			ViewTypeID typeId = ViewType<T>();
			registeredViews[name] = typeId;

			// DEBUG: Test the metadata during registration
			std::cout << "[DEBUG REG] Registering " << name << " (type: " << typeid(T).name() << ")" << std::endl;

			const char* json = T::GetMetadataJSON();
			std::cout << "[DEBUG REG] JSON: " << json << std::endl;

			// Use the template method to get the correct metadata
			auto testMeta = BaseView::GetMetadataFor<T>();
			std::cout << "[DEBUG REG] Parsed: " << testMeta.displayName << " [" << testMeta.category << "] - " << testMeta.description << std::endl;

			// Register metadata getter using the template method
			viewMetadata[name] = []() -> ViewMetadata {
				return BaseView::GetMetadataFor<T>();
			};
			viewSources[name] = source;

			// Register factory function
			viewFactories[name] = [](ECS::EntityManager& mgr) -> std::unique_ptr<BaseView> {
				return std::make_unique<T>(mgr);
			};

			std::cout << "Registered view type: " << name << " with ID: " << typeId
				<< " from source: " << source << std::endl;
		}

		// Register view with custom factory (for views with special constructors)
		void RegisterViewWithFactory(const std::string& name, const std::string& source,
			ViewCreationCallback factory, std::function<ViewMetadata()> metadataGetter) {
			// Generate a dummy type ID for non-template views
			static ViewTypeID nextCustomTypeId = 10000; // Start high to avoid conflicts
			ViewTypeID typeId = nextCustomTypeId++;

			registeredViews[name] = typeId;
			viewSources[name] = source;
			viewFactories[name] = factory;
			viewMetadata[name] = metadataGetter;

			std::cout << "Registered custom view type: " << name << " with ID: " << typeId
				<< " from source: " << source << std::endl;
		}

		// Create view by name using factory
		ViewListID CreateViewByName(const std::string& viewTypeName, ECS::EntityManager& entityMgr) {
			auto factoryIt = viewFactories.find(viewTypeName);
			if (factoryIt == viewFactories.end()) {
				std::cerr << "[ViewManager] No factory registered for view type: " << viewTypeName << std::endl;
				return 0; // Invalid ViewListID
			}

			try {
				// Create the ViewListID
				ViewListID viewID = CreateView();

				// Create the view instance using the factory
				auto view = factoryIt->second(entityMgr);
				if (!view) {
					DestroyView(viewID);
					return 0;
				}

				// Set the view ID
				view->viewID = viewID;

				// Initialize the view immediately
				view->Init();

				// Store in generic storage
				genericViews[viewID] = std::move(view);

				std::cout << "[ViewManager] Created and initialized view: " << viewTypeName << " with ID: " << viewID << std::endl;
				return viewID;
			}
			catch (const std::exception& e) {
				std::cerr << "[ViewManager] Failed to create view " << viewTypeName << ": " << e.what() << std::endl;
				return 0;
			}
		}

		// Unregister views from a source (for plugin cleanup)
		void UnregisterViewSource(const std::string& source) {
			auto it = viewSources.begin();
			while (it != viewSources.end()) {
				if (it->second == source) {
					const std::string& viewName = it->first;
					std::cout << "Unregistering view: " << viewName
						<< " from source: " << source << std::endl;

					// Remove from all tracking maps
					registeredViews.erase(viewName);
					viewMetadata.erase(viewName);
					viewFactories.erase(viewName);
					it = viewSources.erase(it);
				}
				else {
					++it;
				}
			}
		}

		// Get metadata for a view type
		ViewMetadata GetViewMetadata(const std::string& viewTypeName) const {
			auto it = viewMetadata.find(viewTypeName);
			if (it != viewMetadata.end()) {
				return it->second();
			}

			// Return default metadata for unknown types
			ViewMetadata meta;
			meta.displayName = viewTypeName;
			meta.category = "Unknown";
			meta.description = "";
			return meta;
		}

		// Get all view types in a category
		std::vector<std::string> GetViewsByCategory(const std::string& category) const {
			std::vector<std::string> viewsInCategory;

			for (const auto&[viewTypeName, typeID] : registeredViews) {
				ViewMetadata meta = GetViewMetadata(viewTypeName);
				if (meta.category == category) {
					viewsInCategory.push_back(viewTypeName);
				}
			}

			return viewsInCategory;
		}

		// Get all categories
		std::vector<std::string> GetViewCategories() const {
			std::set<std::string> categories;

			for (const auto&[viewTypeName, typeID] : registeredViews) {
				ViewMetadata meta = GetViewMetadata(viewTypeName);
				categories.insert(meta.category);
			}

			return std::vector<std::string>(categories.begin(), categories.end());
		}

		// Get views by source
		std::vector<std::string> GetViewsBySource(const std::string& source) const {
			std::vector<std::string> views;
			for (const auto&[viewTypeName, viewSource] : viewSources) {
				if (viewSource == source) {
					views.push_back(viewTypeName);
				}
			}
			return views;
		}

		// Update generic views
		void UpdateGenericViews(float deltaT) {
			for (auto&[viewID, view] : genericViews) {
				view->Update(deltaT);
			}
		}

		// Render generic views
		void RenderGenericViews() {
			for (auto&[viewID, view] : genericViews) {
				view->Render();
			}
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
			genericViews.clear();

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
			viewMetadata.clear();
			viewSources.clear();
			viewFactories.clear();
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

		// Metadata and creation tracking
		std::unordered_map<std::string, std::function<ViewMetadata()>> viewMetadata;
		std::unordered_map<std::string, std::string> viewSources;
		std::unordered_map<std::string, ViewCreationCallback> viewFactories;

		// Generic view storage for views created by name
		std::unordered_map<ViewListID, std::unique_ptr<BaseView>> genericViews;
	};

} // namespace GUI