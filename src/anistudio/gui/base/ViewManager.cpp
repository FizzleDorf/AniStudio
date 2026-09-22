#include "ViewManager.hpp"
#include "Log.hpp"
#include <cassert>
#include <imgui.h>

namespace GUI {

    void* ViewManager::m_windowHandle = nullptr;

    ViewManager::ViewManager() : workspaceCount(0), m_activeWorkspaceID(0), m_imguiContext(nullptr) {
        for (WorkspaceID view = 0u; view < MAX_VIEW_COUNT; view++) {
            availableWorkspaces.push(view);
        }
        ANI_LOG_DEBUG("[ViewManager] Constructed");
    }

    void ViewManager::Init() {
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
            if (contextSwitched && previousContext) {
                ImGui::SetCurrentContext(previousContext);
            }
            return;
        }

        {
            const auto& signature = *(signatureIt->second);
            std::vector<GUI::ViewTypeID> viewsToRemove;

            for (const auto& viewTypeID : signature) {
                auto workspaceIt = workspaces.find(m_activeWorkspaceID);
                if (workspaceIt != workspaces.end()) {
                    auto viewIt = workspaceIt->second.find(viewTypeID);
                    if (viewIt != workspaceIt->second.end() && viewIt->second) {
                        if (!viewIt->second->IsWindowOpen()) {
                            viewsToRemove.push_back(viewTypeID);
                            continue;
                        }
                        try {
                            viewIt->second->Render();
                        }
                        catch (const std::exception& e) {
                            ANI_LOG_ERROR("[ViewManager] Exception rendering view: %s", e.what());
                        }
                    }
                }
            }

            for (const auto& viewTypeID : viewsToRemove) {
                RemoveViewByType(m_activeWorkspaceID, viewTypeID);
            }
        }

