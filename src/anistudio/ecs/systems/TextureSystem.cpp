#include "TextureSystem.hpp"

#include <cstdlib>
#include <cstring>

namespace ECS {

    TextureSystem::TextureSystem(EntityManager& entityMgr)
        : BaseSystem(entityMgr), m_needsTextureCreation(false) {
        sysName = "TextureSystem";
        AddComponentSignature<ImageComponent>();
        AddComponentSignature<TextureComponent>();
    }

    TextureSystem::~TextureSystem() {
        for (auto entity : entities) {
            if (mgr.HasComponent<TextureComponent>(entity)) {
                auto& tc = mgr.GetComponent<TextureComponent>(entity);
                DeleteTexture(tc);
            }
        }

        std::lock_guard<std::mutex> lock(queueMutex);
        while (!textureQueue.empty()) {
            auto& req = textureQueue.front();
            if (req.imageData) {
                if (req.isVideo) free(req.imageData);
                else Utils::ImageUtils::FreeImageData(req.imageData);
            }
            textureQueue.pop();
        }
    }

    void TextureSystem::Start() {
        auto video = mgr.GetSystem<VideoSystem>();
        if (video) {
            video->RegisterVideoRemovedCallback(this, [this](EntityID e) {
                RemoveTexture(e);
                });
        }

        auto image = mgr.GetSystem<ImageSystem>();
        if (image) {
            image->RegisterImageRemovedCallback(this, [this](EntityID e) {
                RemoveTexture(e);
                });

            image->RegisterImageReadyCallback(this,
                [this](EntityID e, unsigned char* data, int w, int h, int ch) {
                    if (!data || w <= 0 || h <= 0 || ch <= 0) return;
                    size_t sz = static_cast<size_t>(w) * h * ch;
                    unsigned char* copy = static_cast<unsigned char*>(malloc(sz));
                    if (!copy) {
                        ANI_LOG_ERROR("[TextureSystem] malloc failed (%zu bytes)", sz);
                        return;
                    }
                    memcpy(copy, data, sz);
                    QueueTextureCreation(e, copy, w, h, ch);
                });
        }
    }

    void TextureSystem::Update(const float deltaT) {
        (void)deltaT;
        CreatePendingTextures();
    }

    void TextureSystem::QueueTextureCreation(EntityID e, unsigned char* data,
        int w, int h, int ch) {
        std::lock_guard<std::mutex> lock(queueMutex);
        TextureCreationRequest req;
        req.entityID = e;
        req.imageData = data;
        req.width = w;
        req.height = h;
        req.channels = ch;
        req.isVideo = false;
        textureQueue.push(req);
        m_needsTextureCreation = true;
    }

    void TextureSystem::QueueVideoTextureCreation(EntityID e, unsigned char* data,
        int w, int h, int ch, GLuint* target) {
        std::lock_guard<std::mutex> lock(queueMutex);
        TextureCreationRequest req;
        req.entityID = e;
        req.imageData = data;
        req.width = w;
        req.height = h;
        req.channels = ch;
        req.isVideo = true;
        req.targetTexture = target;
        textureQueue.push(req);
        m_needsTextureCreation = true;
    }

    void TextureSystem::CancelPendingRequests(EntityID e) {
        std::lock_guard<std::mutex> lock(queueMutex);
        std::queue<TextureCreationRequest> kept;
        while (!textureQueue.empty()) {
            auto req = textureQueue.front();
            textureQueue.pop();
            if (req.entityID == e) {
                if (req.isVideo) {
                    if (req.targetTexture && *req.targetTexture != 0) {
                        glDeleteTextures(1, req.targetTexture);
                        *req.targetTexture = 0;
                    }
                    if (req.imageData) free(req.imageData);
                }
                else if (req.imageData) {
                    Utils::ImageUtils::FreeImageData(req.imageData);
                }
            }
            else {
                kept.push(req);
            }
        }
        textureQueue.swap(kept);
        m_needsTextureCreation = !textureQueue.empty();
    }

