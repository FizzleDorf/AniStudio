#include "AVStreamingSystem.hpp"
#include "Log.hpp"

#include <chrono>
#include <cmath>

namespace ECS {

    PacketQueue::PacketQueue() = default;
    PacketQueue::~PacketQueue() { flush(); }

    void PacketQueue::push(AVPacket* pkt) {
        std::lock_guard<std::mutex> lock(mutex);
        if (aborted) {
            av_packet_free(&pkt);
            return;
        }
        queue.push_back(pkt);
        cond.notify_one();
    }

    bool PacketQueue::pop(AVPacket*& pkt, int timeout_ms) {
        std::unique_lock<std::mutex> lock(mutex);
        if (aborted) return false;
        if (queue.empty()) {
            if (timeout_ms == 0) {
                cond.wait(lock, [this] { return !queue.empty() || aborted; });
            }
            else {
                auto dur = std::chrono::milliseconds(timeout_ms);
                cond.wait_for(lock, dur, [this] { return !queue.empty() || aborted; });
            }
            if (aborted || queue.empty()) return false;
        }
        pkt = queue.front();
        queue.pop_front();
        return true;
    }

    void PacketQueue::flush() {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto pkt : queue) av_packet_free(&pkt);
        queue.clear();
        cond.notify_all();
    }

    size_t PacketQueue::size() const {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.size();
    }

    void PacketQueue::abort() {
        std::lock_guard<std::mutex> lock(mutex);
        aborted = true;
        cond.notify_all();
    }

    bool PacketQueue::is_aborted() const {
        std::lock_guard<std::mutex> lock(mutex);
        return aborted;
    }

    VideoFrameQueue::VideoFrameQueue() = default;
    VideoFrameQueue::~VideoFrameQueue() { flush(); }

    void VideoFrameQueue::push(AVFrame* frame) {
        std::lock_guard<std::mutex> lock(mutex);
        if (aborted) {
            av_frame_free(&frame);
            return;
        }
        queue.push_back(frame);
        cond.notify_one();
    }

    bool VideoFrameQueue::pop(AVFrame*& frame, int timeout_ms) {
        std::unique_lock<std::mutex> lock(mutex);
        if (aborted) return false;
        if (queue.empty()) {
            if (timeout_ms == 0) {
                cond.wait(lock, [this] { return !queue.empty() || aborted; });
            }
            else {
                auto dur = std::chrono::milliseconds(timeout_ms);
                cond.wait_for(lock, dur, [this] { return !queue.empty() || aborted; });
            }
            if (aborted || queue.empty()) return false;
        }
        frame = queue.front();
        queue.pop_front();
        return true;
    }

    void VideoFrameQueue::flush() {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto f : queue) av_frame_free(&f);
        queue.clear();
        cond.notify_all();
    }

    size_t VideoFrameQueue::size() const {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.size();
    }

    void VideoFrameQueue::abort() {
        std::lock_guard<std::mutex> lock(mutex);
        aborted = true;
        cond.notify_all();
    }

    bool VideoFrameQueue::is_aborted() const {
        std::lock_guard<std::mutex> lock(mutex);
        return aborted;
    }

    AudioRingBuffer::AudioRingBuffer(size_t capacity_frames)
        : capacity(capacity_frames * 2), buffer(capacity_frames * 2, 0.0f) {
    }

    AudioRingBuffer::~AudioRingBuffer() = default;

    size_t AudioRingBuffer::write(const float* data, size_t frames) {
        std::lock_guard<std::mutex> lock(mutex);
        size_t available = capacity - ((write_pos - read_pos) % capacity);
        size_t to_write = std::min(frames, available);
        for (size_t i = 0; i < to_write; ++i) {
            buffer[(write_pos + i) % capacity] = data[i];
        }
        write_pos = (write_pos + to_write) % capacity;
        return to_write;
    }

    size_t AudioRingBuffer::read(float* data, size_t frames) {
        std::lock_guard<std::mutex> lock(mutex);
        size_t available = (write_pos - read_pos) % capacity;
        size_t to_read = std::min(frames, available);
        for (size_t i = 0; i < to_read; ++i) {
            data[i] = buffer[(read_pos + i) % capacity];
        }
        read_pos = (read_pos + to_read) % capacity;
        return to_read;
    }

    void AudioRingBuffer::reset() {
        std::lock_guard<std::mutex> lock(mutex);
        write_pos = read_pos = 0;
    }

    size_t AudioRingBuffer::available() const {
        std::lock_guard<std::mutex> lock(mutex);
        return (write_pos - read_pos) % capacity;
    }

    size_t AudioRingBuffer::free_space() const {
        std::lock_guard<std::mutex> lock(mutex);
        return capacity - ((write_pos - read_pos) % capacity);
    }

    StreamContext::StreamContext(EntityID id)
        : entity_id(id), audio_ring(44100 * 2 * 2) {
    }

    StreamContext::~StreamContext() {
        stop_threads();
        if (fmt_ctx) avformat_close_input(&fmt_ctx);
        if (video_codec_ctx) avcodec_free_context(&video_codec_ctx);
        if (audio_codec_ctx) avcodec_free_context(&audio_codec_ctx);
        if (sws_ctx) sws_freeContext(sws_ctx);
        if (swr_ctx) swr_free(&swr_ctx);
    }

    void StreamContext::start_threads() {
        demux_thread = std::thread(AVStreamingSystem::demux_thread_func, this);
        video_decoder_thread = std::thread(AVStreamingSystem::video_decoder_thread_func, this);
        audio_decoder_thread = std::thread(AVStreamingSystem::audio_decoder_thread_func, this);
    }

    void StreamContext::stop_threads() {
        video_pkt_queue.abort();
        audio_pkt_queue.abort();
        video_frame_queue.abort();

        if (demux_thread.joinable()) demux_thread.join();
        if (video_decoder_thread.joinable()) video_decoder_thread.join();
        if (audio_decoder_thread.joinable()) audio_decoder_thread.join();

        flush_queues();
    }

    void StreamContext::flush_queues() {
        video_pkt_queue.flush();
        audio_pkt_queue.flush();
        video_frame_queue.flush();
        audio_ring.reset();
        if (video_codec_ctx) avcodec_flush_buffers(video_codec_ctx);
        if (audio_codec_ctx) avcodec_flush_buffers(audio_codec_ctx);
    }

    void StreamContext::seek(double time) {
        std::lock_guard<std::mutex> lock(state_mutex);
        is_seeking = true;
        flush_queues();

        int stream_idx = video_stream_idx >= 0 ? video_stream_idx : audio_stream_idx;
        if (stream_idx < 0) return;

        int64_t seek_target = av_rescale_q(static_cast<int64_t>(time * AV_TIME_BASE),
            AVRational{ 1, AV_TIME_BASE },
            fmt_ctx->streams[stream_idx]->time_base);

        if (video_stream_idx >= 0) {
            av_seek_frame(fmt_ctx, video_stream_idx, seek_target, AVSEEK_FLAG_BACKWARD);
        }
        else if (audio_stream_idx >= 0) {
            av_seek_frame(fmt_ctx, audio_stream_idx, seek_target, AVSEEK_FLAG_BACKWARD);
        }

        if (video_codec_ctx) avcodec_flush_buffers(video_codec_ctx);
        if (audio_codec_ctx) avcodec_flush_buffers(audio_codec_ctx);

        is_seeking = false;
        state_cv.notify_all();
    }

    AVStreamingSystem::AVStreamingSystem(EntityManager& entityMgr)
        : BaseSystem(entityMgr) {
        sysName = "AVStreamingSystem";
    }

    AVStreamingSystem::~AVStreamingSystem() {
        Destroy();
    }

    void AVStreamingSystem::Start() {
        if (!init_audio()) {
            ANI_LOG_WARN("[AVStreamingSystem] Failed to initialize audio output.");
        }
    }

    void AVStreamingSystem::Update(float deltaT) {
        process_commands();

        std::lock_guard<std::mutex> lock(contexts_mutex);
        for (auto& pair : contexts) {
            auto& ctx = *pair.second;
            if (ctx.is_playing && !ctx.is_paused) {
                process_video_frames(ctx);
            }
        }
    }

    void AVStreamingSystem::Destroy() {
        shutdown_audio();
        std::lock_guard<std::mutex> lock(contexts_mutex);
        for (auto& pair : contexts) {
            pair.second->stop_threads();
        }
        contexts.clear();
    }

    void AVStreamingSystem::ClearCache(EntityID entity) {
        clean_context(entity);
    }

    void AVStreamingSystem::Load(EntityID entity, const std::string& filePath) {
        std::lock_guard<std::mutex> lock(command_mutex);
        command_queue.push({ CommandType::Load, entity, filePath, false, 0.0, 1.0f, 1.0f });
    }

    void AVStreamingSystem::Play(EntityID entity, bool loop) {
        std::lock_guard<std::mutex> lock(command_mutex);
        command_queue.push({ CommandType::Play, entity, "", loop, 0.0, 1.0f, 1.0f });
    }

    void AVStreamingSystem::Pause(EntityID entity) {
        std::lock_guard<std::mutex> lock(command_mutex);
        command_queue.push({ CommandType::Pause, entity, "", false, 0.0, 1.0f, 1.0f });
    }

    void AVStreamingSystem::Stop(EntityID entity) {
        std::lock_guard<std::mutex> lock(command_mutex);
        command_queue.push({ CommandType::Stop, entity, "", false, 0.0, 1.0f, 1.0f });
    }

    void AVStreamingSystem::Seek(EntityID entity, double time) {
        std::lock_guard<std::mutex> lock(command_mutex);
        command_queue.push({ CommandType::Seek, entity, "", false, time, 1.0f, 1.0f });
    }

    void AVStreamingSystem::SetSpeed(EntityID entity, float speed) {
        std::lock_guard<std::mutex> lock(command_mutex);
        command_queue.push({ CommandType::SetSpeed, entity, "", false, 0.0, speed, 1.0f });
    }

    void AVStreamingSystem::SetVolume(EntityID entity, float volume) {
        std::lock_guard<std::mutex> lock(command_mutex);
        command_queue.push({ CommandType::SetVolume, entity, "", false, 0.0, 1.0f, volume });
    }

    void AVStreamingSystem::Remove(EntityID entity) {
        std::lock_guard<std::mutex> lock(command_mutex);
        command_queue.push({ CommandType::Remove, entity, "", false, 0.0, 1.0f, 1.0f });
    }

    bool AVStreamingSystem::IsPlaying(EntityID entity) const {
        std::lock_guard<std::mutex> lock(contexts_mutex);
        auto it = contexts.find(entity);
        if (it != contexts.end() && it->second)
            return it->second->is_playing && !it->second->is_paused;
        return false;
    }

    bool AVStreamingSystem::IsPaused(EntityID entity) const {
        std::lock_guard<std::mutex> lock(contexts_mutex);
        auto it = contexts.find(entity);
        if (it != contexts.end() && it->second)
            return it->second->is_paused;
        return false;
    }

    double AVStreamingSystem::GetPosition(EntityID entity) const {
        std::lock_guard<std::mutex> lock(contexts_mutex);
        auto it = contexts.find(entity);
        if (it != contexts.end() && it->second)
            return it->second->current_time;
        return 0.0;
    }

    double AVStreamingSystem::GetDuration(EntityID entity) const {
        std::lock_guard<std::mutex> lock(contexts_mutex);
        auto it = contexts.find(entity);
        if (it != contexts.end() && it->second)
            return it->second->duration;
        return 0.0;
    }

    void AVStreamingSystem::SetTextureCallback(std::function<void(EntityID, unsigned char*, int, int, int, GLuint*)> cb) {
        m_textureCallback = cb;
    }

    void AVStreamingSystem::process_commands() {
        std::queue<Command> local_queue;
        {
            std::lock_guard<std::mutex> lock(command_mutex);
            std::swap(local_queue, command_queue);
        }

        while (!local_queue.empty()) {
            Command cmd = local_queue.front();
            local_queue.pop();

            switch (cmd.type) {
            case CommandType::Load: {
                clean_context(cmd.entity);
                auto ctx = std::make_unique<StreamContext>(cmd.entity);
                ctx->texture_callback = m_textureCallback;

                if (avformat_open_input(&ctx->fmt_ctx, cmd.filePath.c_str(), nullptr, nullptr) < 0) {
                    ANI_LOG_WARN("[AVStreaming] Failed to open %s", cmd.filePath.c_str());
                    break;
                }
                if (avformat_find_stream_info(ctx->fmt_ctx, nullptr) < 0) {
                    ANI_LOG_WARN("[AVStreaming] Failed to find stream info");
                    avformat_close_input(&ctx->fmt_ctx);
                    break;
                }

                for (unsigned i = 0; i < ctx->fmt_ctx->nb_streams; ++i) {
                    AVCodecParameters* par = ctx->fmt_ctx->streams[i]->codecpar;
                    if (par->codec_type == AVMEDIA_TYPE_VIDEO && ctx->video_stream_idx == -1) {
                        ctx->video_stream_idx = i;
                        ctx->video_time_base = ctx->fmt_ctx->streams[i]->time_base;
                        const AVCodec* codec = avcodec_find_decoder(par->codec_id);
                        if (codec) {
                            ctx->video_codec_ctx = avcodec_alloc_context3(codec);
                            avcodec_parameters_to_context(ctx->video_codec_ctx, par);
                            avcodec_open2(ctx->video_codec_ctx, codec, nullptr);
                            ctx->video_width = par->width;
                            ctx->video_height = par->height;
                            ctx->fps = av_q2d(ctx->fmt_ctx->streams[i]->avg_frame_rate);
                            if (ctx->fps <= 0) ctx->fps = 30.0;
                        }
                    }
                    else if (par->codec_type == AVMEDIA_TYPE_AUDIO && ctx->audio_stream_idx == -1) {
                        ctx->audio_stream_idx = i;
                        ctx->audio_time_base = ctx->fmt_ctx->streams[i]->time_base;
                        const AVCodec* codec = avcodec_find_decoder(par->codec_id);
                        if (codec) {
                            ctx->audio_codec_ctx = avcodec_alloc_context3(codec);
                            avcodec_parameters_to_context(ctx->audio_codec_ctx, par);
                            avcodec_open2(ctx->audio_codec_ctx, codec, nullptr);
                            ctx->audio_channels = par->ch_layout.nb_channels;
                            ctx->audio_sample_rate = par->sample_rate;

                            AVChannelLayout out_layout;
                            av_channel_layout_default(&out_layout, 2);
                            ctx->swr_ctx = swr_alloc();
                            av_opt_set_chlayout(ctx->swr_ctx, "in_chlayout", &par->ch_layout, 0);
                            av_opt_set_chlayout(ctx->swr_ctx, "out_chlayout", &out_layout, 0);
                            av_opt_set_int(ctx->swr_ctx, "in_sample_rate", par->sample_rate, 0);
                            av_opt_set_int(ctx->swr_ctx, "out_sample_rate", 44100, 0);
                            av_opt_set_sample_fmt(ctx->swr_ctx, "in_sample_fmt", static_cast<AVSampleFormat>(par->format), 0);
                            av_opt_set_sample_fmt(ctx->swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);
                            swr_init(ctx->swr_ctx);
                        }
                    }
                }

                if (ctx->video_stream_idx == -1 && ctx->audio_stream_idx == -1) {
                    ANI_LOG_WARN("[AVStreaming] No video or audio stream found.");
                    break;
                }

                if (ctx->video_codec_ctx) {
                    ctx->sws_ctx = sws_getContext(ctx->video_width, ctx->video_height,
                        ctx->video_codec_ctx->pix_fmt,
                        ctx->video_width, ctx->video_height,
                        AV_PIX_FMT_RGBA,
                        SWS_BILINEAR, nullptr, nullptr, nullptr);
                }

                ctx->duration = (double)ctx->fmt_ctx->duration / AV_TIME_BASE;
                ctx->start_threads();
                ctx->is_playing = true;
                ctx->is_paused = false;

                std::lock_guard<std::mutex> ctx_lock(contexts_mutex);
                contexts[cmd.entity] = std::move(ctx);
                break;
            }

            case CommandType::Play: {
                std::lock_guard<std::mutex> ctx_lock(contexts_mutex);
                auto it = contexts.find(cmd.entity);
                if (it != contexts.end() && it->second) {
                    it->second->is_playing = true;
                    it->second->is_paused = false;
                    it->second->looping = cmd.loop;
                }
                break;
            }

            case CommandType::Pause: {
                std::lock_guard<std::mutex> ctx_lock(contexts_mutex);
                auto it = contexts.find(cmd.entity);
                if (it != contexts.end() && it->second) {
                    it->second->is_paused = true;
                }
                break;
            }

            case CommandType::Stop: {
                std::lock_guard<std::mutex> ctx_lock(contexts_mutex);
                auto it = contexts.find(cmd.entity);
                if (it != contexts.end() && it->second) {
                    it->second->is_playing = false;
                    it->second->is_paused = false;
                    it->second->flush_queues();
                    it->second->current_time = 0.0;
                }
                break;
            }

            case CommandType::Seek: {
                std::lock_guard<std::mutex> ctx_lock(contexts_mutex);
                auto it = contexts.find(cmd.entity);
                if (it != contexts.end() && it->second) {
                    it->second->seek(cmd.seek_time);
                }
                break;
            }

            case CommandType::SetSpeed: {
                std::lock_guard<std::mutex> ctx_lock(contexts_mutex);
                auto it = contexts.find(cmd.entity);
                if (it != contexts.end() && it->second) {
                    it->second->speed = cmd.speed;
                }
                break;
            }

            case CommandType::SetVolume: {
                std::lock_guard<std::mutex> ctx_lock(contexts_mutex);
                auto it = contexts.find(cmd.entity);
                if (it != contexts.end() && it->second) {
                    it->second->volume = cmd.volume;
                }
                break;
            }

            case CommandType::Remove: {
                clean_context(cmd.entity);
                break;
            }

            default:
                break;
            }
        }
    }

    void AVStreamingSystem::clean_context(EntityID entity) {
        std::lock_guard<std::mutex> lock(contexts_mutex);
        auto it = contexts.find(entity);
        if (it != contexts.end()) {
            it->second->stop_threads();
            contexts.erase(it);
        }
    }

    void AVStreamingSystem::process_video_frames(StreamContext& ctx) {
        AVFrame* frame = nullptr;
        if (ctx.video_frame_queue.pop(frame, 0)) {
            send_video_frame(ctx, frame);
            av_frame_free(&frame);
        }
    }

    void AVStreamingSystem::send_video_frame(StreamContext& ctx, AVFrame* frame) {
        if (!ctx.texture_callback) return;
        int w = frame->width, h = frame->height;
        size_t dataSize = w * h * 4;
        unsigned char* rgba = (unsigned char*)malloc(dataSize);
        if (!rgba) return;
        uint8_t* dst[1] = { rgba };
        int dst_linesize[1] = { w * 4 };
        sws_scale(ctx.sws_ctx, frame->data, frame->linesize, 0, h, dst, dst_linesize);
        ctx.texture_callback(ctx.entity_id, rgba, w, h, 4, nullptr);
    }

    void AVStreamingSystem::demux_thread_func(StreamContext* ctx) {
        AVPacket* pkt = av_packet_alloc();
        while (!ctx->video_pkt_queue.is_aborted() && !ctx->audio_pkt_queue.is_aborted()) {
            int ret = av_read_frame(ctx->fmt_ctx, pkt);
            if (ret < 0) {
                if (ret == AVERROR_EOF) {
                    ctx->eof = true;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            if (pkt->stream_index == ctx->video_stream_idx) {
                ctx->video_pkt_queue.push(av_packet_clone(pkt));
            }
            else if (pkt->stream_index == ctx->audio_stream_idx) {
                ctx->audio_pkt_queue.push(av_packet_clone(pkt));
            }
            av_packet_unref(pkt);
        }
        av_packet_free(&pkt);
    }

    void AVStreamingSystem::video_decoder_thread_func(StreamContext* ctx) {
        AVPacket* pkt = nullptr;
        AVFrame* frame = av_frame_alloc();
        while (!ctx->video_pkt_queue.is_aborted()) {
            if (!ctx->video_pkt_queue.pop(pkt, 100)) {
                if (ctx->eof) break;
                continue;
            }
            if (avcodec_send_packet(ctx->video_codec_ctx, pkt) == 0) {
                while (avcodec_receive_frame(ctx->video_codec_ctx, frame) == 0) {
                    ctx->video_frame_queue.push(av_frame_clone(frame));
                    if (frame->pts != AV_NOPTS_VALUE) {
                        ctx->current_time = av_q2d(ctx->video_time_base) * frame->pts;
                    }
                }
            }
            av_packet_free(&pkt);
        }
        av_frame_free(&frame);
    }

    void AVStreamingSystem::audio_decoder_thread_func(StreamContext* ctx) {
        AVPacket* pkt = nullptr;
        AVFrame* frame = av_frame_alloc();
        uint8_t* out_buffer = nullptr;
        int max_out_samples = 1024;

        while (!ctx->audio_pkt_queue.is_aborted()) {
            if (!ctx->audio_pkt_queue.pop(pkt, 100)) {
                if (ctx->eof) break;
                continue;
            }
            if (avcodec_send_packet(ctx->audio_codec_ctx, pkt) == 0) {
                while (avcodec_receive_frame(ctx->audio_codec_ctx, frame) == 0) {
                    int out_samples = swr_convert(ctx->swr_ctx, &out_buffer, max_out_samples,
                        (const uint8_t**)frame->data, frame->nb_samples);
                    if (out_samples > 0) {
                        int channels = 2;
                        std::vector<float> interleaved(out_samples * channels);
                        float* planar[2] = { (float*)out_buffer, (float*)(out_buffer + out_samples * sizeof(float)) };
                        for (int i = 0; i < out_samples; ++i) {
                            for (int c = 0; c < channels; ++c) {
                                interleaved[i * channels + c] = planar[c][i] * ctx->volume;
                            }
                        }
                        ctx->audio_ring.write(interleaved.data(), interleaved.size());
                    }
                }
            }
            av_packet_free(&pkt);
        }
        av_frame_free(&frame);
        if (out_buffer) av_free(out_buffer);
    }

    int AVStreamingSystem::pa_callback(const void* input, void* output,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData) {
        AVStreamingSystem* self = static_cast<AVStreamingSystem*>(userData);
        float* out = (float*)output;
        size_t samples = framesPerBuffer * 2;
        std::vector<float> mix(samples, 0.0f);

        std::lock_guard<std::mutex> lock(self->contexts_mutex);
        for (auto& pair : self->contexts) {
            auto& ctx = *pair.second;
            if (ctx.is_playing && !ctx.is_paused) {
                std::vector<float> temp(samples);
                size_t read = ctx.audio_ring.read(temp.data(), samples);
                if (read > 0) {
                    for (size_t i = 0; i < read; ++i) {
                        mix[i] += temp[i];
                    }
                }
            }
        }

        for (size_t i = 0; i < samples; ++i) {
            out[i] = std::clamp(mix[i], -1.0f, 1.0f);
        }
        return paContinue;
    }

    bool AVStreamingSystem::init_audio() {
        if (audio_stream) return true;
        PaError err = Pa_Initialize();
        if (err != paNoError) {
            ANI_LOG_WARN("[AVStreaming] PortAudio init failed: %s", Pa_GetErrorText(err));
            return false;
        }
        PaStreamParameters outputParams;
        outputParams.device = Pa_GetDefaultOutputDevice();
        if (outputParams.device == paNoDevice) {
            ANI_LOG_WARN("[AVStreaming] No audio output device");
            return false;
        }
        outputParams.channelCount = 2;
        outputParams.sampleFormat = paFloat32;
        outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
        outputParams.hostApiSpecificStreamInfo = nullptr;

        err = Pa_OpenStream(&audio_stream, nullptr, &outputParams,
            44100, 256, paNoFlag, pa_callback, this);
        if (err != paNoError) {
            ANI_LOG_WARN("[AVStreaming] Failed to open audio stream: %s", Pa_GetErrorText(err));
            return false;
        }
        err = Pa_StartStream(audio_stream);
        if (err != paNoError) {
            ANI_LOG_WARN("[AVStreaming] Failed to start audio stream: %s", Pa_GetErrorText(err));
            Pa_CloseStream(audio_stream);
            audio_stream = nullptr;
            return false;
        }
        audio_running = true;
        return true;
    }

    void AVStreamingSystem::shutdown_audio() {
        if (audio_stream) {
            Pa_StopStream(audio_stream);
            Pa_CloseStream(audio_stream);
            audio_stream = nullptr;
            audio_running = false;
        }
        Pa_Terminate();
    }

}