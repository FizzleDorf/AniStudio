#pragma once

#include "ProjectManager.hpp"
#include "ProjectPopups.hpp"
#include "AutoLoadPopup.hpp"

namespace GUI {

	class ProjectManagerView {
	private:
		ANI::ProjectManager& m_projectManager;
		ProjectPopupState popupState;
		AutoLoadPopupState autoLoadState;

	public:
		ProjectManagerView(ANI::ProjectManager& projectMgr);

		void Init();
		void Update(const float deltaT);
		void Render();

		void ShowNewProjectDialog();
		void ShowLoadProjectDialog();
		void ShowAutoLoadPopup(const std::string& lastProjectPath);


		bool IsAutoLoadPopupActive() const { return autoLoadState.showPopup; }
		bool ShouldAutoLoad() const { return autoLoadState.shouldAutoLoad; }

	private:
		void RenderAutoLoadPopup();
	};

} // namespace GUI