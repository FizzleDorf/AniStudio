#pragma once
#include "Log.hpp"
#include "BaseView.hpp"
#include "ContextMenuUtils.hpp"
#include "ThumbnailUtils.hpp"
#include "ThumbnailFilters.hpp"
#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <set>
#include <memory>

namespace GUI {

    class AssetsView : public BaseView {
    public:
        enum class AssetSource { Global, Project, Both };

        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Assets",
            "category": "Viewers",
            "description": "Displays assets from the global and project assets directories"
        })";
        }

        AssetsView(ECS::EntityManager& mgr, ViewManager& vm);
        ~AssetsView() = default;

        void Init() override;
        void Update(float deltaT) override;
        void Render() override;
        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& j) override;

    private:
        std::vector<std::filesystem::path> assetFiles;
        std::vector<std::string> scanRoots;
        bool needsRefresh;
        std::vector<ECS::EntityID> loadedEntities;
        std::unordered_map<std::string, ECS::EntityID> pathToEntity;
        std::set<std::string> loadedPaths;
        std::unique_ptr<Utils::ContextMenuUtils> contextMenuUtils;

        ThumbnailFilters::Settings filterSettings;
        AssetSource sourceFilter = AssetSource::Both;

        ECS::EntityID selectedEntityID = 0;
        bool needsSort = true;

        void RefreshAssets();
        void RebuildScanRoots();
        void LoadNewAssets();
        void RenderAssetGrid();
        void LoadAsset(const std::filesystem::path& path);
        void ClearLoadedAssets();
        void RenderMenuBar();
        bool CopyFileToAssets(const std::string& sourcePath);
        void SelectAssetEntity(ECS::EntityID entityID);
        void CopyMetadataFromFile(const std::string& filePath);
        void ApplyFiltersAndSort();
    };

}