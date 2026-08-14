#include "VideoMetadataUtils.hpp"
#include "MetadataUtils.hpp"
#include <iostream>
#include <filesystem>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
}

namespace Utils {

    bool VideoMetadataUtils::WriteMetadataToVideo(const std::string& videoPath,
        const nlohmann::json& metadata,
        bool forceSidecar) {
        if (forceSidecar) {
            std::string jsonPath = videoPath + ".json";
            return MetadataUtils::SaveMetadataToJson(jsonPath, metadata);
        }

        std::string tempPath = videoPath + ".tmp";
        AVFormatContext* ifmt_ctx = nullptr;
        AVFormatContext* ofmt_ctx = nullptr;
        int ret;

        ret = avformat_open_input(&ifmt_ctx, videoPath.c_str(), nullptr, nullptr);
        if (ret < 0) {
            std::cerr << "Failed to open input video: " << videoPath << std::endl;
            return false;
        }
        ret = avformat_find_stream_info(ifmt_ctx, nullptr);
        if (ret < 0) {
            std::cerr << "Failed to find stream info" << std::endl;
            avformat_close_input(&ifmt_ctx);
            return false;
        }

        ret = avformat_alloc_output_context2(&ofmt_ctx, nullptr, nullptr, tempPath.c_str());
        if (ret < 0 || !ofmt_ctx) {
            std::cerr << "Failed to create output context" << std::endl;
            avformat_close_input(&ifmt_ctx);
            return false;
        }

        for (unsigned i = 0; i < ifmt_ctx->nb_streams; i++) {
            AVStream* in_stream = ifmt_ctx->streams[i];
            AVStream* out_stream = avformat_new_stream(ofmt_ctx, nullptr);
            if (!out_stream) {
                std::cerr << "Failed to create output stream" << std::endl;
                avformat_free_context(ofmt_ctx);
                avformat_close_input(&ifmt_ctx);
                return false;
            }
            ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
            if (ret < 0) {
                std::cerr << "Failed to copy codec parameters" << std::endl;
                avformat_free_context(ofmt_ctx);
                avformat_close_input(&ifmt_ctx);
                return false;
            }
            out_stream->time_base = in_stream->time_base;
        }

        std::string jsonStr = metadata.dump();
        av_dict_set(&ofmt_ctx->metadata, "comment", jsonStr.c_str(), 0);

        if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            ret = avio_open(&ofmt_ctx->pb, tempPath.c_str(), AVIO_FLAG_WRITE);
            if (ret < 0) {
                std::cerr << "Failed to open output file: " << tempPath << std::endl;
                avformat_free_context(ofmt_ctx);
                avformat_close_input(&ifmt_ctx);
                return false;
            }
        }

        ret = avformat_write_header(ofmt_ctx, nullptr);
        if (ret < 0) {
            std::cerr << "Error writing header" << std::endl;
            avio_close(ofmt_ctx->pb);
            avformat_free_context(ofmt_ctx);
            avformat_close_input(&ifmt_ctx);
            return false;
        }

        AVPacket pkt;
        av_init_packet(&pkt);
        while (av_read_frame(ifmt_ctx, &pkt) >= 0) {
            av_packet_rescale_ts(&pkt,
                ifmt_ctx->streams[pkt.stream_index]->time_base,
                ofmt_ctx->streams[pkt.stream_index]->time_base);
            ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
            av_packet_unref(&pkt);
            if (ret < 0) {
                std::cerr << "Error writing packet" << std::endl;
                break;
            }
        }

        av_write_trailer(ofmt_ctx);
        avio_close(ofmt_ctx->pb);
        avformat_free_context(ofmt_ctx);
        avformat_close_input(&ifmt_ctx);

        std::error_code ec;
        std::filesystem::remove(videoPath, ec);
        std::filesystem::rename(tempPath, videoPath, ec);
        if (ec) {
            std::cerr << "Failed to replace file: " << ec.message() << std::endl;
            return false;
        }

        std::cout << "Video metadata written to: " << videoPath << std::endl;
        return true;
    }

    nlohmann::json VideoMetadataUtils::ReadMetadataFromVideo(const std::string& videoPath) {
        nlohmann::json result;
        AVFormatContext* fmt_ctx = nullptr;
        if (avformat_open_input(&fmt_ctx, videoPath.c_str(), nullptr, nullptr) < 0) {
            std::cerr << "Failed to open video: " << videoPath << std::endl;
            return result;
        }
        if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
            avformat_close_input(&fmt_ctx);
            return result;
        }

        AVDictionaryEntry* tag = nullptr;
        while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
            if (std::string(tag->key) == "comment") {
                try {
                    result = nlohmann::json::parse(tag->value);
                    std::cout << "Metadata loaded from video comment" << std::endl;
                    break;
                }
                catch (...) {}
            }
        }

        avformat_close_input(&fmt_ctx);
        return result;
    }

} // namespace Utils