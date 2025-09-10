#pragma once
#include <vector>
#include <string>

namespace ANI { class ProjectManager; }

namespace GUI {

	// Standalone load project modal - not derived from BaseView
	class LoadProjectView {
	public:
		LoadProjectView(ANI::ProjectManager& projectMgr);

		void Init();
		void Update(const float deltaT);
		void Render();

		// Control visibility
		void SetVisible(bool visible) { m_showPopup = visible; }

	private:
		ANI::ProjectManager& m_projectManager;
		std::vector<std::string> m_recentProjects;

		// Simple popup state
		bool m_showPopup = false;

		void RefreshRecentProjects();
		void ShowRecentProjects();
		void ShowBrowseOption();
	};

} // namespace GUI