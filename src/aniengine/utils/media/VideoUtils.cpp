#include "VideoUtils.hpp"
#include "ImageUtils.hpp"

#include <algorithm>
#include <filesystem>
#include <cstdlib>
#include <cstring>
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/dict.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

namespace Utils {

    unsigned char* VideoUtils::LoadVideoFrame(const std::string& filePath,
        double timeInSeconds,
        int& width, int& height, int& channels,
        double* actualTime) {
        AVFormatContext* fmt = nullptr;
        if (avformat_open_input(&fmt, filePath.c_str(), nullptr, nullptr) < 0) return nullptr;
        if (avformat_find_stream_info(fmt, nullptr) < 0) {
            avformat_close_input(&fmt); return nullptr;
        }

        int vs = -1;
        for (unsigned i = 0; i < fmt->nb_streams; ++i) {
            if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) { vs = i; break; }
        }
        if (vs < 0) { avformat_close_input(&fmt); return nullptr; }

        AVCodecParameters* par = fmt->streams[vs]->codecpar;
        const AVCodec* codec = avcodec_find_decoder(par->codec_id);
        if (!codec) { avformat_close_input(&fmt); return nullptr; }

        AVCodecContext* cc = avcodec_alloc_context3(codec);
        if (!cc || avcodec_parameters_to_context(cc, par) < 0 ||
            avcodec_open2(cc, codec, nullptr) < 0) {
            if (cc) avcodec_free_context(&cc);
            avformat_close_input(&fmt);
            return nullptr;
        }

        AVStream* stream = fmt->streams[vs];
        int64_t target = static_cast<int64_t>(timeInSeconds * AV_TIME_BASE);
        if (avformat_seek_file(fmt, -1, INT64_MIN, target, target, 0) < 0) {
            avformat_seek_file(fmt, -1, INT64_MIN, 0, 0, 0);
        }
        avcodec_flush_buffers(cc);

        AVFrame* frame = av_frame_alloc();
        AVPacket* pkt = av_packet_alloc();
        unsigned char* result = nullptr;

        bool got = false;
        while (av_read_frame(fmt, pkt) >= 0) {
            if (pkt->stream_index == vs && avcodec_send_packet(cc, pkt) == 0) {
                while (avcodec_receive_frame(cc, frame) == 0) {
                    got = true;
                    break;
                }
            }
            if (got) { av_packet_unref(pkt); break; }
            av_packet_unref(pkt);
        }

