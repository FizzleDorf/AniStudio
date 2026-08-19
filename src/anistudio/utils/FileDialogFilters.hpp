#pragma once

#include <nfd.h>
#include <cstddef>

namespace FileDialog {

    enum class FilterType {
        PATH_SELECTION,
        DIFFUSION_MODEL,
        IMAGE_FILE,
        VIDEO_FILE,
        AUDIO_FILE,
        METADATA_FILE,
        TEXT_FILE,
        ALL_FILES
    };

    static const nfdu8filteritem_t ImageFilters[] = {
        { "All Image Files", "png,jpg,jpeg,bmp,tga,webp,tiff,gif,ico,psd,exr,hdr,pic,pnm" },
        { "PNG", "png" },
        { "JPEG", "jpg,jpeg" },
        { "Bitmap", "bmp" },
        { "Targa", "tga" },
        { "WebP", "webp" },
        { "TIFF", "tiff" },
        { "GIF", "gif" },
        { "HDR", "hdr,pic,pnm" }
    };

    static const nfdu8filteritem_t VideoFilters[] = {
        { "All Video Files", "mp4,webm,mkv,avi,mov,flv,wmv,mpg,mpeg,3gp,ogv,ogg,ts,m4v" },
        { "MP4", "mp4" },
        { "WebM", "webm" },
        { "Matroska", "mkv" },
        { "AVI", "avi" },
        { "QuickTime", "mov" },
        { "FLV", "flv" },
        { "WMV", "wmv" },
        { "MPEG", "mpg,mpeg" }
    };

    static const nfdu8filteritem_t AudioFilters[] = {
        { "All Audio Files", "wav,mp3,flac,aac,ogg,m4a,opus" },
        { "WAV", "wav" },
        { "MP3", "mp3" },
        { "FLAC", "flac" },
        { "AAC", "aac" },
        { "OGG", "ogg" },
        { "M4A", "m4a" },
        { "Opus", "opus" }
    };

    static const nfdu8filteritem_t ModelFilters[] = {
        { "All Model Files", "safetensors,ckpt,pt,gguf,bin,pth,onnx,tflite" },
        { "SafeTensors", "safetensors" },
        { "Checkpoint", "ckpt" },
        { "PyTorch", "pt,pth" },
        { "GGUF", "gguf" },
        { "Binary", "bin" },
        { "ONNX", "onnx" },
        { "TensorFlow Lite", "tflite" }
    };

    static const nfdu8filteritem_t MetadataFilters[] = {
        { "All Metadata Files", "json,png,jpg,jpeg,webp,webm,mp4,mkv,avi,mov" },
        { "JSON", "json" },
        { "PNG with Metadata", "png" },
        { "JPEG with Metadata", "jpg,jpeg" },
        { "WebP with Metadata", "webp" },
        { "Video with Metadata", "webm,mp4,mkv,avi,mov" }
    };

    static const nfdu8filteritem_t TextFilters[] = {
        { "Text Files", "txt" },
        { "All Files", "*" }
    };

    static const nfdu8filteritem_t AllFilesFilter[] = {
        { "All Files", "*" }
    };

    inline void GetFilterItems(FilterType type, const nfdu8filteritem_t*& items, nfdfiltersize_t& count) {
        switch (type) {
        case FilterType::IMAGE_FILE:
            items = ImageFilters;
            count = sizeof(ImageFilters) / sizeof(ImageFilters[0]);
            break;
        case FilterType::VIDEO_FILE:
            items = VideoFilters;
            count = sizeof(VideoFilters) / sizeof(VideoFilters[0]);
            break;
        case FilterType::AUDIO_FILE:
            items = AudioFilters;
            count = sizeof(AudioFilters) / sizeof(AudioFilters[0]);
            break;
        case FilterType::DIFFUSION_MODEL:
            items = ModelFilters;
            count = sizeof(ModelFilters) / sizeof(ModelFilters[0]);
            break;
        case FilterType::METADATA_FILE:
            items = MetadataFilters;
            count = sizeof(MetadataFilters) / sizeof(MetadataFilters[0]);
            break;
        case FilterType::TEXT_FILE:
            items = TextFilters;
            count = sizeof(TextFilters) / sizeof(TextFilters[0]);
            break;
        case FilterType::ALL_FILES:
        default:
            items = AllFilesFilter;
            count = sizeof(AllFilesFilter) / sizeof(AllFilesFilter[0]);
            break;
        }
    }

}