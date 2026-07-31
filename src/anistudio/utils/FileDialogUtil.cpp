// FileDialogUtil.cpp
#include "FileDialogUtil.hpp"
#include "FileDialogFilters.hpp"
#include <nfd.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <filesystem>

#ifdef _WIN32
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

namespace FileDialog {

    static bool EnsureNFDInitialized() {
        static bool initialized = false;
        if (!initialized) {
            nfdresult_t initResult = NFD::Init();
            if (initResult == NFD_OKAY) {
                initialized = true;
            }
            else {
                std::cerr << "NFD::Init() failed with error: " << NFD::GetError() << std::endl;
                return false;
            }
        }
        return true;
    }

    static std::string PrepareDefaultPath(const std::string& path) {
        std::error_code ec;
        std::filesystem::path p(path);
        if (std::filesystem::is_regular_file(p, ec)) {
            p = p.parent_path();
        }
        if (!std::filesystem::is_directory(p, ec)) {
            p = std::filesystem::current_path();
        }
        return std::filesystem::absolute(p).string();
    }

    static GLFWwindow* g_window = nullptr;

    void SetGlobalWindowHandle(GLFWwindow* window) {
        g_window = window;
    }

    bool OpenFile(const std::string& title, FilterType type, std::string& outPath, const std::string& defaultPath) {
        if (!EnsureNFDInitialized()) return false;
        const nfdu8filteritem_t* filterList;
        nfdfiltersize_t filterCount;
        GetFilterItems(type, filterList, filterCount);

        std::string dirToUse = PrepareDefaultPath(defaultPath);

        nfdu8char_t* outPtr = nullptr;

        nfdwindowhandle_t parentWindow = {};
#ifdef _WIN32
        if (g_window) {
            HWND hwnd = glfwGetWin32Window(g_window);
            if (hwnd) {
                parentWindow.type = NFD_WINDOW_HANDLE_TYPE_WINDOWS;
                parentWindow.handle = hwnd;
            }
        }
#endif

        nfdresult_t result = NFD::OpenDialog(
            outPtr,
            (filterCount > 0) ? filterList : nullptr,
            filterCount,
            dirToUse.c_str(),
            parentWindow,
            title.empty() ? nullptr : title.c_str(),
            nullptr,
            nullptr
        );

        if (result == NFD_OKAY) {
            outPath = outPtr;
            NFD::FreePath(outPtr);
            return true;
        }
        else if (result == NFD_CANCEL) {
            return false;
        }
        else {
            std::cerr << "NFD::OpenDialog failed: " << NFD::GetError() << std::endl;
            return false;
        }
    }

    bool OpenFiles(const std::string& title, FilterType type, std::vector<std::string>& outPaths, const std::string& defaultPath) {
        if (!EnsureNFDInitialized()) return false;
        const nfdu8filteritem_t* filterList;
        nfdfiltersize_t filterCount;
        GetFilterItems(type, filterList, filterCount);

        std::string dirToUse = PrepareDefaultPath(defaultPath);

        const nfdpathset_t* outPathsSet = nullptr;

        nfdwindowhandle_t parentWindow = {};
#ifdef _WIN32
        if (g_window) {
            HWND hwnd = glfwGetWin32Window(g_window);
            if (hwnd) {
                parentWindow.type = NFD_WINDOW_HANDLE_TYPE_WINDOWS;
                parentWindow.handle = hwnd;
            }
        }
#endif

        nfdresult_t result = NFD::OpenDialogMultiple(
            outPathsSet,
            (filterCount > 0) ? filterList : nullptr,
            filterCount,
            dirToUse.c_str(),
            parentWindow,
            title.empty() ? nullptr : title.c_str(),
            nullptr,
            nullptr
        );

        if (result == NFD_OKAY) {
            nfdpathsetsize_t count;
            if (NFD::PathSet::Count(outPathsSet, count) == NFD_OKAY) {
                for (nfdpathsetsize_t i = 0; i < count; ++i) {
                    nfdu8char_t* pathPtr = nullptr;
                    if (NFD::PathSet::GetPath(outPathsSet, i, pathPtr) == NFD_OKAY) {
                        outPaths.push_back(pathPtr);
                        NFD::PathSet::FreePath(pathPtr);
                    }
                }
            }
            NFD::PathSet::Free(outPathsSet);
            return true;
        }
        else if (result == NFD_CANCEL) {
            return false;
        }
        else {
            std::cerr << "NFD::OpenDialogMultiple failed: " << NFD::GetError() << std::endl;
            return false;
        }
    }

