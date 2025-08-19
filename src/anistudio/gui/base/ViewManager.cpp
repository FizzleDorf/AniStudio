/*
 * ViewManager.cpp - COMPLETE with all serialization methods
 */

#include "ViewManager.hpp"
#include <iostream>
#include <cassert>

namespace GUI {

	ViewManager::ViewManager() : workspaceCount(0), m_activeWorkspaceID(0) {
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
		// Update workspaces
		UpdateWorkspaces(deltaT);
	}

	void ViewManager::Render() {
		// FIXED: Render only the active workspace
		auto signatureIt = workspaceSignatures.find(m_activeWorkspaceID);
		if (signatureIt == workspaceSignatures.end()) {
			// No signature means no views in this workspace
			return;
		}

		const auto& signature = *(signatureIt->second);

		// ONLY render views that are explicitly in the active workspace signature
		for (const auto& viewTypeID : signature) {
			bool viewRendered = false;

			// Check factory-created views first (these are stored in workspaces map)
			auto workspaceIt = workspaces.find(m_activeWorkspaceID);
			if (workspaceIt != workspaces.end()) {
				auto viewIt = workspaceIt->second.find(viewTypeID);
				if (viewIt != workspaceIt->second.end() && viewIt->second) {
					viewIt->second->Render();
					viewRendered = true;
				}
			}

			// If not found in factory views, check template-based views
			if (!viewRendered) {
				auto arrayIt = workspaceArrays.find(viewTypeID);
				if (arrayIt != workspaceArrays.end()) {
					auto workspace = arrayIt->second;
					auto workspaceData = std::static_pointer_cast<Workspace<BaseView>>(workspace);
					if (workspaceData && workspaceData->Contains(m_activeWorkspaceID)) {
						try {
							auto& view = workspaceData->Get(m_activeWorkspaceID);
							view.Render();
							viewRendered = true;
						}
						catch (...) {
							// View doesn't exist for this workspace, skip silently
						}
					}
				}
			}
		}
	}

	void ViewManager::SetActiveWorkspace(WorkspaceID workspaceID) {
		auto allWorkspaces = GetAllWorkspaces();
		if (std::find(allWorkspaces.begin(), allWorkspaces.end(), workspaceID) != allWorkspaces.end()) {
			m_activeWorkspaceID = workspaceID;
			std::cout << "[ViewManager] Set active workspace to: " << workspaceID << std::endl;
		}
		else {
			std::cerr << "[ViewManager] Cannot set active workspace - ID " << workspaceID << " does not exist" << std::endl;
		}
	}

	WorkspaceID ViewManager::GetActiveWorkspace() const {
		return m_activeWorkspaceID;
	}

	void ViewManager::EnsureValidActiveWorkspace() {
		auto allWorkspaces = GetAllWorkspaces();

		// If no workspaces exist, create one
		if (allWorkspaces.empty()) {
			m_activeWorkspaceID = CreateView();
			std::cout << "[ViewManager] Created default workspace: " << m_activeWorkspaceID << std::endl;
			return;
		}

		// If current active workspace doesn't exist, switch to the first available
		if (std::find(allWorkspaces.begin(), allWorkspaces.end(), m_activeWorkspaceID) == allWorkspaces.end()) {
			m_activeWorkspaceID = allWorkspaces[0];
			std::cout << "[ViewManager] Switched to valid workspace: " << m_activeWorkspaceID << std::endl;
		}
	}

	const WorkspaceID ViewManager::CreateView() {
		const WorkspaceID viewList = availableWorkspaces.front();
		AddViewSignature(viewList);
		availableWorkspaces.pop();
		workspaceCount++;

		// Set default unique name
		std::string defaultName = GenerateUniqueWorkspaceName("Workspace");
		workspaceNames[viewList] = defaultName;

		// If this is the first workspace, make it active
		if (workspaceCount == 1) {
			m_activeWorkspaceID = viewList;
		}

		std::cout << "[ViewManager] Created workspace " << viewList << " with name: " << defaultName << std::endl;

		return viewList;
	}

