#pragma once
#include <string>
#include <vector>
#include <functional>
#include <memory>

// Forward declarations
namespace ECS {
	class IEntityManager;
}

namespace GUI {
	class BaseView;

	using WorkspaceID = uint32_t;
	using ViewTypeID = uint32_t;

	struct ViewMetadata {
		std::string displayName;
		std::string category;
		std::string description;
	};

	class IViewManager {
	public:
		virtual ~IViewManager() = default;

		// View registration
		virtual bool RegisterViewType(const std::string& name,
			const std::string& category,
			std::function<std::unique_ptr<BaseView>(ECS::IEntityManager&)> factory,
			std::function<ViewMetadata()> metadataGetter) = 0;

		// View creation
		virtual WorkspaceID CreateView(const std::string& viewTypeName, ECS::IEntityManager& entityMgr) = 0;
		virtual void CloseView(WorkspaceID viewID) = 0;

		// View management
		virtual void CloseAllViewsOfType(const std::string& viewTypeName) = 0;
		virtual bool UnregisterViewType(const std::string& viewTypeName) = 0;

		// Information
		virtual ViewMetadata GetViewMetadata(const std::string& viewTypeName) const = 0;
		virtual std::vector<std::string> GetViewsByCategory(const std::string& category) const = 0;
		virtual std::vector<std::string> GetViewCategories() const = 0;

		// Workspace management
		virtual void SetActiveWorkspace(WorkspaceID workspaceID) = 0;
		virtual WorkspaceID GetActiveWorkspace() const = 0;

		// Context
		virtual void SetImGuiContext(void* context) = 0;
	};
}