    bool SaveFile(const std::string& title, FilterType type, const std::string& defaultName, std::string& outPath, const std::string& defaultPath) {
        if (!EnsureNFDInitialized()) return false;
        const nfdu8filteritem_t* filterList;
        nfdfiltersize_t filterCount;
        GetFilterItems(type, filterList, filterCount);

        std::string dirToUse = PrepareDefaultPath(defaultPath);

        nfdu8char_t* outPtr = nullptr;

        nfdwindowhandle_t parentWindow = {};
#ifdef _WIN32
        if (g_window) {
            HWND hwnd = glfwGetWin32Window(g_window);
            if (hwnd) {
                parentWindow.type = NFD_WINDOW_HANDLE_TYPE_WINDOWS;
                parentWindow.handle = hwnd;
            }
        }
#endif

        nfdresult_t result = NFD::SaveDialog(
            outPtr,
            (filterCount > 0) ? filterList : nullptr,
            filterCount,
            dirToUse.c_str(),
            defaultName.empty() ? nullptr : defaultName.c_str(),
            parentWindow,
            title.empty() ? nullptr : title.c_str(),
            nullptr,
            nullptr
        );

        if (result == NFD_OKAY) {
            outPath = outPtr;
            NFD::FreePath(outPtr);
            return true;
        }
        else if (result == NFD_CANCEL) {
            return false;
        }
        else {
            std::cerr << "NFD::SaveDialog failed: " << NFD::GetError() << std::endl;
            return false;
        }
    }

    bool SelectFolder(const std::string& title, std::string& outPath, const std::string& defaultPath) {
        if (!EnsureNFDInitialized()) return false;

        std::string dirToUse = PrepareDefaultPath(defaultPath);

        nfdu8char_t* outPtr = nullptr;

        nfdwindowhandle_t parentWindow = {};
#ifdef _WIN32
        if (g_window) {
            HWND hwnd = glfwGetWin32Window(g_window);
            if (hwnd) {
                parentWindow.type = NFD_WINDOW_HANDLE_TYPE_WINDOWS;
                parentWindow.handle = hwnd;
            }
        }
#endif

        nfdresult_t result = NFD::PickFolder(
            outPtr,
            dirToUse.c_str(),
            parentWindow,
            title.empty() ? nullptr : title.c_str(),
            nullptr,
            nullptr
        );

        if (result == NFD_OKAY) {
            outPath = outPtr;
            NFD::FreePath(outPtr);
            return true;
        }
        else if (result == NFD_CANCEL) {
            return false;
        }
        else {
            std::cerr << "NFD::PickFolder failed: " << NFD::GetError() << std::endl;
            return false;
        }
    }

    bool OpenFile(const std::string& title, const std::string& /*filter*/, std::string& outPath, const std::string& defaultPath) {
        return OpenFile(title, FilterType::ALL_FILES, outPath, defaultPath);
    }

    bool OpenFiles(const std::string& title, const std::string& /*filter*/, std::vector<std::string>& outPaths, const std::string& defaultPath) {
        return OpenFiles(title, FilterType::ALL_FILES, outPaths, defaultPath);
    }

    bool SaveFile(const std::string& title, const std::string& /*filter*/, const std::string& defaultName, std::string& outPath, const std::string& defaultPath) {
        return SaveFile(title, FilterType::ALL_FILES, defaultName, outPath, defaultPath);
    }

}