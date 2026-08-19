#pragma once
#include "BaseView.hpp"
#include "Workspace.hpp"
#include "ViewTypes.hpp"
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <exception>
#include <typeinfo>
#include <unordered_map>
#include <map>
#include <queue>
#include <set>
#include <mutex>

using json = nlohmann::json;

namespace GUI {

    using ViewCreationCallback = std::function<std::unique_ptr<BaseView>(ECS::EntityManager&, ViewManager&)>;

    class ViewManager {
    public:
        ViewManager();
        ~ViewManager() = default;

        void Init();
        void Update(const float deltaT);
        void Render();

        void SetActiveWorkspace(WorkspaceID workspaceID);
        WorkspaceID GetActiveWorkspace() const;
        void EnsureValidActiveWorkspace();

        const WorkspaceID CreateView();
        void DestroyView(const WorkspaceID viewList);

        template <typename T>
        void AddView(const WorkspaceID viewList, T&& view);

        template <typename T>
        void RemoveView(const WorkspaceID viewList);

        template <typename T>
        T& GetView(const WorkspaceID viewList);

        template <typename T>
        const bool HasView(const WorkspaceID viewList);

        template <typename T>
        void RegisterView(const std::string& name, const std::string& source = "Core");

        void RegisterViewWithFactory(const std::string& name, const std::string& source,
            ViewCreationCallback factory, std::function<ViewMetadata()> metadataGetter);

        WorkspaceID CreateViewByName(const std::string& viewTypeName, ECS::EntityManager& entityMgr);

        void UnregisterView(const std::string& name);
        void UnregisterViewByType(ViewTypeID viewType);
        void UnregisterViewSource(const std::string& source);
        bool UnregisterViewType(const std::string& viewName);
        void CloseAllViewsOfType(const std::string& viewName);

        ViewMetadata GetViewMetadata(const std::string& viewTypeName) const;
        std::vector<std::string> GetViewsByCategory(const std::string& category) const;
        std::vector<std::string> GetViewCategories() const;
        std::vector<std::string> GetViewsBySource(const std::string& source) const;

        void UpdateWorkspaces(float deltaT);
        void RenderWorkspaces();

        std::unordered_map<WorkspaceID, std::unordered_map<ViewTypeID, std::unique_ptr<BaseView>>>& GetWorkspaces() {
            return workspaces;
        }
        const std::unordered_map<WorkspaceID, std::unordered_map<ViewTypeID, std::unique_ptr<BaseView>>>& GetWorkspaces() const {
            return workspaces;
        }

        void SetEntityManager(ECS::EntityManager& mgr) { entityManager = &mgr; }

        void SetImGuiContext(void* context);
        void* GetImGuiContext() const;

        static void SetWindowHandle(void* handle);
        static void* GetWindowHandle();

        ViewTypeID GetViewType(const std::string& name) const;

        void Reset();
        void FullReset();

        std::vector<WorkspaceID> GetAllWorkspaces() const;
        const std::unordered_map<std::string, ViewTypeID>& GetRegisteredViews() const;
        const std::map<WorkspaceID, std::shared_ptr<ViewSignature>>& GetWorkspaceSignatures() const;

        void AddViewByType(const WorkspaceID viewList, const ViewTypeID viewType);
        void RemoveViewByType(const WorkspaceID viewList, const ViewTypeID viewType);

        void SetWorkspaceName(WorkspaceID workspaceID, const std::string& name);
        std::string GetWorkspaceName(WorkspaceID workspaceID) const;
        bool IsWorkspaceNameTaken(const std::string& name, WorkspaceID excludeID = 0) const;
        std::string GenerateUniqueWorkspaceName(const std::string& baseName) const;

        json SerializeViewLists() const;
        void DeserializeViewLists(const json& viewListsJson);
        std::vector<BaseView*> GetAllViews() const;

    private:
        template <typename T>
        void AddWorkspace();

        template <typename T>
        std::shared_ptr<Workspace<T>> GetWorkspace();

        template <typename T>
        std::shared_ptr<Workspace<T>> GetViewList();

        void AddViewSignature(const WorkspaceID viewList);
        std::shared_ptr<ViewSignature> GetViewSignature(const WorkspaceID viewList);

        void CreateViewInstanceForWorkspace(WorkspaceID workspaceID, ViewTypeID viewTypeID);
        void RemoveViewInstanceFromWorkspace(WorkspaceID workspaceID, ViewTypeID viewTypeID);

        void ResetWorkspaceData();
        void ResetRegistrationData();
        void RemoveViewFromAllWorkspaces(ViewTypeID viewType);

        WorkspaceID workspaceCount;
        std::queue<WorkspaceID> availableWorkspaces;
        std::map<WorkspaceID, std::shared_ptr<ViewSignature>> workspaceSignatures;
        std::map<ViewTypeID, std::shared_ptr<IWorkspace>> workspaceArrays;
        std::unordered_map<std::string, ViewTypeID> registeredViews;

        std::unordered_map<std::string, std::function<ViewMetadata()>> viewMetadata;
        std::unordered_map<std::string, std::string> viewSources;
        std::unordered_map<std::string, ViewCreationCallback> viewFactories;

        std::unordered_map<WorkspaceID, std::unordered_map<ViewTypeID, std::unique_ptr<BaseView>>> workspaces;
        std::unordered_map<WorkspaceID, std::string> workspaceNames;

        ECS::EntityManager* entityManager = nullptr;