        if (contextSwitched && previousContext) {
            ImGui::SetCurrentContext(previousContext);
        }
    }

    void ViewManager::SetActiveWorkspace(WorkspaceID workspaceID) {
        auto allWorkspaces = GetAllWorkspaces();
        if (std::find(allWorkspaces.begin(), allWorkspaces.end(), workspaceID) != allWorkspaces.end()) {
            m_activeWorkspaceID = workspaceID;
            ANI_LOG_INFO("[ViewManager] Set active workspace to: %u", (unsigned)workspaceID);
        }
        else {
            ANI_LOG_ERROR("[ViewManager] Cannot set active workspace - ID %u does not exist", (unsigned)workspaceID);
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
            ANI_LOG_INFO("[ViewManager] Switched to valid workspace: %u", (unsigned)m_activeWorkspaceID);
        }
    }

    const WorkspaceID ViewManager::CreateView() {
        if (availableWorkspaces.empty()) {
            ANI_LOG_ERROR("[ViewManager] CreateView: no available workspace slots (max %u)",
                (unsigned)MAX_VIEW_COUNT);
            return 0;
        }

        const WorkspaceID viewList = availableWorkspaces.front();
        AddViewSignature(viewList);
        availableWorkspaces.pop();
        workspaceCount++;

        std::string defaultName = GenerateUniqueWorkspaceName("Workspace");
        workspaceNames[viewList] = defaultName;

        if (workspaceCount == 1) {
            m_activeWorkspaceID = viewList;
        }

        ANI_LOG_INFO("[ViewManager] Created workspace %u with name: %s",
            (unsigned)viewList, defaultName.c_str());

        return viewList;
    }

    void ViewManager::DestroyView(const WorkspaceID viewList) {
        assert(viewList < MAX_VIEW_COUNT && "WorkspaceID out of range!");

        if (workspaceSignatures.find(viewList) == workspaceSignatures.end()) {
            ANI_LOG_WARN("[ViewManager] DestroyView: workspace %u not found", (unsigned)viewList);
            return;
        }

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

        for (auto& array : workspaceArrays) {
            array.second->Erase(viewList);
        }

        workspaces.erase(viewList);
        workspaceNames.erase(viewList);

        workspaceCount--;
        availableWorkspaces.push(viewList);

        EnsureValidActiveWorkspace();

        ANI_LOG_DEBUG("[ViewManager] Destroyed workspace %u", (unsigned)viewList);
    }

    void ViewManager::RegisterViewWithFactory(const std::string& name, const std::string& source,
        ViewCreationCallback factory, std::function<ViewMetadata()> metadataGetter) {

        std::lock_guard<std::mutex> lock(m_viewRegistryMutex);

        auto nameIt = m_viewNameToID.find(name);
        ViewTypeID typeId;
        if (nameIt != m_viewNameToID.end()) {
            typeId = nameIt->second;
            ANI_LOG_TRACE("[ViewManager] Re-registering view type '%s' (ID: %u)",
                name.c_str(), (unsigned)typeId);
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

        ANI_LOG_INFO("[ViewManager] Registered custom view type: %s with ID: %u from source: %s",
            name.c_str(), (unsigned)typeId, source.c_str());
    }

    WorkspaceID ViewManager::CreateViewByName(const std::string& viewTypeName, ECS::EntityManager& entityMgr) {
        auto factoryIt = viewFactories.find(viewTypeName);
        if (factoryIt == viewFactories.end()) {
            ANI_LOG_ERROR("[ViewManager] No factory registered for view type: %s", viewTypeName.c_str());
            return 0;
        }

        try {
            WorkspaceID id = CreateView();
            if (id == 0) {
                ANI_LOG_ERROR("[ViewManager] CreateViewByName: failed to allocate workspace");
                return 0;
            }

            ImGuiContext* previousContext = nullptr;
            bool contextSwitched = false;

            if (m_imguiContext) {
                previousContext = ImGui::GetCurrentContext();
                if (previousContext != m_imguiContext) {
                    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(m_imguiContext));
                    contextSwitched = true;
                }
            }

            ANI_LOG_INFO("[ViewManager] Calling factory for %s", viewTypeName.c_str());
            auto view = factoryIt->second(entityMgr, *this);

            if (!view) {
                ANI_LOG_ERROR("[ViewManager] Factory returned nullptr for %s", viewTypeName.c_str());
                if (contextSwitched && previousContext) {
                    ImGui::SetCurrentContext(previousContext);
                }
                DestroyView(id);
                return 0;
            }

            view->workspaceID = id;
            ANI_LOG_INFO("[ViewManager] Calling Init() for %s", viewTypeName.c_str());
            view->Init();
            ANI_LOG_INFO("[ViewManager] Init() succeeded for %s", viewTypeName.c_str());

            ViewTypeID typeID = GetViewType(viewTypeName);
            workspaces[id][typeID] = std::move(view);

            if (contextSwitched && previousContext) {
                ImGui::SetCurrentContext(previousContext);
            }

            ANI_LOG_INFO("[ViewManager] Created and initialized view: %s with ID: %u",
                viewTypeName.c_str(), (unsigned)id);
            return id;
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("[ViewManager] Failed to create view %s: %s",
                viewTypeName.c_str(), e.what());
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
            ANI_LOG_WARN("[ViewManager] UnregisterView: view type not registered: %s", name.c_str());
            return;
        }

        UnregisterViewByType(viewType);
    }

    void ViewManager::UnregisterViewByType(ViewTypeID viewType) {
        std::lock_guard<std::mutex> lock(m_viewRegistryMutex);

        ANI_LOG_INFO("[ViewManager] Unregistering view type ID: %u", (unsigned)viewType);

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

        ANI_LOG_INFO("[ViewManager] Successfully unregistered view type ID: %u", (unsigned)viewType);
    }

    void ViewManager::RemoveViewFromAllWorkspaces(ViewTypeID viewType) {
        for (auto& [workspaceId, signature] : workspaceSignatures) {
            if (signature->count(viewType) > 0) {
                signature->erase(viewType);

                auto workspaceIt = workspaces.find(workspaceId);
                if (workspaceIt != workspaces.end()) {
                    auto viewIt = workspaceIt->second.find(viewType);
                    if (viewIt != workspaceIt->second.end()) {
                        workspaceIt->second.erase(viewIt);

                        if (workspaceIt->second.empty()) {
                            workspaces.erase(workspaceIt);
                        }
                    }
                }
            }
        }

        for (auto& [typeId, workspaceArray] : workspaceArrays) {
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
        size_t removed = 0;
        while (it != viewSources.end()) {
            if (it->second == source) {
                const std::string& viewName = it->first;
                ANI_LOG_INFO("[ViewManager] Unregistering view: %s from source: %s",
                    viewName.c_str(), source.c_str());

                registeredViews.erase(viewName);
                viewMetadata.erase(viewName);
                viewFactories.erase(viewName);
                it = viewSources.erase(it);
                removed++;
            }
            else {
                ++it;
            }
        }

        if (removed == 0) {
            ANI_LOG_DEBUG("[ViewManager] UnregisterViewSource: no views found from source: %s",
                source.c_str());
        }
        else {
            ANI_LOG_INFO("[ViewManager] UnregisterViewSource: removed %zu views from source: %s",
                removed, source.c_str());
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
            ANI_LOG_INFO("[ViewManager] Unregistered view type: %s", viewName.c_str());
        }
        else {
            ANI_LOG_WARN("[ViewManager] UnregisterViewType: view type not found: %s", viewName.c_str());
        }

        return removed;
    }

    std::vector<BaseView*> ViewManager::GetAllViews() const {
        std::vector<BaseView*> views;
        for (const auto& ws : workspaces) {
            for (const auto& viewPair : ws.second) {
                views.push_back(viewPair.second.get());
            }
        }
        return views;
    }

    void ViewManager::CloseAllViewsOfType(const std::string& viewName) {
        ViewTypeID viewTypeID;
        try {
            viewTypeID = GetViewType(viewName);
        }
        catch (const std::exception&) {
            ANI_LOG_INFO("[ViewManager] CloseAllViewsOfType: view type not found: %s", viewName.c_str());
            return;
        }

        ANI_LOG_INFO("[ViewManager] Closing all instances of view: %s (ID: %u)",
            viewName.c_str(), (unsigned)viewTypeID);

        std::vector<WorkspaceID> workspacesToRemove;

        for (auto& workspacePair : workspaces) {
            WorkspaceID workspaceID = workspacePair.first;
            auto& viewsMap = workspacePair.second;

            auto viewIt = viewsMap.find(viewTypeID);
            if (viewIt != viewsMap.end()) {
                if (viewIt->second) {
                    json state;
                    try {
                        state = viewIt->second->Serialize();
                    }
                    catch (const std::exception& e) {
                        ANI_LOG_ERROR("[ViewManager] Serialize failed for %s: %s",
                            viewName.c_str(), e.what());
                        state = json::object();
                    }

                    if (m_viewClosingCallback) {
                        m_viewClosingCallback(workspaceID, viewTypeID, viewName, state);
                    }
                }

                ANI_LOG_INFO("[ViewManager] Removing view instance from workspace %u", (unsigned)workspaceID);
                viewsMap.erase(viewIt);

                auto sigIt = workspaceSignatures.find(workspaceID);
                if (sigIt != workspaceSignatures.end()) {
                    sigIt->second->erase(viewTypeID);
                }

                if (m_viewClosedCallback) {
                    m_viewClosedCallback(workspaceID, viewTypeID, viewName);
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

        for (const auto& [viewTypeName, typeID] : registeredViews) {
            ViewMetadata meta = GetViewMetadata(viewTypeName);
            if (meta.category == category) {
                viewsInCategory.push_back(viewTypeName);
            }
        }

        return viewsInCategory;
    }

    std::vector<std::string> ViewManager::GetViewCategories() const {
        std::set<std::string> categories;

        for (const auto& [viewTypeName, typeID] : registeredViews) {
            ViewMetadata meta = GetViewMetadata(viewTypeName);
            categories.insert(meta.category);
        }

        return std::vector<std::string>(categories.begin(), categories.end());
    }

    std::vector<std::string> ViewManager::GetViewsBySource(const std::string& source) const {
        std::vector<std::string> views;
        for (const auto& [viewTypeName, viewSource] : viewSources) {
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

        for (auto& [workspaceID, viewsMap] : workspaces) {
            std::vector<ViewTypeID> viewsToRemove;
            for (auto& [viewTypeID, view] : viewsMap) {
                if (!view) continue;
                if (!view->IsWindowOpen()) {
                    viewsToRemove.push_back(viewTypeID);
                    continue;
                }
                try {
                    view->Update(deltaT);
                }
                catch (const std::exception& e) {
                    ANI_LOG_ERROR("[ViewManager] Exception updating view: %s", e.what());
                }
            }
            for (const auto& viewTypeID : viewsToRemove) {
                RemoveViewByType(workspaceID, viewTypeID);
            }
        }

        if (contextSwitched && previousContext) {
            ImGui::SetCurrentContext(previousContext);
        }
    }

    void ViewManager::RenderWorkspaces() {
        Render();
    }

    ViewTypeID ViewManager::GetViewType(const std::string& name) const {
        auto it = registeredViews.find(name);
        if (it != registeredViews.end()) {
            return it->second;
        }
        throw std::runtime_error("View type not registered: " + name);
    }

    void ViewManager::Reset() {
        ANI_LOG_INFO("[ViewManager] Performing soft reset...");
        ResetWorkspaceData();
        ANI_LOG_INFO("[ViewManager] Soft reset complete. View registrations preserved.");
        ANI_LOG_INFO("[ViewManager] Registered views count: %zu", registeredViews.size());
    }

    void ViewManager::FullReset() {
        ANI_LOG_INFO("[ViewManager] Performing full reset...");
        ResetWorkspaceData();
        ResetRegistrationData();
        ANI_LOG_INFO("[ViewManager] Full reset complete.");
    }

    std::vector<WorkspaceID> ViewManager::GetAllWorkspaces() const {
        std::vector<WorkspaceID> ids;
        for (const auto& pair : workspaceSignatures) {
            ids.push_back(pair.first);
        }
        return ids;
    }

    const std::unordered_map<std::string, ViewTypeID>& ViewManager::GetRegisteredViews() const {
        return registeredViews;
    }

    const std::map<WorkspaceID, std::shared_ptr<ViewSignature>>& ViewManager::GetWorkspaceSignatures() const {
        return workspaceSignatures;
    }

    std::string ViewManager::LookupViewName(ViewTypeID viewType) const {
        for (const auto& [name, id] : registeredViews) {
            if (id == viewType) return name;
        }
        return {};
    }

    void ViewManager::AddViewByType(const WorkspaceID viewList, const ViewTypeID viewType) {
        assert(viewList < MAX_VIEW_COUNT && "WorkspaceID out of range!");

        std::string viewName = LookupViewName(viewType);

        if (m_viewOpeningCallback) {
            m_viewOpeningCallback(viewList, viewType, viewName);
        }

        GetViewSignature(viewList)->insert(viewType);
        CreateViewInstanceForWorkspace(viewList, viewType);

        if (m_viewOpenedCallback) {
            m_viewOpenedCallback(viewList, viewType, viewName);
        }

        ANI_LOG_INFO("[ViewManager] Added view type %u to workspace %u",
            (unsigned)viewType, (unsigned)viewList);
    }

    void ViewManager::RemoveViewByType(const WorkspaceID viewList, const ViewTypeID viewType) {
        assert(viewList < MAX_VIEW_COUNT && "WorkspaceID out of range!");

        std::string viewName = LookupViewName(viewType);
        json state = json::object();
        bool haveState = false;

        auto workspaceIt = workspaces.find(viewList);
        if (workspaceIt != workspaces.end()) {
            auto viewIt = workspaceIt->second.find(viewType);
            if (viewIt != workspaceIt->second.end() && viewIt->second) {
                try {
                    state = viewIt->second->Serialize();
                    haveState = true;
                }
                catch (const std::exception& e) {
                    ANI_LOG_ERROR("[ViewManager] Serialize failed for %s: %s",
                        viewName.c_str(), e.what());
                }
            }
        }

        if (haveState && m_viewClosingCallback) {
            m_viewClosingCallback(viewList, viewType, viewName, state);
        }

        GetViewSignature(viewList)->erase(viewType);
        RemoveViewInstanceFromWorkspace(viewList, viewType);

        if (m_viewClosedCallback) {
            m_viewClosedCallback(viewList, viewType, viewName);
        }

        ANI_LOG_INFO("[ViewManager] Removed view type %u from workspace %u",
            (unsigned)viewType, (unsigned)viewList);
    }

    void ViewManager::CreateViewInstanceForWorkspace(WorkspaceID workspaceID, ViewTypeID viewTypeID) {
        std::string viewTypeName = LookupViewName(viewTypeID);

        if (viewTypeName.empty()) {
            ANI_LOG_ERROR("[ViewManager] No view name found for type ID %u", (unsigned)viewTypeID);
            return;
        }

        ANI_LOG_INFO("[ViewManager] Found view name '%s' for type ID %u",
            viewTypeName.c_str(), (unsigned)viewTypeID);

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

            try {
                ANI_LOG_INFO("[ViewManager] Calling factory for '%s'", viewTypeName.c_str());
                auto view = factoryIt->second(*entityManager, *this);

                if (view) {
                    view->workspaceID = workspaceID;
                    ANI_LOG_INFO("[ViewManager] Calling Init() for %s", viewTypeName.c_str());
                    view->Init();
                    ANI_LOG_INFO("[ViewManager] Init() succeeded for %s", viewTypeName.c_str());
                    workspaces[workspaceID][viewTypeID] = std::move(view);
                    ANI_LOG_INFO("[ViewManager] SUCCESS: Created view instance '%s' for workspace %u",
                        viewTypeName.c_str(), (unsigned)workspaceID);
                }
                else {
                    ANI_LOG_ERROR("[ViewManager] Factory returned nullptr for '%s'", viewTypeName.c_str());
                }
            }
            catch (const std::exception& e) {
                ANI_LOG_ERROR("[ViewManager] EXCEPTION creating view '%s': %s",
                    viewTypeName.c_str(), e.what());
            }

            if (contextSwitched && previousContext) {
                ImGui::SetCurrentContext(previousContext);
            }
        }
        else {
            ANI_LOG_ERROR("[ViewManager] No factory found for '%s'", viewTypeName.c_str());
        }
    }

    void ViewManager::RemoveViewInstanceFromWorkspace(WorkspaceID workspaceID, ViewTypeID viewTypeID) {
        auto workspaceIt = workspaces.find(workspaceID);
        if (workspaceIt != workspaces.end()) {
            auto viewIt = workspaceIt->second.find(viewTypeID);
            if (viewIt != workspaceIt->second.end()) {
                ANI_LOG_INFO("[ViewManager] Removing view instance (type %u) from workspace %u",
                    (unsigned)viewTypeID, (unsigned)workspaceID);
                workspaceIt->second.erase(viewIt);

                if (workspaceIt->second.empty()) {
                    workspaces.erase(workspaceIt);
                }
            }
        }

        for (auto& [typeID, workspace] : workspaceArrays) {
            if (typeID == viewTypeID) {
                workspace->Erase(workspaceID);
                break;
            }
        }
    }

    void ViewManager::SetImGuiContext(void* context) {
        m_imguiContext = context;
        if (!context) {
            ANI_LOG_WARN("[ViewManager] SetImGuiContext: null context");
        }
        else {
            ANI_LOG_DEBUG("[ViewManager] Set ImGui context: %p", context);
        }
    }

    void* ViewManager::GetImGuiContext() const {
        return m_imguiContext;
    }

    void ViewManager::SetWindowHandle(void* handle) {
        m_windowHandle = handle;
        if (!handle) {
            ANI_LOG_WARN("[ViewManager] SetWindowHandle: null handle");
        }
        else {
            ANI_LOG_DEBUG("[ViewManager] Set window handle: %p", handle);
        }
    }

    void* ViewManager::GetWindowHandle() {
        return m_windowHandle;
    }

    void ViewManager::SetWorkspaceName(WorkspaceID workspaceID, const std::string& name) {
        if (name.empty()) {
            ANI_LOG_WARN("[ViewManager] Cannot set empty workspace name (workspace %u)",
                (unsigned)workspaceID);
            return;
        }

        if (IsWorkspaceNameTaken(name, workspaceID)) {
            ANI_LOG_WARN("[ViewManager] Workspace name '%s' is already taken", name.c_str());
            return;
        }

        workspaceNames[workspaceID] = name;
        ANI_LOG_INFO("[ViewManager] Set workspace %u name to: %s",
            (unsigned)workspaceID, name.c_str());
    }

    std::string ViewManager::GetWorkspaceName(WorkspaceID workspaceID) const {
        auto it = workspaceNames.find(workspaceID);
        if (it != workspaceNames.end()) {
            return it->second;
        }
        return "Workspace " + std::to_string(workspaceID);
    }

    bool ViewManager::IsWorkspaceNameTaken(const std::string& name, WorkspaceID excludeID) const {
        for (const auto& [id, workspaceName] : workspaceNames) {
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

        ANI_LOG_INFO("[ViewManager] Serializing workspaces...");

        std::set<WorkspaceID> allWorkspaceIDs;

        for (const auto& [workspaceID, signature] : workspaceSignatures) {
            allWorkspaceIDs.insert(workspaceID);
        }

        for (const auto& [workspaceID, viewsMap] : workspaces) {
            allWorkspaceIDs.insert(workspaceID);
        }

        ANI_LOG_INFO("[ViewManager] Total unique workspaces found: %zu", allWorkspaceIDs.size());

        for (WorkspaceID workspaceID : allWorkspaceIDs) {
            json workspaceJson = json::object();
            workspaceJson["ID"] = workspaceID;
            workspaceJson["name"] = GetWorkspaceName(workspaceID);
            workspaceJson["views"] = json::array();

            ANI_LOG_INFO("[ViewManager] Serializing workspace %u (%s)",
                (unsigned)workspaceID, GetWorkspaceName(workspaceID).c_str());

            auto workspaceIt = workspaces.find(workspaceID);
            if (workspaceIt != workspaces.end()) {
                for (const auto& [viewTypeID, view] : workspaceIt->second) {
                    if (view) {
                        std::string viewTypeName = LookupViewName(viewTypeID);

                        if (!viewTypeName.empty()) {
                            json viewJson = json::object();
                            try {
                                viewJson[viewTypeName] = view->Serialize();
                            }
                            catch (const std::exception& e) {
                                ANI_LOG_ERROR("[ViewManager] Serialize failed for %s: %s",
                                    viewTypeName.c_str(), e.what());
                                viewJson[viewTypeName] = json::object();
                            }
                            workspaceJson["views"].push_back(viewJson);

                            ANI_LOG_INFO("[ViewManager]   Added view: %s with data", viewTypeName.c_str());
                        }
                    }
                }
            }

            auto signatureIt = workspaceSignatures.find(workspaceID);
            if (signatureIt != workspaceSignatures.end()) {
                for (const auto& viewTypeID : *(signatureIt->second)) {
                    auto workspaceIt = workspaces.find(workspaceID);
                    if (workspaceIt != workspaces.end() && workspaceIt->second.find(viewTypeID) != workspaceIt->second.end()) {
                        continue;
                    }

                    std::string viewTypeName = LookupViewName(viewTypeID);

                    if (!viewTypeName.empty()) {
                        json viewJson = json::object();
                        viewJson[viewTypeName] = json::object();
                        workspaceJson["views"].push_back(viewJson);

                        ANI_LOG_INFO("[ViewManager]   Added template view: %s (no data)", viewTypeName.c_str());
                    }
                }
            }

            std::string workspaceKey = std::to_string(workspaceID);
            workspacesJson[workspaceKey] = workspaceJson;

            ANI_LOG_INFO("[ViewManager] Workspace %u serialized with %zu views",
                (unsigned)workspaceID, workspaceJson["views"].size());
        }

        ANI_LOG_INFO("[ViewManager] Serialization complete. Total workspaces: %zu", workspacesJson.size());
        return workspacesJson;
    }

    void ViewManager::DeserializeViewLists(const json& workspacesJson) {
        ANI_LOG_INFO("[ViewManager] Deserializing workspaces...");

        if (workspacesJson.is_null()) {
            ANI_LOG_INFO("[ViewManager] Workspaces JSON is null, nothing to deserialize");
            return;
        }

        if (!workspacesJson.is_object()) {
            ANI_LOG_ERROR("[ViewManager] Workspaces JSON is not an object!");
            return;
        }

        ANI_LOG_INFO("[ViewManager] Found %zu workspaces to deserialize", workspacesJson.size());

        if (!entityManager) {
            ANI_LOG_ERROR("[ViewManager] entityManager is null during deserialization!");
            return;
        }

        std::set<WorkspaceID> usedWorkspaceIDs;

        for (auto workspaceIt = workspacesJson.begin(); workspaceIt != workspacesJson.end(); ++workspaceIt) {
            const json& workspaceJson = workspaceIt.value();

            if (!workspaceJson.contains("ID")) {
                ANI_LOG_ERROR("[ViewManager] Workspace JSON missing ID");
                continue;
            }

            WorkspaceID workspaceID = workspaceJson["ID"];
            ANI_LOG_INFO("[ViewManager] Deserializing workspace %u", (unsigned)workspaceID);

            usedWorkspaceIDs.insert(workspaceID);

            if (workspaceJson.contains("name")) {
                std::string loadedName = workspaceJson["name"];
                if (IsWorkspaceNameTaken(loadedName, workspaceID)) {
                    loadedName = GenerateUniqueWorkspaceName(loadedName);
                    ANI_LOG_INFO("[ViewManager] Name conflict resolved, using: %s", loadedName.c_str());
                }
                workspaceNames[workspaceID] = loadedName;
            }
            else {
                workspaceNames[workspaceID] = GenerateUniqueWorkspaceName("Workspace");
            }

            if (workspaceSignatures.find(workspaceID) == workspaceSignatures.end()) {
                AddViewSignature(workspaceID);
                ANI_LOG_INFO("[ViewManager] Created signature for workspace %u", (unsigned)workspaceID);
            }

            if (!workspaceJson.contains("views") || !workspaceJson["views"].is_array()) {
                ANI_LOG_INFO("[ViewManager] Workspace %u has no views", (unsigned)workspaceID);
                continue;
            }

            for (const auto& viewJson : workspaceJson["views"]) {
                if (!viewJson.is_object()) {
                    ANI_LOG_ERROR("[ViewManager] View JSON is not an object");
                    continue;
                }

                for (auto viewIt = viewJson.begin(); viewIt != viewJson.end(); ++viewIt) {
                    std::string viewTypeName = viewIt.key();
                    const json& viewData = viewIt.value();

                    ANI_LOG_INFO("[ViewManager]   Deserializing view: %s", viewTypeName.c_str());

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

                            try {
                                ANI_LOG_INFO("[ViewManager] Calling factory for %s", viewTypeName.c_str());
                                auto view = factoryIt->second(*entityManager, *this);
                                ANI_LOG_INFO("[ViewManager] Factory call returned for %s", viewTypeName.c_str());

                                if (view) {
                                    view->workspaceID = workspaceID;
                                    ANI_LOG_INFO("[ViewManager] Calling Init() for %s", viewTypeName.c_str());
                                    view->Init();
                                    ANI_LOG_INFO("[ViewManager] Init() succeeded for %s", viewTypeName.c_str());

                                    if (!viewData.is_null() && !viewData.empty()) {
                                        ANI_LOG_INFO("[ViewManager] Calling Deserialize() for %s", viewTypeName.c_str());
                                        view->Deserialize(viewData);
                                        ANI_LOG_INFO("[ViewManager] Deserialize() succeeded for %s", viewTypeName.c_str());
                                        ANI_LOG_INFO("[ViewManager]     Recreated view: %s with data", viewTypeName.c_str());
                                    }
                                    else {
                                        ANI_LOG_INFO("[ViewManager]     Recreated view: %s (no data)", viewTypeName.c_str());
                                    }

                                    workspaces[workspaceID][viewTypeID] = std::move(view);
                                }
                                else {
                                    ANI_LOG_ERROR("[ViewManager] Factory returned nullptr for %s", viewTypeName.c_str());
                                }
                            }
                            catch (const std::exception& e) {
                                ANI_LOG_ERROR("[ViewManager] Exception during view creation/init: %s", e.what());
                            }
                            catch (...) {
                                ANI_LOG_ERROR("[ViewManager] Unknown exception during view creation/init");
                            }

                            if (contextSwitched && previousContext) {
                                ImGui::SetCurrentContext(previousContext);
                            }
                        }
                        else {
                            ANI_LOG_INFO("[ViewManager]     Added view to signature only: %s", viewTypeName.c_str());
                            if (!entityManager) {
                                ANI_LOG_ERROR("[ViewManager] entityManager is null, cannot instantiate view");
                            }
                            if (factoryIt == viewFactories.end()) {
                                ANI_LOG_ERROR("[ViewManager] No factory found for %s", viewTypeName.c_str());
                            }
                        }
                    }
                    catch (const std::exception& e) {
                        ANI_LOG_ERROR("[ViewManager] Failed to deserialize view %s: %s",
                            viewTypeName.c_str(), e.what());
                    }
                    catch (...) {
                        ANI_LOG_ERROR("[ViewManager] Unknown exception deserializing view %s", viewTypeName.c_str());
                    }
                }
            }
        }

        ANI_LOG_INFO("[ViewManager] Updating workspace tracking...");

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

        ANI_LOG_INFO("[ViewManager] Workspace tracking updated:");
        ANI_LOG_INFO("  - Active workspaces: %u", (unsigned)workspaceCount);
        ANI_LOG_INFO("  - Highest used ID: %u", (unsigned)maxUsedID);
        ANI_LOG_INFO("  - Available workspace IDs: %zu", availableWorkspaces.size());

        ANI_LOG_INFO("[ViewManager] Deserialization complete");
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
        for (auto& viewSignaturePair : workspaceSignatures) {
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

        ANI_LOG_INFO("[ViewManager] Registration data cleared");
    }

} // namespace GUI