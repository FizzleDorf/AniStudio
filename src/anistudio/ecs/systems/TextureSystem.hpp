#pragma once

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "ImageComponent.hpp"
#include "TextureComponent.hpp"
#include "ImageSystem.hpp"
#include "VideoSystem.hpp"
#include "ImageUtils.hpp"
#include "OpenGLUtils.hpp"
#include "DragDropUtils.hpp"
#include "Log.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <queue>
#include <mutex>
#include <functional>

namespace ECS {

    struct TextureCreationRequest {
        EntityID entityID = 0;
        unsigned char* imageData = nullptr;
        int width = 0;
        int height = 0;
        int channels = 0;
        bool isVideo = false;
        GLuint* targetTexture = nullptr;
    };

    // Uploads decoded frames to GL textures. Image data is owned by this
    // system; video frame data is owned by VideoSystem.
    class TextureSystem : public BaseSystem {
    public:
        using VideoTextureCallback =
            std::function<void(EntityID, unsigned char*, int, int, int, GLuint*)>;

        TextureSystem(EntityManager& entityMgr);
        ~TextureSystem() override;

        void Start() override;
        void Update(const float deltaT) override;

        void QueueTextureCreation(EntityID e, unsigned char* data, int w, int h, int ch);
        void QueueVideoTextureCreation(EntityID e, unsigned char* data,
            int w, int h, int ch, GLuint* target);
        void CancelPendingRequests(EntityID e);

        void CreatePendingTextures();

        void RemoveTexture(EntityID e);

        GLuint GetTextureID(EntityID e) const;
        bool   HasValidTexture(EntityID e) const;
        bool   HasPendingTextures() const { return m_needsTextureCreation; }

        void RegisterVideoTextureCallback(const VideoTextureCallback& cb);
        void QueueVideoTexture(EntityID e, unsigned char* data,
            int w, int h, int ch, GLuint* target);

    private:
        std::queue<TextureCreationRequest> textureQueue;
        std::mutex queueMutex;
        bool m_needsTextureCreation;
        VideoTextureCallback m_videoTextureCallback;

        void DeleteTexture(TextureComponent& tc);
    };

} // namespace ECS