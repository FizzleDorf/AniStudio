#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

struct AVFormatContextDeleter {
    void operator()(AVFormatContext* ptr) const {
        if (ptr) avformat_close_input(&ptr);
    }
};

struct AVCodecContextDeleter {
    void operator()(AVCodecContext* ptr) const {
        if (ptr) avcodec_free_context(&ptr);
    }
};

struct AVFrameDeleter {
    void operator()(AVFrame* ptr) const {
        if (ptr) av_frame_free(&ptr);
    }
};

struct AVPacketDeleter {
    void operator()(AVPacket* ptr) const {
        if (ptr) av_packet_free(&ptr);
    }
};

struct SwsContextDeleter {
    void operator()(SwsContext* ptr) const {
        if (ptr) sws_freeContext(ptr);
    }
};

struct SwrContextDeleter {
    void operator()(SwrContext* ptr) const {
        if (ptr) swr_free(&ptr);
    }
};