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

#include "ViewManager.hpp"
#include <iostream>
#include <cassert>

namespace GUI {

	ViewManager::ViewManager() : workspaceCount(0) {
		// Initialize available view IDs
		for (WorkspaceID view = 0u; view < MAX_VIEW_COUNT; view++) {
			availableWorkspaces.push(view);
		}
	}

	void ViewManager::Init() {
		// Initialization logic if needed
	}

	void ViewManager::Update(const float deltaT) {
		for (const auto& workspace : workspaceArrays) {
			workspace.second->UpdateViews(deltaT);
		}
		// Update generic views
		UpdateGenericWorkspaces(deltaT);
	}

	void ViewManager::Render() {
		for (const auto &workspace : workspaceArrays) {
			workspace.second->RenderViews();
		}
		// Render generic views
		RenderGenericWorkspaces();
	}

	const WorkspaceID ViewManager::CreateView() {
		const WorkspaceID viewList = availableWorkspaces.front();
		AddViewSignature(viewList);
		availableWorkspaces.pop();
		workspaceCount++;
		return viewList;
	}

	void ViewManager::DestroyView(const WorkspaceID viewList) {
		assert(viewList < MAX_VIEW_COUNT && "WorkspaceID out of range!");

		// Remove this view's signature
		workspaceSignatures.erase(viewList);

		// Remove this view from all view arrays
		for (auto &array : workspaceArrays) {
			array.second->Erase(viewList);
		}

		// Remove from generic views if it's there
		genericWorkspaces.erase(viewList);

		// Return the ID to the available pool
		workspaceCount--;
		availableWorkspaces.push(viewList);
	}

