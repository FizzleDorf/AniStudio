// FileDialogUtil.cpp
#include "FileDialogUtil.hpp"
#include <nfd.h>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <cctype>

namespace FileDialog {

    static std::string Trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t");
        return str.substr(first, last - first + 1);
    }

    // Parse filter string into a vector of filter items
    static std::vector<nfdu8filteritem_t> ParseFilters(const std::string& filterStr) {
        std::vector<nfdu8filteritem_t> filterItems;
        if (filterStr.empty()) {
            nfdu8filteritem_t all = { "All Files", "*" };
            filterItems.push_back(all);
            return filterItems;
        }

        size_t pos = 0;
        while (pos < filterStr.size()) {
            size_t braceOpen = filterStr.find('{', pos);
            if (braceOpen == std::string::npos) break;

            std::string description = filterStr.substr(pos, braceOpen - pos);
            description = Trim(description);
            if (description.empty()) description = "All Files";

            size_t braceClose = filterStr.find('}', braceOpen);
            if (braceClose == std::string::npos) break;

            std::string extList = filterStr.substr(braceOpen + 1, braceClose - braceOpen - 1);
            extList = Trim(extList);

            std::string spec;
            size_t extPos = 0;
            while (extPos < extList.size()) {
                size_t comma = extList.find(',', extPos);
                std::string ext = extList.substr(extPos, comma - extPos);
                ext = Trim(ext);
                if (!ext.empty()) {
                    if (ext.find("*.") == 0) ext = ext.substr(2);
                    else if (ext.find(".") == 0) ext = ext.substr(1);
                    if (ext == "*") ext = "";
                }
                if (!ext.empty()) {
                    if (!spec.empty()) spec += ",";
                    spec += ext;
                }
                if (comma == std::string::npos) break;
                extPos = comma + 1;
            }
            if (spec.empty()) spec = "*";

            nfdu8filteritem_t item = { description.c_str(), spec.c_str() };
            filterItems.push_back(item);

            pos = braceClose + 1;
            if (pos < filterStr.size() && filterStr[pos] == ',') pos++;
        }

        if (filterItems.empty()) {
            nfdu8filteritem_t all = { "All Files", "*" };
            filterItems.push_back(all);
        }
        return filterItems;
    }

    bool OpenFile(const std::string& title, const std::string& filter, std::string& outPath, const std::string& defaultPath) {
        nfdu8char_t* out = nullptr;
        auto filterItems = ParseFilters(filter);
        nfdopendialogu8args_t args = { 0 };
        args.filterList = filterItems.data();
        args.filterCount = static_cast<nfdfiltersize_t>(filterItems.size());
        args.defaultPath = defaultPath.empty() ? nullptr : defaultPath.c_str();
        args.title = title.empty() ? nullptr : title.c_str();

        nfdresult_t result = NFD_OpenDialogU8_With(&out, &args);
        if (result == NFD_OKAY) {
            outPath = out;
            NFD_FreePathU8(out);
            return true;
        }
        return false;
    }

    bool OpenFiles(const std::string& title, const std::string& filter, std::vector<std::string>& outPaths, const std::string& defaultPath) {
        const nfdpathset_t* pathSet = nullptr;
        auto filterItems = ParseFilters(filter);
        nfdopendialogu8args_t args = { 0 };
        args.filterList = filterItems.data();
        args.filterCount = static_cast<nfdfiltersize_t>(filterItems.size());
        args.defaultPath = defaultPath.empty() ? nullptr : defaultPath.c_str();
        args.title = title.empty() ? nullptr : title.c_str();

        nfdresult_t result = NFD_OpenDialogMultipleU8_With(&pathSet, &args);
        if (result == NFD_OKAY) {
            nfdpathsetsize_t count;
            if (NFD_PathSet_GetCount(pathSet, &count) == NFD_OKAY) {
                for (nfdpathsetsize_t i = 0; i < count; ++i) {
                    nfdu8char_t* path = nullptr;
                    if (NFD_PathSet_GetPathU8(pathSet, i, &path) == NFD_OKAY) {
                        outPaths.push_back(path);
                        NFD_PathSet_FreePathU8(path);
                    }
                }
            }
            NFD_PathSet_Free(pathSet);
            return true;
        }
        return false;
    }

    bool SaveFile(const std::string& title, const std::string& filter, const std::string& defaultName, std::string& outPath, const std::string& defaultPath) {
        nfdu8char_t* out = nullptr;
        auto filterItems = ParseFilters(filter);
        nfdsavedialogu8args_t args = { 0 };
        args.filterList = filterItems.data();
        args.filterCount = static_cast<nfdfiltersize_t>(filterItems.size());
        args.defaultPath = defaultPath.empty() ? nullptr : defaultPath.c_str();
        args.defaultName = defaultName.empty() ? nullptr : defaultName.c_str();
        args.title = title.empty() ? nullptr : title.c_str();

        nfdresult_t result = NFD_SaveDialogU8_With(&out, &args);
        if (result == NFD_OKAY) {
            outPath = out;
            NFD_FreePathU8(out);
            return true;
        }
        return false;
    }

    bool OpenFile(const std::string& title, FilterType type, std::string& outPath, const std::string& defaultPath) {
        return OpenFile(title, FileFilters::GetFilter(type), outPath, defaultPath);
    }

    bool OpenFiles(const std::string& title, FilterType type, std::vector<std::string>& outPaths, const std::string& defaultPath) {
        return OpenFiles(title, FileFilters::GetFilter(type), outPaths, defaultPath);
    }

    bool SaveFile(const std::string& title, FilterType type, const std::string& defaultName, std::string& outPath, const std::string& defaultPath) {
        return SaveFile(title, FileFilters::GetFilter(type), defaultName, outPath, defaultPath);
    }

    bool SelectFolder(const std::string& title, std::string& outPath, const std::string& defaultPath) {
        nfdu8char_t* out = nullptr;
        nfdpickfolderu8args_t args = { 0 };
        args.defaultPath = defaultPath.empty() ? nullptr : defaultPath.c_str();
        args.title = title.empty() ? nullptr : title.c_str();

        nfdresult_t result = NFD_PickFolderU8_With(&out, &args);
        if (result == NFD_OKAY) {
            outPath = out;
            NFD_FreePathU8(out);
            return true;
        }
        return false;
    }

}