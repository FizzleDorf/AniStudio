#pragma once

#include "BaseSystem.hpp"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <iostream>

namespace ECS {

    class ThreadPoolSystem : public BaseSystem {
    public:
        explicit ThreadPoolSystem(EntityManager& mgr);
        ~ThreadPoolSystem() override;

        ThreadPoolSystem(const ThreadPoolSystem&) = delete;
        ThreadPoolSystem& operator=(const ThreadPoolSystem&) = delete;

        // Pool class
        class Pool {
        public:
            explicit Pool(size_t numThreads);
            ~Pool();

            Pool(const Pool&) = delete;
            Pool& operator=(const Pool&) = delete;

            template<typename F, typename... Args>
            auto submit(F&& f, Args&&... args)
                -> std::future<std::invoke_result_t<F, Args...>>;

            void terminate();
            void clearQueue();
            size_t queueSize() const;
            size_t activeCount() const;
            bool isTerminating() const;
            size_t size() const;

        private:
            void startThreads(size_t numThreads);

            std::vector<std::thread> m_workers;
            std::queue<std::function<void()>> m_tasks;
            mutable std::mutex m_mutex;
            std::condition_variable m_condition;
            std::atomic<bool> m_stop{ false };
            std::atomic<bool> m_terminate{ false };
            std::atomic<size_t> m_active{ 0 };
        };

        // Pool accessors
        Pool& getDiffusionPool() { return *m_diffusionPool; }
        Pool& getIOPool() { return *m_ioPool; }
        Pool& getGeneralPool() { return *m_generalPool; }

        // Submission helpers
        template<typename F, typename... Args>
        auto submitDiffusion(F&& f, Args&&... args)
            -> std::future<std::invoke_result_t<F, Args...>>;

        template<typename F, typename... Args>
        auto submitIO(F&& f, Args&&... args)
            -> std::future<std::invoke_result_t<F, Args...>>;

        template<typename F, typename... Args>
        auto submitGeneral(F&& f, Args&&... args)
            -> std::future<std::invoke_result_t<F, Args...>>;

        struct Stats {
            size_t diffusionActive;
            size_t diffusionQueued;
            size_t ioActive;
            size_t ioQueued;
            size_t generalActive;
            size_t generalQueued;
        };

        Stats getStats() const;

        void Start() override {}
        void Update(float) override {}
        void Destroy() override;

        void terminateAll();

    private:
        std::unique_ptr<Pool> m_diffusionPool;
        std::unique_ptr<Pool> m_ioPool;
        std::unique_ptr<Pool> m_generalPool;
    };


    template<typename F, typename... Args>
    auto ThreadPoolSystem::submitDiffusion(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>
    {
        return m_diffusionPool->submit(std::forward<F>(f), std::forward<Args>(args)...);
    }

    template<typename F, typename... Args>
    auto ThreadPoolSystem::submitIO(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>
    {
        return m_ioPool->submit(std::forward<F>(f), std::forward<Args>(args)...);
    }

    template<typename F, typename... Args>
    auto ThreadPoolSystem::submitGeneral(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>
    {
        return m_generalPool->submit(std::forward<F>(f), std::forward<Args>(args)...);
    }

    inline ThreadPoolSystem::Pool::Pool(size_t numThreads) {
        startThreads(numThreads > 0 ? numThreads : std::thread::hardware_concurrency());
    }

    inline ThreadPoolSystem::Pool::~Pool() {
        terminate();
    }

    template<typename F, typename... Args>
    auto ThreadPoolSystem::Pool::submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>
    {
        using ReturnType = std::invoke_result_t<F, Args...>;
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<ReturnType> result = task->get_future();
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_stop || m_terminate) {
                throw std::runtime_error("Cannot submit to stopped pool");
            }
            m_tasks.emplace([task]() { (*task)(); });
        }
        m_condition.notify_one();
        return result;
    }

    inline void ThreadPoolSystem::Pool::terminate() {
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

    inline void ThreadPoolSystem::Pool::clearQueue() {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (!m_tasks.empty()) m_tasks.pop();
    }

    inline size_t ThreadPoolSystem::Pool::queueSize() const {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_tasks.size();
    }

    inline size_t ThreadPoolSystem::Pool::activeCount() const {
        return m_active.load();
    }

    inline bool ThreadPoolSystem::Pool::isTerminating() const {
        return m_terminate.load();
    }

    inline size_t ThreadPoolSystem::Pool::size() const {
        return m_workers.size();
    }

    inline void ThreadPoolSystem::Pool::startThreads(size_t numThreads) {
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

    inline ThreadPoolSystem::ThreadPoolSystem(EntityManager& mgr)
        : BaseSystem(mgr)
    {
        size_t hw = std::thread::hardware_concurrency();
        m_diffusionPool = std::make_unique<Pool>(1);
        m_ioPool = std::make_unique<Pool>(2);
        m_generalPool = std::make_unique<Pool>(std::max<size_t>(2, hw / 2));
    }

    inline ThreadPoolSystem::~ThreadPoolSystem() {
        terminateAll();
    }

    inline void ThreadPoolSystem::Destroy() {
        terminateAll();
        BaseSystem::Destroy();
    }

    inline void ThreadPoolSystem::terminateAll() {
        if (m_diffusionPool) m_diffusionPool->terminate();
        if (m_ioPool) m_ioPool->terminate();
        if (m_generalPool) m_generalPool->terminate();
    }

    inline ThreadPoolSystem::Stats ThreadPoolSystem::getStats() const {
        return {
            m_diffusionPool->activeCount(),
            m_diffusionPool->queueSize(),
            m_ioPool->activeCount(),
            m_ioPool->queueSize(),
            m_generalPool->activeCount(),
            m_generalPool->queueSize()
        };
    }

} // namespace ECS