	void ViewManager::DestroyView(const WorkspaceID viewList) {
		assert(viewList < MAX_VIEW_COUNT && "WorkspaceID out of range!");

		// If we're deleting the active workspace, switch to another one first
		if (viewList == m_activeWorkspaceID) {
			auto allWorkspaces = GetAllWorkspaces();
			for (WorkspaceID id : allWorkspaces) {
				if (id != viewList) {
					m_activeWorkspaceID = id;
					break;
				}
			}
			// If no other workspace exists, we'll create one later
			if (allWorkspaces.size() <= 1) {
				m_activeWorkspaceID = 0; // Will be corrected by EnsureValidActiveWorkspace
			}
		}

		// Remove this view's signature
		workspaceSignatures.erase(viewList);

		// Remove this view from all view arrays
		for (auto &array : workspaceArrays) {
			array.second->Erase(viewList);
		}

		// Remove from workspaces if it's there
		workspaces.erase(viewList);

		// Remove workspace name
		workspaceNames.erase(viewList);

		// Return the ID to the available pool
		workspaceCount--;
		availableWorkspaces.push(viewList);

		// Ensure we still have a valid active workspace
		EnsureValidActiveWorkspace();
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

			// Store in workspace storage
			ViewTypeID typeID = GetViewType(viewTypeName);
			workspaces[id][typeID] = std::move(view);

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

	void ViewManager::UpdateWorkspaces(float deltaT) {
		for (auto&[workspaceID, viewsMap] : workspaces) {
			for (auto&[viewTypeID, view] : viewsMap) {
				view->Update(deltaT);
			}
		}
	}

	void ViewManager::RenderWorkspaces() {
		for (auto&[workspaceID, viewsMap] : workspaces) {
			for (auto&[viewTypeID, view] : viewsMap) {
				view->Render();
			}
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

		// Add to signature
		GetViewSignature(viewList)->insert(viewType);

		// Create the actual view instance
		CreateViewInstanceForWorkspace(viewList, viewType);

		std::cout << "[ViewManager] Added view type " << viewType << " to workspace " << viewList << std::endl;
	}

	void ViewManager::RemoveViewByType(const WorkspaceID viewList, const ViewTypeID viewType) {
		assert(viewList < MAX_VIEW_COUNT && "WorkspaceID out of range!");

		// Remove from signature
		GetViewSignature(viewList)->erase(viewType);

		// Remove the view instance
		RemoveViewInstanceFromWorkspace(viewList, viewType);

		std::cout << "[ViewManager] Removed view type " << viewType << " from workspace " << viewList << std::endl;
	}

	void ViewManager::CreateViewInstanceForWorkspace(WorkspaceID workspaceID, ViewTypeID viewTypeID) {
		// Find the view type name for this ID
		for (const auto&[viewTypeName, typeID] : registeredViews) {
			if (typeID == viewTypeID) {
				auto factoryIt = viewFactories.find(viewTypeName);
				if (factoryIt != viewFactories.end() && entityManager) {
					try {
						auto view = factoryIt->second(*entityManager);
						if (view) {
							view->workspaceID = workspaceID;
							view->Init();
							workspaces[workspaceID][viewTypeID] = std::move(view);
							std::cout << "[ViewManager] Created view instance: " << viewTypeName
								<< " for workspace: " << workspaceID << std::endl;
						}
						else {
							std::cerr << "[ViewManager] Factory returned null view for " << viewTypeName << std::endl;
						}
					}
					catch (const std::exception& e) {
						std::cerr << "[ViewManager] Failed to create view instance for " << viewTypeName
							<< ": " << e.what() << std::endl;
					}
				}
				else {
					std::cout << "[ViewManager] No factory found for view type: " << viewTypeName
						<< " (template-based view)" << std::endl;
				}
				return;
			}
		}
		std::cerr << "[ViewManager] Unknown view type ID: " << viewTypeID << std::endl;
	}

	void ViewManager::RemoveViewInstanceFromWorkspace(WorkspaceID workspaceID, ViewTypeID viewTypeID) {
		auto workspaceIt = workspaces.find(workspaceID);
		if (workspaceIt != workspaces.end()) {
			auto viewIt = workspaceIt->second.find(viewTypeID);
			if (viewIt != workspaceIt->second.end()) {
				std::cout << "[ViewManager] Removing view instance (type " << viewTypeID
					<< ") from workspace " << workspaceID << std::endl;
				workspaceIt->second.erase(viewIt);

				// If workspace is now empty, clean it up
				if (workspaceIt->second.empty()) {
					workspaces.erase(workspaceIt);
				}
			}
		}

		// Also remove from template-based views if they exist
		for (auto&[typeID, workspace] : workspaceArrays) {
			if (typeID == viewTypeID) {
				workspace->Erase(workspaceID);
				break;
			}
		}
	}

	// Workspace naming methods
	void ViewManager::SetWorkspaceName(WorkspaceID workspaceID, const std::string& name) {
		// Don't allow empty names
		if (name.empty()) {
			std::cerr << "[ViewManager] Cannot set empty workspace name" << std::endl;
			return;
		}

		// Check if name is already taken by another workspace
		if (IsWorkspaceNameTaken(name, workspaceID)) {
			std::cerr << "[ViewManager] Workspace name '" << name << "' is already taken" << std::endl;
			return;
		}

		workspaceNames[workspaceID] = name;
		std::cout << "[ViewManager] Set workspace " << workspaceID << " name to: " << name << std::endl;
	}

	std::string ViewManager::GetWorkspaceName(WorkspaceID workspaceID) const {
		auto it = workspaceNames.find(workspaceID);
		if (it != workspaceNames.end()) {
			return it->second;
		}
		return "Workspace " + std::to_string(workspaceID);
	}

	bool ViewManager::IsWorkspaceNameTaken(const std::string& name, WorkspaceID excludeID) const {
		for (const auto&[id, workspaceName] : workspaceNames) {
			if (id != excludeID && workspaceName == name) {
				return true;
			}
		}
		return false;
	}

	std::string ViewManager::GenerateUniqueWorkspaceName(const std::string& baseName) const {
		std::string name = baseName;
		int counter = 1;

		while (IsWorkspaceNameTaken(name)) {
			name = baseName + " " + std::to_string(counter);
			counter++;
		}

		return name;
	}

	// CRITICAL: SERIALIZATION METHODS - THESE WERE MISSING!
	json ViewManager::SerializeViewLists() const {
		json workspacesJson = json::object();

		std::cout << "[ViewManager] Serializing workspaces..." << std::endl;
		std::cout << "[ViewManager] workspaceSignatures size: " << workspaceSignatures.size() << std::endl;
		std::cout << "[ViewManager] workspaces size: " << workspaces.size() << std::endl;

		// Collect all workspace IDs from both sources
		std::set<WorkspaceID> allWorkspaceIDs;

		// Add from workspaceSignatures (template views)
		for (const auto &[workspaceID, signature] : workspaceSignatures) {
			allWorkspaceIDs.insert(workspaceID);
		}

		// Add from workspaces (factory views)
		for (const auto &[workspaceID, viewsMap] : workspaces) {
			allWorkspaceIDs.insert(workspaceID);
		}

		std::cout << "[ViewManager] Total unique workspaces found: " << allWorkspaceIDs.size() << std::endl;

		// Serialize each workspace as a JSON object with the workspace ID as key
		for (WorkspaceID workspaceID : allWorkspaceIDs) {
			json workspaceJson = json::object();
			workspaceJson["ID"] = workspaceID;
			workspaceJson["name"] = GetWorkspaceName(workspaceID); // Add name
			workspaceJson["views"] = json::array();

			std::cout << "[ViewManager] Serializing workspace " << workspaceID << " (" << GetWorkspaceName(workspaceID) << ")" << std::endl;

			// Add views from workspaces (factory views) - these have serializable data
			auto workspaceIt = workspaces.find(workspaceID);
			if (workspaceIt != workspaces.end()) {
				for (const auto &[viewTypeID, view] : workspaceIt->second) {
					if (view) {
						// Find view type name
						std::string viewTypeName;
						for (const auto &[name, typeID] : registeredViews) {
							if (typeID == viewTypeID) {
								viewTypeName = name;
								break;
							}
						}

						if (!viewTypeName.empty()) {
							// Create view object with type name as key (like EntityManager does with components)
							json viewJson = json::object();
							viewJson[viewTypeName] = view->Serialize();
							workspaceJson["views"].push_back(viewJson);

							std::cout << "[ViewManager]   Added view: " << viewTypeName << " with data" << std::endl;
						}
					}
				}
			}

			// Add views from workspaceSignatures (template views) - these might not have data
			auto signatureIt = workspaceSignatures.find(workspaceID);
			if (signatureIt != workspaceSignatures.end()) {
				for (const auto &viewTypeID : *(signatureIt->second)) {
					// Skip if we already added this view from factory views
					auto workspaceIt = workspaces.find(workspaceID);
					if (workspaceIt != workspaces.end() && workspaceIt->second.find(viewTypeID) != workspaceIt->second.end()) {
						continue; // Already added from factory views
					}

					// Find view type name
					std::string viewTypeName;
					for (const auto &[name, typeID] : registeredViews) {
						if (typeID == viewTypeID) {
							viewTypeName = name;
							break;
						}
					}

					if (!viewTypeName.empty()) {
						// Create view object with just the type name (no data)
						json viewJson = json::object();
						viewJson[viewTypeName] = json::object(); // Empty object for template views
						workspaceJson["views"].push_back(viewJson);

						std::cout << "[ViewManager]   Added template view: " << viewTypeName << " (no data)" << std::endl;
					}
				}
			}

			// Add workspace to main object using workspace ID as key (like EntityManager)
			std::string workspaceKey = std::to_string(workspaceID);
			workspacesJson[workspaceKey] = workspaceJson;

			std::cout << "[ViewManager] Workspace " << workspaceID << " serialized with " << workspaceJson["views"].size() << " views" << std::endl;
		}

		std::cout << "[ViewManager] Serialization complete. Total workspaces: " << workspacesJson.size() << std::endl;
		return workspacesJson;
	}

	void ViewManager::DeserializeViewLists(const json &workspacesJson) {
		std::cout << "[ViewManager] Deserializing workspaces..." << std::endl;

		if (workspacesJson.is_null()) {
			std::cout << "[ViewManager] Workspaces JSON is null, nothing to deserialize" << std::endl;
			return;
		}

		if (!workspacesJson.is_object()) {
			std::cerr << "[ViewManager] Workspaces JSON is not an object!" << std::endl;
			return;
		}

		std::cout << "[ViewManager] Found " << workspacesJson.size() << " workspaces to deserialize" << std::endl;

		// CRITICAL FIX: Track which workspace IDs are being used
		std::set<WorkspaceID> usedWorkspaceIDs;

		// Iterate through workspace objects (like EntityManager does with entities)
		for (auto workspaceIt = workspacesJson.begin(); workspaceIt != workspacesJson.end(); ++workspaceIt) {
			const json& workspaceJson = workspaceIt.value();

			if (!workspaceJson.contains("ID")) {
				std::cerr << "[ViewManager] Workspace JSON missing ID" << std::endl;
				continue;
			}

			WorkspaceID workspaceID = workspaceJson["ID"];
			std::cout << "[ViewManager] Deserializing workspace " << workspaceID << std::endl;

			// CRITICAL FIX: Track this workspace ID as used
			usedWorkspaceIDs.insert(workspaceID);

			// Load workspace name if it exists
			if (workspaceJson.contains("name")) {
				std::string loadedName = workspaceJson["name"];
				// Ensure the name is unique
				if (IsWorkspaceNameTaken(loadedName, workspaceID)) {
					loadedName = GenerateUniqueWorkspaceName(loadedName);
					std::cout << "[ViewManager] Name conflict resolved, using: " << loadedName << std::endl;
				}
				workspaceNames[workspaceID] = loadedName;
			}
			else {
				workspaceNames[workspaceID] = GenerateUniqueWorkspaceName("Workspace");
			}

			// Create the workspace signature if it doesn't exist
			if (workspaceSignatures.find(workspaceID) == workspaceSignatures.end()) {
				AddViewSignature(workspaceID);
				std::cout << "[ViewManager] Created signature for workspace " << workspaceID << std::endl;
			}

			if (!workspaceJson.contains("views") || !workspaceJson["views"].is_array()) {
				std::cout << "[ViewManager] Workspace " << workspaceID << " has no views" << std::endl;
				continue;
			}

			// Process views array (like EntityManager does with components)
			for (const auto &viewJson : workspaceJson["views"]) {
				if (!viewJson.is_object()) {
					std::cerr << "[ViewManager] View JSON is not an object" << std::endl;
					continue;
				}

				// Iterate through view types in this view object (like EntityManager does with components)
				for (auto viewIt = viewJson.begin(); viewIt != viewJson.end(); ++viewIt) {
					std::string viewTypeName = viewIt.key();
					const json& viewData = viewIt.value();

					std::cout << "[ViewManager]   Deserializing view: " << viewTypeName << std::endl;

					try {
						ViewTypeID viewTypeID = GetViewType(viewTypeName);

						// Add to workspace signature
						GetViewSignature(workspaceID)->insert(viewTypeID);

						// Create view instance if we have a factory
						auto factoryIt = viewFactories.find(viewTypeName);
						if (factoryIt != viewFactories.end() && entityManager) {
							auto view = factoryIt->second(*entityManager);
							if (view) {
								view->workspaceID = workspaceID;
								view->Init();

								// Deserialize view data if it exists
								if (!viewData.is_null() && !viewData.empty()) {
									view->Deserialize(viewData);
									std::cout << "[ViewManager]     Recreated view: " << viewTypeName << " with data" << std::endl;
								}
								else {
									std::cout << "[ViewManager]     Recreated view: " << viewTypeName << " (no data)" << std::endl;
								}

								workspaces[workspaceID][viewTypeID] = std::move(view);
							}
						}
						else {
							std::cout << "[ViewManager]     Added view to signature only: " << viewTypeName << std::endl;
						}
					}
					catch (const std::exception& e) {
						std::cerr << "[ViewManager] Failed to deserialize view " << viewTypeName << ": " << e.what() << std::endl;
					}
				}
			}
		}

		// CRITICAL FIX: Update the ViewManager's workspace tracking
		std::cout << "[ViewManager] Updating workspace tracking..." << std::endl;

		// Clear the available workspaces queue
		std::queue<WorkspaceID> empty;
		std::swap(availableWorkspaces, empty);

		// Set workspace count to the number of used workspaces
		workspaceCount = usedWorkspaceIDs.size();

		// Find the highest used workspace ID to know where to start new ones
		WorkspaceID maxUsedID = 0;
		for (WorkspaceID id : usedWorkspaceIDs) {
			if (id > maxUsedID) {
				maxUsedID = id;
			}
		}

		// Add all unused workspace IDs to the available queue
		for (WorkspaceID id = 0; id < MAX_VIEW_COUNT; id++) {
			if (usedWorkspaceIDs.find(id) == usedWorkspaceIDs.end()) {
				availableWorkspaces.push(id);
			}
		}

		std::cout << "[ViewManager] Workspace tracking updated:" << std::endl;
		std::cout << "  - Active workspaces: " << workspaceCount << std::endl;
		std::cout << "  - Highest used ID: " << maxUsedID << std::endl;
		std::cout << "  - Available workspace IDs: " << availableWorkspaces.size() << std::endl;

		std::cout << "[ViewManager] Deserialization complete" << std::endl;
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
		workspaces.clear();
		workspaceNames.clear();

		// Reset available views queue
		while (!availableWorkspaces.empty()) {
			availableWorkspaces.pop();
		}
		for (WorkspaceID view = 0u; view < MAX_VIEW_COUNT; ++view) {
			availableWorkspaces.push(view);
		}

		workspaceCount = 0;
		workspaceArrays.clear();
		m_activeWorkspaceID = 0;
	}

	void ViewManager::ResetRegistrationData() {
		registeredViews.clear();
		viewMetadata.clear();
		viewSources.clear();
		viewFactories.clear();
	}

} // namespace GUI