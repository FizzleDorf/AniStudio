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

        Pool& getDiffusionPool() { return *m_diffusionPool; }
        Pool& getIOPool() { return *m_ioPool; }
        Pool& getGeneralPool() { return *m_generalPool; }

        template<typename F, typename... Args>
        auto submitDiffusion(F&& f, Args&&... args)
            -> std::future<std::invoke_result_t<F, Args...>>;

        template<typename F, typename... Args>
        auto submitIO(F&& f, Args&&... args)
            -> std::future<std::invoke_result_t<F, Args...>>;

        template<typename F, typename... Args>
        auto submitGeneral(F&& f, Args&&... args)
            -> std::future<std::invoke_result_t<F, Args...>>;

        std::future<bool> submitDiffusionTask(std::function<bool()> job);

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

} // namespace ECS