	void ViewManager::RegisterViewWithFactory(const std::string& name, const std::string& source,
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

	WorkspaceID ViewManager::CreateViewByName(const std::string& viewTypeName, ECS::EntityManager& entityMgr) {
		auto factoryIt = viewFactories.find(viewTypeName);
		if (factoryIt == viewFactories.end()) {
			std::cerr << "[ViewManager] No factory registered for view type: " << viewTypeName << std::endl;
			return 0; // Invalid WorkspaceID
		}

		try {
			// Create the WorkspaceID
			WorkspaceID id = CreateView();

			// Create the view instance using the factory
			auto view = factoryIt->second(entityMgr);
			if (!view) {
				DestroyView(id);
				return 0;
			}

			// Set the view ID
			view->workspaceID = id;

			// Initialize the view immediately
			view->Init();

			// Store in generic storage
			genericWorkspaces[id] = std::move(view);

			std::cout << "[ViewManager] Created and initialized view: " << viewTypeName << " with ID: " << id << std::endl;
			return id;
		}
		catch (const std::exception& e) {
			std::cerr << "[ViewManager] Failed to create view " << viewTypeName << ": " << e.what() << std::endl;
			return 0;
		}
	}

	void ViewManager::UnregisterViewSource(const std::string& source) {
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

	ViewMetadata ViewManager::GetViewMetadata(const std::string& viewTypeName) const {
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

	std::vector<std::string> ViewManager::GetViewsByCategory(const std::string& category) const {
		std::vector<std::string> viewsInCategory;

		for (const auto&[viewTypeName, typeID] : registeredViews) {
			ViewMetadata meta = GetViewMetadata(viewTypeName);
			if (meta.category == category) {
				viewsInCategory.push_back(viewTypeName);
			}
		}

		return viewsInCategory;
	}

	std::vector<std::string> ViewManager::GetViewCategories() const {
		std::set<std::string> categories;

		for (const auto&[viewTypeName, typeID] : registeredViews) {
			ViewMetadata meta = GetViewMetadata(viewTypeName);
			categories.insert(meta.category);
		}

		return std::vector<std::string>(categories.begin(), categories.end());
	}

	std::vector<std::string> ViewManager::GetViewsBySource(const std::string& source) const {
		std::vector<std::string> views;
		for (const auto&[viewTypeName, viewSource] : viewSources) {
			if (viewSource == source) {
				views.push_back(viewTypeName);
			}
		}
		return views;
	}

	void ViewManager::UpdateGenericWorkspaces(float deltaT) {
		for (auto&[viewID, view] : genericWorkspaces) {
			view->Update(deltaT);
		}
	}

	void ViewManager::RenderGenericWorkspaces() {
		for (auto&[viewID, view] : genericWorkspaces) {
			view->Render();
		}
	}

	ViewTypeID ViewManager::GetViewType(const std::string &name) const {
		auto it = registeredViews.find(name);
		if (it != registeredViews.end()) {
			return it->second;
		}
		throw std::runtime_error("View type not registered: " + name);
	}

	void ViewManager::Reset() {
		std::cout << "[ViewManager] Performing soft reset (retaining registered views)..." << std::endl;

		// Reset workspace-related data only
		ResetWorkspaceData();

		// Re-initialize
		Init();

		std::cout << "[ViewManager] Soft reset complete. Registered views retained." << std::endl;
	}

	void ViewManager::FullReset() {
		std::cout << "[ViewManager] Performing full reset..." << std::endl;

		// Reset workspace data
		ResetWorkspaceData();

		// Reset registration data
		ResetRegistrationData();

		// Re-initialize
		Init();

		std::cout << "[ViewManager] Full reset complete." << std::endl;
	}

	std::vector<WorkspaceID> ViewManager::GetAllWorkspaces() const {
		std::vector<WorkspaceID> ids;
		for (const auto &pair : workspaceSignatures) {
			ids.push_back(pair.first);
		}
		return ids;
	}

	const std::unordered_map<std::string, ViewTypeID> &ViewManager::GetRegisteredViews() const {
		return registeredViews;
	}

	const std::map<WorkspaceID, std::shared_ptr<ViewSignature>> &ViewManager::GetWorkspaceSignatures() const {
		return workspaceSignatures;
	}

	void ViewManager::AddViewByType(const WorkspaceID viewList, const ViewTypeID viewType) {
		assert(viewList < MAX_VIEW_COUNT && "WorkspaceID out of range!");
		GetViewSignature(viewList)->insert(viewType);
	}

	void ViewManager::RemoveViewByType(const WorkspaceID viewList, const ViewTypeID viewType) {
		assert(viewList < MAX_VIEW_COUNT && "WorkspaceID out of range!");
		GetViewSignature(viewList)->erase(viewType);
	}

	json ViewManager::SerializeViewLists() const {
		json viewListsJson;
		for (const auto &[WorkspaceID, signature] : workspaceSignatures) {
			json viewListJson;
			viewListJson["WorkspaceID"] = WorkspaceID;

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

	void ViewManager::DeserializeViewLists(const json &viewListsJson) {
		for (const auto &viewListJson : viewListsJson) {
			WorkspaceID WorkspaceID = viewListJson["WorkspaceID"];
			AddViewSignature(WorkspaceID); // Create a new ViewList

			for (const auto &viewJson : viewListJson["Views"]) {
				ViewTypeID viewTypeID = viewJson["ViewTypeID"];
				AddViewByType(WorkspaceID, viewTypeID); // Add the view to the ViewList
			}
		}
	}

	void ViewManager::AddViewSignature(const WorkspaceID viewList) {
		assert(workspaceSignatures.find(viewList) == workspaceSignatures.end() && "Signature already exists");
		workspaceSignatures[viewList] = std::make_shared<ViewSignature>();
	}

	std::shared_ptr<ViewSignature> ViewManager::GetViewSignature(const WorkspaceID viewList) {
		assert(workspaceSignatures.find(viewList) != workspaceSignatures.end() && "Signature Not Found");
		return workspaceSignatures.at(viewList);
	}

	void ViewManager::ResetWorkspaceData() {
		// Destroy all views
		for (auto &viewSignaturePair : workspaceSignatures) {
			DestroyView(viewSignaturePair.first);
		}
		workspaceSignatures.clear();
		genericWorkspaces.clear();

		// Reset available views queue
		while (!availableWorkspaces.empty()) {
			availableWorkspaces.pop();
		}
		for (WorkspaceID view = 0u; view < MAX_VIEW_COUNT; ++view) {
			availableWorkspaces.push(view);
		}

		workspaceCount = 0;
		workspaceArrays.clear();
	}

	void ViewManager::ResetRegistrationData() {
		registeredViews.clear();
		viewMetadata.clear();
		viewSources.clear();
		viewFactories.clear();
	}

} // namespace GUI