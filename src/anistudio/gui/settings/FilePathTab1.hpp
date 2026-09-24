#pragma once
#include "BaseSettingsTab.hpp"
#include "FilePathSystem.hpp"
#include <unordered_map>
#include <string>
#include <vector>
#include <functional>
#include <map>

namespace ECS {

    class FilePathTab : public BaseSettingsTab {
    public:
        explicit FilePathTab(FilePathSystem& fs);
        ~FilePathTab() = default;

        std::string GetTabName() const override { return "Paths"; }
        std::string GetTabCategory() const override { return "System"; }
        void Render() override;
        void SetFilter(const std::string& filter) override { m_filter = filter; }

        bool HasUnsavedChanges() const override { return m_hasChanges; }
        void CreateBackup() override;
        void RestoreFromBackup() override;
        void ResetToDefaults() override;
        bool SaveSettings() override;
        bool LoadSettings() override;
        void SetImGuiContext(ImGuiContext* ctx) override {}

        static void RegisterDefaultPath(const std::string& key, const std::string& defaultPath);
        static void RegisterModelRootDependentPath(const std::string& key, const std::string& subdirectory);
        static void UpdateModelRootDefaults(const std::string& modelRoot);
        using CategoryMapper = std::function<std::string(const std::string&)>;
        static void RegisterCategoryMapper(CategoryMapper mapper);

    private:
        FilePathSystem& m_fs;
        std::string m_filter;
        char m_pathFilter[256] = "";
        bool m_hasChanges = false;
        std::unordered_map<std::string, std::string> m_backupPaths;

        bool FilterPass(const std::string& key) const;
        void RenderPathsTable();
        void RenderActionButtons();
        std::string GetDefaultPath(const std::string& key) const;
        void InitializeDefaults();
        std::string GetCategoryForPath(const std::string& key) const;
        void DetermineCategoryOrder(const std::map<std::string, std::vector<std::string>>& categoryMap,
            std::vector<std::string>& order) const;
        bool IsFileSelector(const std::string& key) const;
        bool IsPathHidden(const std::string& key) const;

        static std::unordered_map<std::string, std::string> s_defaultPaths;
        static std::unordered_map<std::string, std::string> s_modelRootDependentPaths;
        static std::vector<CategoryMapper> s_categoryMappers;
        static std::string s_currentModelRoot;
    };

}