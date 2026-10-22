#include "VideoMetadataUtils.hpp"
#include "MetadataUtils.hpp"
#include <iostream>
#include <filesystem>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
}

namespace {

    static bool LooksLikeAniStudio(const nlohmann::json& meta) {
        if (!meta.is_object()) return false;

        if (meta.contains("software") && meta["software"].is_string() &&
            meta["software"].get<std::string>() == "AniStudio") {
            return true;
        }
        if (meta.contains("components") && meta["components"].is_array()) {
            return true;
        }
        if (meta.contains("dataType") && meta["dataType"] == "entity" &&
            meta.contains("data")) {
            return true;
        }
        for (auto it = meta.begin(); it != meta.end(); ++it) {
            const auto& v = it.value();
            if (!v.is_object()) continue;
            if (v.contains("software") && v["software"].is_string() &&
                v["software"].get<std::string>() == "AniStudio") {
                return true;
            }
            if (v.contains("components") && v["components"].is_array()) {
                return true;
            }
        }
        return false;
    }

} // namespace

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
            std::string key = tag->key;
            std::string value = tag->value;
            try {
                nlohmann::json parsed = nlohmann::json::parse(value);
                if (parsed.is_object() || parsed.is_array()) {
                    result[key] = parsed;
                }
                else {
                    result[key] = value;
                }
            }
            catch (...) {
                result[key] = value;
            }
        }

        avformat_close_input(&fmt_ctx);

        result = MetadataUtils::NormalizeAniStudioMetadata(result);
        return result;
    }

    bool VideoMetadataUtils::HasAniStudioMetadata(const nlohmann::json& meta) {
        return LooksLikeAniStudio(meta);
    }

    bool VideoMetadataUtils::HasExifMetadata(const nlohmann::json& meta) {
        if (!meta.is_object() || meta.empty()) return false;

        static const std::vector<std::string> exifish = {
            "exif", "EXIF", "Exif",
            "comment", "description", "title",
            "artist", "copyright",
            "creation_time", "location",
            "make", "model", "encoder"
        };
        for (const auto& k : exifish) {
            if (!meta.contains(k)) continue;
            const auto& v = meta[k];
            if (v.is_string() && !v.get<std::string>().empty()) return true;
            if (v.is_object() && !v.empty()) return true;
            if (v.is_array() && !v.empty()) return true;
        }
        for (auto it = meta.begin(); it != meta.end(); ++it) {
            if (it.value().is_object() && !it.value().empty()) return true;
        }
        return false;
    }

    bool VideoMetadataUtils::HasLSBMetadata(const nlohmann::json& meta) {
        if (!meta.is_object()) return false;
        static const std::vector<std::string> keys = {
            "LSB", "lsb", "Stealth", "steganography", "Hidden"
        };
        for (const auto& k : keys) {
            if (meta.contains(k)) return true;
        }
        for (auto it = meta.begin(); it != meta.end(); ++it) {
            if (!it.value().is_object()) continue;
            for (const auto& k : keys) {
                if (it.value().contains(k)) return true;
            }
        }
        return false;
    }

} // namespace Utils