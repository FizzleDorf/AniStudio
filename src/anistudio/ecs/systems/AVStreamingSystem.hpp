#pragma once

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include <portaudio.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

namespace ECS {

    class PacketQueue {
    public:
        PacketQueue();
        ~PacketQueue();

        void push(AVPacket* pkt);
        bool pop(AVPacket*& pkt, int timeout_ms = 0);
        void flush();
        size_t size() const;
        void abort();
        bool is_aborted() const;

    private:
        std::deque<AVPacket*> queue;
        mutable std::mutex mutex;
        std::condition_variable cond;
        bool aborted = false;
    };

    class VideoFrameQueue {
    public:
        VideoFrameQueue();
        ~VideoFrameQueue();

        void push(AVFrame* frame);
        bool pop(AVFrame*& frame, int timeout_ms = 0);
        void flush();
        size_t size() const;
        void abort();
        bool is_aborted() const;

    private:
        std::deque<AVFrame*> queue;
        mutable std::mutex mutex;
        std::condition_variable cond;
        bool aborted = false;
    };

    class AudioRingBuffer {
    public:
        AudioRingBuffer(size_t capacity_frames);
        ~AudioRingBuffer();

        size_t write(const float* data, size_t frames);
        size_t read(float* data, size_t frames);
        void reset();
        size_t available() const;
        size_t free_space() const;

    private:
        std::vector<float> buffer;
        size_t capacity;
        size_t write_pos = 0;
        size_t read_pos = 0;
        mutable std::mutex mutex;
    };

    struct StreamContext {
        EntityID entity_id = 0;
        AVFormatContext* fmt_ctx = nullptr;
        AVCodecContext* video_codec_ctx = nullptr;
        AVCodecContext* audio_codec_ctx = nullptr;
        SwsContext* sws_ctx = nullptr;
        SwrContext* swr_ctx = nullptr;
        int video_stream_idx = -1;
        int audio_stream_idx = -1;
        AVRational video_time_base{ 1, 1 };
        AVRational audio_time_base{ 1, 1 };
        double duration = 0.0;
        double fps = 30.0;
        int video_width = 0, video_height = 0;

        PacketQueue video_pkt_queue;
        PacketQueue audio_pkt_queue;
        VideoFrameQueue video_frame_queue;
        AudioRingBuffer audio_ring;

        int audio_channels = 0;
        int audio_sample_rate = 0;

        std::atomic<bool> is_playing{ false };
        std::atomic<bool> is_paused{ false };
        std::atomic<bool> is_seeking{ false };
        std::atomic<bool> eof{ false };
        double current_time = 0.0;
        float speed = 1.0f;
        float volume = 1.0f;
        bool looping = false;

        std::thread demux_thread;
        std::thread video_decoder_thread;
        std::thread audio_decoder_thread;

        std::mutex state_mutex;
        std::condition_variable state_cv;

        std::function<void(EntityID, unsigned char*, int, int, int, GLuint*)> texture_callback;

        StreamContext(EntityID id);
        ~StreamContext();

        void start_threads();
        void stop_threads();
        void flush_queues();
        void seek(double time);
    };

    class AVStreamingSystem : public BaseSystem {
    public:
        AVStreamingSystem(EntityManager& entityMgr);
        ~AVStreamingSystem() override;

        void Start() override;
        void Update(float deltaT) override;
        void Destroy() override;

        void Load(EntityID entity, const std::string& filePath);
        void Play(EntityID entity, bool loop = false);
        void Pause(EntityID entity);
        void Stop(EntityID entity);
        void Seek(EntityID entity, double time);
        void SetSpeed(EntityID entity, float speed);
        void SetVolume(EntityID entity, float volume);

        bool IsPlaying(EntityID entity) const;
        bool IsPaused(EntityID entity) const;
        double GetPosition(EntityID entity) const;
        double GetDuration(EntityID entity) const;
        void ClearCache(EntityID entity);

        void Remove(EntityID entity);

        void SetTextureCallback(std::function<void(EntityID, unsigned char*, int, int, int, GLuint*)> cb);

        static void demux_thread_func(StreamContext* ctx);
        static void video_decoder_thread_func(StreamContext* ctx);
        static void audio_decoder_thread_func(StreamContext* ctx);

    private:
        enum class CommandType {
            Load,
            Play,
            Pause,
            Stop,
            Seek,
            SetSpeed,
            SetVolume,
            Remove
        };

        struct Command {
            CommandType type;
            EntityID entity;
            std::string filePath;
            bool loop;
            double seek_time;
            float speed;
            float volume;
        };

        std::queue<Command> command_queue;
        mutable std::mutex command_mutex;

        std::unordered_map<EntityID, std::unique_ptr<StreamContext>> contexts;
        mutable std::mutex contexts_mutex;

        std::function<void(EntityID, unsigned char*, int, int, int, GLuint*)> m_textureCallback;

        void process_commands();
        void process_video_frames(StreamContext& ctx);
        void clean_context(EntityID entity);

        bool init_audio();
        void shutdown_audio();
        PaStream* audio_stream = nullptr;
        std::atomic<bool> audio_running{ false };

        static int pa_callback(const void* input, void* output,
            unsigned long framesPerBuffer,
            const PaStreamCallbackTimeInfo* timeInfo,
            PaStreamCallbackFlags statusFlags,
            void* userData);

        void send_video_frame(StreamContext& ctx, AVFrame* frame);
    };

}