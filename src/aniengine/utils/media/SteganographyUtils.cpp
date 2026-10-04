#include "SteganographyUtils.hpp"
#include <iostream>
#include <cstring>
#include <zlib.h>

namespace Utils {

    void SteganographyUtils::writeBit(unsigned char* pixelAlpha, int bit) {
        if (bit)
            *pixelAlpha |= 1;
        else
            *pixelAlpha &= ~1;
    }

    int SteganographyUtils::readBit(const unsigned char* pixelAlpha) {
        return (*pixelAlpha) & 1;
    }

    int SteganographyUtils::BitReader::readBit() {
        if (!ok()) return 0;
        const size_t x = bitIndex % static_cast<size_t>(width);
        const size_t y = bitIndex / static_cast<size_t>(width);
        const unsigned char* pixelAlpha =
            data + y * static_cast<size_t>(width) * 4 + x * 4 + 3;
        bitIndex++;
        return (*pixelAlpha) & 1;
    }

    unsigned char SteganographyUtils::BitReader::readByteMSB() {
        unsigned char byte = 0;
        for (int b = 0; b < 8; ++b) {
            byte = static_cast<unsigned char>((byte << 1) | readBit());
        }
        return byte;
    }

    uint32_t SteganographyUtils::BitReader::readU32BE() {
        uint32_t v = 0;
        for (int b = 0; b < 32; ++b) {
            v = (v << 1) | static_cast<uint32_t>(readBit());
        }
        return v;
    }

