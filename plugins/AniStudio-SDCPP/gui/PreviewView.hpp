// PreviewView.hpp
#pragma once

#include "BaseView.hpp"
#include "DiffusionCallbackUtils.hpp"
#include "ImageComponent.hpp"
#include "ECS.h"
#include <imgui.h>
#include <string>
#include <cstdint>

namespace GUI {

    class PreviewView : public BaseView {
    public:
        PreviewView(ECS::EntityManager& entityMgr, ViewManager& viewMgr);
        ~PreviewView() override;

        static constexpr const char* GetMetadataJSON() {
            return R"({
                "displayName": "Preview",
                "category": "Diffusion",
                "description": "Live preview of the current diffusion generation."
            })";
        }

        void Init() override;
        void Update(const float deltaT) override;
        void Render() override;
        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& j) override;

    private:
        enum class PreviewMode { None = 0, Proj, Tae, Vae };

        static int PreviewModeToInt(PreviewMode m);
        void ApplyModeToLibrary();

        void RefreshTextureIfNeeded();
        void PushPreviewFrameToComponent(const PreviewFrame& frame);
        void DeleteLastTempFile();
        void DrawMenuBar();
        void LoadModeFromSettings();
        void SaveModeToSettings();

        ECS::EntityID m_previewEntity = 0;
        bool          m_previewEntityHasComponent = false;
        uint64_t      m_lastSequence = 0;
        std::string   m_lastTempPath;

        PreviewMode m_mode = PreviewMode::Tae;
        int         m_interval = 1;
        bool        m_modeLoaded = false;
    };

} // namespace GUI