#pragma once

#include <nlohmann/json.hpp>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include "ViewTypes.hpp"

namespace GUI {
	class ViewManager;

	class ViewState {
	public:
		ViewState() = default;
		~ViewState() = default;

		// Save/Load ViewManager state
		bool SaveViewManagerState(const ViewManager& viewManager, const std::string& filepath) const;
		bool LoadViewManagerState(ViewManager& viewManager, const std::string& filepath);

		// Active workspace tracking
		void SetLastActiveWorkspace(WorkspaceID workspaceID) { m_lastActiveWorkspace = workspaceID; }
		WorkspaceID GetLastActiveWorkspace() const { return m_lastActiveWorkspace; }

		// Serialization
		nlohmann::json SerializeViewManagerState(const ViewManager& viewManager) const;
		void DeserializeViewManagerState(ViewManager& viewManager, const nlohmann::json& j);

		// Reset to defaults
		void Reset();

	private:
		WorkspaceID m_lastActiveWorkspace = 0;
	};

} // namespace GUI