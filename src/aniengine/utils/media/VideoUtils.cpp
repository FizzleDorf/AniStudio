#include "VideoUtils.hpp"
#include <iostream>
#include <filesystem>
#include "ImageUtils.hpp"

#ifdef USE_WEBP
#include <webp/encode.h>
#endif

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/dict.h>
}

namespace Utils {

    unsigned char* VideoUtils::LoadVideoFrame(const std::string& filePath, double timeInSeconds,
        int& width, int& height, int& channels, double* actualTime) {
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

    bool VideoUtils::GetVideoInfo(const std::string& filePath, int& width, int& height,
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

    GLuint VideoUtils::GenerateTextureFromVideoFrame(unsigned char* data, int width, int height) {
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

    void VideoUtils::DeleteTexture(GLuint& textureID) {
        if (textureID != 0) {
            glDeleteTextures(1, &textureID);
            textureID = 0;
        }
    }

    void VideoUtils::FreeVideoFrameData(unsigned char* data) {
        if (data) free(data);
    }

    bool VideoUtils::SaveVideoFrameAsImage(const std::string& videoPath, const std::string& imagePath,
        double timeInSeconds) {
        int width, height, channels;
        unsigned char* frameData = LoadVideoFrame(videoPath, timeInSeconds, width, height, channels);
        if (!frameData)
            return false;

        bool success = Utils::ImageUtils::SaveImage(imagePath, width, height, channels, frameData);

        FreeVideoFrameData(frameData);
        return success;
    }

    bool VideoUtils::EncodeFramesToVideo(const std::vector<VideoFrame>& frames,
        const std::string& outputPath,
        int fps,
        const nlohmann::json& metadata) {
        if (frames.empty() || outputPath.empty())
            return false;

        AVFormatContext* fmtCtx = nullptr;
        int ret = avformat_alloc_output_context2(&fmtCtx, nullptr, nullptr, outputPath.c_str());
        if (ret < 0 || !fmtCtx) {
            std::cerr << "Failed to allocate output context for " << outputPath << std::endl;
            return false;
        }

        std::string ext = std::filesystem::path(outputPath).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        AVCodecID codecId = AV_CODEC_ID_H264;
        if (ext == ".webm") {
            codecId = AV_CODEC_ID_VP9;
        }
        else if (ext == ".webp") {
            codecId = AV_CODEC_ID_VP8;
        }
        else if (ext == ".mp4" || ext == ".mov") {
            codecId = AV_CODEC_ID_H264;
        }
        else if (ext == ".avi") {
            codecId = AV_CODEC_ID_MPEG4;
        }
        else if (ext == ".mkv") {
            codecId = AV_CODEC_ID_H264;
        }

        const AVCodec* codec = avcodec_find_encoder(codecId);
        if (!codec) {
            std::cerr << "No encoder found for codec ID " << codecId << std::endl;
            avformat_free_context(fmtCtx);
            return false;
        }

        AVStream* stream = avformat_new_stream(fmtCtx, nullptr);
        if (!stream) {
            std::cerr << "Failed to create video stream" << std::endl;
            avformat_free_context(fmtCtx);
            return false;
        }

        AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
        if (!codecCtx) {
            std::cerr << "Failed to allocate codec context" << std::endl;
            avformat_free_context(fmtCtx);
            return false;
        }

        int inputChannels = frames[0].channels;
        AVPixelFormat inputPixFmt;
        if (inputChannels == 3)
            inputPixFmt = AV_PIX_FMT_RGB24;
        else if (inputChannels == 4)
            inputPixFmt = AV_PIX_FMT_RGBA;
        else {
            std::cerr << "Unsupported channel count: " << inputChannels << std::endl;
            avcodec_free_context(&codecCtx);
            avformat_free_context(fmtCtx);
            return false;
        }

        int width = frames[0].width;
        int height = frames[0].height;

        codecCtx->width = width;
        codecCtx->height = height;
        codecCtx->time_base = AVRational{ 1, fps };
        codecCtx->framerate = AVRational{ fps, 1 };
        codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
        codecCtx->bit_rate = 4000000;
        codecCtx->gop_size = 10;

        if (fmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
            codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        ret = avcodec_open2(codecCtx, codec, nullptr);
        if (ret < 0) {
            std::cerr << "Failed to open video encoder" << std::endl;
            avcodec_free_context(&codecCtx);
            avformat_free_context(fmtCtx);
            return false;
        }

        avcodec_parameters_from_context(stream->codecpar, codecCtx);
        stream->time_base = codecCtx->time_base;

        if (!metadata.is_null() && !metadata.empty()) {
            std::string jsonStr = metadata.dump();
            av_dict_set(&fmtCtx->metadata, "comment", jsonStr.c_str(), 0);
        }

        ret = avio_open(&fmtCtx->pb, outputPath.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            std::cerr << "Failed to open output file " << outputPath << std::endl;
            avcodec_free_context(&codecCtx);
            avformat_free_context(fmtCtx);
            return false;
        }

        ret = avformat_write_header(fmtCtx, nullptr);
        if (ret < 0) {
            std::cerr << "Failed to write header" << std::endl;
            avio_close(fmtCtx->pb);
            avcodec_free_context(&codecCtx);
            avformat_free_context(fmtCtx);
            return false;
        }

        SwsContext* swsCtx = sws_getContext(width, height, inputPixFmt,
            width, height, AV_PIX_FMT_YUV420P,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!swsCtx) {
            std::cerr << "Failed to create sws context" << std::endl;
            avio_close(fmtCtx->pb);
            avcodec_free_context(&codecCtx);
            avformat_free_context(fmtCtx);
            return false;
        }

        AVFrame* frame = av_frame_alloc();
        AVFrame* yuvFrame = av_frame_alloc();
        if (!frame || !yuvFrame) {
            av_frame_free(&frame);
            av_frame_free(&yuvFrame);
            sws_freeContext(swsCtx);
            avio_close(fmtCtx->pb);
            avcodec_free_context(&codecCtx);
            avformat_free_context(fmtCtx);
            return false;
        }

        int bufferSize = av_image_get_buffer_size(inputPixFmt, width, height, 1);
        uint8_t* buffer = (uint8_t*)av_malloc(bufferSize);
        av_image_fill_arrays(frame->data, frame->linesize, buffer, inputPixFmt, width, height, 1);
        frame->width = width;
        frame->height = height;
        frame->format = inputPixFmt;

        yuvFrame->width = width;
        yuvFrame->height = height;
        yuvFrame->format = AV_PIX_FMT_YUV420P;
        int yuvBufferSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 1);
        uint8_t* yuvBuffer = (uint8_t*)av_malloc(yuvBufferSize);
        av_image_fill_arrays(yuvFrame->data, yuvFrame->linesize, yuvBuffer, AV_PIX_FMT_YUV420P, width, height, 1);

        AVPacket* pkt = av_packet_alloc();
        int64_t pts = 0;

        for (const auto& vf : frames) {
            if (vf.width != width || vf.height != height || vf.channels != inputChannels || !vf.data) {
                std::cerr << "Frame size mismatch or invalid data" << std::endl;
                continue;
            }
            size_t dataSize = width * height * inputChannels;
            memcpy(frame->data[0], vf.data, dataSize);

            sws_scale(swsCtx, frame->data, frame->linesize, 0, height,
                yuvFrame->data, yuvFrame->linesize);

            yuvFrame->pts = pts++;

            ret = avcodec_send_frame(codecCtx, yuvFrame);
            if (ret < 0) {
                std::cerr << "Error sending frame" << std::endl;
                break;
            }

            while (ret >= 0) {
                ret = avcodec_receive_packet(codecCtx, pkt);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                if (ret < 0) {
                    std::cerr << "Error receiving packet" << std::endl;
                    break;
                }
                av_packet_rescale_ts(pkt, codecCtx->time_base, stream->time_base);
                pkt->stream_index = stream->index;
                av_interleaved_write_frame(fmtCtx, pkt);
                av_packet_unref(pkt);
            }
        }

        avcodec_send_frame(codecCtx, nullptr);
        while (1) {
            ret = avcodec_receive_packet(codecCtx, pkt);
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                break;
            if (ret < 0)
                break;
            av_packet_rescale_ts(pkt, codecCtx->time_base, stream->time_base);
            pkt->stream_index = stream->index;
            av_interleaved_write_frame(fmtCtx, pkt);
            av_packet_unref(pkt);
        }

        av_write_trailer(fmtCtx);

        av_packet_free(&pkt);
        av_frame_free(&frame);
        av_frame_free(&yuvFrame);
        sws_freeContext(swsCtx);
        av_free(buffer);
        av_free(yuvBuffer);
        avcodec_free_context(&codecCtx);
        avio_close(fmtCtx->pb);
        avformat_free_context(fmtCtx);

        return true;
    }

} // namespace Utils