    bool SteganographyUtils::BitReader::readBytes(std::vector<unsigned char>& out, size_t count) {
        out.clear();
        out.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            if (!ok()) return false;
            out.push_back(readByteMSB());
        }
        return true;
    }

    bool SteganographyUtils::EmbedInAlpha(unsigned char* rgbaData, int width, int height,
        const std::vector<unsigned char>& payload,
        const std::string& signature) {
        if (!rgbaData || width <= 0 || height <= 0 || payload.empty())
            return false;

        std::vector<unsigned char> fullPayload;
        fullPayload.reserve(signature.size() * 8 + 32 + payload.size() * 8);

        for (char c : signature) {
            for (int bit = 7; bit >= 0; --bit) {
                fullPayload.push_back((c >> bit) & 1);
            }
        }

        uint32_t payloadLenBits = static_cast<uint32_t>(payload.size() * 8);
        for (int bit = 31; bit >= 0; --bit) {
            fullPayload.push_back((payloadLenBits >> bit) & 1);
        }

        for (unsigned char byte : payload) {
            for (int bit = 7; bit >= 0; --bit) {
                fullPayload.push_back((byte >> bit) & 1);
            }
        }

        size_t totalBits = fullPayload.size();
        size_t maxBits = static_cast<size_t>(width) * height;

        if (totalBits > maxBits) {
            std::cerr << "Payload too large to embed in alpha channel" << std::endl;
            return false;
        }

        const int row_stride = width * 4;
        size_t bitOffset = 0;

        for (int y = 0; y < height && bitOffset < totalBits; ++y) {
            for (int x = 0; x < width && bitOffset < totalBits; ++x) {
                unsigned char* pixelAlpha = rgbaData + y * row_stride + x * 4 + 3;
                writeBit(pixelAlpha, fullPayload[bitOffset]);
                bitOffset++;
            }
        }

        return true;
    }

    std::vector<unsigned char> SteganographyUtils::ExtractFromAlpha(const unsigned char* rgbaData,
        int width, int height,
        const std::string& signature) {
        std::vector<unsigned char> result;
        if (!rgbaData || width <= 0 || height <= 0)
            return result;

        const size_t maxBits = static_cast<size_t>(width) * height;

        if (signature.empty()) {
            return result;
        }

        const size_t sigBits = signature.size() * 8;
        if (sigBits + 32 > maxBits) {
            return result;
        }

        auto bitAt = [&](size_t bitIndex) -> int {
            const size_t x = bitIndex % static_cast<size_t>(width);
            const size_t y = bitIndex / static_cast<size_t>(width);
            const unsigned char* pixelAlpha =
                rgbaData + y * static_cast<size_t>(width) * 4 + x * 4 + 3;
            return readBit(pixelAlpha);
            };

        for (size_t byteIdx = 0; byteIdx < signature.size(); ++byteIdx) {
            unsigned char byte = 0;
            for (int b = 0; b < 8; ++b) {
                byte = static_cast<unsigned char>((byte << 1) | bitAt(byteIdx * 8 + b));
            }
            if (byte != static_cast<unsigned char>(signature[byteIdx])) {
                return result;
            }
        }

        size_t bitOffset = sigBits;

        uint32_t payloadLenBits = 0;
        for (int i = 0; i < 32; ++i) {
            payloadLenBits = (payloadLenBits << 1) | static_cast<uint32_t>(bitAt(bitOffset++));
        }

        const size_t maxPayloadBits = maxBits - bitOffset;
        if (payloadLenBits == 0 || payloadLenBits > maxPayloadBits) {
            return result;
        }

        size_t numBytes = (payloadLenBits + 7) / 8;
        result.reserve(numBytes);

        for (size_t byteIdx = 0; byteIdx < numBytes; ++byteIdx) {
            unsigned char byte = 0;
            int bitsToRead = (byteIdx == numBytes - 1)
                ? static_cast<int>(payloadLenBits % 8) : 8;
            if (bitsToRead == 0) bitsToRead = 8;
            for (int b = 0; b < bitsToRead; ++b) {
                byte = static_cast<unsigned char>((byte << 1) | bitAt(bitOffset++));
            }
            if (bitsToRead < 8) byte <<= (8 - bitsToRead);
            result.push_back(byte);
        }

        return result;
    }

    namespace {

        enum class BitLayout {
            RowMajor,
            ColumnMajor
        };

        std::vector<unsigned char> InflateGzip(const std::vector<unsigned char>& in) {
            std::vector<unsigned char> out;
            if (in.size() < 2) return out;

            z_stream strm{};
            strm.zalloc = Z_NULL;
            strm.zfree = Z_NULL;
            strm.opaque = Z_NULL;
            strm.avail_in = static_cast<uInt>(in.size());
            strm.next_in = const_cast<Bytef*>(in.data());

            if (inflateInit2(&strm, 16 + 15) != Z_OK) {
                return out;
            }

            out.resize(4096);
            int ret = Z_OK;
            while (ret != Z_STREAM_END) {
                if (strm.total_out >= out.size()) {
                    out.resize(out.size() * 2);
                }
                strm.avail_out = static_cast<uInt>(out.size() - strm.total_out);
                strm.next_out = out.data() + strm.total_out;
                ret = inflate(&strm, Z_NO_FLUSH);
                if (ret != Z_OK && ret != Z_STREAM_END) {
                    inflateEnd(&strm);
                    out.clear();
                    return out;
                }
            }
            out.resize(strm.total_out);
            inflateEnd(&strm);
            return out;
        }

        nlohmann::json TryParseJson(const std::vector<unsigned char>& bytes) {
            if (bytes.empty()) return nlohmann::json();
            try {
                size_t start = 0;
                while (start < bytes.size() &&
                    (bytes[start] == ' ' || bytes[start] == '\t' ||
                        bytes[start] == '\r' || bytes[start] == '\n'))
                {
                    ++start;
                }
                if (start >= bytes.size()) return nlohmann::json();

                std::string text(reinterpret_cast<const char*>(bytes.data() + start),
                    bytes.size() - start);
                nlohmann::json parsed = nlohmann::json::parse(text);
                if (parsed.is_object() || parsed.is_array()) {
                    return parsed;
                }
            }
            catch (...) {
            }
            return nlohmann::json();
        }

        std::vector<unsigned char> ReadLengthPrefixedPayload(
            const unsigned char* rgba, int w, int h,
            const std::string& signature,
            BitLayout layout)
        {
            std::vector<unsigned char> out;

            const size_t maxBits = static_cast<size_t>(w) * h;
            const size_t sigBits = signature.size() * 8;
            if (sigBits + 32 > maxBits) return out;

            auto bitAt = [&](size_t bitIndex) -> int {
                size_t x, y;
                if (layout == BitLayout::RowMajor) {
                    x = bitIndex % static_cast<size_t>(w);
                    y = bitIndex / static_cast<size_t>(w);
                }
                else {
                    x = bitIndex / static_cast<size_t>(h);
                    y = bitIndex % static_cast<size_t>(h);
                }
                return rgba[y * static_cast<size_t>(w) * 4 + x * 4 + 3] & 1;
                };

            for (size_t i = 0; i < signature.size(); ++i) {
                unsigned char byte = 0;
                for (int b = 0; b < 8; ++b) {
                    byte = static_cast<unsigned char>((byte << 1) | bitAt(i * 8 + b));
                }
                if (byte != static_cast<unsigned char>(signature[i])) {
                    return out;
                }
            }

            size_t bitOffset = sigBits;

            uint32_t lenBits = 0;
            for (int i = 0; i < 32; ++i) {
                lenBits = (lenBits << 1) | static_cast<uint32_t>(bitAt(bitOffset++));
            }

            const size_t remaining = maxBits - bitOffset;
            if (lenBits == 0 || lenBits > remaining) return out;

            size_t numBytes = (lenBits + 7) / 8;
            out.reserve(numBytes);
            for (size_t byteIdx = 0; byteIdx < numBytes; ++byteIdx) {
                unsigned char byte = 0;
                int bits = (byteIdx == numBytes - 1) ? static_cast<int>(lenBits % 8) : 8;
                if (bits == 0) bits = 8;
                for (int b = 0; b < bits; ++b) {
                    byte = static_cast<unsigned char>((byte << 1) | bitAt(bitOffset++));
                }
                if (bits < 8) byte <<= (8 - bits);
                out.push_back(byte);
            }
            return out;
        }

        nlohmann::json TryDecodeWithLayout(
            const unsigned char* rgbaData, int width, int height,
            const std::string& signature,
            bool requireGzip,
            bool requireJson,
            BitLayout layout,
            std::vector<unsigned char>* outRaw)
        {
            std::vector<unsigned char> raw =
                ReadLengthPrefixedPayload(rgbaData, width, height, signature, layout);
            if (raw.empty()) return nlohmann::json();

            if (requireGzip) {
                if (raw.size() < 2 || raw[0] != 0x1f || raw[1] != 0x8b) {
                    return nlohmann::json();
                }
            }
            if (requireJson) {
                size_t i = 0;
                while (i < raw.size() &&
                    (raw[i] == ' ' || raw[i] == '\t' ||
                        raw[i] == '\r' || raw[i] == '\n'))
                {
                    ++i;
                }
                if (i >= raw.size()) return nlohmann::json();
                if (raw[i] != '{' && raw[i] != '[') return nlohmann::json();
            }

            std::vector<unsigned char> inflated = InflateGzip(raw);
            if (!inflated.empty()) {
                nlohmann::json parsed = TryParseJson(inflated);
                if (!parsed.is_null()) {
                    if (outRaw) *outRaw = raw;
                    return parsed;
                }
            }

            nlohmann::json parsed = TryParseJson(raw);
            if (!parsed.is_null()) {
                if (outRaw) *outRaw = raw;
                return parsed;
            }

            return nlohmann::json();
        }

    } // anonymous namespace

    nlohmann::json SteganographyUtils::ExtractJsonFromAlpha(const unsigned char* rgbaData,
        int width, int height)
    {
        return ExtractJsonFromAlpha(rgbaData, width, height, nullptr);
    }

    nlohmann::json SteganographyUtils::ExtractJsonFromAlpha(const unsigned char* rgbaData,
        int width, int height,
        std::vector<unsigned char>* outRaw)
    {
        nlohmann::json result;
        if (!rgbaData || width <= 0 || height <= 0) return result;

        const size_t maxBits = static_cast<size_t>(width) * height;
        if (maxBits < 15 * 8 + 32) return result;

        struct Candidate {
            const char* sig;
            bool requireGzip;
            bool requireJson;
            BitLayout layout;
        };
        static const Candidate candidates[] = {
            { "stealth_pngcomp", false, false, BitLayout::ColumnMajor },
            { "stealth_pngcomp", false, false, BitLayout::RowMajor    },
            { "",                true,  false, BitLayout::ColumnMajor },
            { "",                true,  false, BitLayout::RowMajor    },
            { "",                false, true,  BitLayout::ColumnMajor },
            { "",                false, true,  BitLayout::RowMajor    },
        };

        for (const auto& c : candidates) {
            nlohmann::json parsed = TryDecodeWithLayout(
                rgbaData, width, height,
                c.sig, c.requireGzip, c.requireJson, c.layout,
                outRaw);
            if (!parsed.is_null()) {
                return parsed;
            }
        }

        return result;
    }

    bool SteganographyUtils::LooksLikeAniStudioMetadata(const nlohmann::json& j) {
        if (j.is_null() || !j.is_object()) return false;

        if (j.contains("dataType") && j["dataType"] == "entity") return true;

        if (j.contains("components") && j["components"].is_array()) return true;
        if (j.contains("parameters") && j["parameters"].is_object()) return true;

        if (j.contains("data") && j["data"].is_object()) {
            const auto& d = j["data"];
            if (d.contains("components") && d["components"].is_array()) return true;
            if (d.contains("parameters") && d["parameters"].is_object()) return true;
        }

        if (j.contains("parameters") && j["parameters"].is_object()) {
            const auto& p = j["parameters"];
            if (p.contains("components") && p["components"].is_array()) return true;
        }

        return false;
    }

} // namespace Utils