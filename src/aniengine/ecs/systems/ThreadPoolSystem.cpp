#include "ThreadPoolSystem.hpp"
#include <iostream>

namespace ECS {

    ThreadPoolSystem::ThreadPoolSystem(EntityManager& mgr)
        : BaseSystem(mgr)
    {
        size_t hw = std::thread::hardware_concurrency();
        m_diffusionPool = std::make_unique<Pool>(1);
        m_ioPool = std::make_unique<Pool>(2);
        m_generalPool = std::make_unique<Pool>(std::max<size_t>(2, hw / 2));
    }

    ThreadPoolSystem::~ThreadPoolSystem() {
        terminateAll();
    }

    void ThreadPoolSystem::Destroy() {
        terminateAll();
        BaseSystem::Destroy();
    }

    std::future<bool> ThreadPoolSystem::submitDiffusionTask(std::function<bool()> job) {
        // Instantiated exactly once, here, inside AniEngineCore - never inside a
        // plugin DLL. Keeps Pool::submit<F,Args...>()'s template instantiation
        // confined to the binary that owns m_diffusionPool.
        return m_diffusionPool->submit(std::move(job));
    }

    void ThreadPoolSystem::terminateAll() {
        if (m_diffusionPool) m_diffusionPool->terminate();
        if (m_ioPool) m_ioPool->terminate();
        if (m_generalPool) m_generalPool->terminate();
    }

    ThreadPoolSystem::Stats ThreadPoolSystem::getStats() const {
        return {
            m_diffusionPool->activeCount(),
            m_diffusionPool->queueSize(),
            m_ioPool->activeCount(),
            m_ioPool->queueSize(),
            m_generalPool->activeCount(),
            m_generalPool->queueSize()
        };
    }

    ThreadPoolSystem::Pool::Pool(size_t numThreads) {
        startThreads(numThreads > 0 ? numThreads : std::thread::hardware_concurrency());
    }

    ThreadPoolSystem::Pool::~Pool() {
        terminate();
    }

    void ThreadPoolSystem::Pool::terminate() {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_terminate) return;
            m_terminate = true;
            m_stop = true;
            while (!m_tasks.empty()) m_tasks.pop();
        }
        m_condition.notify_all();
        for (auto& worker : m_workers) {
            if (worker.joinable()) worker.join();
        }
        m_workers.clear();
    }

    void ThreadPoolSystem::Pool::clearQueue() {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (!m_tasks.empty()) m_tasks.pop();
    }

    size_t ThreadPoolSystem::Pool::queueSize() const {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_tasks.size();
    }

    size_t ThreadPoolSystem::Pool::activeCount() const {
        return m_active.load();
    }

    bool ThreadPoolSystem::Pool::isTerminating() const {
        return m_terminate.load();
    }

    size_t ThreadPoolSystem::Pool::size() const {
        return m_workers.size();
    }

    void ThreadPoolSystem::Pool::startThreads(size_t numThreads) {
        for (size_t i = 0; i < numThreads; ++i) {
            m_workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(m_mutex);
                        m_condition.wait(lock, [this] {
                            return m_terminate || (!m_tasks.empty() && !m_stop);
                            });
                        if (m_terminate) return;
                        if (m_stop && m_tasks.empty()) return;
                        if (!m_tasks.empty()) {
                            task = std::move(m_tasks.front());
                            m_tasks.pop();
                            ++m_active;
                        }
                    }
                    if (task && !m_terminate) {
                        try {
                            task();
                        }
                        catch (...) {
                            std::cerr << "Exception in thread pool task\n";
                        }
                        --m_active;
                    }
                }
                });
        }
    }

} // namespace ECS