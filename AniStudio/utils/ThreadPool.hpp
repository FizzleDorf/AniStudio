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
#include <type_traits>

namespace Utils {

	class ThreadPool {
	public:
		// Constructor: initialize the thread pool with hardware concurrency by default
		ThreadPool(size_t numThreads = 0) : stop(false), activeThreads(0) {
			size_t threadCount = numThreads > 0 ? numThreads : std::thread::hardware_concurrency();
			startThreads(threadCount);
		}

		// Destructor: clean shutdown
		~ThreadPool() {
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
		}

		// Delete copy and move constructors/assignments
		ThreadPool(const ThreadPool&) = delete;
		ThreadPool& operator=(const ThreadPool&) = delete;
		ThreadPool(ThreadPool&&) = delete;
		ThreadPool& operator=(ThreadPool&&) = delete;

		// Submit a function with args and get a future for the result
		template<typename F, typename... Args>
		auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
			using ReturnType = std::invoke_result_t<F, Args...>;

			// Create a shared pointer to a packaged task
			auto task = std::make_shared<std::packaged_task<ReturnType()>>(
				std::bind(std::forward<F>(f), std::forward<Args>(args)...)
				);

			// Get the future before we move the task
			std::future<ReturnType> result = task->get_future();

			// Add task to the queue
			{
				std::unique_lock<std::mutex> lock(queueMutex);

				// Don't allow enqueueing after stopping the pool
				if (stop) {
					throw std::runtime_error("Cannot enqueue task on stopped ThreadPool");
				}

				// Wrap the packaged task in a void function
				tasks.emplace([task]() { (*task)(); });
			}

			// Notify a waiting thread
			condition.notify_one();
			return result;
		}

		// Size accessor
		size_t size() const {
			return workers.size();
		}

		// Queue size accessor
		size_t getQueueSize() const {
			std::unique_lock<std::mutex> lock(queueMutex);
			return tasks.size();
		}

		// Number of active threads
		size_t getActiveCount() const {
			return activeThreads;
		}

		// Wait for all currently running tasks to complete
		void waitForTasks() {
			std::unique_lock<std::mutex> lock(queueMutex);
			completionCondition.wait(lock, [this]() {
				return (activeThreads == 0) && tasks.empty();
			});
		}

	private:
		void startThreads(size_t numThreads) {
			for (size_t i = 0; i < numThreads; ++i) {
				workers.emplace_back([this] {
					while (true) {
						std::function<void()> task;

						{
							std::unique_lock<std::mutex> lock(queueMutex);

							// Wait for a task or stop signal
							condition.wait(lock, [this] {
								return stop || !tasks.empty();
							});

							// Exit if stopped and no more tasks
							if (stop && tasks.empty()) {
								return;
							}

							// Get the next task
							if (!tasks.empty()) {
								task = std::move(tasks.front());
								tasks.pop();
								++activeThreads;
							}
						}

						// Execute the task
						if (task) {
							try {
								task();
							}
							catch (const std::exception& e) {
								// Log exception but don't crash the thread
								std::cerr << "Exception in thread pool task: " << e.what() << std::endl;
							}
							catch (...) {
								std::cerr << "Unknown exception in thread pool task" << std::endl;
							}

							// Decrement active count and notify waiters
							{
								std::unique_lock<std::mutex> lock(queueMutex);
								--activeThreads;
								if (tasks.empty() && activeThreads == 0) {
									completionCondition.notify_all();
								}
							}
						}
					}
				});
			}
		}

		// Thread workers
		std::vector<std::thread> workers;

		// Task queue
		std::queue<std::function<void()>> tasks;

		// Synchronization
		mutable std::mutex queueMutex;
		std::condition_variable condition;
		std::condition_variable completionCondition;

		// Status flags
		bool stop;
		std::atomic<size_t> activeThreads;
	};

} // namespace Utils