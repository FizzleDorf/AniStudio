#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace Utils {

    // Base Task class with cancellation support
    class Task {
    public:
        Task() : cancelled(false), done(false) {}
        virtual ~Task() = default;

        virtual void execute() = 0;

        void cancel() { cancelled = true; }
        bool isCancelled() const { return cancelled; }
        bool isDone() const { return done; }

    protected:
        void markDone() { done = true; }

    private:
        std::atomic<bool> cancelled;
        std::atomic<bool> done;
    };

    // Thread Pool for managing worker threads
    class ThreadPool {
    public:
        ThreadPool(size_t numThreads = 0) {
            size_t threadCount = numThreads > 0 ? numThreads : std::thread::hardware_concurrency();
            startThreads(threadCount);
        }

        ~ThreadPool() {
            shutdown();
        }

        void addTask(std::shared_ptr<Task> task) {
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                tasks.push(task);
            }
            condition.notify_one();
        }

        size_t size() const {
            return workers.size();
        }

        size_t getQueueSize() const {
            std::unique_lock<std::mutex> lock(queueMutex);
            return tasks.size();
        }

        void shutdown() {
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                stop = true;
            }

            condition.notify_all();

            for (auto& worker : workers) {
                if (worker.joinable()) {
                    worker.join();
                }
            }

            workers.clear();
        }

    private:
        void startThreads(size_t numThreads) {
            stop = false;
            for (size_t i = 0; i < numThreads; ++i) {
                workers.emplace_back([this] {
                    while (true) {
                        std::shared_ptr<Task> task;

                        {
                            std::unique_lock<std::mutex> lock(queueMutex);
                            condition.wait(lock, [this] {
                                return stop || !tasks.empty();
                                });

                            if (stop && tasks.empty()) {
                                return;
                            }

                            task = tasks.front();
                            tasks.pop();
                        }

                        if (task && !task->isCancelled()) {
                            task->execute();
                        }
                    }
                    });
            }
        }

        std::vector<std::thread> workers;
        std::queue<std::shared_ptr<Task>> tasks;

        mutable std::mutex queueMutex;
        std::condition_variable condition;
        bool stop;
    };

} // namespace Utils