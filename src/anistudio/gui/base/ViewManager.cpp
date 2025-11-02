#include "ViewManager.hpp"
#include <iostream>
#include <cassert>
#include <imgui.h>

namespace GUI {

	ViewManager::ViewManager() : workspaceCount(0), m_activeWorkspaceID(0), m_imguiContext(nullptr) {
		for (WorkspaceID view = 0u; view < MAX_VIEW_COUNT; view++) {
			availableWorkspaces.push(view);
		}
	}

	void ViewManager::Init() {
		// NO AUTOMATIC WORKSPACE CREATION - workspaces must be created externally
	}

	void ViewManager::Update(const float deltaT) {
		for (const auto& workspace : workspaceArrays) {
			workspace.second->UpdateViews(deltaT);
		}
		UpdateWorkspaces(deltaT);
	}

	void ViewManager::Render() {
		ImGuiContext* previousContext = nullptr;
		bool contextSwitched = false;

		if (m_imguiContext) {
			previousContext = ImGui::GetCurrentContext();
			if (previousContext != m_imguiContext) {
				ImGui::SetCurrentContext(static_cast<ImGuiContext*>(m_imguiContext));
				contextSwitched = true;
			}
		}

		auto signatureIt = workspaceSignatures.find(m_activeWorkspaceID);
		if (signatureIt == workspaceSignatures.end()) {
			goto restore_context;
		}

		{
			const auto& signature = *(signatureIt->second);

			for (const auto& viewTypeID : signature) {
				bool viewRendered = false;

				auto workspaceIt = workspaces.find(m_activeWorkspaceID);
				if (workspaceIt != workspaces.end()) {
					auto viewIt = workspaceIt->second.find(viewTypeID);
					if (viewIt != workspaceIt->second.end() && viewIt->second) {
						try {
							viewIt->second->Render();
							viewRendered = true;
						}
						catch (const std::exception& e) {
							std::cerr << "[ViewManager] Exception rendering view: " << e.what() << std::endl;
						}
					}
				}

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
								// View doesn't exist for this workspace
							}
						}
					}
				}
			}
		}

	restore_context:
		if (contextSwitched && previousContext) {
			ImGui::SetCurrentContext(previousContext);
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

		if (allWorkspaces.empty()) {
			m_activeWorkspaceID = 0;
			return;
		}

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

		std::string defaultName = GenerateUniqueWorkspaceName("Workspace");
		workspaceNames[viewList] = defaultName;

		if (workspaceCount == 1) {
			m_activeWorkspaceID = viewList;
		}

		std::cout << "[ViewManager] Created workspace " << viewList << " with name: " << defaultName << std::endl;

		return viewList;
	}

	void ViewManager::DestroyView(const WorkspaceID viewList) {
		assert(viewList < MAX_VIEW_COUNT && "WorkspaceID out of range!");

		if (viewList == m_activeWorkspaceID) {
			auto allWorkspaces = GetAllWorkspaces();
			for (WorkspaceID id : allWorkspaces) {
				if (id != viewList) {
					m_activeWorkspaceID = id;
					break;
				}
			}
			if (allWorkspaces.size() <= 1) {
				m_activeWorkspaceID = 0;
			}
		}

		workspaceSignatures.erase(viewList);

		for (auto &array : workspaceArrays) {
			array.second->Erase(viewList);
		}

		workspaces.erase(viewList);
		workspaceNames.erase(viewList);

		workspaceCount--;
		availableWorkspaces.push(viewList);

		EnsureValidActiveWorkspace();
	}

	void ViewManager::RegisterViewWithFactory(const std::string& name, const std::string& source,
		ViewCreationCallback factory, std::function<ViewMetadata()> metadataGetter) {

		std::lock_guard<std::mutex> lock(m_viewRegistryMutex);

		auto nameIt = m_viewNameToID.find(name);
		ViewTypeID typeId;
		if (nameIt != m_viewNameToID.end()) {
			typeId = nameIt->second;
		}
		else {
			typeId = m_nextViewID++;
			m_viewNameToID[name] = typeId;
			m_viewIDToName[typeId] = name;
		}

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
			return 0;
		}

		try {
			WorkspaceID id = CreateView();

			ImGuiContext* previousContext = nullptr;
			bool contextSwitched = false;

			if (m_imguiContext) {
				previousContext = ImGui::GetCurrentContext();
				if (previousContext != m_imguiContext) {
					ImGui::SetCurrentContext(static_cast<ImGuiContext*>(m_imguiContext));
					contextSwitched = true;
				}
			}

			auto view = factoryIt->second(entityMgr);

			if (contextSwitched && previousContext) {
				ImGui::SetCurrentContext(previousContext);
			}

			if (!view) {
				DestroyView(id);
				return 0;
			}

			view->workspaceID = id;
			view->Init();

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

	void ViewManager::UnregisterView(const std::string& name) {
		std::lock_guard<std::mutex> lock(m_viewRegistryMutex);

		ViewTypeID viewType;
		try {
			viewType = GetViewType(name);
		}
		catch (const std::exception&) {
			std::cerr << "[ViewManager] View type not registered: " << name << std::endl;
			return;
		}

		UnregisterViewByType(viewType);
	}

	void ViewManager::UnregisterViewByType(ViewTypeID viewType) {
		std::lock_guard<std::mutex> lock(m_viewRegistryMutex);

		std::cout << "[ViewManager] Unregistering view type ID: " << viewType << std::endl;

		RemoveViewFromAllWorkspaces(viewType);

		auto arrayIt = workspaceArrays.find(viewType);
		if (arrayIt != workspaceArrays.end()) {
			workspaceArrays.erase(arrayIt);
		}

		std::string viewName;

		auto idToNameIt = m_viewIDToName.find(viewType);
		if (idToNameIt != m_viewIDToName.end()) {
			viewName = idToNameIt->second;
			m_viewIDToName.erase(idToNameIt);
		}

		if (!viewName.empty()) {
			m_viewNameToID.erase(viewName);
		}

		for (auto it = registeredViews.begin(); it != registeredViews.end(); ) {
			if (it->second == viewType) {
				it = registeredViews.erase(it);
			}
			else {
				++it;
			}
		}

		for (auto it = m_viewTypeToID.begin(); it != m_viewTypeToID.end(); ) {
			if (it->second == viewType) {
				it = m_viewTypeToID.erase(it);
			}
			else {
				++it;
			}
		}

		if (!viewName.empty()) {
			viewMetadata.erase(viewName);
			viewSources.erase(viewName);
			viewFactories.erase(viewName);
		}

		std::cout << "[ViewManager] Successfully unregistered view type ID: " << viewType << std::endl;
	}

	void ViewManager::RemoveViewFromAllWorkspaces(ViewTypeID viewType) {
		// Remove this view type from all workspace signatures and instances
		for (auto&[workspaceId, signature] : workspaceSignatures) {
			if (signature->count(viewType) > 0) {
				signature->erase(viewType);

				// Remove the actual view instance
				auto workspaceIt = workspaces.find(workspaceId);
				if (workspaceIt != workspaces.end()) {
					auto viewIt = workspaceIt->second.find(viewType);
					if (viewIt != workspaceIt->second.end()) {
						workspaceIt->second.erase(viewIt);

						// Remove workspace if empty
						if (workspaceIt->second.empty()) {
							workspaces.erase(workspaceIt);
						}
					}
				}
			}
		}

		for (auto&[typeId, workspaceArray] : workspaceArrays) {
			if (typeId == viewType) {
				auto allWorkspaces = GetAllWorkspaces();
				for (WorkspaceID workspaceId : allWorkspaces) {
					workspaceArray->Erase(workspaceId);
				}
				break;
			}
		}
	}

	void ViewManager::UnregisterViewSource(const std::string& source) {
		auto it = viewSources.begin();
		while (it != viewSources.end()) {
			if (it->second == source) {
				const std::string& viewName = it->first;
				std::cout << "Unregistering view: " << viewName
					<< " from source: " << source << std::endl;

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

	bool ViewManager::UnregisterViewType(const std::string& viewName) {
		CloseAllViewsOfType(viewName);

		bool removed = false;
		if (registeredViews.erase(viewName) > 0) removed = true;
		if (viewMetadata.erase(viewName) > 0) removed = true;
		if (viewSources.erase(viewName) > 0) removed = true;
		if (viewFactories.erase(viewName) > 0) removed = true;

		if (removed) {
			std::cout << "[ViewManager] Unregistered view type: " << viewName << std::endl;
		}

		return removed;
	}

	void ViewManager::CloseAllViewsOfType(const std::string& viewName) {
		ViewTypeID viewTypeID;
		try {
			viewTypeID = GetViewType(viewName);
		}
		catch (const std::exception&) {
			std::cout << "[ViewManager] View type not found: " << viewName << std::endl;
			return;
		}

		std::cout << "[ViewManager] Closing all instances of view: " << viewName
			<< " (ID: " << viewTypeID << ")" << std::endl;

		std::vector<WorkspaceID> workspacesToRemove;

		for (auto& workspacePair : workspaces) {
			WorkspaceID workspaceID = workspacePair.first;
			auto& viewsMap = workspacePair.second;

			auto viewIt = viewsMap.find(viewTypeID);
			if (viewIt != viewsMap.end()) {
				std::cout << "[ViewManager] Removing view instance from workspace " << workspaceID << std::endl;
				viewsMap.erase(viewIt);

				auto sigIt = workspaceSignatures.find(workspaceID);
				if (sigIt != workspaceSignatures.end()) {
					sigIt->second->erase(viewTypeID);
				}
			}

			if (viewsMap.empty()) {
				workspacesToRemove.push_back(workspaceID);
			}
		}

		for (WorkspaceID workspaceID : workspacesToRemove) {
			workspaces.erase(workspaceID);
			workspaceSignatures.erase(workspaceID);
		}
	}

	ViewMetadata ViewManager::GetViewMetadata(const std::string& viewTypeName) const {
		auto it = viewMetadata.find(viewTypeName);
		if (it != viewMetadata.end()) {
			return it->second();
		}

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
		ImGuiContext* previousContext = nullptr;
		bool contextSwitched = false;

		if (m_imguiContext) {
			previousContext = ImGui::GetCurrentContext();
			if (previousContext != m_imguiContext) {
				ImGui::SetCurrentContext(static_cast<ImGuiContext*>(m_imguiContext));
				contextSwitched = true;
			}
		}

		for (auto&[workspaceID, viewsMap] : workspaces) {
			for (auto&[viewTypeID, view] : viewsMap) {
				try {
					view->Update(deltaT);
				}
				catch (const std::exception& e) {
					std::cerr << "[ViewManager] Exception updating view: " << e.what() << std::endl;
				}
			}
		}

		if (contextSwitched && previousContext) {
			ImGui::SetCurrentContext(previousContext);
		}
	}

	void ViewManager::RenderWorkspaces() {
		ImGuiContext* previousContext = nullptr;
		bool contextSwitched = false;

		if (m_imguiContext) {
			previousContext = ImGui::GetCurrentContext();
			if (previousContext != m_imguiContext) {
				ImGui::SetCurrentContext(static_cast<ImGuiContext*>(m_imguiContext));
				contextSwitched = true;
			}
		}

		for (auto&[workspaceID, viewsMap] : workspaces) {
			for (auto&[viewTypeID, view] : viewsMap) {
				try {
					view->Render();
				}
				catch (const std::exception& e) {
					std::cerr << "[ViewManager] Exception rendering view: " << e.what() << std::endl;
				}
			}
		}

		if (contextSwitched && previousContext) {
			ImGui::SetCurrentContext(previousContext);
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
		std::cout << "[ViewManager] Performing soft reset..." << std::endl;
		ResetWorkspaceData(); // THIS CLEARS ALL WORKSPACES
		std::cout << "[ViewManager] Soft reset complete. View registrations preserved." << std::endl;
		std::cout << "[ViewManager] Registered views count: " << registeredViews.size() << std::endl;
	}

	void ViewManager::FullReset() {
		std::cout << "[ViewManager] Performing full reset..." << std::endl;
		ResetWorkspaceData(); // THIS CLEARS ALL WORKSPACES
		ResetRegistrationData();
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
		CreateViewInstanceForWorkspace(viewList, viewType);

		std::cout << "[ViewManager] Added view type " << viewType << " to workspace " << viewList << std::endl;
	}

	void ViewManager::RemoveViewByType(const WorkspaceID viewList, const ViewTypeID viewType) {
		assert(viewList < MAX_VIEW_COUNT && "WorkspaceID out of range!");

		GetViewSignature(viewList)->erase(viewType);
		RemoveViewInstanceFromWorkspace(viewList, viewType);

		std::cout << "[ViewManager] Removed view type " << viewType << " from workspace " << viewList << std::endl;
	}

	void ViewManager::CreateViewInstanceForWorkspace(WorkspaceID workspaceID, ViewTypeID viewTypeID) {
		std::string viewTypeName;
		for (const auto&[name, typeID] : registeredViews) {
			if (typeID == viewTypeID) {
				viewTypeName = name;
				break;
			}
		}

		if (viewTypeName.empty()) {
			std::cerr << "[ViewManager] ERROR: No view name found for type ID " << viewTypeID << std::endl;
			return;
		}

		std::cout << "[ViewManager] Found view name '" << viewTypeName << "' for type ID " << viewTypeID << std::endl;

		auto factoryIt = viewFactories.find(viewTypeName);
		if (factoryIt != viewFactories.end() && entityManager) {
			try {
				ImGuiContext* previousContext = nullptr;
				bool contextSwitched = false;

				if (m_imguiContext) {
					previousContext = ImGui::GetCurrentContext();
					if (previousContext != m_imguiContext) {
						ImGui::SetCurrentContext(static_cast<ImGuiContext*>(m_imguiContext));
						contextSwitched = true;
					}
				}

				std::cout << "[ViewManager] Calling factory for '" << viewTypeName << "'" << std::endl;
				auto view = factoryIt->second(*entityManager);

				if (contextSwitched && previousContext) {
					ImGui::SetCurrentContext(previousContext);
				}

				if (view) {
					view->workspaceID = workspaceID;
					view->Init();
					workspaces[workspaceID][viewTypeID] = std::move(view);
					std::cout << "[ViewManager] SUCCESS: Created view instance '" << viewTypeName
						<< "' for workspace " << workspaceID << std::endl;
				}
				else {
					std::cerr << "[ViewManager] ERROR: Factory returned nullptr for '" << viewTypeName << "'" << std::endl;
				}
			}
			catch (const std::exception& e) {
				std::cerr << "[ViewManager] EXCEPTION creating view '" << viewTypeName
					<< "': " << e.what() << std::endl;
			}
		}
		else {
			std::cerr << "[ViewManager] ERROR: No factory found for '" << viewTypeName << "'" << std::endl;
		}
	}

	void ViewManager::RemoveViewInstanceFromWorkspace(WorkspaceID workspaceID, ViewTypeID viewTypeID) {
		auto workspaceIt = workspaces.find(workspaceID);
		if (workspaceIt != workspaces.end()) {
			auto viewIt = workspaceIt->second.find(viewTypeID);
			if (viewIt != workspaceIt->second.end()) {
				std::cout << "[ViewManager] Removing view instance (type " << viewTypeID
					<< ") from workspace " << workspaceID << std::endl;
				workspaceIt->second.erase(viewIt);

				if (workspaceIt->second.empty()) {
					workspaces.erase(workspaceIt);
				}
			}
		}

		for (auto&[typeID, workspace] : workspaceArrays) {
			if (typeID == viewTypeID) {
				workspace->Erase(workspaceID);
				break;
			}
		}
	}

	void ViewManager::SetImGuiContext(void* context) {
		m_imguiContext = context;
		std::cout << "[ViewManager] Set ImGui context: " << m_imguiContext << std::endl;
	}

	void* ViewManager::GetImGuiContext() const {
		return m_imguiContext;
	}

	void ViewManager::SetWorkspaceName(WorkspaceID workspaceID, const std::string& name) {
		if (name.empty()) {
			std::cerr << "[ViewManager] Cannot set empty workspace name" << std::endl;
			return;
		}

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
	json ViewManager::SerializeViewLists() const {
		json workspacesJson = json::object();

		std::cout << "[ViewManager] Serializing workspaces..." << std::endl;

		std::set<WorkspaceID> allWorkspaceIDs;

		for (const auto &[workspaceID, signature] : workspaceSignatures) {
			allWorkspaceIDs.insert(workspaceID);
		}

		for (const auto &[workspaceID, viewsMap] : workspaces) {
			allWorkspaceIDs.insert(workspaceID);
		}

		std::cout << "[ViewManager] Total unique workspaces found: " << allWorkspaceIDs.size() << std::endl;

		for (WorkspaceID workspaceID : allWorkspaceIDs) {
			json workspaceJson = json::object();
			workspaceJson["ID"] = workspaceID;
			workspaceJson["name"] = GetWorkspaceName(workspaceID);
			workspaceJson["views"] = json::array();

			std::cout << "[ViewManager] Serializing workspace " << workspaceID << " (" << GetWorkspaceName(workspaceID) << ")" << std::endl;

			auto workspaceIt = workspaces.find(workspaceID);
			if (workspaceIt != workspaces.end()) {
				for (const auto &[viewTypeID, view] : workspaceIt->second) {
					if (view) {
						std::string viewTypeName;
						for (const auto &[name, typeID] : registeredViews) {
							if (typeID == viewTypeID) {
								viewTypeName = name;
								break;
							}
						}

						if (!viewTypeName.empty()) {
							json viewJson = json::object();
							viewJson[viewTypeName] = view->Serialize();
							workspaceJson["views"].push_back(viewJson);

							std::cout << "[ViewManager]   Added view: " << viewTypeName << " with data" << std::endl;
						}
					}
				}
			}

			auto signatureIt = workspaceSignatures.find(workspaceID);
			if (signatureIt != workspaceSignatures.end()) {
				for (const auto &viewTypeID : *(signatureIt->second)) {
					auto workspaceIt = workspaces.find(workspaceID);
					if (workspaceIt != workspaces.end() && workspaceIt->second.find(viewTypeID) != workspaceIt->second.end()) {
						continue;
					}

					std::string viewTypeName;
					for (const auto &[name, typeID] : registeredViews) {
						if (typeID == viewTypeID) {
							viewTypeName = name;
							break;
						}
					}

					if (!viewTypeName.empty()) {
						json viewJson = json::object();
						viewJson[viewTypeName] = json::object();
						workspaceJson["views"].push_back(viewJson);

						std::cout << "[ViewManager]   Added template view: " << viewTypeName << " (no data)" << std::endl;
					}
				}
			}

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

		std::set<WorkspaceID> usedWorkspaceIDs;

		for (auto workspaceIt = workspacesJson.begin(); workspaceIt != workspacesJson.end(); ++workspaceIt) {
			const json& workspaceJson = workspaceIt.value();

			if (!workspaceJson.contains("ID")) {
				std::cerr << "[ViewManager] Workspace JSON missing ID" << std::endl;
				continue;
			}

			WorkspaceID workspaceID = workspaceJson["ID"];
			std::cout << "[ViewManager] Deserializing workspace " << workspaceID << std::endl;

			usedWorkspaceIDs.insert(workspaceID);

			if (workspaceJson.contains("name")) {
				std::string loadedName = workspaceJson["name"];
				if (IsWorkspaceNameTaken(loadedName, workspaceID)) {
					loadedName = GenerateUniqueWorkspaceName(loadedName);
					std::cout << "[ViewManager] Name conflict resolved, using: " << loadedName << std::endl;
				}
				workspaceNames[workspaceID] = loadedName;
			}
			else {
				workspaceNames[workspaceID] = GenerateUniqueWorkspaceName("Workspace");
			}

			if (workspaceSignatures.find(workspaceID) == workspaceSignatures.end()) {
				AddViewSignature(workspaceID);
				std::cout << "[ViewManager] Created signature for workspace " << workspaceID << std::endl;
			}

			if (!workspaceJson.contains("views") || !workspaceJson["views"].is_array()) {
				std::cout << "[ViewManager] Workspace " << workspaceID << " has no views" << std::endl;
				continue;
			}

			for (const auto &viewJson : workspaceJson["views"]) {
				if (!viewJson.is_object()) {
					std::cerr << "[ViewManager] View JSON is not an object" << std::endl;
					continue;
				}

				for (auto viewIt = viewJson.begin(); viewIt != viewJson.end(); ++viewIt) {
					std::string viewTypeName = viewIt.key();
					const json& viewData = viewIt.value();

					std::cout << "[ViewManager]   Deserializing view: " << viewTypeName << std::endl;

					try {
						ViewTypeID viewTypeID = GetViewType(viewTypeName);

						GetViewSignature(workspaceID)->insert(viewTypeID);

						auto factoryIt = viewFactories.find(viewTypeName);
						if (factoryIt != viewFactories.end() && entityManager) {
							ImGuiContext* previousContext = nullptr;
							bool contextSwitched = false;

							if (m_imguiContext) {
								previousContext = ImGui::GetCurrentContext();
								if (previousContext != m_imguiContext) {
									ImGui::SetCurrentContext(static_cast<ImGuiContext*>(m_imguiContext));
									contextSwitched = true;
								}
							}

							auto view = factoryIt->second(*entityManager);

							if (contextSwitched && previousContext) {
								ImGui::SetCurrentContext(previousContext);
							}

							if (view) {
								view->workspaceID = workspaceID;
								view->Init();

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

		std::cout << "[ViewManager] Updating workspace tracking..." << std::endl;

		std::queue<WorkspaceID> empty;
		std::swap(availableWorkspaces, empty);

		workspaceCount = usedWorkspaceIDs.size();

		WorkspaceID maxUsedID = 0;
		for (WorkspaceID id : usedWorkspaceIDs) {
			if (id > maxUsedID) {
				maxUsedID = id;
			}
		}

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
		for (auto &viewSignaturePair : workspaceSignatures) {
			DestroyView(viewSignaturePair.first);
		}
		workspaceSignatures.clear();
		workspaces.clear();
		workspaceNames.clear();

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

		std::cout << "[ViewManager] Registration data cleared" << std::endl;
	}

} // namespace GUI