    void TextureSystem::CreatePendingTextures() {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (textureQueue.empty()) {
            m_needsTextureCreation = false;
            return;
        }

        GLFWwindow* ctx = glfwGetCurrentContext();
        if (!ctx) {
            ctx = static_cast<GLFWwindow*>(GUI::DragDrop::GetWindowHandle());
            if (ctx) glfwMakeContextCurrent(ctx);
            if (!ctx) return;
        }

        glGetError();

        while (!textureQueue.empty()) {
            auto req = textureQueue.front();
            textureQueue.pop();

            if (!mgr.IsEntityValid(req.entityID)) {
                if (req.isVideo) {
                    if (req.targetTexture && *req.targetTexture != 0) {
                        glDeleteTextures(1, req.targetTexture);
                        *req.targetTexture = 0;
                    }
                    if (req.imageData) free(req.imageData);
                }
                else if (req.imageData) {
                    Utils::ImageUtils::FreeImageData(req.imageData);
                }
                continue;
            }

            if (req.isVideo) {
                if (!req.targetTexture) {
                    if (req.imageData) free(req.imageData);
                    continue;
                }
                if (*req.targetTexture != 0) {
                    glDeleteTextures(1, req.targetTexture);
                    *req.targetTexture = 0;
                }
                GLuint id = Utils::OpenGLUtils::GenerateTexture(
                    req.width, req.height, req.channels, req.imageData);
                if (id != 0) *req.targetTexture = id;
                if (req.imageData) free(req.imageData);
            }
            else {
                if (!mgr.HasComponent<TextureComponent>(req.entityID)) {
                    mgr.AddComponent<TextureComponent>(req.entityID);
                }
                auto& tc = mgr.GetComponent<TextureComponent>(req.entityID);
                if (tc.textureID != 0) {
                    Utils::OpenGLUtils::DeleteTexture(tc.textureID);
                    tc.textureID = 0;
                }
                tc.textureID = Utils::OpenGLUtils::GenerateTexture(
                    req.width, req.height, req.channels, req.imageData);
                if (tc.textureID != 0) {
                    tc.width = req.width;
                    tc.height = req.height;
                    tc.channels = req.channels;
                }
                if (req.imageData) Utils::ImageUtils::FreeImageData(req.imageData);
            }
        }
        m_needsTextureCreation = false;
    }

    void TextureSystem::RemoveTexture(EntityID e) {
        if (mgr.HasComponent<TextureComponent>(e)) {
            auto& tc = mgr.GetComponent<TextureComponent>(e);
            DeleteTexture(tc);
        }
        CancelPendingRequests(e);
    }

    GLuint TextureSystem::GetTextureID(EntityID e) const {
        if (mgr.HasComponent<TextureComponent>(e))
            return mgr.GetComponent<TextureComponent>(e).textureID;
        return 0;
    }

    bool TextureSystem::HasValidTexture(EntityID e) const {
        GLuint id = GetTextureID(e);
        return id != 0 && glIsTexture(id);
    }

    void TextureSystem::RegisterVideoTextureCallback(const VideoTextureCallback& cb) {
        m_videoTextureCallback = cb;
    }

    void TextureSystem::QueueVideoTexture(EntityID e, unsigned char* data,
        int w, int h, int ch, GLuint* target) {
        if (m_videoTextureCallback) {
            m_videoTextureCallback(e, data, w, h, ch, target);
        }
        else {
            QueueVideoTextureCreation(e, data, w, h, ch, target);
        }
    }

    void TextureSystem::DeleteTexture(TextureComponent& tc) {
        if (tc.textureID != 0) {
            if (glfwGetCurrentContext()) {
                Utils::OpenGLUtils::DeleteTexture(tc.textureID);
            }
            tc.textureID = 0;
            tc.width = 0;
            tc.height = 0;
            tc.channels = 0;
            tc.needsUpdate = false;
        }
    }

} // namespace ECS