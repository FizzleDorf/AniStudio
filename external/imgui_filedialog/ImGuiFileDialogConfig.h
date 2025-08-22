#pragma once

// ImGuiFileDialog Configuration for AniStudio
// This file enables thumbnails and other useful features

/////////////////////////////////
//// STB FILE SYSTEM ////////////
/////////////////////////////////

// Use standard filesystem (requires C++17)
#define USE_STD_FILESYSTEM

/////////////////////////////////
//// THUMBNAILS /////////////////
/////////////////////////////////

// Enable thumbnails for image preview in file dialogs
// #define USE_THUMBNAILS

// Prevent duplicate STB implementations if already defined in your project
#define DONT_DEFINE_AGAIN__STB_IMAGE_IMPLEMENTATION
#define DONT_DEFINE_AGAIN__STB_IMAGE_RESIZE_IMPLEMENTATION

// Thumbnail display settings
// #define DisplayMode_ThumbailsList_ImageHeight 64.0f
// #define tableHeaderFileThumbnailsString "Thumbnails"

// Display mode button configurations
#define DisplayMode_FilesList_ButtonString "FL"
#define DisplayMode_FilesList_ButtonHelp "File List"
#define DisplayMode_ThumbailsList_ButtonString "TL"
#define DisplayMode_ThumbailsList_ButtonHelp "Thumbnails List"
#define DisplayMode_ThumbailsGrid_ButtonString "TG"
#define DisplayMode_ThumbailsGrid_ButtonHelp "Thumbnails Grid"

// Custom radio button for display mode selection
#define IMGUI_RADIO_BUTTON ImGui::RadioButton

/////////////////////////////////
//// EXPLORATION BY KEYS ////////
/////////////////////////////////

// Enable keyboard navigation in file lists
#define USE_EXPLORATION_BY_KEYS

// Key mappings for navigation
#define IGFD_KEY_UP ImGuiKey_UpArrow
#define IGFD_KEY_DOWN ImGuiKey_DownArrow
#define IGFD_KEY_ENTER ImGuiKey_Enter
#define IGFD_KEY_BACKSPACE ImGuiKey_Backspace

/////////////////////////////////
//// DIALOG EXIT ////////////////
/////////////////////////////////

// Allow ESC key to exit dialog
#define USE_DIALOG_EXIT_WITH_KEY
#define IGFD_EXIT_KEY ImGuiKey_Escape

/////////////////////////////////
//// SHORTCUTS => ctrl + KEY ////
/////////////////////////////////

// Ctrl+A to select all files
#define SelectAllFilesKey ImGuiKey_A

/////////////////////////////////
//// QUICK PATH /////////////////
/////////////////////////////////

// Enable quick path selection via path buttons
#define USE_QUICK_PATH_SELECT

/////////////////////////////////
//// WIDGETS ////////////////////
/////////////////////////////////

// Auto-resize filter combo box
#define FILTER_COMBO_AUTO_SIZE 1
#define FILTER_COMBO_MIN_WIDTH 150.0f

// Widget definitions
#define IMGUI_BEGIN_COMBO ImGui::BeginCombo
#define IMGUI_PATH_BUTTON ImGui::Button
#define IMGUI_BUTTON ImGui::Button

/////////////////////////////////
//// PLACES FEATURES ////////////
/////////////////////////////////

// Enable places sidebar for bookmarks and quick access
#define USE_PLACES_FEATURE
#define PLACES_PANE_DEFAULT_SHOWN false
#define defaultPlacePaneWith 200.0f

// Toggle button for places pane
#define IMGUI_TOGGLE_BUTTON ImGui::Checkbox

// Places strings
#define placesButtonString "Places"
#define placesButtonHelpString "Show/Hide Places Panel"
#define addPlaceButtonString "+"
#define removePlaceButtonString "-"
#define validatePlaceButtonString "OK"
#define editPlaceButtonString "Edit"

//////////////////////////////////////
//// PLACES FEATURES : BOOKMARKS /////
//////////////////////////////////////

// Enable bookmarks in places
#define USE_PLACES_BOOKMARKS
#define PLACES_BOOKMARK_DEFAULT_OPEPEND true
#define placesBookmarksGroupName "Bookmarks"
#define placesBookmarksDisplayOrder 0

//////////////////////////////////////
//// PLACES FEATURES : DEVICES ///////
//////////////////////////////////////

// Enable system devices in places
#define USE_PLACES_DEVICES
#define PLACES_DEVICES_DEFAULT_OPEPEND true
#define placesDevicesGroupName "Devices"
#define placesDevicesDisplayOrder 10

/////////////////////////////////
//// SORTING ICONS //////////////
/////////////////////////////////

// Enable custom sorting icons in table headers
#define USE_CUSTOM_SORTING_ICON
#define tableHeaderAscendingIcon "Å£ "
#define tableHeaderDescendingIcon "Å• "

// Table header strings
#define tableHeaderFileNameString "Name"
#define tableHeaderFileTypeString "Type"
#define tableHeaderFileSizeString "Size"
#define tableHeaderFileDateString "Modified"

// File size units
#define fileSizeBytes "B"
#define fileSizeKiloBytes "KB"
#define fileSizeMegaBytes "MB"
#define fileSizeGigaBytes "GB"

// Default sorting
#define defaultSortField FIELD_FILENAME
#define defaultSortOrderFilename true
#define defaultSortOrderType true
#define defaultSortOrderSize false
#define defaultSortOrderDate false
// #define defaultSortOrderThumbnails true

/////////////////////////////////
//// STRING'S ///////////////////
/////////////////////////////////

// UI strings for better UX
#define createDirButtonString "+"
#define resetButtonString "R"
#define devicesButtonString "Devices"
#define editPathButtonString "E"
#define searchString "Search:"

// Entry type indicators
#define dirEntryString "[DIR] "
#define linkEntryString "[LINK] "
#define fileEntryString "[FILE] "

// Dialog strings
#define fileNameString "File Name:"
#define dirNameString "Directory:"
#define buttonResetSearchString "Reset search"
#define buttonDriveString "Show Devices"
#define buttonEditPathString "Edit Path\nRight-click path buttons for quick edit"
#define buttonResetPathString "Reset to current directory"
#define buttonCreateDirString "Create New Directory"

// Validation buttons
#define okButtonString "OK"
#define okButtonWidth 80.0f
#define cancelButtonString "Cancel"
#define cancelButtonWidth 80.0f
#define okCancelButtonAlignement 1.0f  // Right-aligned
#define invertOkAndCancelButtons 0

// Overwrite dialog
#define OverWriteDialogTitleString "File Already Exists"
#define OverWriteDialogMessageString "A file with this name already exists. Do you want to overwrite it?"
#define OverWriteDialogConfirmButtonString "Overwrite"
#define OverWriteDialogCancelButtonString "Cancel"

// Date format (see strftime)
#define DateTimeFormat "%Y-%m-%d %H:%M"

/////////////////////////////////
//// MISC ///////////////////////
/////////////////////////////////

// Path spacing
#define CUSTOM_PATH_SPACING 4

// Buffer sizes
#define MAX_FILE_DIALOG_NAME_BUFFER 1024
#define MAX_PATH_BUFFER_SIZE 2048