#include "VideoUtils.hpp"
#include <iostream>
#include <filesystem>
#include "ImageUtils.hpp"
#include "VideoMetadataUtils.hpp"
#include "FileFormats.hpp"
#include "MetadataUtils.hpp"

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
#include <libswresample/swresample.h>
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
        const nlohmann::json& metadata,
        const AudioData* audio) {
        if (frames.empty() || outputPath.empty())
            return false;

        std::string ext = std::filesystem::path(outputPath).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        std::string container = "mp4";
        AVCodecID videoCodecId = AV_CODEC_ID_H264;
        AVCodecID audioCodecId = AV_CODEC_ID_AAC;
        int targetAudioSampleRate = 44100;

        if (ext == ".webm") {
            container = "webm";
            videoCodecId = AV_CODEC_ID_VP9;
            audioCodecId = AV_CODEC_ID_VORBIS;
            targetAudioSampleRate = 44100;
        }
        else if (ext == ".mkv") {
            container = "matroska";
            videoCodecId = AV_CODEC_ID_H264;
            audioCodecId = AV_CODEC_ID_AAC;
            targetAudioSampleRate = 44100;
        }
        else if (ext == ".mov") {
            container = "mov";
            videoCodecId = AV_CODEC_ID_H264;
            audioCodecId = AV_CODEC_ID_AAC;
            targetAudioSampleRate = 44100;
        }
        else if (ext == ".avi") {
            container = "avi";
            videoCodecId = AV_CODEC_ID_MPEG4;
            audioCodecId = AV_CODEC_ID_MP3;
            targetAudioSampleRate = 44100;
        }
        else {
            container = "mp4";
            videoCodecId = AV_CODEC_ID_H264;
            audioCodecId = AV_CODEC_ID_AAC;
            targetAudioSampleRate = 44100;
        }

        const AVOutputFormat* fmt = av_guess_format(container.c_str(), nullptr, nullptr);
        if (!fmt) {
            fmt = av_guess_format("mp4", nullptr, nullptr);
            if (!fmt) {
                std::cerr << "Failed to find output format" << std::endl;
                return false;
            }
        }

        AVFormatContext* fmtCtx = nullptr;
        int ret = avformat_alloc_output_context2(&fmtCtx, const_cast<AVOutputFormat*>(fmt), nullptr, outputPath.c_str());
        if (ret < 0 || !fmtCtx) {
            std::cerr << "Failed to allocate output context for " << outputPath << std::endl;
            return false;
        }

        const AVCodec* videoCodec = avcodec_find_encoder(videoCodecId);
        if (!videoCodec) {
            if (videoCodecId == AV_CODEC_ID_VP9) {
                std::cerr << "VP9 encoder not found, trying VP8..." << std::endl;
                videoCodec = avcodec_find_encoder(AV_CODEC_ID_VP8);
            }
            if (!videoCodec) {
                videoCodec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
            }
            if (!videoCodec) {
                std::cerr << "No video encoder found" << std::endl;
                avformat_free_context(fmtCtx);
                return false;
            }
        }

        AVStream* videoStream = avformat_new_stream(fmtCtx, nullptr);
        if (!videoStream) {
            std::cerr << "Failed to create video stream" << std::endl;
            avformat_free_context(fmtCtx);
            return false;
        }

        AVCodecContext* videoCodecCtx = avcodec_alloc_context3(videoCodec);
        if (!videoCodecCtx) {
            std::cerr << "Failed to allocate video codec context" << std::endl;
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
            avcodec_free_context(&videoCodecCtx);
            avformat_free_context(fmtCtx);
            return false;
        }

        int width = frames[0].width;
        int height = frames[0].height;

        videoCodecCtx->width = width;
        videoCodecCtx->height = height;
        videoCodecCtx->time_base = AVRational{ 1, fps };
        videoCodecCtx->framerate = AVRational{ fps, 1 };
        videoCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
        videoCodecCtx->bit_rate = 2000000;
        videoCodecCtx->gop_size = 10;

        if (videoCodecId == AV_CODEC_ID_VP9 || videoCodecId == AV_CODEC_ID_VP8) {
            av_opt_set(videoCodecCtx->priv_data, "speed", "4", 0);
            av_opt_set(videoCodecCtx->priv_data, "row-mt", "1", 0);
        }

        if (fmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
            videoCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        ret = avcodec_open2(videoCodecCtx, videoCodec, nullptr);
        if (ret < 0) {
            std::cerr << "Failed to open video encoder" << std::endl;
            avcodec_free_context(&videoCodecCtx);
            avformat_free_context(fmtCtx);
            return false;
        }

        avcodec_parameters_from_context(videoStream->codecpar, videoCodecCtx);
        videoStream->time_base = videoCodecCtx->time_base;

        AVStream* audioStream = nullptr;
        AVCodecContext* audioCodecCtx = nullptr;
        bool hasAudio = false;

        if (audio && !audio->pcmData.empty()) {
            hasAudio = true;
            std::cout << "VideoUtils: Encoding with audio: " << audio->channels << "ch, "
                << audio->sampleRate << "Hz, " << audio->duration << "s" << std::endl;

            const AVCodec* audioCodec = avcodec_find_encoder(audioCodecId);
            if (!audioCodec) {
                audioCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
            }
            if (!audioCodec) {
                audioCodec = avcodec_find_encoder(AV_CODEC_ID_MP3);
            }
            if (!audioCodec) {
                std::cerr << "Could not find audio encoder" << std::endl;
                hasAudio = false;
            }
            else {
                audioStream = avformat_new_stream(fmtCtx, nullptr);
                if (!audioStream) {
                    std::cerr << "Failed to create audio stream" << std::endl;
                    hasAudio = false;
                }
                else {
                    audioCodecCtx = avcodec_alloc_context3(audioCodec);
                    if (!audioCodecCtx) {
                        std::cerr << "Failed to allocate audio codec context" << std::endl;
                        hasAudio = false;
                    }
                    else {
                        audioCodecCtx->sample_rate = targetAudioSampleRate;
                        audioCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
                        audioCodecCtx->channels = audio->channels;
                        audioCodecCtx->sample_fmt = audioCodec->sample_fmts ? audioCodec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
                        audioCodecCtx->bit_rate = 128000;
                        audioCodecCtx->time_base = AVRational{ 1, audioCodecCtx->sample_rate };

                        if (fmtCtx->oformat->flags & AVFMT_GLOBALHEADER) {
                            audioCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
                        }

                        if (avcodec_open2(audioCodecCtx, audioCodec, nullptr) < 0) {
                            std::cerr << "Failed to open audio encoder" << std::endl;
                            avcodec_free_context(&audioCodecCtx);
                            hasAudio = false;
                        }
                        else {
                            avcodec_parameters_from_context(audioStream->codecpar, audioCodecCtx);
                            audioStream->time_base = audioCodecCtx->time_base;
                            std::cout << "Audio encoder opened: " << audioCodecCtx->sample_rate << "Hz" << std::endl;
                        }
                    }
                }
            }

            if (!hasAudio) {
                std::cerr << "Warning: Audio encoding failed, continuing without audio" << std::endl;
            }
        }

        if (!metadata.is_null() && !metadata.empty()) {
            std::string jsonStr = metadata.dump();
            av_dict_set(&fmtCtx->metadata, "comment", jsonStr.c_str(), 0);
            av_dict_set(&fmtCtx->metadata, "software", "AniStudio", 0);
        }

        ret = avio_open(&fmtCtx->pb, outputPath.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            std::cerr << "Failed to open output file " << outputPath << std::endl;
            avcodec_free_context(&videoCodecCtx);
            if (hasAudio) avcodec_free_context(&audioCodecCtx);
            avformat_free_context(fmtCtx);
            return false;
        }

        ret = avformat_write_header(fmtCtx, nullptr);
        if (ret < 0) {
            std::cerr << "Failed to write header" << std::endl;
            avio_close(fmtCtx->pb);
            avcodec_free_context(&videoCodecCtx);
            if (hasAudio) avcodec_free_context(&audioCodecCtx);
            avformat_free_context(fmtCtx);
            return false;
        }

        SwsContext* swsCtx = sws_getContext(width, height, inputPixFmt,
            width, height, AV_PIX_FMT_YUV420P,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!swsCtx) {
            std::cerr << "Failed to create sws context" << std::endl;
            avio_close(fmtCtx->pb);
            avcodec_free_context(&videoCodecCtx);
            if (hasAudio) avcodec_free_context(&audioCodecCtx);
            avformat_free_context(fmtCtx);
            return false;
        }

        SwrContext* audioSwrCtx = nullptr;
        if (hasAudio && audioCodecCtx) {
            if (audio->sampleRate != targetAudioSampleRate) {
                audioSwrCtx = swr_alloc();
                if (audioSwrCtx) {
                    av_opt_set_int(audioSwrCtx, "in_channel_layout", AV_CH_LAYOUT_STEREO, 0);
                    av_opt_set_int(audioSwrCtx, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
                    av_opt_set_int(audioSwrCtx, "in_sample_rate", audio->sampleRate, 0);
                    av_opt_set_int(audioSwrCtx, "out_sample_rate", audioCodecCtx->sample_rate, 0);
                    av_opt_set_sample_fmt(audioSwrCtx, "in_sample_fmt", AV_SAMPLE_FMT_FLT, 0);
                    av_opt_set_sample_fmt(audioSwrCtx, "out_sample_fmt", audioCodecCtx->sample_fmt, 0);

                    if (swr_init(audioSwrCtx) < 0) {
                        std::cerr << "Failed to initialize audio resampler" << std::endl;
                        swr_free(&audioSwrCtx);
                        audioSwrCtx = nullptr;
                    }
                    else {
                        std::cout << "Audio resampler initialized: " << audio->sampleRate << "Hz -> "
                            << audioCodecCtx->sample_rate << "Hz" << std::endl;
                    }
                }
            }
            else {
                std::cout << "No resampling needed, sample rates match." << std::endl;
            }
        }

        AVFrame* frame = av_frame_alloc();
        AVFrame* yuvFrame = av_frame_alloc();
        if (!frame || !yuvFrame) {
            av_frame_free(&frame);
            av_frame_free(&yuvFrame);
            sws_freeContext(swsCtx);
            if (audioSwrCtx) swr_free(&audioSwrCtx);
            avio_close(fmtCtx->pb);
            avcodec_free_context(&videoCodecCtx);
            if (hasAudio) avcodec_free_context(&audioCodecCtx);
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

        AVPacket* videoPkt = av_packet_alloc();
        AVPacket* audioPkt = av_packet_alloc();
        if (!videoPkt || !audioPkt) {
            av_packet_free(&videoPkt);
            av_packet_free(&audioPkt);
            av_frame_free(&frame);
            av_frame_free(&yuvFrame);
            sws_freeContext(swsCtx);
            if (audioSwrCtx) swr_free(&audioSwrCtx);
            avio_close(fmtCtx->pb);
            avcodec_free_context(&videoCodecCtx);
            if (hasAudio) avcodec_free_context(&audioCodecCtx);
            avformat_free_context(fmtCtx);
            return false;
        }

        int64_t pts = 0;
        size_t totalFrames = frames.size();
        size_t progressStep = totalFrames / 10;
        if (progressStep == 0) progressStep = 1;
        std::cout << "Encoding " << totalFrames << " video frames..." << std::endl;

        bool encodingFailed = false;

        for (size_t i = 0; i < frames.size(); ++i) {
            const auto& vf = frames[i];
            if (vf.width != width || vf.height != height || vf.channels != inputChannels || !vf.data) {
                std::cerr << "Frame size mismatch or invalid data" << std::endl;
                continue;
            }
            size_t dataSize = width * height * inputChannels;
            memcpy(frame->data[0], vf.data, dataSize);

            sws_scale(swsCtx, frame->data, frame->linesize, 0, height,
                yuvFrame->data, yuvFrame->linesize);

            yuvFrame->pts = pts++;

            ret = avcodec_send_frame(videoCodecCtx, yuvFrame);
            if (ret < 0) {
                std::cerr << "Error sending video frame: " << ret << std::endl;
                encodingFailed = true;
                break;
            }

            while (ret >= 0) {
                ret = avcodec_receive_packet(videoCodecCtx, videoPkt);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                if (ret < 0) {
                    std::cerr << "Error receiving video packet: " << ret << std::endl;
                    encodingFailed = true;
                    break;
                }
                av_packet_rescale_ts(videoPkt, videoCodecCtx->time_base, videoStream->time_base);
                videoPkt->stream_index = videoStream->index;
                ret = av_interleaved_write_frame(fmtCtx, videoPkt);
                if (ret < 0) {
                    std::cerr << "Error writing video frame: " << ret << std::endl;
                    encodingFailed = true;
                    break;
                }
                av_packet_unref(videoPkt);
            }
            if (encodingFailed) break;

            if ((i + 1) % progressStep == 0 || i == totalFrames - 1) {
                int percent = (int)((i + 1) * 100 / totalFrames);
                std::cout << "Encoding video: " << percent << "% (" << (i + 1) << "/" << totalFrames << " frames)" << std::endl;
            }
        }

        // --- Encode all audio samples separately ---
        if (hasAudio && audioCodecCtx) {
            std::cout << "Encoding audio..." << std::endl;
            size_t audioSamplePos = 0;
            size_t totalAudioSamples = audio->pcmData.size() / audio->channels;
            int samplesPerFrame = audioCodecCtx->frame_size > 0 ? audioCodecCtx->frame_size : 1024;

            while (audioSamplePos < totalAudioSamples) {
                size_t samplesToRead = std::min(static_cast<size_t>(samplesPerFrame),
                    totalAudioSamples - audioSamplePos);

                std::vector<float> audioSamples(samplesToRead * audio->channels);
                std::memcpy(audioSamples.data(),
                    audio->pcmData.data() + audioSamplePos * audio->channels,
                    samplesToRead * audio->channels * sizeof(float));

                AVFrame* audioFrame = av_frame_alloc();
                if (!audioFrame) {
                    std::cerr << "Failed to allocate audio frame" << std::endl;
                    encodingFailed = true;
                    break;
                }

                int convertedSamples = static_cast<int>(samplesToRead);
                if (audioSwrCtx) {
                    int maxOutSamples = samplesToRead * 2;
                    int bytesPerSample = av_get_bytes_per_sample(audioCodecCtx->sample_fmt);
                    uint8_t* outBuffer = (uint8_t*)av_malloc(maxOutSamples * audioCodecCtx->channels * bytesPerSample);
                    if (!outBuffer) {
                        av_frame_free(&audioFrame);
                        std::cerr << "Failed to allocate audio buffer" << std::endl;
                        encodingFailed = true;
                        break;
                    }
                    const uint8_t* inData = reinterpret_cast<const uint8_t*>(audioSamples.data());
                    convertedSamples = swr_convert(audioSwrCtx, &outBuffer, maxOutSamples,
                        &inData, static_cast<int>(samplesToRead));
                    if (convertedSamples <= 0) {
                        av_free(outBuffer);
                        av_frame_free(&audioFrame);
                        std::cerr << "Audio resampling failed" << std::endl;
                        continue;
                    }
                    audioFrame->nb_samples = convertedSamples;
                    audioFrame->format = audioCodecCtx->sample_fmt;
                    audioFrame->channel_layout = audioCodecCtx->channel_layout;
                    audioFrame->sample_rate = audioCodecCtx->sample_rate;
                    ret = av_frame_get_buffer(audioFrame, 0);
                    if (ret < 0) {
                        av_free(outBuffer);
                        av_frame_free(&audioFrame);
                        std::cerr << "Failed to get audio frame buffer" << std::endl;
                        encodingFailed = true;
                        break;
                    }
                    int bytesPerSampleOut = av_get_bytes_per_sample(audioCodecCtx->sample_fmt);
                    for (int ch = 0; ch < audioCodecCtx->channels; ch++) {
                        std::memcpy(audioFrame->data[ch],
                            outBuffer + ch * convertedSamples * bytesPerSampleOut,
                            convertedSamples * bytesPerSampleOut);
                    }
                    av_free(outBuffer);
                }
                else {
                    audioFrame->nb_samples = static_cast<int>(samplesToRead);
                    audioFrame->format = audioCodecCtx->sample_fmt;
                    audioFrame->channel_layout = audioCodecCtx->channel_layout;
                    audioFrame->sample_rate = audioCodecCtx->sample_rate;
                    ret = av_frame_get_buffer(audioFrame, 0);
                    if (ret < 0) {
                        av_frame_free(&audioFrame);
                        std::cerr << "Failed to get audio frame buffer" << std::endl;
                        encodingFailed = true;
                        break;
                    }
                    if (audioCodecCtx->sample_fmt == AV_SAMPLE_FMT_FLTP) {
                        for (int ch = 0; ch < audioCodecCtx->channels; ch++) {
                            float* out = (float*)audioFrame->data[ch];
                            for (int j = 0; j < convertedSamples; j++) {
                                out[j] = audioSamples[j * audioCodecCtx->channels + ch];
                            }
                        }
                    }
                    else {
                        std::memcpy(audioFrame->data[0], audioSamples.data(),
                            convertedSamples * audioCodecCtx->channels * sizeof(float));
                    }
                }

                audioFrame->pts = static_cast<int64_t>(audioSamplePos);

                ret = avcodec_send_frame(audioCodecCtx, audioFrame);
                av_frame_free(&audioFrame);
                if (ret < 0) {
                    std::cerr << "Error sending audio frame: " << ret << std::endl;
                    encodingFailed = true;
                    break;
                }

                while (ret >= 0) {
                    ret = avcodec_receive_packet(audioCodecCtx, audioPkt);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    if (ret < 0) {
                        std::cerr << "Error receiving audio packet: " << ret << std::endl;
                        encodingFailed = true;
                        break;
                    }
                    av_packet_rescale_ts(audioPkt, audioCodecCtx->time_base, audioStream->time_base);
                    audioPkt->stream_index = audioStream->index;
                    ret = av_interleaved_write_frame(fmtCtx, audioPkt);
                    if (ret < 0) {
                        std::cerr << "Error writing audio frame: " << ret << std::endl;
                        encodingFailed = true;
                        break;
                    }
                    av_packet_unref(audioPkt);
                }
                if (encodingFailed) break;

                audioSamplePos += samplesToRead;
            }
            std::cout << "Audio encoding complete." << std::endl;
        }

        // Flush encoders
        if (!encodingFailed) {
            avcodec_send_frame(videoCodecCtx, nullptr);
            while (1) {
                ret = avcodec_receive_packet(videoCodecCtx, videoPkt);
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                    break;
                if (ret < 0) {
                    std::cerr << "Error flushing video: " << ret << std::endl;
                    encodingFailed = true;
                    break;
                }
                av_packet_rescale_ts(videoPkt, videoCodecCtx->time_base, videoStream->time_base);
                videoPkt->stream_index = videoStream->index;
                av_interleaved_write_frame(fmtCtx, videoPkt);
                av_packet_unref(videoPkt);
            }

            if (hasAudio && audioCodecCtx && !encodingFailed) {
                avcodec_send_frame(audioCodecCtx, nullptr);
                while (1) {
                    ret = avcodec_receive_packet(audioCodecCtx, audioPkt);
                    if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                        break;
                    if (ret < 0) {
                        std::cerr << "Error flushing audio: " << ret << std::endl;
                        encodingFailed = true;
                        break;
                    }
                    av_packet_rescale_ts(audioPkt, audioCodecCtx->time_base, audioStream->time_base);
                    audioPkt->stream_index = audioStream->index;
                    av_interleaved_write_frame(fmtCtx, audioPkt);
                    av_packet_unref(audioPkt);
                }
            }

            if (!encodingFailed) {
                av_write_trailer(fmtCtx);
                std::cout << "Encoding complete!" << std::endl;
            }
        }

        // Cleanup
        av_packet_free(&videoPkt);
        av_packet_free(&audioPkt);
        av_frame_free(&frame);
        av_frame_free(&yuvFrame);
        sws_freeContext(swsCtx);
        if (audioSwrCtx) swr_free(&audioSwrCtx);
        av_free(buffer);
        av_free(yuvBuffer);
        avcodec_free_context(&videoCodecCtx);
        if (hasAudio) avcodec_free_context(&audioCodecCtx);
        avio_close(fmtCtx->pb);
        avformat_free_context(fmtCtx);

        if (encodingFailed) {
            std::cerr << "Encoding failed, removing partial file" << std::endl;
            std::filesystem::remove(outputPath);
            return false;
        }

        return true;
    }

    bool VideoUtils::HasExifMetadata(const std::string& filePath) {
        nlohmann::json meta = VideoMetadataUtils::ReadMetadataFromVideo(filePath);
        if (meta.is_null() || meta.empty()) return false;
        meta = MetadataUtils::NormalizeAniStudioMetadata(meta);
        if (meta.contains("components") && meta["components"].is_array()) {
            for (const auto& comp : meta["components"]) {
                if (comp.is_object() && !comp.empty()) {
                    for (auto it = comp.begin(); it != comp.end(); ++it) {
                        if (!it.value().is_null() && !it.value().empty()) {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    bool VideoUtils::HasLSBMetadata(const std::string& filePath) {
        nlohmann::json meta = VideoMetadataUtils::ReadMetadataFromVideo(filePath);
        if (meta.is_null() || meta.empty()) return false;
        meta = MetadataUtils::NormalizeAniStudioMetadata(meta);
        std::vector<std::string> stealthKeys = { "LSB", "Stealth", "Hidden", "steganography", "lsb" };
        for (const auto& key : stealthKeys) {
            if (meta.contains(key)) {
                return true;
            }
        }
        if (meta.contains("components") && meta["components"].is_array()) {
            for (const auto& comp : meta["components"]) {
                if (comp.is_object()) {
                    for (const auto& key : stealthKeys) {
                        if (comp.contains(key)) {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    int VideoUtils::GetMetadataStatus(const std::string& filePath) {
        nlohmann::json meta = VideoMetadataUtils::ReadMetadataFromVideo(filePath);
        if (meta.is_null() || meta.empty()) return 0;
        meta = MetadataUtils::NormalizeAniStudioMetadata(meta);
        if (meta.contains("components") && meta["components"].is_array()) {
            for (const auto& comp : meta["components"]) {
                if (comp.is_object() && !comp.empty()) {
                    for (auto it = comp.begin(); it != comp.end(); ++it) {
                        if (!it.value().is_null() && !it.value().empty()) {
                            return 1;
                        }
                    }
                }
            }
        }
        if (meta.contains("dataType") && meta["dataType"] == "entity" && meta.contains("data")) {
            return 1;
        }
        return 0;
    }

}