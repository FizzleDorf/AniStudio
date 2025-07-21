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
#include "Workspace.hpp"
#include "pch.h"

using json = nlohmann::json;

namespace GUI {

	// Factory function type
	using ViewCreationCallback = std::function<std::unique_ptr<BaseView>(ECS::EntityManager&)>;

	class ViewManager {
	public:
		ViewManager();
		~ViewManager() = default;

		void Init();

		// Update all view lists
		void Update(const float deltaT);

		// Render all views
		void Render();

		// Adds a viewlist
		const WorkspaceID CreateView();

		// Removes a viewlist and all of its views
		void DestroyView(const WorkspaceID viewList);

		// Adds a view to the viewlist by template class
		template <typename T>
		void AddView(const WorkspaceID viewList, T &&view);

		// Removes a view from the viewlist by template class
		template <typename T>
		void RemoveView(const WorkspaceID viewList);

		// Returns a designated view type by template class
		template <typename T>
		T &GetView(const WorkspaceID viewList);

		// Returns true if view is in a view list
		template <typename T>
		const bool HasView(const WorkspaceID viewList);

		// Registers a view by string name WITH FACTORY AND METADATA
		template <typename T>
		void RegisterView(const std::string &name, const std::string& source = "Core");

		// Register view with custom factory (for views with special constructors)
		void RegisterViewWithFactory(const std::string& name, const std::string& source,
			ViewCreationCallback factory, std::function<ViewMetadata()> metadataGetter);

		// Create view by name using factory
		WorkspaceID CreateViewByName(const std::string& viewTypeName, ECS::EntityManager& entityMgr);

		// Unregister views from a source (for plugin cleanup)
		void UnregisterViewSource(const std::string& source);

		// Get metadata for a view type
		ViewMetadata GetViewMetadata(const std::string& viewTypeName) const;

		// Get all view types in a category
		std::vector<std::string> GetViewsByCategory(const std::string& category) const;

		// Get all categories
		std::vector<std::string> GetViewCategories() const;

		// Get views by source
		std::vector<std::string> GetViewsBySource(const std::string& source) const;

		// Update generic views
		void UpdateGenericWorkspaces(float deltaT);

		// Render generic views
		void RenderGenericWorkspaces();

		// Returns a designated view type by string name
		ViewTypeID GetViewType(const std::string &name) const;

		// State Management

		// Resets to init state but retains registered views
		void Reset();

		// Complete reset including registered views (for shutdown)
		void FullReset();

		// Return all view IDs
		std::vector<WorkspaceID> GetAllWorkspaces() const;

		// Returns all registered view names and types
		const std::unordered_map<std::string, ViewTypeID> &GetRegisteredViews() const;

		// Returns all view list IDs and view signatures
		const std::map<WorkspaceID, std::shared_ptr<ViewSignature>> &GetWorkspaceSignatures() const;

		// Adds a view in the view list by type
		void AddViewByType(const WorkspaceID viewList, const ViewTypeID viewType);

		// Removes a view in the view list by type
		void RemoveViewByType(const WorkspaceID viewList, const ViewTypeID viewType);

		// Serialization

		// Serialize JSON into view lists
		json SerializeViewLists() const;

		// Deserialize JSON into view lists
		void DeserializeViewLists(const json &viewListsJson);

	private:
		// Adds a new view list
		template <typename T>
		void AddWorkspace();

		// Returns all view lists
		template <typename T>
		std::shared_ptr<Workspace<T>> GetWorkspace();

		template <typename T>
		std::shared_ptr<Workspace<T>> GetViewList();

		// Adds a new view signature if it doesn't exist
		void AddViewSignature(const WorkspaceID viewList);

		// Returns view signatures
		std::shared_ptr<ViewSignature> GetViewSignature(const WorkspaceID viewList);

		// Reset workspace-related data without touching registrations
		void ResetWorkspaceData();

		// Reset registration data (for full reset)
		void ResetRegistrationData();

	private:
		WorkspaceID workspaceCount;
		std::queue<WorkspaceID> availableWorkspaces;
		std::map<WorkspaceID, std::shared_ptr<ViewSignature>> workspaceSignatures;
		std::map<ViewTypeID, std::shared_ptr<IWorkspace>> workspaceArrays;
		std::unordered_map<std::string, ViewTypeID> registeredViews;

		// Metadata and creation tracking
		std::unordered_map<std::string, std::function<ViewMetadata()>> viewMetadata;
		std::unordered_map<std::string, std::string> viewSources;
		std::unordered_map<std::string, ViewCreationCallback> viewFactories;

		// Generic view storage for views created by name
		std::unordered_map<WorkspaceID, std::unique_ptr<BaseView>> genericWorkspaces;
	};

	// Template implementations
	template <typename T>
	void ViewManager::AddView(const WorkspaceID viewList, T &&view) {
		assert(viewList < MAX_VIEW_COUNT && "WorkspaceID out of range!");
		assert(GetViewSignature(viewList)->size() < MAX_VIEW_COUNT && "View count limit reached!");

		// Check if view already exists
		if (HasView<T>(viewList)) {
			std::cerr << "View with ID " << viewList << " already exists! Skipping AddView." << std::endl;
			return;
		}

		// Set the view's ID and add it to signatures
		view.workspaceID = viewList;
		GetViewSignature(viewList)->insert(ViewType<T>());

		// Initialize the view before adding it
		view.Init();

		// Add to appropriate view list
		auto viewListPtr = GetViewList<T>();
		std::cout << "Adding and initializing view - ID: " << viewList << ", Type: " << typeid(T).name() << std::endl;
		viewListPtr->Insert(std::forward<T>(view));
	}

	template <typename T>
	void ViewManager::RemoveView(const WorkspaceID viewList) {
		assert(viewList < MAX_VIEW_COUNT && "WorkspaceID out of range!");

		// Remove from signatures
		const ViewTypeID viewType = ViewType<T>();
		GetViewSignature(viewList)->erase(viewType);

		// Remove from view list
		GetViewList<T>()->Erase(viewList);
	}

	template <typename T>
	T &ViewManager::GetView(const WorkspaceID viewList) {
		assert(viewList < MAX_VIEW_COUNT && "WorkspaceID out of range!");
		return GetViewList<T>()->Get(viewList);
	}

	template <typename T>
	const bool ViewManager::HasView(const WorkspaceID viewList) {
		assert(viewList < MAX_VIEW_COUNT && "WorkspaceID out of range!");

		auto it = workspaceSignatures.find(viewList);
		if (it == workspaceSignatures.end()) {
			return false;
		}

		const ViewSignature &signature = *(it->second);
		const ViewTypeID viewType = ViewType<T>();
		return (signature.count(viewType) > 0);
	}

	template <typename T>
	void ViewManager::RegisterView(const std::string &name, const std::string& source) {
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

	template <typename T>
	void ViewManager::AddWorkspace() {
		const ViewTypeID viewType = ViewType<T>();
		assert(workspaceArrays.find(viewType) == workspaceArrays.end() && "ViewList already registered!");
		workspaceArrays[viewType] = std::make_shared<Workspace<T>>();
	}

	template <typename T>
	std::shared_ptr<Workspace<T>> ViewManager::GetWorkspace() {
		const ViewTypeID viewType = ViewType<T>();
		if (workspaceArrays.count(viewType) == 0) {
			AddWorkspace<T>();
		}
		return std::static_pointer_cast<Workspace<T>>(workspaceArrays.at(viewType));
	}

	template <typename T>
	std::shared_ptr<Workspace<T>> ViewManager::GetViewList() {
		return GetWorkspace<T>();
	}

} // namespace GUI