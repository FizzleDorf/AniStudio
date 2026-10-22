#pragma once
#include "BaseSettingsTab.hpp"
#include <unordered_map>
#include <string>
#include <vector>
#include <map>

namespace ECS {
    class SettingsSystem;

    class FilePathTab : public BaseSettingsTab {
    public:
        explicit FilePathTab(SettingsSystem& sys);
        ~FilePathTab() = default;

        std::string GetTabName() const override { return "Paths"; }
        std::string GetTabCategory() const override { return "System"; }
        void Render() override;
        void SetFilter(const std::string& filter) override { m_filter = filter; }

    private:
        SettingsSystem& m_settingsSystem;
        std::string m_filter;
        char m_pathFilter[256] = "";

        bool FilterPass(const std::string& key) const;
        void RenderPathsTable();
        std::string GetCategoryForPath(const std::string& key) const;
        void DetermineCategoryOrder(const std::map<std::string, std::vector<std::string>>& categoryMap,
            std::vector<std::string>& order) const;
        bool IsFileSelector(const std::string& key) const;
    };

}