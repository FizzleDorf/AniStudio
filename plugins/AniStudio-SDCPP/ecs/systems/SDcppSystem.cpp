// SDCPPSystem.cpp
#include "SDCPPSystem.hpp"
#include "rng.hpp"
#include <stb_image.h>
#include <stb_image_write.h>
#include "VideoUtils.hpp"
#include "VideoMetadataUtils.hpp"

namespace ECS {

    // ---------------------------------------------------------------------
    // TaskData
    // ---------------------------------------------------------------------
    void SDCPPSystem::TaskData::Cancel() {
        cancelled = true;
        cancelTime = std::chrono::steady_clock::now();
        if (ctxHandle && ctxHandle->get())
            sd_cancel_generation(ctxHandle->get(), SD_CANCEL_ALL);
    }

    // ---------------------------------------------------------------------
    // Lifecycle
    // ---------------------------------------------------------------------
    SDCPPSystem::SDCPPSystem(EntityManager& entityMgr)
        : BaseSystem(entityMgr), pauseWorker(false), hasActiveTask(false), clearRequested(false) {
        sysName = "SDCPPSystem";
        m_filePathSystem = mgr.GetSystem<FilePathSystem>();
    }

    SDCPPSystem::~SDCPPSystem() {
        Shutdown();
    }

    void SDCPPSystem::Shutdown() {
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
    }

