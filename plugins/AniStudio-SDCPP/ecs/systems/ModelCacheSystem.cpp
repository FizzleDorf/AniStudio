// SDCPPSystem.cpp
#include "SDCPPSystem.hpp"
#include "rng.hpp"
#include "Log.hpp"

#include <stb_image.h>
#include <stb_image_write.h>
#include "VideoUtils.hpp"
#include "VideoMetadataUtils.hpp"

#include <algorithm>
#include <filesystem>

namespace ECS {

    void SDCPPSystem::TaskData::Cancel() {
        cancelled = true;
        cancelTime = std::chrono::steady_clock::now();
        if (ctxHandle && ctxHandle->get())
            sd_cancel_generation(ctxHandle->get(), SD_CANCEL_ALL);
    }

    SDCPPSystem::SDCPPSystem(EntityManager& entityMgr)
        : BaseSystem(entityMgr), pauseWorker(false), hasActiveTask(false), clearRequested(false) {
        sysName = "SDCPPSystem";
        m_filePathSystem = mgr.GetSystem<FilePathSystem>();
        ANI_LOG_DEBUG("Constructed");
    }

    SDCPPSystem::~SDCPPSystem() {
        ANI_LOG_DEBUG("Destructor");
        Shutdown();
    }

    void SDCPPSystem::Shutdown() {
        ANI_LOG_INFO("Shutting down SDCPPSystem");

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            shuttingDown = true;
            pauseWorker = true;
        }
        StopCurrentTask();
        if (workerThread.joinable()) workerThread.join();
        if (m_threadPool) {
            m_threadPool->terminateAll();
            m_threadPool.reset();
        }
        std::lock_guard<std::mutex> lock(queueMutex);
        taskQueue.clear();