        if (got) {
            width = frame->width;
            height = frame->height;
            channels = 4;

            SwsContext* sws = sws_getContext(width, height, cc->pix_fmt,
                width, height, AV_PIX_FMT_RGBA,
                SWS_BILINEAR, nullptr, nullptr, nullptr);
            if (sws) {
                size_t sz = static_cast<size_t>(width) * height * 4;
                unsigned char* rgba = static_cast<unsigned char*>(malloc(sz));
                if (rgba) {
                    uint8_t* dst[1] = { rgba };
                    int lines[1] = { width * 4 };
                    sws_scale(sws, frame->data, frame->linesize, 0, height, dst, lines);
                    result = rgba;
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
        avcodec_free_context(&cc);
        avformat_close_input(&fmt);
        return result;
    }

    bool VideoUtils::GetVideoInfo(const std::string& filePath,
        int& width, int& height,
        double& duration, double& frameRate) {
        AVFormatContext* fmt = nullptr;
        if (avformat_open_input(&fmt, filePath.c_str(), nullptr, nullptr) < 0) return false;
        if (avformat_find_stream_info(fmt, nullptr) < 0) {
            avformat_close_input(&fmt); return false;
        }

        int vs = -1;
        for (unsigned i = 0; i < fmt->nb_streams; ++i) {
            if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) { vs = i; break; }
        }
        if (vs < 0) { avformat_close_input(&fmt); return false; }

        AVCodecParameters* par = fmt->streams[vs]->codecpar;
        width = par->width;
        height = par->height;

        AVStream* stream = fmt->streams[vs];
        double fps = av_q2d(stream->avg_frame_rate);
        if (fps <= 0) fps = av_q2d(stream->r_frame_rate);
        if (fps <= 0) fps = 30.0;
        frameRate = fps;

        duration = (fmt->duration != AV_NOPTS_VALUE)
            ? static_cast<double>(fmt->duration) / AV_TIME_BASE : 0.0;

        avformat_close_input(&fmt);
        return (width > 0 && height > 0 && frameRate > 0 && duration > 0);
    }

    GLuint VideoUtils::GenerateTextureFromVideoFrame(unsigned char* data, int width, int height) {
        if (!data || width <= 0 || height <= 0) return 0;
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, data);
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

    bool VideoUtils::SaveVideoFrameAsImage(const std::string& videoPath,
        const std::string& imagePath,
        double timeInSeconds) {
        int w, h, c;
        unsigned char* frame = LoadVideoFrame(videoPath, timeInSeconds, w, h, c);
        if (!frame) return false;
        bool ok = Utils::ImageUtils::SaveImage(imagePath, w, h, c, frame);
        FreeVideoFrameData(frame);
        return ok;
    }

    bool VideoUtils::EncodeFramesToVideo(const std::vector<VideoFrame>& frames,
        const std::string& outputPath,
        int fps,
        const nlohmann::json& metadata,
        const AudioData* audio) {
        if (frames.empty() || outputPath.empty()) return false;

        std::string ext = std::filesystem::path(outputPath).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        std::string container = "mp4";
        AVCodecID videoCodecId = AV_CODEC_ID_H264;
        AVCodecID audioCodecId = AV_CODEC_ID_AAC;
        int targetAudioRate = 44100;

        if (ext == ".webm") {
            container = "webm";
            videoCodecId = AV_CODEC_ID_VP9;
            audioCodecId = AV_CODEC_ID_VORBIS;
        }
        else if (ext == ".mkv") {
            container = "matroska";
        }
        else if (ext == ".mov") {
            container = "mov";
        }
        else if (ext == ".avi") {
            container = "avi";
            videoCodecId = AV_CODEC_ID_MPEG4;
            audioCodecId = AV_CODEC_ID_MP3;
        }

        const AVOutputFormat* fmt = av_guess_format(container.c_str(), nullptr, nullptr);
        if (!fmt) fmt = av_guess_format("mp4", nullptr, nullptr);
        if (!fmt) return false;

        AVFormatContext* fmtCtx = nullptr;
        if (avformat_alloc_output_context2(&fmtCtx, const_cast<AVOutputFormat*>(fmt),
            nullptr, outputPath.c_str()) < 0 || !fmtCtx) {
            return false;
        }

        const AVCodec* vcodec = avcodec_find_encoder(videoCodecId);
        if (!vcodec && videoCodecId == AV_CODEC_ID_VP9) vcodec = avcodec_find_encoder(AV_CODEC_ID_VP8);
        if (!vcodec) vcodec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
        if (!vcodec) { avformat_free_context(fmtCtx); return false; }

        AVStream* vstream = avformat_new_stream(fmtCtx, nullptr);
        AVCodecContext* vcc = avcodec_alloc_context3(vcodec);
        if (!vstream || !vcc) {
            if (vcc) avcodec_free_context(&vcc);
            avformat_free_context(fmtCtx);
            return false;
        }

        int inCh = frames[0].channels;
        AVPixelFormat inFmt = (inCh == 3) ? AV_PIX_FMT_RGB24
            : (inCh == 4) ? AV_PIX_FMT_RGBA
            : AV_PIX_FMT_NONE;
        if (inFmt == AV_PIX_FMT_NONE) {
            avcodec_free_context(&vcc);
            avformat_free_context(fmtCtx);
            return false;
        }

        int width = frames[0].width;
        int height = frames[0].height;

        vcc->width = width;
        vcc->height = height;
        vcc->time_base = AVRational{ 1, fps };
        vcc->framerate = AVRational{ fps, 1 };
        vcc->pix_fmt = AV_PIX_FMT_YUV420P;
        vcc->bit_rate = 2000000;
        vcc->gop_size = 10;

        if (videoCodecId == AV_CODEC_ID_VP9 || videoCodecId == AV_CODEC_ID_VP8) {
            av_opt_set(vcc->priv_data, "speed", "4", 0);
            av_opt_set(vcc->priv_data, "row-mt", "1", 0);
        }
        if (fmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
            vcc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        if (avcodec_open2(vcc, vcodec, nullptr) < 0) {
            avcodec_free_context(&vcc);
            avformat_free_context(fmtCtx);
            return false;
        }

        avcodec_parameters_from_context(vstream->codecpar, vcc);
        vstream->time_base = vcc->time_base;

        AVStream* astream = nullptr;
        AVCodecContext* acc = nullptr;
        bool hasAudio = false;

        if (audio && !audio->pcmData.empty() && audio->channels > 0 && audio->sampleRate > 0) {
            const AVCodec* acodec = avcodec_find_encoder(audioCodecId);
            if (!acodec) acodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
            if (!acodec) acodec = avcodec_find_encoder(AV_CODEC_ID_MP3);

            if (acodec) {
                astream = avformat_new_stream(fmtCtx, nullptr);
                acc = astream ? avcodec_alloc_context3(acodec) : nullptr;
                if (astream && acc) {
                    AVChannelLayout layout;
                    av_channel_layout_default(&layout, audio->channels);
                    av_channel_layout_copy(&acc->ch_layout, &layout);
                    acc->sample_rate = targetAudioRate;
                    acc->sample_fmt = acodec->sample_fmts
                        ? acodec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
                    acc->bit_rate = 128000;
                    acc->time_base = AVRational{ 1, acc->sample_rate };
                    if (fmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
                        acc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

                    if (avcodec_open2(acc, acodec, nullptr) >= 0) {
                        avcodec_parameters_from_context(astream->codecpar, acc);
                        astream->time_base = acc->time_base;
                        hasAudio = true;
                    }
                    else {
                        avcodec_free_context(&acc);
                        acc = nullptr;
                        astream = nullptr;
                    }
                }
                else {
                    if (acc) avcodec_free_context(&acc);
                    acc = nullptr;
                }
            }
        }

        if (!metadata.is_null() && !metadata.empty()) {
            std::string jsonStr = metadata.dump();
            av_dict_set(&fmtCtx->metadata, "comment", jsonStr.c_str(), 0);
            av_dict_set(&fmtCtx->metadata, "software", "AniStudio", 0);
        }

        if (avio_open(&fmtCtx->pb, outputPath.c_str(), AVIO_FLAG_WRITE) < 0) {
            avcodec_free_context(&vcc);
            if (acc) avcodec_free_context(&acc);
            avformat_free_context(fmtCtx);
            return false;
        }

        if (avformat_write_header(fmtCtx, nullptr) < 0) {
            avio_close(fmtCtx->pb);
            avcodec_free_context(&vcc);
            if (acc) avcodec_free_context(&acc);
            avformat_free_context(fmtCtx);
            return false;
        }

        SwsContext* sws = sws_getContext(width, height, inFmt,
            width, height, AV_PIX_FMT_YUV420P,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
        AVFrame* frame = av_frame_alloc();
        AVFrame* yuv = av_frame_alloc();
        AVPacket* vpkt = av_packet_alloc();
        AVPacket* apkt = av_packet_alloc();

        int bufSize = av_image_get_buffer_size(inFmt, width, height, 1);
        uint8_t* buf = (uint8_t*)av_malloc(bufSize);
        av_image_fill_arrays(frame->data, frame->linesize, buf, inFmt, width, height, 1);
        frame->width = width; frame->height = height; frame->format = inFmt;

        yuv->width = width; yuv->height = height; yuv->format = AV_PIX_FMT_YUV420P;
        int yuvSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 1);
        uint8_t* yuvBuf = (uint8_t*)av_malloc(yuvSize);
        av_image_fill_arrays(yuv->data, yuv->linesize, yuvBuf,
            AV_PIX_FMT_YUV420P, width, height, 1);

        bool failed = false;
        int64_t pts = 0;
        for (const auto& vf : frames) {
            if (vf.width != width || vf.height != height ||
                vf.channels != inCh || !vf.data) continue;

            memcpy(frame->data[0], vf.data,
                static_cast<size_t>(width) * height * inCh);
            sws_scale(sws, frame->data, frame->linesize, 0, height,
                yuv->data, yuv->linesize);
            yuv->pts = pts++;

            if (avcodec_send_frame(vcc, yuv) < 0) { failed = true; break; }
            while (true) {
                int r = avcodec_receive_packet(vcc, vpkt);
                if (r == AVERROR(EAGAIN) || r == AVERROR_EOF) break;
                if (r < 0) { failed = true; break; }
                av_packet_rescale_ts(vpkt, vcc->time_base, vstream->time_base);
                vpkt->stream_index = vstream->index;
                if (av_interleaved_write_frame(fmtCtx, vpkt) < 0) { failed = true; break; }
                av_packet_unref(vpkt);
            }
            if (failed) break;
        }

        if (hasAudio && !failed) {
            const int srcCh = audio->channels;
            const int dstCh = acc->ch_layout.nb_channels;
            const int dstRate = acc->sample_rate;
            const int dstFrameSize = acc->frame_size > 0 ? acc->frame_size : 1024;
            const size_t totalSrcFrames = audio->pcmData.size() / srcCh;

            SwrContext* swr = nullptr;
            AVChannelLayout inL, outL;
            av_channel_layout_default(&inL, srcCh);
            av_channel_layout_copy(&outL, &acc->ch_layout);
            swr_alloc_set_opts2(&swr, &outL, acc->sample_fmt, dstRate,
                &inL, AV_SAMPLE_FMT_FLT, audio->sampleRate, 0, nullptr);
            if (!swr || swr_init(swr) < 0) {
                if (swr) swr_free(&swr);
                swr = nullptr;
                failed = true;
            }

            int64_t audioPts = 0;
            size_t srcPos = 0;

            while (!failed && srcPos < totalSrcFrames) {
                size_t srcFrames = std::min<size_t>(dstFrameSize, totalSrcFrames - srcPos);
                const uint8_t* inPlanes[1] = {
                    reinterpret_cast<const uint8_t*>(audio->pcmData.data() + srcPos * srcCh)
                };

                AVFrame* af = av_frame_alloc();
                af->format = acc->sample_fmt;
                af->sample_rate = dstRate;
                av_channel_layout_copy(&af->ch_layout, &acc->ch_layout);
                af->nb_samples = dstFrameSize;
                if (av_frame_get_buffer(af, 0) < 0) {
                    av_frame_free(&af); failed = true; break;
                }

                int conv = swr_convert(swr, af->data, dstFrameSize,
                    inPlanes, static_cast<int>(srcFrames));
                if (conv <= 0) {
                    av_frame_free(&af);
                    srcPos += srcFrames;
                    continue;
                }
                af->nb_samples = conv;
                af->pts = audioPts;
                audioPts += conv;

                if (avcodec_send_frame(acc, af) < 0) {
                    av_frame_free(&af); failed = true; break;
                }
                av_frame_free(&af);

                while (true) {
                    int r = avcodec_receive_packet(acc, apkt);
                    if (r == AVERROR(EAGAIN) || r == AVERROR_EOF) break;
                    if (r < 0) { failed = true; break; }
                    av_packet_rescale_ts(apkt, acc->time_base, astream->time_base);
                    apkt->stream_index = astream->index;
                    if (av_interleaved_write_frame(fmtCtx, apkt) < 0) { failed = true; break; }
                    av_packet_unref(apkt);
                }
                srcPos += srcFrames;
            }

            if (swr && !failed) {
                for (;;) {
                    AVFrame* af = av_frame_alloc();
                    af->format = acc->sample_fmt;
                    af->sample_rate = dstRate;
                    av_channel_layout_copy(&af->ch_layout, &acc->ch_layout);
                    af->nb_samples = dstFrameSize;
                    if (av_frame_get_buffer(af, 0) < 0) { av_frame_free(&af); break; }
                    int conv = swr_convert(swr, af->data, dstFrameSize, nullptr, 0);
                    if (conv <= 0) { av_frame_free(&af); break; }
                    af->nb_samples = conv;
                    af->pts = audioPts;
                    audioPts += conv;
                    if (avcodec_send_frame(acc, af) < 0) { av_frame_free(&af); break; }
                    av_frame_free(&af);
                    while (true) {
                        int r = avcodec_receive_packet(acc, apkt);
                        if (r == AVERROR(EAGAIN) || r == AVERROR_EOF) break;
                        if (r < 0) { failed = true; break; }
                        av_packet_rescale_ts(apkt, acc->time_base, astream->time_base);
                        apkt->stream_index = astream->index;
                        av_interleaved_write_frame(fmtCtx, apkt);
                        av_packet_unref(apkt);
                    }
                }
            }
            if (swr) swr_free(&swr);
        }

        if (!failed) {
            avcodec_send_frame(vcc, nullptr);
            while (true) {
                int r = avcodec_receive_packet(vcc, vpkt);
                if (r == AVERROR_EOF || r == AVERROR(EAGAIN)) break;
                if (r < 0) { failed = true; break; }
                av_packet_rescale_ts(vpkt, vcc->time_base, vstream->time_base);
                vpkt->stream_index = vstream->index;
                av_interleaved_write_frame(fmtCtx, vpkt);
                av_packet_unref(vpkt);
            }
        }
        if (hasAudio && !failed && acc) {
            avcodec_send_frame(acc, nullptr);
            while (true) {
                int r = avcodec_receive_packet(acc, apkt);
                if (r == AVERROR_EOF || r == AVERROR(EAGAIN)) break;
                if (r < 0) { failed = true; break; }
                av_packet_rescale_ts(apkt, acc->time_base, astream->time_base);
                apkt->stream_index = astream->index;
                av_interleaved_write_frame(fmtCtx, apkt);
                av_packet_unref(apkt);
            }
        }

        if (!failed) av_write_trailer(fmtCtx);

        av_packet_free(&vpkt);
        av_packet_free(&apkt);
        av_frame_free(&frame);
        av_frame_free(&yuv);
        sws_freeContext(sws);
        av_free(buf);
        av_free(yuvBuf);
        avcodec_free_context(&vcc);
        if (acc) avcodec_free_context(&acc);
        avio_close(fmtCtx->pb);
        avformat_free_context(fmtCtx);

        if (failed) {
            std::error_code ec;
            std::filesystem::remove(outputPath, ec);
            return false;
        }
        return true;
    }

} // namespace Utils