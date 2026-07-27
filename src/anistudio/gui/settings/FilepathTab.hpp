#pragma once
#include "BaseSettingsTab.hpp"
#include "FilePathSystem.hpp"
#include <unordered_map>
#include <string>

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

    private:
        FilePathSystem& m_fs;
        std::string m_filter;
        char m_pathFilter[256] = "";
        bool m_hasChanges = false;
        std::unordered_map<std::string, std::string> m_backupPaths;
        std::string m_settingsFilePath = "./data/paths.json";

        bool FilterPass(const std::string& key) const;
        void RenderPathsTable();
    };

}