#pragma once

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "VideoComponent.hpp"
#include "VideoUtils.hpp"
#include <memory>
#include <functional>
#include <vector>
#include <chrono>
#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace ECS {

    class VideoSystem : public BaseSystem {
    public:
        using VideoCallback = std::function<void(EntityID)>;
        using VideoTextureCallback = std::function<void(EntityID, unsigned char*, int, int, int, GLuint*)>;

        VideoSystem(EntityManager& entityMgr)
            : BaseSystem(entityMgr), lastFrameTime(std::chrono::high_resolution_clock::now()) {
            sysName = "VideoSystem";
            AddComponentSignature<VideoComponent>();
        }

        ~VideoSystem() override {
            for (auto entity : entities) {
                if (mgr.HasComponent<VideoComponent>(entity)) {
                    auto& videoComp = mgr.GetComponent<VideoComponent>(entity);
                    videoComp.UnloadVideo();
                }
            }
        }

        void Start() override {
            auto allEntities = mgr.GetAllEntities();
            for (auto entity : allEntities) {
                if (mgr.HasComponent<VideoComponent>(entity)) {
                    entities.insert(entity);
                    auto& videoComp = mgr.GetComponent<VideoComponent>(entity);
                    if (!videoComp.filePath.empty()) {
                        LoadVideo(videoComp, entity);
                        NotifyVideoAdded(entity);
                    }
                }
            }
        }

        void Update(float deltaT) override {
        }

        void SetVideo(EntityID entity, const std::string& filePath) {
            if (mgr.HasComponent<VideoComponent>(entity)) {
                auto& videoComp = mgr.GetComponent<VideoComponent>(entity);
                videoComp.UnloadVideo();
                videoComp.filePath = filePath;
                size_t lastSlash = filePath.find_last_of("/\\");
                videoComp.fileName = (lastSlash != std::string::npos) ?
                    filePath.substr(lastSlash + 1) : filePath;
                LoadVideo(videoComp, entity);
                entities.insert(entity);  // FIX: Ensure entity is tracked
                NotifyVideoAdded(entity);
            }
        }

        void RemoveVideo(EntityID entity) {
            if (mgr.HasComponent<VideoComponent>(entity)) {
                auto& videoComp = mgr.GetComponent<VideoComponent>(entity);
                videoComp.UnloadVideo();
                entities.erase(entity);  // FIX: Remove entity from tracking
                NotifyVideoRemoved(entity);
                mgr.DestroyEntity(entity);
            }
        }

        std::vector<EntityID> GetAllVideoEntities() const {
            std::vector<EntityID> result;
            for (auto entity : entities) {
                if (mgr.IsEntityValid(entity) && mgr.HasComponent<VideoComponent>(entity))
                    result.push_back(entity);
            }
            return result;
        }

        void RegisterVideoAddedCallback(const VideoCallback& cb) { videoAddedCallbacks.push_back(cb); }
        void RegisterVideoRemovedCallback(const VideoCallback& cb) { videoRemovedCallbacks.push_back(cb); }

        void SetVideoTextureCallback(const VideoTextureCallback& callback) {
            m_textureCallback = callback;
        }

        bool SeekToFrame(VideoComponent& videoComp, long long frameIndex) {
            if (!videoComp.fmtCtx || videoComp.videoStreamIndex < 0 || frameIndex < 0 || frameIndex >= videoComp.frameCount)
                return false;

            double timeSec = static_cast<double>(frameIndex) / videoComp.fps;
            int64_t target_ts = (int64_t)(timeSec * AV_TIME_BASE);
            if (avformat_seek_file(videoComp.fmtCtx, -1, INT64_MIN, target_ts, target_ts, 0) < 0) {
                avformat_seek_file(videoComp.fmtCtx, -1, INT64_MIN, 0, 0, 0);
            }
            avcodec_flush_buffers(videoComp.codecCtx);

            videoComp.frameAccumulator = 0.0f;

            if (DecodeNextFrame(videoComp)) {
                if (m_textureCallback) {
                    m_textureCallback(videoComp.GetID(), videoComp.frameDataRGBA.data(),
                        videoComp.width, videoComp.height, 4,
                        &videoComp.currentTexture);
                }
                videoComp.currentFrame = frameIndex;
                return true;
            }
            return false;
        }

        bool AdvanceOneFrame(VideoComponent& videoComp) {
            if (!videoComp.fmtCtx || videoComp.videoStreamIndex < 0)
                return false;
            if (DecodeNextFrame(videoComp)) {
                if (m_textureCallback) {
                    m_textureCallback(videoComp.GetID(), videoComp.frameDataRGBA.data(),
                        videoComp.width, videoComp.height, 4,
                        &videoComp.currentTexture);
                }
                return true;
            }
            return false;
        }

        void UpdateMetadataFlags(EntityID entity) {
            if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<VideoComponent>(entity))
                return;
            auto& videoComp = mgr.GetComponent<VideoComponent>(entity);
            if (videoComp.filePath.empty())
                return;
            try {
                videoComp.hasExifData = Utils::VideoUtils::HasExifMetadata(videoComp.filePath);
                videoComp.hasLSBData = Utils::VideoUtils::HasLSBMetadata(videoComp.filePath);
            }
            catch (const std::exception& e) {
                std::cerr << "[VideoSystem] Failed to read metadata for " << videoComp.filePath << ": " << e.what() << std::endl;
                videoComp.hasExifData = false;
                videoComp.hasLSBData = false;
            }
            catch (...) {
                std::cerr << "[VideoSystem] Unknown error reading metadata for " << videoComp.filePath << std::endl;
                videoComp.hasExifData = false;
                videoComp.hasLSBData = false;
            }
        }

    private:
        std::vector<VideoCallback> videoAddedCallbacks;
        std::vector<VideoCallback> videoRemovedCallbacks;
        std::chrono::high_resolution_clock::time_point lastFrameTime;
        VideoTextureCallback m_textureCallback;

        void LoadVideo(VideoComponent& videoComp, EntityID entity) {
            videoComp.UnloadVideo();

            AVFormatContext* fmtCtx = nullptr;
            if (avformat_open_input(&fmtCtx, videoComp.filePath.c_str(), nullptr, nullptr) < 0) {
                std::cerr << "FFmpeg: could not open " << videoComp.filePath << std::endl;
                return;
            }
            if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
                avformat_close_input(&fmtCtx);
                std::cerr << "FFmpeg: no stream info" << std::endl;
                return;
            }

            int videoStream = -1;
            for (unsigned i = 0; i < fmtCtx->nb_streams; ++i) {
                if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                    videoStream = i;
                    break;
                }
            }
            if (videoStream == -1) {
                avformat_close_input(&fmtCtx);
                std::cerr << "FFmpeg: no video stream" << std::endl;
                return;
            }

            AVCodecParameters* codecPar = fmtCtx->streams[videoStream]->codecpar;
            const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
            if (!codec) {
                avformat_close_input(&fmtCtx);
                std::cerr << "FFmpeg: decoder not found" << std::endl;
                return;
            }

            AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
            if (!codecCtx) {
                avformat_close_input(&fmtCtx);
                std::cerr << "FFmpeg: could not allocate codec context" << std::endl;
                return;
            }
            if (avcodec_parameters_to_context(codecCtx, codecPar) < 0) {
                avcodec_free_context(&codecCtx);
                avformat_close_input(&fmtCtx);
                std::cerr << "FFmpeg: failed to set codec parameters" << std::endl;
                return;
            }
            if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
                avcodec_free_context(&codecCtx);
                avformat_close_input(&fmtCtx);
                std::cerr << "FFmpeg: could not open codec" << std::endl;
                return;
            }

            videoComp.fmtCtx = fmtCtx;
            videoComp.codecCtx = codecCtx;
            videoComp.videoStreamIndex = videoStream;
            videoComp.width = codecPar->width;
            videoComp.height = codecPar->height;

            AVStream* stream = fmtCtx->streams[videoStream];
            double fps = av_q2d(stream->avg_frame_rate);
            if (fps <= 0) fps = av_q2d(stream->r_frame_rate);
            if (fps <= 0) fps = 30.0;
            videoComp.fps = fps;

            double duration = (fmtCtx->duration != AV_NOPTS_VALUE) ? (double)fmtCtx->duration / AV_TIME_BASE : 0.0;
            videoComp.frameCount = (long long)(duration * fps);
            if (videoComp.frameCount <= 0)
                videoComp.frameCount = 1000;

            videoComp.frame = av_frame_alloc();
            videoComp.pkt = av_packet_alloc();

            videoComp.swsCtx = sws_getContext(videoComp.width, videoComp.height, codecCtx->pix_fmt,
                videoComp.width, videoComp.height, AV_PIX_FMT_RGBA,
                SWS_BILINEAR, nullptr, nullptr, nullptr);

            avformat_seek_file(fmtCtx, -1, INT64_MIN, 0, 0, 0);
            avcodec_flush_buffers(codecCtx);
            videoComp.currentFrame = 0;
            videoComp.isPlaying = false;
            videoComp.playbackSpeed = 1.0f;
            videoComp.frameAccumulator = 0.0f;

            if (DecodeNextFrame(videoComp)) {
                if (m_textureCallback) {
                    m_textureCallback(videoComp.GetID(), videoComp.frameDataRGBA.data(),
                        videoComp.width, videoComp.height, 4,
                        &videoComp.currentTexture);
                }
            }

            std::cout << "Video loaded: " << videoComp.filePath
                << " (" << videoComp.width << "x" << videoComp.height
                << ", " << videoComp.fps << " fps, " << videoComp.frameCount << " frames)" << std::endl;
        }

        bool DecodeNextFrame(VideoComponent& videoComp) {
            if (!videoComp.fmtCtx || !videoComp.codecCtx || videoComp.videoStreamIndex < 0)
                return false;

            AVPacket* pkt = videoComp.pkt;
            AVFrame* frame = videoComp.frame;

            while (av_read_frame(videoComp.fmtCtx, pkt) >= 0) {
                if (pkt->stream_index == videoComp.videoStreamIndex) {
                    if (avcodec_send_packet(videoComp.codecCtx, pkt) == 0) {
                        while (avcodec_receive_frame(videoComp.codecCtx, frame) == 0) {
                            int width = frame->width;
                            int height = frame->height;
                            if (width != videoComp.width || height != videoComp.height) {
                                videoComp.width = width;
                                videoComp.height = height;
                                if (videoComp.swsCtx) {
                                    sws_freeContext(videoComp.swsCtx);
                                }
                                videoComp.swsCtx = sws_getContext(width, height, videoComp.codecCtx->pix_fmt,
                                    width, height, AV_PIX_FMT_RGBA,
                                    SWS_BILINEAR, nullptr, nullptr, nullptr);
                            }

                            size_t dataSize = width * height * 4;
                            videoComp.frameDataRGBA.resize(dataSize);

                            uint8_t* dst[1] = { videoComp.frameDataRGBA.data() };
                            int dstLinesize[1] = { width * 4 };
                            sws_scale(videoComp.swsCtx, frame->data, frame->linesize, 0, height, dst, dstLinesize);

                            videoComp.width = width;
                            videoComp.height = height;
                            av_packet_unref(pkt);
                            videoComp.currentFrame++;
                            return true;
                        }
                    }
                }
                av_packet_unref(pkt);
            }

            if (videoComp.looping) {
                avformat_seek_file(videoComp.fmtCtx, -1, INT64_MIN, 0, 0, 0);
                avcodec_flush_buffers(videoComp.codecCtx);
                videoComp.currentFrame = 0;
                videoComp.frameAccumulator = 0.0f;
                return DecodeNextFrame(videoComp);
            }
            else {
                videoComp.isPlaying = false;
                videoComp.frameAccumulator = 0.0f;
                return false;
            }
        }

        void NotifyVideoAdded(EntityID entity) {
            for (const auto& cb : videoAddedCallbacks) cb(entity);
        }
        void NotifyVideoRemoved(EntityID entity) {
            for (const auto& cb : videoRemovedCallbacks) cb(entity);
        }
    };

} // namespace ECS