        ANI_LOG_INFO("SDCPPSystem shutdown complete");
    }

    void SDCPPSystem::TerminateImmediately() {
        ANI_LOG_INFO("Terminating immediately");

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            shuttingDown = true;
            pauseWorker = true;
        }
        ClearAllTasks();
        if (m_threadPool) m_threadPool->terminateAll();
    }

    void SDCPPSystem::Start() {
        ANI_LOG_DEBUG("Starting SDCPPSystem");

        m_cacheSystem = mgr.GetSystem<ModelCacheSystem>();
        m_threadPool = mgr.GetSystem<ThreadPoolSystem>();
        if (!m_threadPool) {
            ANI_LOG_WARN("ThreadPoolSystem not available");
        }
        workerThread = std::thread([this]() { WorkerThread(); });
        ANI_LOG_DEBUG("Worker thread started");
    }

    void SDCPPSystem::Destroy() {
        ANI_LOG_DEBUG("Destroying SDCPPSystem");
        Shutdown();
        BaseSystem::Destroy();
    }

    // ---------------------------------------------------------------------
    // Queue
    // ---------------------------------------------------------------------
    void SDCPPSystem::QueueTask(EntityID entityID, TaskType taskType) {
        if (!mgr.IsEntityValid(entityID)) {
            ANI_LOG_WARN("QueueTask: invalid entity %u", entityID);
            return;
        }

        auto settingsSys = mgr.GetSystem<SettingsSystem>();
        if (settingsSys) {
            EntityID settingsEntity = settingsSys->GetSettingsEntity();
            if (mgr.IsEntityValid(settingsEntity) &&
                mgr.HasComponent<SDCPPSettingsComponent>(settingsEntity)) {
                auto& globalSettings = mgr.GetComponent<SDCPPSettingsComponent>(settingsEntity);
                if (!mgr.HasComponent<SDCPPSettingsComponent>(entityID))
                    mgr.AddComponent<SDCPPSettingsComponent>(entityID);
                auto& taskSettings = mgr.GetComponent<SDCPPSettingsComponent>(entityID);
                taskSettings.Deserialize(globalSettings.Serialize());
            }
        }

        if (taskType == TaskType::Inference || taskType == TaskType::Img2Img ||
            taskType == TaskType::Img2Vid || taskType == TaskType::Edit) {
            if (mgr.HasComponent<SamplerComponent>(entityID)) {
                auto& sampler = mgr.GetComponent<SamplerComponent>(entityID);
                if (sampler.seed < 0) {
                    sampler.seed = (int64_t)STDDefaultRNG::generate_seed();
                    if (sampler.seed == 0) sampler.seed = 31337;
                    ANI_LOG_TRACE("Auto-generated seed %lld for entity %u",
                        (long long)sampler.seed, entityID);
                }
            }
        }

        TaskData task;
        task.entityID = entityID;
        task.taskType = taskType;
        task.enqueueTime = std::chrono::steady_clock::now();
        task.genRes = std::make_shared<SDCPP::ResourceManager>();

        switch (taskType) {
        case TaskType::Inference:
        case TaskType::Img2Img:
        case TaskType::Edit:
            SDCPP::FillImageParams(mgr, entityID, task.imgParams, *task.genRes);
            break;
        case TaskType::Img2Vid:
            SDCPP::FillVideoParams(mgr, entityID, task.vidParams, *task.genRes);
            break;
        case TaskType::Upscaling:
        case TaskType::Conversion:
            break;
        }

        try {
            task.metadataForWrite = mgr.SerializeEntity(entityID);
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("QueueTask: serialization failed for entity %u: %s",
                entityID, e.what());
            return;
        }

        if (taskType == TaskType::Inference || taskType == TaskType::Img2Img ||
            taskType == TaskType::Img2Vid || taskType == TaskType::Edit) {
            if (!m_cacheSystem) m_cacheSystem = mgr.GetSystem<ModelCacheSystem>();
            if (!m_cacheSystem) {
                ANI_LOG_ERROR("QueueTask: ModelCacheSystem not available");
                return;
            }

            auto localCtxRes = std::make_shared<SDCPP::ResourceManager>();
            sd_ctx_params_t localCtxParams{};
            SDCPP::FillContextParams(mgr, entityID, localCtxParams, *localCtxRes);

            auto handle = m_cacheSystem->acquireOrCreateContext(localCtxParams, localCtxRes);
            if (!handle) {
                ANI_LOG_ERROR("QueueTask: failed to acquire context: %s",
                    m_cacheSystem->getLastError().c_str());
                return;
            }
            task.ctxHandle = std::make_shared<SDCPP::SDContextHandle>(std::move(*handle));
        }
        else if (taskType == TaskType::Upscaling) {
            if (!m_cacheSystem) m_cacheSystem = mgr.GetSystem<ModelCacheSystem>();
            if (!m_cacheSystem) {
                ANI_LOG_ERROR("QueueTask: ModelCacheSystem not available");
                return;
            }

            auto localCtxRes = std::make_shared<SDCPP::ResourceManager>();
            sd_ctx_params_t localCtxParams{};
            SDCPP::FillContextParams(mgr, entityID, localCtxParams, *localCtxRes);

            auto handle = m_cacheSystem->acquireOrCreateUpscaler(localCtxParams, localCtxRes);
            if (!handle) {
                ANI_LOG_ERROR("QueueTask: failed to acquire upscaler: %s",
                    m_cacheSystem->getLastError().c_str());
                return;
            }
            task.upscalerHandle = std::make_shared<SDCPP::UpscalerHandle>(std::move(*handle));
        }
        else if (taskType == TaskType::Conversion) {
            SDCPP::FillContextParams(mgr, entityID, task.convParams, *task.genRes);
        }

        std::lock_guard<std::mutex> lock(queueMutex);
        if (shuttingDown) {
            ANI_LOG_WARN("QueueTask: shutting down, dropping task for entity %u", entityID);
            return;
        }
        taskQueue.push_back(std::move(task));
        ANI_LOG_DEBUG("QueueTask: enqueued task for entity %u (queue size: %zu)",
            entityID, taskQueue.size());
    }

    void SDCPPSystem::Update(float deltaT) {
        if (shuttingDown) return;
        if (clearRequested) {
            HandleClearRequest();
            clearRequested = false;
        }
        ProcessQueues();
        CheckTaskCompletion();
    }

    // ---------------------------------------------------------------------
    // Queue manipulation
    // ---------------------------------------------------------------------
    void SDCPPSystem::RemoveFromQueue(size_t index) {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (index < taskQueue.size() && !taskQueue[index].processing) {
            taskQueue.erase(taskQueue.begin() + index);
            ANI_LOG_TRACE("RemoveFromQueue: removed task at index %zu", index);
        }
        else if (index < taskQueue.size()) {
            ANI_LOG_TRACE("RemoveFromQueue: index %zu is currently processing, skip", index);
        }
    }

    void SDCPPSystem::MoveInQueue(size_t fromIndex, size_t toIndex) {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (fromIndex >= taskQueue.size() || toIndex >= taskQueue.size()) return;
        if (taskQueue[fromIndex].processing) return;
        TaskData task = std::move(taskQueue[fromIndex]);
        taskQueue.erase(taskQueue.begin() + fromIndex);
        taskQueue.insert(taskQueue.begin() + toIndex, std::move(task));
        ANI_LOG_TRACE("MoveInQueue: moved task from %zu to %zu", fromIndex, toIndex);
    }

    std::vector<SDCPPSystem::QueueItem> SDCPPSystem::GetQueueSnapshot() {
        std::lock_guard<std::mutex> lock(queueMutex);
        std::vector<QueueItem> result;
        result.reserve(taskQueue.size());
        for (const auto& t : taskQueue) {
            QueueItem q;
            q.entityID = t.entityID;
            q.processing = t.processing;
            q.taskType = t.taskType;
            result.push_back(q);
        }
        return result;
    }

    void SDCPPSystem::StopCurrentTask() {
        std::lock_guard<std::mutex> lock(queueMutex);
        for (auto& t : taskQueue) if (t.processing) { t.Cancel(); break; }
        pauseWorker = true;
        ANI_LOG_DEBUG("StopCurrentTask: cancelled active task and paused worker");
    }

    void SDCPPSystem::CancelCurrentTask() {
        std::lock_guard<std::mutex> lock(queueMutex);
        for (auto& t : taskQueue) if (t.processing) { t.Cancel(); break; }
        pauseWorker = false;
        ANI_LOG_DEBUG("CancelCurrentTask: cancelled active task, worker resumed");
    }

    void SDCPPSystem::ClearQueuedTasks() {
        std::lock_guard<std::mutex> lock(queueMutex);
        size_t before = taskQueue.size();
        taskQueue.erase(std::remove_if(taskQueue.begin(), taskQueue.end(),
            [](const TaskData& t) { return !t.processing; }), taskQueue.end());
        if (taskQueue.empty()) hasActiveTask = false;
        size_t removed = before - taskQueue.size();
        if (removed > 0) {
            ANI_LOG_DEBUG("ClearQueuedTasks: removed %zu queued task(s)", removed);
        }
    }

    void SDCPPSystem::ClearAllTasks() {
        std::lock_guard<std::mutex> lock(queueMutex);
        size_t count = taskQueue.size();
        for (auto& t : taskQueue) if (t.processing) t.Cancel();
        taskQueue.clear();
        hasActiveTask = false;
        clearRequested = false;
        ANI_LOG_INFO("ClearAllTasks: cleared %zu task(s)", count);
    }

    void SDCPPSystem::PauseWorker() {
        std::lock_guard<std::mutex> l(queueMutex);
        pauseWorker = true;
        ANI_LOG_DEBUG("Worker paused");
    }

    void SDCPPSystem::ResumeWorker() {
        std::lock_guard<std::mutex> l(queueMutex);
        pauseWorker = false;
        ANI_LOG_DEBUG("Worker resumed");
    }

    bool SDCPPSystem::IsPaused() const {
        std::lock_guard<std::mutex> l(queueMutex);
        return pauseWorker;
    }

    // ---------------------------------------------------------------------
    // Introspection
    // ---------------------------------------------------------------------
    size_t SDCPPSystem::GetNumThreads() const {
        return m_threadPool ? m_threadPool->getDiffusionPool().size() : 0;
    }
    size_t SDCPPSystem::GetQueuedTaskCount() const {
        return m_threadPool ? m_threadPool->getDiffusionPool().queueSize() : 0;
    }
    size_t SDCPPSystem::GetActiveTaskCount() const {
        return m_threadPool ? m_threadPool->getDiffusionPool().activeCount() : 0;
    }
    bool SDCPPSystem::HasActiveTask() const {
        std::lock_guard<std::mutex> l(queueMutex);
        return hasActiveTask;
    }
    size_t SDCPPSystem::GetQueueSize() const {
        std::lock_guard<std::mutex> l(queueMutex);
        return taskQueue.size();
    }

    std::vector<std::pair<SDCPPSystem::TaskType, nlohmann::json>>
        SDCPPSystem::GetQueueTasksWithMetadata() const {
        std::lock_guard<std::mutex> lock(queueMutex);
        std::vector<std::pair<TaskType, nlohmann::json>> result;
        result.reserve(taskQueue.size());
        for (const auto& t : taskQueue)
            result.emplace_back(t.taskType, t.metadataForWrite);
        return result;
    }

    void SDCPPSystem::QueueTaskFromSerialized(const nlohmann::json& entityData, TaskType taskType) {
        EntityID newEntity = mgr.DeserializeEntity(entityData);
        if (newEntity == 0) {
            ANI_LOG_ERROR("QueueTaskFromSerialized: failed to deserialize entity");
            return;
        }
        ANI_LOG_DEBUG("QueueTaskFromSerialized: deserialized to entity %u", newEntity);
        QueueTask(newEntity, taskType);
    }

    // ---------------------------------------------------------------------
    // Path resolution
    // ---------------------------------------------------------------------
    std::string SDCPPSystem::ResolveOutputDirectory(const std::string& raw) {
        std::string dir = raw;

        if (!dir.empty() && std::filesystem::path(dir).has_extension())
            dir = std::filesystem::path(dir).parent_path().string();

        if (!dir.empty() && !std::filesystem::path(dir).is_absolute() && m_filePathSystem) {
            std::string resolved = m_filePathSystem->GetPath(dir);
            if (!resolved.empty())
                dir = resolved;
        }

        if (dir.empty() || !std::filesystem::path(dir).is_absolute()) {
            if (m_filePathSystem) {
                std::string def = m_filePathSystem->GetPath("DefaultProject");
                if (!def.empty()) {
                    dir = def;
                    ANI_LOG_TRACE("ResolveOutputDirectory: fell back to DefaultProject: %s",
                        dir.c_str());
                }
            }
        }

        if (dir.empty()) {
            dir = std::filesystem::current_path().string();
            ANI_LOG_TRACE("ResolveOutputDirectory: fell back to cwd: %s", dir.c_str());
        }

        std::error_code ec;
        std::filesystem::create_directories(dir, ec);

        return dir;
    }

    std::string SDCPPSystem::ResolveFullPathForTask(const TaskData& task) {
        bool isVideo = IsVideoTask(task.taskType);

        std::string baseName = "AniStudio";
        std::string extension = isVideo ? ".mp4" : ".png";
        std::string rawDir;

        if (isVideo && mgr.HasComponent<OutputVideoComponent>(task.entityID)) {
            auto& output = mgr.GetComponent<OutputVideoComponent>(task.entityID);
            if (!output.fileName.empty()) baseName = output.fileName;
            extension = GetOutputExtension(task.taskType, task.entityID);
            rawDir = output.filePath;
        }
        else if (!isVideo && mgr.HasComponent<OutputImageComponent>(task.entityID)) {
            auto& output = mgr.GetComponent<OutputImageComponent>(task.entityID);
            if (!output.fileName.empty()) baseName = output.fileName;
            extension = GetOutputExtension(task.taskType, task.entityID);
            rawDir = output.filePath;
        }
        else {
            ANI_LOG_WARN("ResolveFullPathForTask: entity %u missing Output*Component, using default output dir",
                task.entityID);
        }

        size_t lastDot = baseName.find_last_of('.');
        if (lastDot != std::string::npos) baseName = baseName.substr(0, lastDot);
        std::string fullFileName = baseName + extension;

        std::string outputDir = ResolveOutputDirectory(rawDir);
        std::string full = Utils::PngMetadata::CreateUniqueFilename(fullFileName, outputDir);

        ANI_LOG_DEBUG("ResolveFullPathForTask: entity %u rawDir='%s' dir='%s' full='%s'",
            task.entityID, rawDir.c_str(), outputDir.c_str(), full.c_str());
        return full;
    }

    // ---------------------------------------------------------------------
    // Load helpers
    // ---------------------------------------------------------------------
    void SDCPPSystem::LoadImageViaImageSystem(EntityID targetEntity, const std::string& filePath) {
        if (auto imgSys = mgr.GetSystem<ImageSystem>()) {
            if (!mgr.HasComponent<ImageComponent>(targetEntity))
                mgr.AddComponent<ImageComponent>(targetEntity);
            imgSys->SetImage(targetEntity, filePath);
        }
        else {
            ANI_LOG_WARN("LoadImageViaImageSystem: ImageSystem unavailable for %s",
                filePath.c_str());
        }
    }

    void SDCPPSystem::LoadVideoViaVideoSystem(EntityID targetEntity, const std::string& filePath) {
        if (auto vidSys = mgr.GetSystem<VideoSystem>()) {
            if (!mgr.HasComponent<VideoComponent>(targetEntity))
                mgr.AddComponent<VideoComponent>(targetEntity);
            auto& vc = mgr.GetComponent<VideoComponent>(targetEntity);
            vc.filePath = filePath;
            vc.fileName = std::filesystem::path(filePath).filename().string();
            vidSys->SetVideo(targetEntity, filePath);
        }
        else {
            ANI_LOG_WARN("LoadVideoViaVideoSystem: VideoSystem unavailable for %s",
                filePath.c_str());
        }
    }

    EntityID SDCPPSystem::LoadVideoWithAudio(const std::string& filePath) {
        if (auto vaSys = mgr.GetSystem<VideoAudioSystem>()) {
            return vaSys->LoadVideoWithAudio(filePath);
        }
        ANI_LOG_WARN("LoadVideoWithAudio: VideoAudioSystem unavailable for %s",
            filePath.c_str());
        return 0;
    }

    void SDCPPSystem::HandleClearRequest() {
        std::lock_guard<std::mutex> lock(queueMutex);
        ClearQueuedTasks();
    }

    // ---------------------------------------------------------------------
    // Static task runners
    // ---------------------------------------------------------------------
    bool SDCPPSystem::RunInference(const sd_img_gen_params_t& params,
        const nlohmann::json& metadataForWrite,
        const std::string& fullPath,
        sd_ctx_t* context)
    {
        if (context) sd_cancel_generation(context, SD_CANCEL_RESET);
        sd_image_t* images = nullptr;
        int count = 0;
        bool ok = generate_image(context, &params, &images, &count);
        if (ok && count > 0 && images && images[0].data) {
            Utils::ImageUtils::SaveImage(fullPath, images[0].width, images[0].height,
                images[0].channel, images[0].data);
            Utils::ImageUtils::WriteMetadataToImage(fullPath, metadataForWrite, true, false);
            free_sd_images(images, count);
            bool exists = std::filesystem::exists(fullPath);
            if (exists) {
                ANI_LOG_INFO("RunInference: wrote %s (%dx%d)", fullPath.c_str(),
                    images[0].width, images[0].height);
            }
            return exists;
        }
        if (images) free_sd_images(images, count);
        ANI_LOG_WARN("RunInference: generate_image failed for %s", fullPath.c_str());
        return false;
    }

    bool SDCPPSystem::RunImg2Img(const sd_img_gen_params_t& params,
        const nlohmann::json& metadataForWrite,
        const std::string& fullPath,
        sd_ctx_t* context) {
        return RunInference(params, metadataForWrite, fullPath, context);
    }

    bool SDCPPSystem::RunEdit(const sd_img_gen_params_t& params,
        const nlohmann::json& metadataForWrite,
        const std::string& fullPath,
        sd_ctx_t* context) {
        return RunInference(params, metadataForWrite, fullPath, context);
    }

    bool SDCPPSystem::RunImg2Vid(const sd_vid_gen_params_t& params,
        const nlohmann::json& metadataForWrite,
        const std::string& fullPath,
        sd_ctx_t* context)
    {
        if (context) sd_cancel_generation(context, SD_CANCEL_RESET);

        sd_image_t* frames = nullptr;
        int frameCount = 0;
        sd_audio_t* audio = nullptr;
        int fps_out = params.fps;
        bool ok = generate_video(context, &params, &frames, &frameCount, &audio, &fps_out);

        if (!ok || frameCount <= 0 || !frames) {
            if (frames) free(frames);
            if (audio)  free_sd_audio(audio);
            ANI_LOG_ERROR("RunImg2Vid: generate_video failed for %s", fullPath.c_str());
            return false;
        }

        std::vector<Utils::VideoFrame> videoFrames;
        videoFrames.reserve(frameCount);
        for (int i = 0; i < frameCount; ++i) {
            Utils::VideoFrame vf;
            vf.width = frames[i].width;
            vf.height = frames[i].height;
            vf.channels = frames[i].channel;
            vf.data = frames[i].data;
            videoFrames.push_back(vf);
        }

        Utils::AudioData audioData;
        bool haveAudio = false;
        if (audio && audio->data && audio->sample_count > 0 && audio->channels > 0) {
            audioData = Utils::AudioData::FromInterleavedFloat(
                audio->data,
                audio->sample_count,
                static_cast<int>(audio->channels),
                static_cast<int>(audio->sample_rate));
            haveAudio = !audioData.pcmData.empty();

            ANI_LOG_DEBUG("RunImg2Vid: audio %d ch @ %d Hz, %.2fs, %zu floats",
                audioData.channels, audioData.sampleRate,
                audioData.duration, audioData.pcmData.size());
        }
        else {
            ANI_LOG_DEBUG("RunImg2Vid: no audio returned by generate_video");
        }

        int outFps = fps_out > 0 ? fps_out : params.fps;

        bool encoded = Utils::VideoUtils::EncodeFramesToVideo(
            videoFrames,
            fullPath,
            outFps,
            metadataForWrite,
            haveAudio ? &audioData : nullptr);

        if (audio)  free_sd_audio(audio);
        if (frames) free(frames);

        if (!encoded) {
            ANI_LOG_ERROR("RunImg2Vid: EncodeFramesToVideo failed for %s", fullPath.c_str());
            return false;
        }

        if (!std::filesystem::exists(fullPath)) {
            ANI_LOG_ERROR("RunImg2Vid: encoder returned true but file missing: %s", fullPath.c_str());
            return false;
        }

        if (!metadataForWrite.is_null() && !metadataForWrite.empty()) {
            if (!Utils::VideoMetadataUtils::WriteMetadataToVideo(fullPath, metadataForWrite, false)) {
                ANI_LOG_WARN("RunImg2Vid: metadata rewrite failed for %s", fullPath.c_str());
            }
        }

        std::error_code ec;
        auto size = std::filesystem::file_size(fullPath, ec);
        ANI_LOG_INFO("RunImg2Vid: saved '%s' frames=%d hasAudio=%s fps=%d size=%llu",
            fullPath.c_str(), frameCount, haveAudio ? "true" : "false", outFps,
            ec ? (unsigned long long)0 : (unsigned long long)size);
        return true;
    }

    bool SDCPPSystem::RunUpscaling(const nlohmann::json& metadataForWrite,
        const std::string& fullPath,
        upscaler_ctx_t* upscaler,
        const std::string& inputImagePath,
        uint32_t upscaleFactor)
    {
        if (!upscaler || inputImagePath.empty()) {
            ANI_LOG_WARN("RunUpscaling: missing upscaler or input path (input='%s')",
                inputImagePath.c_str());
            return false;
        }

        int w = 0, h = 0, c = 0;
        unsigned char* data = stbi_load(inputImagePath.c_str(), &w, &h, &c, 0);
        if (!data) {
            ANI_LOG_WARN("RunUpscaling: stbi_load failed for %s", inputImagePath.c_str());
            return false;
        }

        sd_image_t input{ (uint32_t)w, (uint32_t)h, (uint32_t)c, data };
        sd_image_t* out = nullptr;
        int count = 0;
        bool ok = upscale(upscaler, input, upscaleFactor, &out, &count);
        stbi_image_free(data);

        if (ok && count > 0 && out && out[0].data) {
            Utils::ImageUtils::SaveImage(fullPath, out[0].width, out[0].height,
                out[0].channel, out[0].data);
            Utils::ImageUtils::WriteMetadataToImage(fullPath, metadataForWrite, true, false);
            free_sd_images(out, count);
            bool exists = std::filesystem::exists(fullPath);
            if (exists) {
                ANI_LOG_INFO("RunUpscaling: wrote %s (%dx%d, factor %u)",
                    fullPath.c_str(), out[0].width, out[0].height, upscaleFactor);
            }
            return exists;
        }
        if (out) free_sd_images(out, count);
        ANI_LOG_WARN("RunUpscaling: upscale failed for %s", inputImagePath.c_str());
        return false;
    }

    bool SDCPPSystem::RunConversion(const sd_ctx_params_t& ctx) {
        std::string input = ctx.model_path ? ctx.model_path : "";
        std::string vae = ctx.vae_path ? ctx.vae_path : "";
        if (input.empty()) {
            ANI_LOG_WARN("RunConversion: no model_path in context");
            return false;
        }
        std::string output = std::filesystem::path(input).stem().string() + "_converted.gguf";
        bool ok = convert(input.c_str(), vae.c_str(), output.c_str(),
            ctx.wtype, ctx.tensor_type_rules, true);
        if (ok) {
            ANI_LOG_INFO("RunConversion: converted %s -> %s", input.c_str(), output.c_str());
        }
        else {
            ANI_LOG_WARN("RunConversion: convert() failed for %s", input.c_str());
        }
        return ok;
    }

    bool SDCPPSystem::IsVideoTask(TaskType taskType) const {
        return taskType == TaskType::Img2Vid || taskType == TaskType::Edit;
    }

    std::string SDCPPSystem::GetOutputExtension(TaskType taskType, EntityID entityID) const {
        if (IsVideoTask(taskType)) {
            if (mgr.IsEntityValid(entityID) && mgr.HasComponent<OutputVideoComponent>(entityID)) {
                auto& out = mgr.GetComponent<OutputVideoComponent>(entityID);
                if (!out.fileExtension.empty()) {
                    std::string ext = out.fileExtension;
                    if (ext[0] != '.') ext = "." + ext;
                    return ext;
                }
            }
            ANI_LOG_TRACE("GetOutputExtension: defaulting to .mp4 for entity %u", entityID);
            return ".mp4";
        }
        if (mgr.IsEntityValid(entityID) && mgr.HasComponent<OutputImageComponent>(entityID)) {
            auto& out = mgr.GetComponent<OutputImageComponent>(entityID);
            if (!out.fileExtension.empty()) {
                std::string ext = out.fileExtension;
                if (ext[0] != '.') ext = "." + ext;
                return ext;
            }
        }
        ANI_LOG_TRACE("GetOutputExtension: defaulting to .png for entity %u", entityID);
        return ".png";
    }

    // ---------------------------------------------------------------------
    // Dispatch
    // ---------------------------------------------------------------------
    void SDCPPSystem::ProcessQueues() {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (pauseWorker || shuttingDown) return;
        if (taskQueue.empty() || hasActiveTask) return;

        if (!m_threadPool) {
            m_threadPool = mgr.GetSystem<ThreadPoolSystem>();
            if (!m_threadPool) {
                ANI_LOG_ERROR("ProcessQueues: ThreadPoolSystem missing");
                return;
            }
        }
        if (!m_cacheSystem) {
            m_cacheSystem = mgr.GetSystem<ModelCacheSystem>();
            if (!m_cacheSystem) {
                ANI_LOG_ERROR("ProcessQueues: ModelCacheSystem missing");
                return;
            }
        }

        auto& diffusionPool = m_threadPool->getDiffusionPool();

        for (auto it = taskQueue.begin(); it != taskQueue.end(); ++it) {
            auto& task = *it;
            if (task.processing) continue;

            if (task.fullPath.empty()) {
                task.fullPath = ResolveFullPathForTask(task);
                if (task.fullPath.empty()) {
                    ANI_LOG_ERROR("ProcessQueues: cannot resolve output path for entity %u, removing task",
                        task.entityID);
                    it = taskQueue.erase(it);
                    continue;
                }
            }

            try {
                switch (task.taskType) {
                case TaskType::Inference:
                case TaskType::Img2Img:
                case TaskType::Edit: {
                    auto params = task.imgParams;
                    auto meta = task.metadataForWrite;
                    auto path = task.fullPath;
                    auto handle = task.ctxHandle;
                    auto genRes = task.genRes;
                    task.result = diffusionPool.submit(
                        [params, meta, path, handle, genRes]() -> bool {
                            return RunInference(params, meta, path, handle->get());
                        });
                    break;
                }
                case TaskType::Img2Vid: {
                    auto params = task.vidParams;
                    auto meta = task.metadataForWrite;
                    auto path = task.fullPath;
                    auto handle = task.ctxHandle;
                    auto genRes = task.genRes;
                    task.result = diffusionPool.submit(
                        [params, meta, path, handle, genRes]() -> bool {
                            return RunImg2Vid(params, meta, path, handle->get());
                        });
                    break;
                }
                case TaskType::Upscaling: {
                    auto meta = task.metadataForWrite;
                    auto path = task.fullPath;
                    auto handle = task.upscalerHandle;
                    auto genRes = task.genRes;
                    std::string inputPath;
                    if (mgr.HasComponent<InputImageComponent>(task.entityID))
                        inputPath = mgr.GetComponent<InputImageComponent>(task.entityID).filePath;
                    uint32_t factor = 2;
                    if (mgr.HasComponent<EsrganComponent>(task.entityID))
                        factor = mgr.GetComponent<EsrganComponent>(task.entityID).upscaleFactor;
                    task.result = diffusionPool.submit(
                        [meta, path, handle, inputPath, factor, genRes]() -> bool {
                            return RunUpscaling(meta, path, handle->get(), inputPath, factor);
                        });
                    break;
                }
                case TaskType::Conversion: {
                    auto params = task.convParams;
                    auto genRes = task.genRes;
                    task.result = diffusionPool.submit(
                        [params, genRes]() -> bool {
                            return RunConversion(params);
                        });
                    break;
                }
                default:
                    continue;
                }
                task.processing = true;
                task.startTime = std::chrono::steady_clock::now();
                hasActiveTask = true;
                activeThreadId = std::this_thread::get_id();

                ANI_LOG_DEBUG("ProcessQueues: submitted task for entity %u (type %d)",
                    task.entityID, (int)task.taskType);
                break;
            }
            catch (const std::exception& e) {
                ANI_LOG_ERROR("ProcessQueues: exception submitting task for entity %u: %s",
                    task.entityID, e.what());
                it = taskQueue.erase(it);
                break;
            }
        }
    }

    // ---------------------------------------------------------------------
    // Completion
    // ---------------------------------------------------------------------
    void SDCPPSystem::CheckTaskCompletion() {
        if (taskQueue.empty()) return;
        std::vector<std::tuple<std::string, TaskType, EntityID>> completedTasks;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            const auto now = std::chrono::steady_clock::now();
            for (auto it = taskQueue.begin(); it != taskQueue.end();) {
                if (!it->processing) { ++it; continue; }

                bool remove = false;
                bool success = false;

                if (it->cancelled) {
                    if (it->result.valid() &&
                        it->result.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                        try { it->result.get(); }
                        catch (...) {}
                        remove = true;
                    }
                    else if (now - it->cancelTime > std::chrono::seconds(10)) {
                        remove = true;
                    }
                }
                else if (it->result.valid() &&
                    it->result.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                    try { success = it->result.get(); }
                    catch (...) {}
                    remove = true;
                }

                if (remove) {
                    if (success && std::filesystem::exists(it->fullPath)) {
                        completedTasks.emplace_back(it->fullPath, it->taskType, it->entityID);
                    }
                    else {
                        bool exists = std::filesystem::exists(it->fullPath);
                        ANI_LOG_WARN("CheckTaskCompletion: task finished but not delivering "
                            "(success=%s exists=%s path='%s')",
                            success ? "true" : "false",
                            exists ? "true" : "false",
                            it->fullPath.c_str());
                        if (exists)
                            std::filesystem::remove(it->fullPath);
                    }
                    it = taskQueue.erase(it);
                    hasActiveTask = false;
                }
                else {
                    ++it;
                }
            }
            if (taskQueue.empty() && !hasActiveTask)
                activeThreadId = std::thread::id{};
        }
        if (!shuttingDown) {
            for (const auto& [path, type, id] : completedTasks)
                ProcessCompletedTask(path, type, id);
        }
    }

    void SDCPPSystem::ProcessCompletedTask(const std::string& fullPath,
        TaskType taskType,
        EntityID entityID)
    {
        try {
            bool exists = std::filesystem::exists(fullPath);

            if (!shuttingDown && exists) {
                if (IsVideoTask(taskType)) {
                    EntityID loaded = LoadVideoWithAudio(fullPath);
                    if (loaded == 0) {
                        EntityID e = mgr.AddNewEntity();
                        LoadVideoViaVideoSystem(e, fullPath);
                        ANI_LOG_INFO("ProcessCompletedTask: loaded video as entity %u: %s",
                            e, fullPath.c_str());
                    }
                    else {
                        ANI_LOG_INFO("ProcessCompletedTask: loaded video+audio as entity %u: %s",
                            loaded, fullPath.c_str());
                    }
                }
                else {
                    EntityID e = mgr.AddNewEntity();
                    LoadImageViaImageSystem(e, fullPath);
                    ANI_LOG_INFO("ProcessCompletedTask: loaded image as entity %u: %s",
                        e, fullPath.c_str());
                }
            }
            else if (exists) {
                ANI_LOG_DEBUG("ProcessCompletedTask: shutting down, removing %s",
                    fullPath.c_str());
                std::filesystem::remove(fullPath);
            }
        }
        catch (const std::exception& e) {
            ANI_LOG_WARN("ProcessCompletedTask: exception loading %s: %s",
                fullPath.c_str(), e.what());
            std::error_code ec;
            if (std::filesystem::exists(fullPath, ec))
                std::filesystem::remove(fullPath, ec);
        }

        if (mgr.IsEntityValid(entityID)) {
            mgr.DestroyEntity(entityID);
        }
    }

    void SDCPPSystem::WorkerThread() {
        while (!shuttingDown)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

} // namespace ECS