    void SDCPPSystem::TerminateImmediately() {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            shuttingDown = true;
            pauseWorker = true;
        }
        ClearAllTasks();
        if (m_threadPool) m_threadPool->terminateAll();
    }

    void SDCPPSystem::Start() {
        m_cacheSystem = mgr.GetSystem<ModelCacheSystem>();
        m_threadPool = mgr.GetSystem<ThreadPoolSystem>();
        if (!m_threadPool)
            std::cerr << "[SDCPPSystem] ThreadPoolSystem not available\n";
        workerThread = std::thread([this]() { WorkerThread(); });
    }

    void SDCPPSystem::Destroy() {
        Shutdown();
        BaseSystem::Destroy();
    }

    // ---------------------------------------------------------------------
    // Queue
    // ---------------------------------------------------------------------
    void SDCPPSystem::QueueTask(EntityID entityID, TaskType taskType) {
        if (!mgr.IsEntityValid(entityID)) {
            std::cerr << "[QueueTask] Invalid entity\n";
            return;
        }

        // Copy global SDCPP settings onto the task entity.
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

        // Seed generation.
        if (taskType == TaskType::Inference || taskType == TaskType::Img2Img ||
            taskType == TaskType::Img2Vid || taskType == TaskType::Edit) {
            if (mgr.HasComponent<SamplerComponent>(entityID)) {
                auto& sampler = mgr.GetComponent<SamplerComponent>(entityID);
                if (sampler.seed < 0) {
                    sampler.seed = (int64_t)STDDefaultRNG::generate_seed();
                    if (sampler.seed == 0) sampler.seed = 31337;
                }
            }
        }

        TaskData task;
        task.entityID = entityID;
        task.taskType = taskType;
        task.enqueueTime = std::chrono::steady_clock::now();
        task.genRes = std::make_shared<SDCPP::ResourceManager>();

        // Fill per-task params into genRes.
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

        // Metadata for writing at the end.
        try {
            task.metadataForWrite = mgr.SerializeEntity(entityID);
        }
        catch (...) {
            std::cerr << "[QueueTask] Serialization failed\n";
            return;
        }

        // ---- Context acquisition -------------------------------------------------
        // ctx params are built in a LOCAL ResourceManager. On a hit, the cache
        // hands back a shared_ptr to the entry's manager. On a miss, ownership
        // moves into the new entry. Either way, the handle keeps that manager
        // alive for the task's lifetime, so nothing outside the cache entry
        // ever points at strings the entry might free.
        if (taskType == TaskType::Inference || taskType == TaskType::Img2Img ||
            taskType == TaskType::Img2Vid || taskType == TaskType::Edit) {
            if (!m_cacheSystem) m_cacheSystem = mgr.GetSystem<ModelCacheSystem>();
            if (!m_cacheSystem) {
                std::cerr << "[QueueTask] ModelCacheSystem not available\n";
                return;
            }

            auto localCtxRes = std::make_shared<SDCPP::ResourceManager>();
            sd_ctx_params_t localCtxParams{};
            SDCPP::FillContextParams(mgr, entityID, localCtxParams, *localCtxRes);

            auto handle = m_cacheSystem->acquireOrCreateContext(localCtxParams, localCtxRes);
            if (!handle) {
                std::cerr << "[QueueTask] Failed to acquire context: "
                    << m_cacheSystem->getLastError() << "\n";
                return;
            }
            task.ctxHandle = std::make_shared<SDCPP::SDContextHandle>(std::move(*handle));
        }
        else if (taskType == TaskType::Upscaling) {
            if (!m_cacheSystem) m_cacheSystem = mgr.GetSystem<ModelCacheSystem>();
            if (!m_cacheSystem) {
                std::cerr << "[QueueTask] ModelCacheSystem not available\n";
                return;
            }

            auto localCtxRes = std::make_shared<SDCPP::ResourceManager>();
            sd_ctx_params_t localCtxParams{};
            SDCPP::FillContextParams(mgr, entityID, localCtxParams, *localCtxRes);

            auto handle = m_cacheSystem->acquireOrCreateUpscaler(localCtxParams, localCtxRes);
            if (!handle) {
                std::cerr << "[QueueTask] Failed to acquire upscaler: "
                    << m_cacheSystem->getLastError() << "\n";
                return;
            }
            task.upscalerHandle = std::make_shared<SDCPP::UpscalerHandle>(std::move(*handle));
        }
        else if (taskType == TaskType::Conversion) {
            // Conversion doesn't need a cache entry. Build the params into a
            // ResourceManager that lives with the task, and keep a reference
            // to it via genRes so the strings outlive the task.
            SDCPP::FillContextParams(mgr, entityID, task.convParams, *task.genRes);
        }

        std::lock_guard<std::mutex> lock(queueMutex);
        if (shuttingDown) return;
        taskQueue.push_back(std::move(task));
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
        if (index < taskQueue.size() && !taskQueue[index].processing)
            taskQueue.erase(taskQueue.begin() + index);
    }

    void SDCPPSystem::MoveInQueue(size_t fromIndex, size_t toIndex) {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (fromIndex >= taskQueue.size() || toIndex >= taskQueue.size()) return;
        if (taskQueue[fromIndex].processing) return;
        TaskData task = std::move(taskQueue[fromIndex]);
        taskQueue.erase(taskQueue.begin() + fromIndex);
        taskQueue.insert(taskQueue.begin() + toIndex, std::move(task));
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
    }

    void SDCPPSystem::CancelCurrentTask() {
        std::lock_guard<std::mutex> lock(queueMutex);
        for (auto& t : taskQueue) if (t.processing) { t.Cancel(); break; }
        pauseWorker = false;
    }

    void SDCPPSystem::ClearQueuedTasks() {
        std::lock_guard<std::mutex> lock(queueMutex);
        taskQueue.erase(std::remove_if(taskQueue.begin(), taskQueue.end(),
            [](const TaskData& t) { return !t.processing; }), taskQueue.end());
        if (taskQueue.empty()) hasActiveTask = false;
    }

    void SDCPPSystem::ClearAllTasks() {
        std::lock_guard<std::mutex> lock(queueMutex);
        for (auto& t : taskQueue) if (t.processing) t.Cancel();
        taskQueue.clear();
        hasActiveTask = false;
        clearRequested = false;
    }

    void SDCPPSystem::PauseWorker() { std::lock_guard<std::mutex> l(queueMutex); pauseWorker = true; }
    void SDCPPSystem::ResumeWorker() { std::lock_guard<std::mutex> l(queueMutex); pauseWorker = false; }
    bool SDCPPSystem::IsPaused() const { std::lock_guard<std::mutex> l(queueMutex); return pauseWorker; }

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
        std::lock_guard<std::mutex> l(queueMutex); return hasActiveTask;
    }
    size_t SDCPPSystem::GetQueueSize() const {
        std::lock_guard<std::mutex> l(queueMutex); return taskQueue.size();
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
            std::cerr << "[SDCPPSystem] Failed to deserialize entity from saved data.\n";
            return;
        }
        QueueTask(newEntity, taskType);
    }

    // ---------------------------------------------------------------------
    // Path resolution
    // ---------------------------------------------------------------------
    std::string SDCPPSystem::ResolveOutputDirectory(const std::string& raw) {
        std::string dir = raw;

        // If the user gave us a full filename, take its parent.
        if (!dir.empty() && std::filesystem::path(dir).has_extension())
            dir = std::filesystem::path(dir).parent_path().string();

        // If it's not absolute, it may be a FilePathSystem key.
        if (!dir.empty() && !std::filesystem::path(dir).is_absolute() && m_filePathSystem) {
            std::string resolved = m_filePathSystem->GetPath(dir);
            if (!resolved.empty())
                dir = resolved;
        }

        // Fall back to DefaultProject.
        if (dir.empty() || !std::filesystem::path(dir).is_absolute()) {
            if (m_filePathSystem) {
                std::string def = m_filePathSystem->GetPath("DefaultProject");
                if (!def.empty())
                    dir = def;
            }
        }

        // Last resort: CWD.
        if (dir.empty())
            dir = std::filesystem::current_path().string();

        std::error_code ec;
        std::filesystem::create_directories(dir, ec);

        return dir;
    }

    std::string SDCPPSystem::ResolveFullPathForTask(const TaskData& task) {
        bool isVideo = IsVideoTask(task.taskType);

        // Pick a base name + extension from whichever Output*Component exists.
        // If neither exists, fall back to defaults rather than dropping the task.
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
            std::cerr << "[SDCPPSystem] entity=" << task.entityID
                << " missing Output*Component, using default output dir\n";
        }

        size_t lastDot = baseName.find_last_of('.');
        if (lastDot != std::string::npos) baseName = baseName.substr(0, lastDot);
        std::string fullFileName = baseName + extension;

        std::string outputDir = ResolveOutputDirectory(rawDir);
        std::string full = Utils::PngMetadata::CreateUniqueFilename(fullFileName, outputDir);

        std::cerr << "[SDCPPSystem] entity=" << task.entityID
            << " rawDir='" << rawDir
            << "' dir='" << outputDir
            << "' full='" << full << "'\n";
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
    }

    EntityID SDCPPSystem::LoadVideoWithAudio(const std::string& filePath) {
        if (auto vaSys = mgr.GetSystem<VideoAudioSystem>()) {
            return vaSys->LoadVideoWithAudio(filePath);
        }
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
            return std::filesystem::exists(fullPath);
        }
        if (images) free_sd_images(images, count);
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
            std::cerr << "[RunImg2Vid] generate_video failed\n";
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

            std::cerr << "[RunImg2Vid] audio: " << audioData.channels << "ch @ "
                << audioData.sampleRate << "Hz, "
                << audioData.duration << "s, "
                << audioData.pcmData.size() << " floats\n";
        }
        else {
            std::cerr << "[RunImg2Vid] no audio returned by generate_video\n";
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
            std::cerr << "[RunImg2Vid] EncodeFramesToVideo failed for " << fullPath << "\n";
            return false;
        }

        if (!std::filesystem::exists(fullPath)) {
            std::cerr << "[RunImg2Vid] encoder returned true but file missing: " << fullPath << "\n";
            return false;
        }

        if (!metadataForWrite.is_null() && !metadataForWrite.empty()) {
            if (!Utils::VideoMetadataUtils::WriteMetadataToVideo(fullPath, metadataForWrite, false)) {
                std::cerr << "[RunImg2Vid] Warning: metadata rewrite failed\n";
            }
        }

        std::cerr << "[RunImg2Vid] saved='" << fullPath
            << "' frames=" << frameCount
            << " hasAudio=" << haveAudio
            << " fps=" << outFps
            << " size=" << std::filesystem::file_size(fullPath) << "\n";
        return true;
    }

    bool SDCPPSystem::RunUpscaling(const nlohmann::json& metadataForWrite,
        const std::string& fullPath,
        upscaler_ctx_t* upscaler,
        const std::string& inputImagePath,
        uint32_t upscaleFactor)
    {
        if (!upscaler || inputImagePath.empty()) return false;

        int w = 0, h = 0, c = 0;
        unsigned char* data = stbi_load(inputImagePath.c_str(), &w, &h, &c, 0);
        if (!data) return false;

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
            return std::filesystem::exists(fullPath);
        }
        if (out) free_sd_images(out, count);
        return false;
    }

    bool SDCPPSystem::RunConversion(const sd_ctx_params_t& ctx) {
        std::string input = ctx.model_path ? ctx.model_path : "";
        std::string vae = ctx.vae_path ? ctx.vae_path : "";
        if (input.empty()) return false;
        std::string output = std::filesystem::path(input).stem().string() + "_converted.gguf";
        return convert(input.c_str(), vae.c_str(), output.c_str(),
            ctx.wtype, ctx.tensor_type_rules, true);
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
            if (!m_threadPool) { std::cerr << "[SDCPPSystem] ThreadPoolSystem missing!\n"; return; }
        }
        if (!m_cacheSystem) {
            m_cacheSystem = mgr.GetSystem<ModelCacheSystem>();
            if (!m_cacheSystem) { std::cerr << "[SDCPPSystem] ModelCacheSystem missing!\n"; return; }
        }

        auto& diffusionPool = m_threadPool->getDiffusionPool();

        for (auto it = taskQueue.begin(); it != taskQueue.end(); ++it) {
            auto& task = *it;
            if (task.processing) continue;

            if (task.fullPath.empty()) {
                task.fullPath = ResolveFullPathForTask(task);
                if (task.fullPath.empty()) {
                    std::cerr << "[SDCPPSystem] Cannot resolve output path for entity "
                        << task.entityID << ", removing task\n";
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
                    auto genRes = task.genRes;   // keeps the strings alive
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
                break;
            }
            catch (...) {
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
                        std::cerr << "[SDCPPSystem] task finished but not delivering: success="
                            << success << " exists="
                            << std::filesystem::exists(it->fullPath)
                            << " path='" << it->fullPath << "'\n";
                        if (std::filesystem::exists(it->fullPath))
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
            if (shuttingDown || !std::filesystem::exists(fullPath)) return;

            if (IsVideoTask(taskType)) {
                EntityID vaEntity = LoadVideoWithAudio(fullPath);
                if (vaEntity == 0) {
                    std::cerr << "[SDCPPSystem] VideoAudioSystem missing or failed, "
                        "falling back to silent VideoSystem load\n";
                    if (mgr.IsEntityValid(entityID))
                        LoadVideoViaVideoSystem(entityID, fullPath);
                    else {
                        EntityID newEntity = mgr.AddNewEntity();
                        LoadVideoViaVideoSystem(newEntity, fullPath);
                    }
                }
                return;
            }

            if (mgr.IsEntityValid(entityID)) {
                LoadImageViaImageSystem(entityID, fullPath);
            }
            else {
                std::cerr << "[SDCPPSystem] entity " << entityID
                    << " invalid, loading result into a new entity\n";
                EntityID newEntity = mgr.AddNewEntity();
                LoadImageViaImageSystem(newEntity, fullPath);
            }
        }
        catch (...) {
            if (std::filesystem::exists(fullPath))
                std::filesystem::remove(fullPath);
        }
    }

    void SDCPPSystem::WorkerThread() {
        while (!shuttingDown)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

} // namespace ECS