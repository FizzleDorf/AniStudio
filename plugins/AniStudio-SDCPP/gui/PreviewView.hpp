// PreviewView.hpp
#pragma once

#include "BaseView.hpp"
#include "DiffusionCallbackUtils.hpp"
#include "ImageComponent.hpp"
#include "VideoComponent.hpp"       // PreviewVideoComponent
#include "TextureComponent.hpp"
#include "TextureSystem.hpp"
#include "VideoSystem.hpp"
#include "AVSystem.hpp"
#include "PlaybackStateComponent.hpp"
#include "ECS.h"
#include <imgui.h>
#include <string>
#include <cstdint>
#include <memory>

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

        // ---- Public video preview API ----
        // Attaches a file to the video preview entity in Streaming mode.
        // Playback starts automatically once the decoder is ready.
        void ShowVideo(const std::string& path,
            const std::string& label = "preview",
            bool loop = true);

        void ClearVideo();
        bool HasVideo() const { return m_videoAttached; }

    private:
        enum class PreviewMode { None = 0, Proj, Tae, Vae };

        static int PreviewModeToInt(PreviewMode m);
        void ApplyModeToLibrary();

        // image preview
        void RefreshTextureIfNeeded();
        void PushPreviewFrameToComponent(const PreviewFrame& frame);

        // video preview
        void RefreshVideoTextureIfNeeded();
        void PushVideoFrameToComponent(const std::vector<uint8_t>& rgba,
            int w, int h);

        // UI
        void DrawMenuBar();
        bool DrawVideoControls();
        void LoadModeFromSettings();
        void SaveModeToSettings();

        // ---- image preview state ----
        ECS::EntityID m_previewEntity = 0;
        bool          m_previewEntityHasComponent = false;
        uint64_t      m_lastSequence = 0;

        // ---- video preview state ----
        ECS::EntityID m_videoPreviewEntity = 0;
        bool          m_videoPreviewEntityHasComponent = false;
        bool          m_videoAttached = false;
        bool          m_videoLoops = true;
        int           m_lastVideoWidth = 0;
        int           m_lastVideoHeight = 0;

        std::shared_ptr<ECS::TextureSystem> m_textureSystem;
        std::shared_ptr<ECS::VideoSystem>   m_videoSystem;

        PreviewMode m_mode = PreviewMode::Tae;
        int         m_interval = 1;
        bool        m_modeLoaded = false;
    };

} // namespace GUI