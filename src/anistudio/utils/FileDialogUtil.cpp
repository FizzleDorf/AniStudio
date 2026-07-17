// FileDialogUtil.cpp
#include "FileDialogUtil.hpp"
#include "FileDialogFilters.hpp"
#include <nfd.hpp>
#include <vector>
#include <string>
#include <iostream>

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

    bool OpenFile(const std::string& title, FilterType type, std::string& outPath, const std::string& defaultPath) {
        if (!EnsureNFDInitialized()) return false;
        const nfdu8filteritem_t* filterList;
        nfdfiltersize_t filterCount;
        GetFilterItems(type, filterList, filterCount);

        nfdu8char_t* outPtr = nullptr;
        nfdresult_t result = NFD::OpenDialog(outPtr,
            filterList,
            filterCount,
            defaultPath.empty() ? nullptr : defaultPath.c_str(),
            {},
            title.empty() ? nullptr : title.c_str());
        if (result == NFD_OKAY) {
            outPath = outPtr;
            NFD::FreePath(outPtr);
            return true;
        }
        return false;
    }

    bool OpenFiles(const std::string& title, FilterType type, std::vector<std::string>& outPaths, const std::string& defaultPath) {
        if (!EnsureNFDInitialized()) return false;
        const nfdu8filteritem_t* filterList;
        nfdfiltersize_t filterCount;
        GetFilterItems(type, filterList, filterCount);

        const nfdpathset_t* outPathsSet = nullptr;
        nfdresult_t result = NFD::OpenDialogMultiple(outPathsSet,
            filterList,
            filterCount,
            defaultPath.empty() ? nullptr : defaultPath.c_str(),
            {},
            title.empty() ? nullptr : title.c_str());
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
        return false;
    }

    bool SaveFile(const std::string& title, FilterType type, const std::string& defaultName, std::string& outPath, const std::string& defaultPath) {
        if (!EnsureNFDInitialized()) return false;
        const nfdu8filteritem_t* filterList;
        nfdfiltersize_t filterCount;
        GetFilterItems(type, filterList, filterCount);

        nfdu8char_t* outPtr = nullptr;
        nfdresult_t result = NFD::SaveDialog(outPtr,
            filterList,
            filterCount,
            defaultPath.empty() ? nullptr : defaultPath.c_str(),
            defaultName.empty() ? nullptr : defaultName.c_str(),
            {},
            title.empty() ? nullptr : title.c_str());
        if (result == NFD_OKAY) {
            outPath = outPtr;
            NFD::FreePath(outPtr);
            return true;
        }
        return false;
    }

    bool SelectFolder(const std::string& title, std::string& outPath, const std::string& defaultPath) {
        if (!EnsureNFDInitialized()) return false;
        nfdu8char_t* outPtr = nullptr;
        nfdresult_t result = NFD::PickFolder(outPtr,
            defaultPath.empty() ? nullptr : defaultPath.c_str(),
            {},
            title.empty() ? nullptr : title.c_str());
        if (result == NFD_OKAY) {
            outPath = outPtr;
            NFD::FreePath(outPtr);
            return true;
        }
        return false;
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