        WorkspaceID m_activeWorkspaceID = 0;

        void* m_imguiContext = nullptr;

        static void* m_windowHandle;

        ViewTypeID m_nextViewID = 0;
        std::unordered_map<std::string, ViewTypeID> m_viewNameToID;
        std::unordered_map<ViewTypeID, std::string> m_viewIDToName;
        std::unordered_map<std::type_index, ViewTypeID> m_viewTypeToID;
        std::mutex m_viewRegistryMutex;

        ViewManager(const ViewManager&) = delete;
        ViewManager& operator=(const ViewManager&) = delete;
    };

    template <typename T>
    void ViewManager::AddView(const WorkspaceID viewList, T&& view) {
        assert(viewList < MAX_VIEW_COUNT && "WorkspaceID out of range!");
        assert(GetViewSignature(viewList)->size() < MAX_VIEW_COUNT && "View count limit reached!");

        if (HasView<T>(viewList)) {
            std::cerr << "View with ID " << viewList << " already exists! Skipping AddView." << std::endl;
            return;
        }

        std::lock_guard<std::mutex> lock(m_viewRegistryMutex);
        std::type_index typeIdx = std::type_index(typeid(T));
        auto it = m_viewTypeToID.find(typeIdx);
        ViewTypeID viewType;
        if (it != m_viewTypeToID.end()) {
            viewType = it->second;
        }
        else {
            viewType = m_nextViewID++;
            m_viewTypeToID[typeIdx] = viewType;
        }

        view.workspaceID = viewList;
        GetViewSignature(viewList)->insert(viewType);
        view.Init();

        auto viewListPtr = GetViewList<T>();
        std::cout << "Adding and initializing view - ID: " << viewList << ", Type: " << typeid(T).name() << std::endl;
        viewListPtr->Insert(std::forward<T>(view));
    }

    template <typename T>
    void ViewManager::RemoveView(const WorkspaceID viewList) {
        assert(viewList < MAX_VIEW_COUNT && "WorkspaceID out of range!");

        std::lock_guard<std::mutex> lock(m_viewRegistryMutex);
        std::type_index typeIdx = std::type_index(typeid(T));
        auto it = m_viewTypeToID.find(typeIdx);
        if (it != m_viewTypeToID.end()) {
            GetViewSignature(viewList)->erase(it->second);
            GetViewList<T>()->Erase(viewList);
        }
    }

    template <typename T>
    T& ViewManager::GetView(const WorkspaceID viewList) {
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

        std::lock_guard<std::mutex> lock(m_viewRegistryMutex);
        std::type_index typeIdx = std::type_index(typeid(T));
        auto viewIt = m_viewTypeToID.find(typeIdx);
        if (viewIt == m_viewTypeToID.end()) {
            return false;
        }

        const ViewSignature& signature = *(it->second);
        return (signature.count(viewIt->second) > 0);
    }

    template <typename T>
    void ViewManager::RegisterView(const std::string& name, const std::string& source) {
        std::lock_guard<std::mutex> lock(m_viewRegistryMutex);

        std::type_index typeIdx = std::type_index(typeid(T));

        auto it = m_viewTypeToID.find(typeIdx);
        ViewTypeID typeId;
        if (it != m_viewTypeToID.end()) {
            typeId = it->second;
        }
        else {
            auto nameIt = m_viewNameToID.find(name);
            if (nameIt != m_viewNameToID.end()) {
                typeId = nameIt->second;
                m_viewTypeToID[typeIdx] = typeId;
            }
            else {
                typeId = m_nextViewID++;
                m_viewNameToID[name] = typeId;
                m_viewIDToName[typeId] = name;
                m_viewTypeToID[typeIdx] = typeId;
            }
        }

        registeredViews[name] = typeId;
        viewSources[name] = source;
        viewMetadata[name] = []() -> ViewMetadata {
            return BaseView::GetMetadataFor<T>();
            };
        viewFactories[name] = [](ECS::EntityManager& mgr, ViewManager& vm) -> std::unique_ptr<BaseView> {
            return std::make_unique<T>(mgr, vm);
            };

        std::cout << "Registered view type: " << name << " with ID: " << typeId << std::endl;
    }

    template <typename T>
    void ViewManager::AddWorkspace() {
        std::lock_guard<std::mutex> lock(m_viewRegistryMutex);
        std::type_index typeIdx = std::type_index(typeid(T));
        auto it = m_viewTypeToID.find(typeIdx);
        ViewTypeID viewType;
        if (it != m_viewTypeToID.end()) {
            viewType = it->second;
        }
        else {
            viewType = m_nextViewID++;
            m_viewTypeToID[typeIdx] = viewType;
        }
        assert(workspaceArrays.find(viewType) == workspaceArrays.end() && "ViewList already registered!");
        workspaceArrays[viewType] = std::make_shared<Workspace<T>>();
    }

    template <typename T>
    std::shared_ptr<Workspace<T>> ViewManager::GetWorkspace() {
        std::lock_guard<std::mutex> lock(m_viewRegistryMutex);
        std::type_index typeIdx = std::type_index(typeid(T));
        auto it = m_viewTypeToID.find(typeIdx);
        ViewTypeID viewType;
        if (it != m_viewTypeToID.end()) {
            viewType = it->second;
        }
        else {
            viewType = m_nextViewID++;
            m_viewTypeToID[typeIdx] = viewType;
        }
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