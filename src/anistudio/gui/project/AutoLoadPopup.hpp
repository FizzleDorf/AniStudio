#pragma once

#include <imgui.h>
#include <string>
#include <functional>

namespace GUI {

	struct AutoLoadPopupState {
		bool showPopup = false;
		bool userChoiceMade = false;
		bool shouldAutoLoad = false;
		std::string lastProjectPath;
		std::string lastProjectName;
	};

	class AutoLoadPopup {
	public:
		// Show the auto-load popup
		static bool Show(AutoLoadPopupState& state);

		// Check if we should show the popup (has valid last project)
		static bool ShouldShow(const std::string& lastProjectPath);

		// Get the project name from path for display
		static std::string GetProjectNameFromPath(const std::string& path);
	};

} // namespace GUI