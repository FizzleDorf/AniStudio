#pragma once

#include "ProjectPopups.hpp"
#include <vector>
#include <string>

namespace ANI { class ProjectManager; }

namespace GUI {

	class ProjectManagerView {
	public:
		ProjectManagerView(ANI::ProjectManager& projectMgr);

		void Init();
		void Update(const float deltaT);
		void Render();

		// Public methods for MenuBar to trigger popups
		void ShowNewProjectDialog();
		void ShowLoadProjectDialog();

	private:
		ANI::ProjectManager& m_projectManager;

		// Use the same popup system as MenuBar!
		ProjectPopupState popupState;
	};

} // namespace GUI