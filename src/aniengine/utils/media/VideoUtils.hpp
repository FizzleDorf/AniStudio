// VideoUtils.hpp
#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <iostream>
#include <filesystem>
#include <cstdio>
#include <cstring>
#include <GL/glew.h>
#include "stb_image_write.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace Utils {

    class VideoUtils {
    public:
        static unsigned char* LoadVideoFrame(const std::string& filePath, double timeInSeconds,
            int& width, int& height, int& channels,
            double* actualTime = nullptr) {

            AVFormatContext* fmtCtx = nullptr;
            if (avformat_open_input(&fmtCtx, filePath.c_str(), nullptr, nullptr) < 0)
                return nullptr;
            if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
                avformat_close_input(&fmtCtx);
                return nullptr;
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
                return nullptr;
            }

            AVCodecParameters* codecPar = fmtCtx->streams[videoStream]->codecpar;
            const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
            if (!codec) {
                avformat_close_input(&fmtCtx);
                return nullptr;
            }

            AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
            if (!codecCtx) {
                avformat_close_input(&fmtCtx);
                return nullptr;
            }
            if (avcodec_parameters_to_context(codecCtx, codecPar) < 0) {
                avcodec_free_context(&codecCtx);
                avformat_close_input(&fmtCtx);
                return nullptr;
            }
            if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
                avcodec_free_context(&codecCtx);
                avformat_close_input(&fmtCtx);
                return nullptr;
            }

            AVStream* stream = fmtCtx->streams[videoStream];
            double fps = av_q2d(stream->avg_frame_rate);
            if (fps <= 0) fps = av_q2d(stream->r_frame_rate);
            if (fps <= 0) fps = 30.0;

            int64_t target_ts = (int64_t)(timeInSeconds * AV_TIME_BASE);
            if (avformat_seek_file(fmtCtx, -1, INT64_MIN, target_ts, target_ts, 0) < 0)
                avformat_seek_file(fmtCtx, -1, INT64_MIN, 0, 0, 0);
            avcodec_flush_buffers(codecCtx);

            AVFrame* frame = av_frame_alloc();
            AVPacket* pkt = av_packet_alloc();
            if (!frame || !pkt) {
                av_packet_free(&pkt);
                av_frame_free(&frame);
                avcodec_free_context(&codecCtx);
                avformat_close_input(&fmtCtx);
                return nullptr;
            }

            unsigned char* result = nullptr;
            int got_frame = 0;
            while (av_read_frame(fmtCtx, pkt) >= 0) {
                if (pkt->stream_index == videoStream) {
                    if (avcodec_send_packet(codecCtx, pkt) == 0) {
                        while (avcodec_receive_frame(codecCtx, frame) == 0) {
                            got_frame = 1;
                            break;
                        }
                    }
                    if (got_frame) break;
                }
                av_packet_unref(pkt);
            }

            if (got_frame) {
                width = frame->width;
                height = frame->height;
                channels = 4;

                SwsContext* sws = sws_getContext(width, height, codecCtx->pix_fmt,
                    width, height, AV_PIX_FMT_RGBA,
                    SWS_BILINEAR, nullptr, nullptr, nullptr);
                if (sws) {
                    size_t dataSize = width * height * 4;
                    uint8_t* rgba_data = (uint8_t*)malloc(dataSize);
                    if (rgba_data) {
                        uint8_t* dst[1] = { rgba_data };
                        int dstLinesize[1] = { width * 4 };
                        sws_scale(sws, frame->data, frame->linesize, 0, height, dst, dstLinesize);
                        result = rgba_data;
                        if (actualTime) {
                            double pts = frame->pts * av_q2d(stream->time_base);
                            *actualTime = (pts >= 0) ? pts : timeInSeconds;
                        }
                    }
                    sws_freeContext(sws);
                }
            }

            av_packet_free(&pkt);
            av_frame_free(&frame);
            avcodec_free_context(&codecCtx);
            avformat_close_input(&fmtCtx);
            return result;
        }

        static bool GetVideoInfo(const std::string& filePath, int& width, int& height,
            double& duration, double& frameRate) {

            AVFormatContext* fmtCtx = nullptr;
            if (avformat_open_input(&fmtCtx, filePath.c_str(), nullptr, nullptr) < 0)
                return false;
            if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
                avformat_close_input(&fmtCtx);
                return false;
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
                return false;
            }

            AVCodecParameters* codecPar = fmtCtx->streams[videoStream]->codecpar;
            width = codecPar->width;
            height = codecPar->height;

            AVStream* stream = fmtCtx->streams[videoStream];
            double fps = av_q2d(stream->avg_frame_rate);
            if (fps <= 0) fps = av_q2d(stream->r_frame_rate);
            if (fps <= 0) fps = 30.0;
            frameRate = fps;

            duration = (fmtCtx->duration != AV_NOPTS_VALUE) ? (double)fmtCtx->duration / AV_TIME_BASE : 0.0;

            avformat_close_input(&fmtCtx);
            return (width > 0 && height > 0 && frameRate > 0 && duration > 0);
        }

        static GLuint GenerateTextureFromVideoFrame(unsigned char* data, int width, int height) {
            if (!data || width <= 0 || height <= 0)
                return 0;
            GLuint tex = 0;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glBindTexture(GL_TEXTURE_2D, 0);
            return tex;
        }

        static void DeleteTexture(GLuint& textureID) {
            if (textureID != 0) {
                glDeleteTextures(1, &textureID);
                textureID = 0;
            }
        }

        static void FreeVideoFrameData(unsigned char* data) {
            if (data) free(data);
        }

        static bool SaveVideoFrameAsImage(const std::string& videoPath, const std::string& imagePath,
            double timeInSeconds) {
            int width, height, channels;
            unsigned char* frameData = LoadVideoFrame(videoPath, timeInSeconds, width, height, channels);
            if (!frameData)
                return false;

            std::filesystem::path outputDir = std::filesystem::path(imagePath).parent_path();
            if (!outputDir.empty() && !std::filesystem::exists(outputDir))
                std::filesystem::create_directories(outputDir);

            bool success = false;
            std::string ext = std::filesystem::path(imagePath).extension().string();
            if (ext == ".png")
                success = stbi_write_png(imagePath.c_str(), width, height, channels, frameData, width * channels) != 0;
            else if (ext == ".jpg" || ext == ".jpeg")
                success = stbi_write_jpg(imagePath.c_str(), width, height, channels, frameData, 90) != 0;
            else if (ext == ".bmp")
                success = stbi_write_bmp(imagePath.c_str(), width, height, channels, frameData) != 0;
            else if (ext == ".tga")
                success = stbi_write_tga(imagePath.c_str(), width, height, channels, frameData) != 0;
            else
                success = stbi_write_png(imagePath.c_str(), width, height, channels, frameData, width * channels) != 0;

            FreeVideoFrameData(frameData);
            return success;
        }
    };

} // namespace Utils