#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace Utils {

    // Custom task wrapper that can be safely copied (unlike std::future)
    class Task {
    public:
        Task() : _done(false), _cancelled(false) {}
        virtual ~Task() = default;

        // Execute the task
        virtual void execute() = 0;

        // Check if task is done
        bool isDone() const { return _done; }

        // Check if task was cancelled
        bool isCancelled() const { return _cancelled; }

        // Cancel the task
        void cancel() { _cancelled = true; }

    protected:
        void markDone() { _done = true; }

    private:
        std::atomic<bool> _done;
        std::atomic<bool> _cancelled;
    };

    // Thread pool for executing tasks in parallel
    class ThreadPool {
    public:
        ThreadPool(size_t numThreads = std::thread::hardware_concurrency()) : stop(false) {
            // Create worker threads
            for (size_t i = 0; i < numThreads; ++i) {
                workers.emplace_back([this] {
                    workerLoop();
                    });
            }
        }

        ~ThreadPool() {
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                stop = true;
            }

            condition.notify_all();

            for (std::thread& worker : workers) {
                if (worker.joinable()) {
                    worker.join();
                }
            }
        }

        // Disable copying
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        // Add a task to the pool
        void addTask(std::shared_ptr<Task> task) {
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                if (stop) {
                    throw std::runtime_error("Cannot add task to stopped ThreadPool");
                }
                tasks.push(task);
            }
            condition.notify_one();
        }

        // Get the number of worker threads
        size_t size() const {
            return workers.size();
        }

        // Get the number of tasks in the queue
        size_t getQueueSize() const {
            std::unique_lock<std::mutex> lock(queueMutex);
            return tasks.size();
        }

    private:
        // Worker thread main loop
        void workerLoop() {
            while (true) {
                std::shared_ptr<Task> task;

                {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    condition.wait(lock, [this] { return stop || !tasks.empty(); });

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
        }

        // Worker threads
        std::vector<std::thread> workers;

        // Task queue
        std::queue<std::shared_ptr<Task>> tasks;

        // Synchronization
        mutable std::mutex queueMutex;
        std::condition_variable condition;

        // Flag to signal threads to stop
        std::atomic<bool> stop;
    };

} // namespace Utils