#include "AudioSystem.hpp"
#include "Log.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace ECS {

    AudioSystem::Track::Track(AudioSystem* o, EntityID e)
        : entity(e), owner(o) {
    }

    AudioSystem::Track::~Track() {
        running.store(false);
        ringCV.notify_all();
        if (producer.joinable()) producer.join();
    }

    void AudioSystem::Track::StartProducerThread() {
        ANI_LOG_DEBUG("[AudioSystem::Track] StartProducerThread entity=%u", entity);
        producer = std::thread([this]() {
            if (!owner->mgr.IsEntityValid(entity) ||
                !owner->mgr.HasComponent<AudioComponent>(entity)) return;
            auto& ac = owner->mgr.GetComponent<AudioComponent>(entity);
            if (!ac.fmtCtx || !ac.codecCtx || !ac.swrCtx || ac.audioStreamIndex < 0) return;

            AVFormatContext* fmt = ac.fmtCtx.get();
            AVCodecContext* codec = ac.codecCtx.get();
            SwrContext* swr = ac.swrCtx.get();
            const int streamIndex = ac.audioStreamIndex;
            const int outCh = channels;
            const int outRate = sampleRate;

            AVFrame* frame = av_frame_alloc();
            AVPacket* pkt = av_packet_alloc();
            if (!frame || !pkt) {
                if (frame) av_frame_free(&frame);
                if (pkt) av_packet_free(&pkt);
                return;
            }

            const size_t ringTarget =
                static_cast<size_t>(outRate) * static_cast<size_t>(outCh);

            while (running.load()) {
                // Handle a pending seek before doing anything else.
                if (seekRequested.load()) {
                    double target = seekTarget.load();
                    seekRequested.store(false);

                    ANI_LOG_DEBUG("[AudioSystem::Track] Producer seek entity=%u time=%.3f",
                        entity, target);

                    AVStream* stream = fmt->streams[streamIndex];
                    int64_t ts = static_cast<int64_t>(target / av_q2d(stream->time_base));

                    if (avformat_seek_file(fmt, streamIndex,
                        INT64_MIN, ts, ts, AVSEEK_FLAG_BACKWARD) >= 0) {
                        avcodec_flush_buffers(codec);
                    }

                    // Drop everything currently in the ring. The consumer
                    // is about to read from the new position.
                    {
                        std::lock_guard<std::mutex> lock(ringMutex);
                        ringRead = 0;
                        ringWrite = 0;
                        ringAvailable = 0;
                        eofReached = false;
                    }
                    ringCV.notify_all();
                }

                // If the ring is nearly full, wait for the consumer.
                {
                    std::unique_lock<std::mutex> lock(ringMutex);
                    ringCV.wait_for(lock, std::chrono::milliseconds(20), [&] {
                        return ringAvailable < ringTarget || !running.load()
                            || seekRequested.load();
                        });
                }
                if (!running.load()) break;
                if (seekRequested.load()) continue;
                if (ringAvailable >= ringTarget) continue;

                int readRet = av_read_frame(fmt, pkt);
                if (readRet < 0) {
                    avcodec_send_packet(codec, nullptr);
                    while (avcodec_receive_frame(codec, frame) == 0) {
                        int maxOut = frame->nb_samples;
                        size_t bufFloats = static_cast<size_t>(maxOut) * outCh;
                        std::vector<float> tmp(bufFloats);
                        uint8_t* outPlanes[1] = { (uint8_t*)tmp.data() };
                        int conv = swr_convert(swr, outPlanes, maxOut,
                            (const uint8_t**)frame->data, frame->nb_samples);
                        if (conv > 0) {
                            std::lock_guard<std::mutex> lock(ringMutex);
                            for (int i = 0; i < conv * outCh; ++i) {
                                if (ringAvailable >= ringCapacitySamples) break;
                                ring[ringWrite] = tmp[i];
                                ringWrite = (ringWrite + 1) % ringCapacitySamples;
                                ++ringAvailable;
                            }
                            ringCV.notify_all();
                        }
                        av_frame_unref(frame);
                    }
                    eofReached = true;
                    {
                        std::unique_lock<std::mutex> lock(ringMutex);
                        ringCV.wait_for(lock, std::chrono::milliseconds(50), [&] {
                            return ringAvailable == 0 || !running.load()
                                || seekRequested.load();
                            });
                    }
                    if (ringAvailable == 0) break;
                    continue;
                }

                if (pkt->stream_index != streamIndex) {
                    av_packet_unref(pkt);
                    continue;
                }

                if (avcodec_send_packet(codec, pkt) == 0) {
                    while (avcodec_receive_frame(codec, frame) == 0) {
                        int maxOut = frame->nb_samples;
                        size_t bufFloats = static_cast<size_t>(maxOut) * outCh;
                        std::vector<float> tmp(bufFloats);
                        uint8_t* outPlanes[1] = { (uint8_t*)tmp.data() };
                        int conv = swr_convert(swr, outPlanes, maxOut,
                            (const uint8_t**)frame->data, frame->nb_samples);
                        if (conv > 0) {
                            std::lock_guard<std::mutex> lock(ringMutex);
                            for (int i = 0; i < conv * outCh; ++i) {
                                if (ringAvailable >= ringCapacitySamples) break;
                                ring[ringWrite] = tmp[i];
                                ringWrite = (ringWrite + 1) % ringCapacitySamples;
                                ++ringAvailable;
                            }
                            ringCV.notify_all();
                        }
                        av_frame_unref(frame);
                    }
                }
                av_packet_unref(pkt);
            }

            av_packet_free(&pkt);
            av_frame_free(&frame);
            });
    }

    AudioSystem::AudioSystem(EntityManager& entityMgr)
        : BaseSystem(entityMgr)
        , m_state(std::make_unique<StreamState>()) {
        sysName = "AudioSystem";
        AddComponentSignature<AudioComponent>();

        PaError err = Pa_Initialize();
        if (err != paNoError) {
            ANI_LOG_ERROR("[AudioSystem] Pa_Initialize failed: %s", Pa_GetErrorText(err));
        }
    }

    AudioSystem::~AudioSystem() {
        Destroy();
        Pa_Terminate();
    }

    void AudioSystem::Start() {
        OpenStream();
        for (auto entity : mgr.GetAllEntities()) {
            if (mgr.HasComponent<AudioComponent>(entity)) {
                auto& ac = mgr.GetComponent<AudioComponent>(entity);
                if (!ac.filePath.empty() && ac.pcmData.empty() && !ac.fmtCtx) {
                    PlaybackMode mode = PlaybackMode::Cached;
                    if (mgr.HasComponent<PlaybackStateComponent>(entity)) {
                        mode = mgr.GetComponent<PlaybackStateComponent>(entity).mode;
                    }
                    LoadAudioAsync(entity, ac.filePath, mode);
                }
            }
        }
    }

    void AudioSystem::OnEntityDestroyed(EntityID entity) {
        std::unique_ptr<Track> detached;
        {
            std::lock_guard<std::mutex> lock(m_state->mutex);
            auto it = m_state->tracks.find(entity);
            if (it != m_state->tracks.end()) {
                detached = std::move(it->second);
                m_state->tracks.erase(it);
            }
        }
        {
            std::lock_guard<std::mutex> lock(m_restoreMutex);
            m_pendingRestores.erase(entity);
        }
    }

    void AudioSystem::Update(float deltaT) {
        (void)deltaT;
        ProcessCompletedLoads();
    }

    void AudioSystem::Destroy() {
        m_destroying.store(true);

        CloseStream();

        {
            std::lock_guard<std::mutex> lock(m_loadMutex);
            m_pendingLoads.clear();
        }
        {
            std::lock_guard<std::mutex> lock(m_state->mutex);
            m_state->tracks.clear();
        }
        {
            std::lock_guard<std::mutex> lock(m_restoreMutex);
            m_pendingRestores.clear();
        }
    }

    bool AudioSystem::OpenStream() {
        if (m_state->streamOpen) return true;

        PaStreamParameters out{};
        out.device = Pa_GetDefaultOutputDevice();
        if (out.device == paNoDevice) {
            ANI_LOG_WARN("[AudioSystem] No output device available");
            return false;
        }
        out.channelCount = 2;
        out.sampleFormat = paFloat32;
        out.suggestedLatency = Pa_GetDeviceInfo(out.device)->defaultLowOutputLatency;
        out.hostApiSpecificStreamInfo = nullptr;

        PaError err = Pa_OpenStream(&m_state->stream, nullptr, &out,
            44100, 256, paNoFlag,
            &AudioSystem::PaCallback, m_state.get());
        if (err != paNoError) {
            ANI_LOG_WARN("[AudioSystem] Pa_OpenStream failed: %s", Pa_GetErrorText(err));
            return false;
        }

        m_state->streamOpen = true;
        m_state->running.store(true);

        err = Pa_StartStream(m_state->stream);
        if (err != paNoError) {
            ANI_LOG_WARN("[AudioSystem] Pa_StartStream failed: %s", Pa_GetErrorText(err));
            CloseStream();
            return false;
        }
        ANI_LOG_INFO("[AudioSystem] Stream opened");
        return true;
    }

    void AudioSystem::CloseStream() {
        m_state->running.store(false);
        if (m_state->stream) {
            Pa_StopStream(m_state->stream);
            Pa_CloseStream(m_state->stream);
            m_state->stream = nullptr;
            m_state->streamOpen = false;
        }
    }

    int AudioSystem::PaCallback(const void* input, void* output,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData) {
        (void)input; (void)timeInfo; (void)statusFlags;

        StreamState* state = static_cast<StreamState*>(userData);
        if (!state || !state->running.load()) return paComplete;

        float* out = static_cast<float*>(output);
        const size_t outSamples = framesPerBuffer * 2;
        std::fill(out, out + outSamples, 0.0f);

        std::lock_guard<std::mutex> lock(state->mutex);

        for (auto& [id, trackPtr] : state->tracks) {
            (void)id;
            Track& track = *trackPtr;
            if (track.stopped || track.paused) continue;

            const float spd = (track.speed > 0.0f) ? track.speed : 1.0f;

            if (track.silent) {
                double advance = static_cast<double>(framesPerBuffer) * spd;
                track.readPosition += static_cast<size_t>(advance * track.channels);
                if (track.readPosition >= track.totalSamples) {
                    if (track.loop) track.readPosition = 0;
                    else { track.endReached = true; continue; }
                }
                track.streamTime = static_cast<double>(track.readPosition) /
                    (track.sampleRate * track.channels);
                continue;
            }

            const int ch = track.channels;
            if (ch <= 0) continue;

            if (track.mode == PlaybackMode::Streaming) {
                std::lock_guard<std::mutex> ringLock(track.ringMutex);
                size_t availableFrames = track.ringAvailable / ch;
                size_t framesToRead = std::min<size_t>(framesPerBuffer, availableFrames);

                if (framesToRead > 0) {
                    for (size_t f = 0; f < framesToRead; ++f) {
                        for (int c = 0; c < ch; ++c) {
                            size_t src = (track.ringRead + f * ch + c) % track.ringCapacitySamples;
                            out[f * 2 + c] += track.ring[src] * track.volume;
                        }
                    }
                    track.ringRead = (track.ringRead + framesToRead * ch) % track.ringCapacitySamples;
                    track.ringAvailable -= framesToRead * ch;
                    track.readPosition += framesToRead * ch;
                    track.ringCV.notify_all();
                }

                if (track.ringAvailable == 0 && track.eofReached) {
                    if (track.loop) {
                        track.ringRead = track.ringWrite = 0;
                        track.ringAvailable = 0;
                        track.eofReached = false;
                    }
                    else {
                        track.endReached = true;
                    }
                }

                track.streamTime = static_cast<double>(track.readPosition) /
                    (track.sampleRate * ch);
                continue;
            }

            if (track.pcmData.empty()) continue;

            const size_t totalInterleaved = track.totalSamples * ch;

            if (track.readPosition >= totalInterleaved) {
                if (track.loop) track.readPosition = 0;
                else { track.endReached = true; continue; }
            }

            for (unsigned long f = 0; f < framesPerBuffer; ++f) {
                size_t srcIndex = track.readPosition +
                    static_cast<size_t>(static_cast<double>(f) * spd * ch);

                if (srcIndex >= totalInterleaved) {
                    if (track.loop) srcIndex %= totalInterleaved;
                    else break;
                }

                if (ch == 2) {
                    out[f * 2 + 0] += track.pcmData[srcIndex + 0] * track.volume;
                    out[f * 2 + 1] += track.pcmData[srcIndex + 1] * track.volume;
                }
                else {
                    float s = track.pcmData[srcIndex] * track.volume;
                    out[f * 2 + 0] += s;
                    out[f * 2 + 1] += s;
                }
            }

            double advance = static_cast<double>(framesPerBuffer) * spd * ch;
            size_t newPos = track.readPosition + static_cast<size_t>(advance);
            if (newPos >= totalInterleaved) {
                if (track.loop) {
                    newPos %= totalInterleaved;
                    track.readPosition = newPos;
                }
                else {
                    track.readPosition = totalInterleaved;
                    track.endReached = true;
                }
            }
            else {
                track.readPosition = newPos;
            }
            track.streamTime = static_cast<double>(track.readPosition) /
                (track.sampleRate * ch);
        }

        return paContinue;
    }

    void AudioSystem::LoadAudio(EntityID entity, const std::string& filePath, PlaybackMode mode) {
        if (!mgr.HasComponent<AudioComponent>(entity)) {
            ANI_LOG_WARN("[AudioSystem] Entity %u lacks AudioComponent", entity);
            return;
        }
        ANI_LOG_INFO("[AudioSystem] LoadAudio entity=%u mode=%d path=%s",
            entity, static_cast<int>(mode), filePath.c_str());

        auto& ac = mgr.GetComponent<AudioComponent>(entity);
        ac.UnloadAudio();
        ac.filePath = filePath;
        size_t slash = filePath.find_last_of("/\\");
        ac.fileName = (slash != std::string::npos) ? filePath.substr(slash + 1) : filePath;

        LoadAudioAsync(entity, filePath, mode);
    }

    void AudioSystem::AddLoadedAudio(EntityID entity) {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<AudioComponent>(entity)) return;
        auto& ac = mgr.GetComponent<AudioComponent>(entity);

        auto track = std::make_unique<Track>(this, entity);
        track->entity = entity;
        track->channels = ac.channels;
        track->sampleRate = ac.sampleRate;
        track->duration = ac.duration;
        track->totalSamples = ac.totalSamples > 0
            ? static_cast<size_t>(ac.totalSamples)
            : ac.pcmData.size() / std::max(1, ac.channels);
        track->readPosition = 0;
        track->paused = true;
        track->stopped = true;

        bool hasPcm = !ac.pcmData.empty();
        if (hasPcm) {
            track->mode = PlaybackMode::Cached;
            track->pcmData = ac.pcmData;
            ANI_LOG_DEBUG("[AudioSystem] AddLoadedAudio entity=%u mode=Cached samples=%zu",
                entity, track->pcmData.size());
        }
        else if (ac.HasDecoder()) {
            track->mode = PlaybackMode::Streaming;
            track->ringCapacitySamples =
                static_cast<size_t>(ac.sampleRate) * 4 * std::max(1, ac.channels);
            track->ring.assign(track->ringCapacitySamples, 0.0f);
            track->ringRead = 0;
            track->ringWrite = 0;
            track->ringAvailable = 0;
            track->eofReached = false;
            track->StartProducerThread();
            ANI_LOG_DEBUG("[AudioSystem] AddLoadedAudio entity=%u mode=Streaming ringCapacity=%zu",
                entity, track->ringCapacitySamples);
        }
        else {
            ANI_LOG_WARN("[AudioSystem] AddLoadedAudio entity=%u no pcm and no decoder", entity);
            return;
        }

        Track* rawTrack = track.get();
        {
            std::lock_guard<std::mutex> lock(m_state->mutex);
            m_state->tracks[entity] = std::move(track);
        }

        PendingRestore restore;
        bool hasRestore = false;
        {
            std::lock_guard<std::mutex> lock(m_restoreMutex);
            auto it = m_pendingRestores.find(entity);
            if (it != m_pendingRestores.end()) {
                restore = it->second;
                hasRestore = true;
                m_pendingRestores.erase(it);
            }
        }

        if (hasRestore && rawTrack) {
            if (rawTrack->duration > 0.0 && restore.keepTime > 0.0) {
                double ct = std::clamp(restore.keepTime, 0.0,
                    rawTrack->duration - 0.001);
                rawTrack->readPosition =
                    static_cast<size_t>(ct * rawTrack->sampleRate) *
                    std::max(1, rawTrack->channels);
                rawTrack->streamTime = ct;
                ANI_LOG_DEBUG("[AudioSystem] AddLoadedAudio entity=%u restored keepTime=%.3f",
                    entity, ct);
            }
            if (restore.wasPlaying) {
                rawTrack->paused = false;
                rawTrack->stopped = false;
                rawTrack->endReached = false;
                ANI_LOG_DEBUG("[AudioSystem] AddLoadedAudio entity=%u restored wasPlaying=true", entity);
            }
        }

        NotifyAudioAdded(entity);
        if (hasPcm) {
            NotifyAudioData(entity, ac.pcmData.data(), ac.pcmData.size(),
                ac.channels, ac.sampleRate);
        }
    }

    void AudioSystem::AddSilentTrack(EntityID entity, double duration) {
        if (duration <= 0.0) return;

        ANI_LOG_DEBUG("[AudioSystem] AddSilentTrack entity=%u duration=%.3f", entity, duration);

        auto track = std::make_unique<Track>(this, entity);
        track->entity = entity;
        track->silent = true;
        track->channels = 2;
        track->sampleRate = 44100;
        // readPosition is interleaved: advances by framesPerBuffer * channels
        // per callback, and streamTime = readPosition / (sampleRate * channels).
        // So the total must be in interleaved samples, not frames.
        track->totalSamples = static_cast<size_t>(
            duration * track->sampleRate * track->channels);
        track->duration = duration;
        track->readPosition = 0;
        track->paused = true;
        track->stopped = true;

        {
            std::lock_guard<std::mutex> lock(m_state->mutex);
            m_state->tracks[entity] = std::move(track);
        }
    }

    bool AudioSystem::HasTrack(EntityID entity) const {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        return m_state->tracks.find(entity) != m_state->tracks.end();
    }

    bool AudioSystem::IsSilentTrack(EntityID entity) const {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        auto it = m_state->tracks.find(entity);
        return it != m_state->tracks.end() && it->second->silent;
    }

    void AudioSystem::RemoveAudio(EntityID entity) {
        ANI_LOG_INFO("[AudioSystem] RemoveAudio entity=%u", entity);
        OnEntityDestroyed(entity);
        if (mgr.HasComponent<AudioComponent>(entity)) {
            mgr.GetComponent<AudioComponent>(entity).UnloadAudio();
        }
        NotifyAudioRemoved(entity);
    }

    void AudioSystem::ClearCache(EntityID entity) {
        ANI_LOG_DEBUG("[AudioSystem] ClearCache entity=%u", entity);
        {
            std::lock_guard<std::mutex> lock(m_loadMutex);
            m_pendingLoads.erase(
                std::remove_if(m_pendingLoads.begin(), m_pendingLoads.end(),
                    [entity](const LoadingTask& t) { return t.entityID == entity; }),
                m_pendingLoads.end());
        }
        OnEntityDestroyed(entity);
        if (mgr.IsEntityValid(entity) && mgr.HasComponent<AudioComponent>(entity)) {
            mgr.GetComponent<AudioComponent>(entity).UnloadAudio();
        }
    }

    void AudioSystem::SetMode(EntityID entity, PlaybackMode mode,
        double keepTime, bool wasPlaying) {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<AudioComponent>(entity)) return;

        ANI_LOG_INFO("[AudioSystem] SetMode entity=%u mode=%d keepTime=%.3f wasPlaying=%d",
            entity, static_cast<int>(mode), keepTime, wasPlaying ? 1 : 0);

        {
            std::lock_guard<std::mutex> lock(m_restoreMutex);
            PendingRestore pr;
            pr.keepTime = keepTime;
            pr.wasPlaying = wasPlaying;
            m_pendingRestores[entity] = pr;
        }

        OnEntityDestroyed(entity);

        auto& ac = mgr.GetComponent<AudioComponent>(entity);
        std::string path = ac.filePath;
        ac.UnloadAudio();

        if (!path.empty()) {
            LoadAudioAsync(entity, path, mode);
        }
    }

    void AudioSystem::LoadAudioAsync(EntityID entity, const std::string& filePath, PlaybackMode mode) {
        auto pool = mgr.GetSystem<ThreadPoolSystem>();
        if (!pool) {
            ANI_LOG_WARN("[AudioSystem] ThreadPoolSystem unavailable");
            return;
        }

        auto future = pool->getIOPool().submit([filePath, entity, mode]() -> LoadResult {
            if (mode == PlaybackMode::Streaming) {
                return OpenAudioStream(filePath, entity, 44100, 2);
            }
            return DecodeAudioFile(filePath, entity, 44100, 2);
            });

        std::lock_guard<std::mutex> lock(m_loadMutex);
        LoadingTask task;
        task.entityID = entity;
        task.filePath = filePath;
        task.mode = mode;
        task.future = std::move(future);
        m_pendingLoads.push_back(std::move(task));
    }

    void AudioSystem::ProcessCompletedLoads() {
        std::lock_guard<std::mutex> lock(m_loadMutex);

        for (auto it = m_pendingLoads.begin(); it != m_pendingLoads.end();) {
            if (it->future.valid() &&
                it->future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {

                PlaybackMode mode = it->mode;
                EntityID entityID = it->entityID;
                try {
                    LoadResult result = it->future.get();
                    if (mgr.HasComponent<AudioComponent>(result.entityID)) {
                        auto& ac = mgr.GetComponent<AudioComponent>(result.entityID);
                        if (result.success) {
                            ANI_LOG_DEBUG("[AudioSystem] Load complete entity=%u mode=%d hasPcm=%d hasDecoder=%d",
                                result.entityID, static_cast<int>(mode),
                                result.pcmData.empty() ? 0 : 1,
                                result.fmtCtx ? 1 : 0);

                            ac.pcmData = std::move(result.pcmData);
                            ac.channels = result.channels;
                            ac.sampleRate = result.sampleRate;
                            ac.totalSamples = result.totalSamples;
                            ac.duration = result.duration;
                            ac.fileName = result.fileName;
                            ac.filePath = result.filePath;
                            ac.hasExifData = result.hasExif;
                            ac.hasLSBData = result.hasLSB;
                            ac.hasAniStudioMetadata = result.hasAniStudio;

                            ac.fmtCtx.reset(result.fmtCtx);
                            ac.codecCtx.reset(result.codecCtx);
                            ac.swrCtx.reset(result.swrCtx);
                            ac.audioStreamIndex = result.audioStreamIndex;
                            result.fmtCtx = nullptr;
                            result.codecCtx = nullptr;
                            result.swrCtx = nullptr;
                            result.audioStreamIndex = -1;

                            if (mgr.HasComponent<PlaybackStateComponent>(result.entityID)) {
                                mgr.GetComponent<PlaybackStateComponent>(result.entityID).mode = mode;
                            }

                            AddLoadedAudio(result.entityID);
                        }
                        else {
                            ANI_LOG_WARN("[AudioSystem] Decode failed: %s", result.filePath.c_str());
                            std::lock_guard<std::mutex> rlock(m_restoreMutex);
                            m_pendingRestores.erase(result.entityID);
                        }
                    }
                    else {
                        std::lock_guard<std::mutex> rlock(m_restoreMutex);
                        m_pendingRestores.erase(result.entityID);
                    }
                }
                catch (const std::exception& e) {
                    ANI_LOG_ERROR("[AudioSystem] Load exception: %s", e.what());
                    std::lock_guard<std::mutex> rlock(m_restoreMutex);
                    m_pendingRestores.erase(entityID);
                }
                it = m_pendingLoads.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void AudioSystem::Play(EntityID entity, bool loop) {
        if (!m_state->streamOpen) OpenStream();
        std::lock_guard<std::mutex> lock(m_state->mutex);
        auto it = m_state->tracks.find(entity);
        if (it == m_state->tracks.end()) {
            ANI_LOG_DEBUG("[AudioSystem] Play entity=%u but no track", entity);
            return;
        }

        ANI_LOG_DEBUG("[AudioSystem] Play entity=%u loop=%d", entity, loop ? 1 : 0);

        Track& t = *it->second;
        t.paused = false;
        t.stopped = false;
        t.endReached = false;
        t.loop = loop;
        t.ringCV.notify_all();
    }

    void AudioSystem::Pause(EntityID entity) {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        auto it = m_state->tracks.find(entity);
        if (it != m_state->tracks.end()) {
            ANI_LOG_DEBUG("[AudioSystem] Pause entity=%u", entity);
            it->second->paused = true;
        }
    }

    void AudioSystem::Resume(EntityID entity) {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        auto it = m_state->tracks.find(entity);
        if (it != m_state->tracks.end()) {
            ANI_LOG_DEBUG("[AudioSystem] Resume entity=%u", entity);
            it->second->paused = false;
        }
    }

    void AudioSystem::Stop(EntityID entity) {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        auto it = m_state->tracks.find(entity);
        if (it == m_state->tracks.end()) return;
        ANI_LOG_DEBUG("[AudioSystem] Stop entity=%u", entity);
        Track& t = *it->second;
        t.stopped = true;
        t.paused = true;
        t.endReached = false;
        t.readPosition = 0;
        t.streamTime = 0.0;
        t.ringRead = 0;
        t.ringWrite = 0;
        t.ringAvailable = 0;
        t.eofReached = false;
    }

    void AudioSystem::Seek(EntityID entity, double time) {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        auto it = m_state->tracks.find(entity);
        if (it == m_state->tracks.end()) {
            ANI_LOG_DEBUG("[AudioSystem] Seek entity=%u but no track", entity);
            return;
        }

        ANI_LOG_DEBUG("[AudioSystem] Seek entity=%u time=%.3f mode=%d",
            entity, time, static_cast<int>(it->second->mode));

        Track& t = *it->second;
        if (t.duration <= 0.0) return;
        time = std::clamp(time, 0.0, t.duration - 0.001);

        // Update the consumer-visible position immediately.
        size_t pos = static_cast<size_t>(time * t.sampleRate) * std::max(1, t.channels);
        t.readPosition = pos;
        t.streamTime = time;
        t.endReached = false;
        t.ringRead = 0;
        t.ringWrite = 0;
        t.ringAvailable = 0;
        t.eofReached = false;

        // If the producer thread is decoding ahead, it must also seek its
        // own decoder and drop whatever it has queued. Without this, the
        // producer keeps pushing old-position samples into the ring and
        // the consumer plays stale audio.
        if (t.mode == PlaybackMode::Streaming && t.producer.joinable()) {
            t.seekTarget.store(time);
            t.seekRequested.store(true);
            t.ringCV.notify_all();
        }
    }

    void AudioSystem::SetVolume(EntityID entity, float volume) {
        volume = std::clamp(volume, 0.0f, 1.0f);
        std::lock_guard<std::mutex> lock(m_state->mutex);
        auto it = m_state->tracks.find(entity);
        if (it != m_state->tracks.end()) it->second->volume = volume;
    }

    void AudioSystem::SetSpeed(EntityID entity, float speed) {
        speed = std::clamp(speed, 0.1f, 4.0f);
        std::lock_guard<std::mutex> lock(m_state->mutex);
        auto it = m_state->tracks.find(entity);
        if (it != m_state->tracks.end()) it->second->speed = speed;
    }

    bool AudioSystem::IsPlaying(EntityID entity) const {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        auto it = m_state->tracks.find(entity);
        return it != m_state->tracks.end() && !it->second->paused && !it->second->stopped;
    }

    bool AudioSystem::IsPaused(EntityID entity) const {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        auto it = m_state->tracks.find(entity);
        return it != m_state->tracks.end() && it->second->paused && !it->second->stopped;
    }

    bool AudioSystem::IsLoading(EntityID entity) const {
        if (!mgr.IsEntityValid(entity)) return false;
        if (!mgr.HasComponent<PlaybackStateComponent>(entity)) return false;
        return !mgr.GetComponent<PlaybackStateComponent>(entity).isLoaded;
    }

    double AudioSystem::GetCurrentPosition(EntityID entity) const {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        auto it = m_state->tracks.find(entity);
        if (it == m_state->tracks.end()) return 0.0;
        return it->second->streamTime;
    }

    double AudioSystem::GetDuration(EntityID entity) const {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        auto it = m_state->tracks.find(entity);
        if (it == m_state->tracks.end()) return 0.0;
        return it->second->duration;
    }

    std::vector<EntityID> AudioSystem::GetAllAudioEntities() const {
        std::vector<EntityID> result;
        for (auto e : mgr.GetAllEntities()) {
            if (mgr.HasComponent<AudioComponent>(e)) result.push_back(e);
        }
        return result;
    }

    void AudioSystem::RegisterAudioAddedCallback(void* owner, const AudioCallback& cb) {
        m_addedCallbacks.emplace_back(owner, cb);
    }
    void AudioSystem::RegisterAudioRemovedCallback(void* owner, const AudioCallback& cb) {
        m_removedCallbacks.emplace_back(owner, cb);
    }
    void AudioSystem::RegisterAudioDataCallback(void* owner, const AudioDataCallback& cb) {
        m_dataCallbacks.emplace_back(owner, cb);
    }
    void AudioSystem::RegisterEndCallback(void* owner, const EndCallback& cb) {
        m_endCallbacks.emplace_back(owner, cb);
    }
    void AudioSystem::UnregisterCallbacksForOwner(void* owner) {
        auto drop = [owner](auto& vec) {
            vec.erase(std::remove_if(vec.begin(), vec.end(),
                [owner](const auto& p) { return p.first == owner; }), vec.end());
            };
        drop(m_addedCallbacks);
        drop(m_removedCallbacks);
        drop(m_dataCallbacks);
        drop(m_endCallbacks);
    }

    void AudioSystem::NotifyAudioAdded(EntityID entity) {
        for (auto& [o, cb] : m_addedCallbacks) { (void)o; try { cb(entity); } catch (...) {} }
    }
    void AudioSystem::NotifyAudioRemoved(EntityID entity) {
        for (auto& [o, cb] : m_removedCallbacks) { (void)o; try { cb(entity); } catch (...) {} }
    }
    void AudioSystem::NotifyAudioData(EntityID entity, const float* data, size_t size,
        int channels, int sampleRate) {
        for (auto& [o, cb] : m_dataCallbacks) {
            (void)o;
            try { cb(entity, data, size, channels, sampleRate); }
            catch (...) {}
        }
    }
    void AudioSystem::NotifyEnd(EntityID entity) {
        for (auto& [o, cb] : m_endCallbacks) { (void)o; try { cb(entity); } catch (...) {} }
    }

    void AudioSystem::PlayTestTone() {
        if (!m_state->streamOpen && !OpenStream()) return;

        constexpr int sr = 44100;
        constexpr int ch = 2;
        constexpr int dur = 2;
        constexpr float freq = 440.0f;
        constexpr float amp = 0.5f;

        EntityID e = 0;
        for (auto id : mgr.GetAllEntities()) {
            if (mgr.HasComponent<AudioComponent>(id)) { e = id; break; }
        }
        if (e == 0) {
            e = mgr.AddNewEntity();
            mgr.AddComponent<AudioComponent>(e);
        }
        auto& ac = mgr.GetComponent<AudioComponent>(e);
        ac.pcmData.resize(static_cast<size_t>(sr) * dur * ch);
        for (size_t i = 0; i < ac.pcmData.size(); i += ch) {
            float s = amp * std::sin(2.0f * 3.14159265f * freq *
                static_cast<float>(i / ch) / sr);
            ac.pcmData[i] = s;
            ac.pcmData[i + 1] = s;
        }
        ac.channels = ch;
        ac.sampleRate = sr;
        ac.duration = dur;
        ac.totalSamples = ac.pcmData.size() / ch;
        ac.fileName = "Test Tone";

        AddLoadedAudio(e);
        Play(e, false);
    }

    AudioSystem::LoadResult AudioSystem::OpenAudioStream(const std::string& filePath, EntityID entity,
        int targetSampleRate, int targetChannels) {
        LoadResult r;
        r.filePath = filePath;
        r.entityID = entity;
        size_t slash = filePath.find_last_of("/\\");
        r.fileName = (slash != std::string::npos) ? filePath.substr(slash + 1) : filePath;

        AVFormatContext* fmt = nullptr;
        if (avformat_open_input(&fmt, filePath.c_str(), nullptr, nullptr) < 0) return r;
        if (avformat_find_stream_info(fmt, nullptr) < 0) { avformat_close_input(&fmt); return r; }

        int audioStream = -1;
        for (unsigned i = 0; i < fmt->nb_streams; ++i) {
            if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) { audioStream = i; break; }
        }
        if (audioStream < 0) { avformat_close_input(&fmt); return r; }

        AVCodecParameters* par = fmt->streams[audioStream]->codecpar;
        const AVCodec* codec = avcodec_find_decoder(par->codec_id);
        if (!codec) { avformat_close_input(&fmt); return r; }

        AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
        if (!codecCtx) { avformat_close_input(&fmt); return r; }
        if (avcodec_parameters_to_context(codecCtx, par) < 0) {
            avcodec_free_context(&codecCtx); avformat_close_input(&fmt); return r;
        }
        if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
            avcodec_free_context(&codecCtx); avformat_close_input(&fmt); return r;
        }

        AVChannelLayout inLayout, outLayout;
        if (!av_channel_layout_check(&codecCtx->ch_layout)) {
            av_channel_layout_default(&codecCtx->ch_layout, par->ch_layout.nb_channels > 0
                ? par->ch_layout.nb_channels : 2);
        }
        av_channel_layout_copy(&inLayout, &codecCtx->ch_layout);
        av_channel_layout_default(&outLayout, targetChannels);

        SwrContext* swr = swr_alloc();
        av_opt_set_chlayout(swr, "in_chlayout", &inLayout, 0);
        av_opt_set_chlayout(swr, "out_chlayout", &outLayout, 0);
        av_opt_set_int(swr, "in_sample_rate", codecCtx->sample_rate, 0);
        av_opt_set_int(swr, "out_sample_rate", targetSampleRate, 0);
        av_opt_set_sample_fmt(swr, "in_sample_fmt", codecCtx->sample_fmt, 0);
        av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);
        if (swr_init(swr) < 0) {
            swr_free(&swr);
            avcodec_free_context(&codecCtx);
            avformat_close_input(&fmt);
            return r;
        }

        r.duration = (fmt->duration != AV_NOPTS_VALUE)
            ? static_cast<double>(fmt->duration) / AV_TIME_BASE : 0.0;
        r.channels = targetChannels;
        r.sampleRate = targetSampleRate;
        r.totalSamples = static_cast<int64_t>(r.duration * targetSampleRate);

        r.fmtCtx = fmt;
        r.codecCtx = codecCtx;
        r.swrCtx = swr;
        r.audioStreamIndex = audioStream;

        r.success = true;
        return r;
    }

    AudioSystem::LoadResult AudioSystem::DecodeAudioFile(const std::string& filePath, EntityID entity,
        int targetSampleRate, int targetChannels) {
        LoadResult r;
        r.filePath = filePath;
        r.entityID = entity;
        size_t slash = filePath.find_last_of("/\\");
        r.fileName = (slash != std::string::npos) ? filePath.substr(slash + 1) : filePath;

        AVFormatContext* fmt = nullptr;
        if (avformat_open_input(&fmt, filePath.c_str(), nullptr, nullptr) < 0) return r;
        if (avformat_find_stream_info(fmt, nullptr) < 0) { avformat_close_input(&fmt); return r; }

        int audioStream = -1;
        for (unsigned i = 0; i < fmt->nb_streams; ++i) {
            if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) { audioStream = i; break; }
        }
        if (audioStream < 0) { avformat_close_input(&fmt); return r; }

        AVCodecParameters* par = fmt->streams[audioStream]->codecpar;
        const AVCodec* codec = avcodec_find_decoder(par->codec_id);
        if (!codec) { avformat_close_input(&fmt); return r; }

        AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
        if (!codecCtx) { avformat_close_input(&fmt); return r; }
        if (avcodec_parameters_to_context(codecCtx, par) < 0) {
            avcodec_free_context(&codecCtx); avformat_close_input(&fmt); return r;
        }
        if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
            avcodec_free_context(&codecCtx); avformat_close_input(&fmt); return r;
        }

        AVChannelLayout inLayout, outLayout;
        if (!av_channel_layout_check(&codecCtx->ch_layout)) {
            av_channel_layout_default(&codecCtx->ch_layout, par->ch_layout.nb_channels > 0
                ? par->ch_layout.nb_channels : 2);
        }
        av_channel_layout_copy(&inLayout, &codecCtx->ch_layout);
        av_channel_layout_default(&outLayout, targetChannels);

        SwrContext* swr = swr_alloc();
        av_opt_set_chlayout(swr, "in_chlayout", &inLayout, 0);
        av_opt_set_chlayout(swr, "out_chlayout", &outLayout, 0);
        av_opt_set_int(swr, "in_sample_rate", codecCtx->sample_rate, 0);
        av_opt_set_int(swr, "out_sample_rate", targetSampleRate, 0);
        av_opt_set_sample_fmt(swr, "in_sample_fmt", codecCtx->sample_fmt, 0);
        av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);
        if (swr_init(swr) < 0) {
            swr_free(&swr); avcodec_free_context(&codecCtx); avformat_close_input(&fmt); return r;
        }

        std::vector<float> pcm;
        AVFrame* frame = av_frame_alloc();
        AVPacket* pkt = av_packet_alloc();

        while (av_read_frame(fmt, pkt) >= 0) {
            if (pkt->stream_index == audioStream && avcodec_send_packet(codecCtx, pkt) == 0) {
                while (avcodec_receive_frame(codecCtx, frame) == 0) {
                    int maxOut = frame->nb_samples;
                    uint8_t* buf = (uint8_t*)av_malloc(
                        static_cast<size_t>(maxOut) * targetChannels * sizeof(float));
                    if (buf) {
                        uint8_t* outPlanes[1] = { buf };
                        int conv = swr_convert(swr, outPlanes, maxOut,
                            (const uint8_t**)frame->data, frame->nb_samples);
                        if (conv > 0) {
                            float* f = (float*)buf;
                            size_t old = pcm.size();
                            pcm.resize(old + static_cast<size_t>(conv) * targetChannels);
                            std::memcpy(pcm.data() + old, f,
                                static_cast<size_t>(conv) * targetChannels * sizeof(float));
                        }
                        av_free(buf);
                    }
                    av_frame_unref(frame);
                }
            }
            av_packet_unref(pkt);
        }

        {
            int maxOut = 8192;
            uint8_t* buf = (uint8_t*)av_malloc(
                static_cast<size_t>(maxOut) * targetChannels * sizeof(float));
            if (buf) {
                uint8_t* outPlanes[1] = { buf };
                int conv = swr_convert(swr, outPlanes, maxOut, nullptr, 0);
                if (conv > 0) {
                    float* f = (float*)buf;
                    size_t old = pcm.size();
                    pcm.resize(old + static_cast<size_t>(conv) * targetChannels);
                    std::memcpy(pcm.data() + old, f,
                        static_cast<size_t>(conv) * targetChannels * sizeof(float));
                }
                av_free(buf);
            }
        }

        av_packet_free(&pkt);
        av_frame_free(&frame);
        swr_free(&swr);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmt);

        if (pcm.empty()) return r;

        r.success = true;
        r.pcmData = std::move(pcm);
        r.channels = targetChannels;
        r.sampleRate = targetSampleRate;
        r.totalSamples = r.pcmData.size() / targetChannels;
        r.duration = static_cast<double>(r.totalSamples) / targetSampleRate;
        return r;
    }

    AudioSystem::LoadResult AudioSystem::ExtractAudioFromVideoFile(const std::string& filePath,
        EntityID entity) {
        return DecodeAudioFile(filePath, entity, 44100, 2);
    }

} // namespace ECS