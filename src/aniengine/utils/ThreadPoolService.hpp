// ThreadPool.hpp - Modified for immediate termination
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
#include <iostream>

namespace Utils {

	class ThreadPool {
	public:
		// Constructor: initialize the thread pool with hardware concurrency by default
		ThreadPool(size_t numThreads = 0) : stop(false), terminateImmediately(false), activeThreads(0) {
			size_t threadCount = numThreads > 0 ? numThreads : std::thread::hardware_concurrency();
			startThreads(threadCount);
		}

		// Destructor: immediate termination
		~ThreadPool() {
			terminate();
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
				if (stop || terminateImmediately) {
					throw std::runtime_error("Cannot enqueue task on stopped ThreadPool");
				}

				// Wrap the packaged task in a void function
				tasks.emplace([task]() { (*task)(); });
			}

			// Notify a waiting thread
			condition.notify_one();
			return result;
		}

		// Immediate termination - clear queue and stop threads
		void terminate() {
			{
				std::unique_lock<std::mutex> lock(queueMutex);
				terminateImmediately = true;
				stop = true;

				// Clear all pending tasks
				while (!tasks.empty()) {
					tasks.pop();
				}
			}

			// Notify all threads to wake up and exit
			condition.notify_all();

			// Join all threads
			for (auto& worker : workers) {
				if (worker.joinable()) {
					worker.join();
				}
			}

			workers.clear();
		}

		// Graceful shutdown (if needed occasionally)
		void shutdownGracefully() {
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

		// Check if termination was requested
		bool isTerminating() const {
			return terminateImmediately;
		}

		// Clear all pending tasks immediately
		void clearQueue() {
			std::unique_lock<std::mutex> lock(queueMutex);
			while (!tasks.empty()) {
				tasks.pop();
			}
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
								return terminateImmediately || stop || !tasks.empty();
							});

							// Exit immediately if termination requested
							if (terminateImmediately) {
								return;
							}

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

						// Execute the task (unless terminating)
						if (task && !terminateImmediately) {
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

							// Decrement active count
							{
								std::unique_lock<std::mutex> lock(queueMutex);
								--activeThreads;
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

		// Status flags
		std::atomic<bool> stop;
		std::atomic<bool> terminateImmediately;
		std::atomic<size_t> activeThreads;
	};

	// Singleton ThreadPool Manager with immediate termination support
	class ThreadPoolManager {
	public:
		enum class PoolType {
			DIFFUSION,    // For AI diffusion tasks (single threaded)
			IO,          // For file I/O operations 
			GENERAL      // For general background tasks
		};

		// Get singleton instance
		static ThreadPoolManager& getInstance() {
			static ThreadPoolManager instance;
			return instance;
		}

		// Get a specific thread pool
		ThreadPool& getPool(PoolType type) {
			switch (type) {
			case PoolType::DIFFUSION:
				return *diffusionPool;
			case PoolType::IO:
				return *ioPool;
			case PoolType::GENERAL:
				return *generalPool;
			default:
				throw std::invalid_argument("Unknown pool type");
			}
		}

		// Convenience methods for each pool
		ThreadPool& getDiffusionPool() { return *diffusionPool; }
		ThreadPool& getIOPool() { return *ioPool; }
		ThreadPool& getGeneralPool() { return *generalPool; }

		// Get stats for all pools
		struct PoolStats {
			size_t diffusionActive;
			size_t diffusionQueued;
			size_t ioActive;
			size_t ioQueued;
			size_t generalActive;
			size_t generalQueued;
		};

		PoolStats getStats() const {
			return {
				diffusionPool->getActiveCount(),
				diffusionPool->getQueueSize(),
				ioPool->getActiveCount(),
				ioPool->getQueueSize(),
				generalPool->getActiveCount(),
				generalPool->getQueueSize()
			};
		}

		// IMMEDIATE TERMINATION - Clear all queues and stop all threads
		void terminateAll() {
			std::cout << "TERMINATING ALL THREAD POOLS IMMEDIATELY..." << std::endl;

			// Terminate in reverse order (general first, diffusion last)
			if (generalPool) {
				std::cout << "Terminating general pool..." << std::endl;
				generalPool->terminate();
			}

			if (ioPool) {
				std::cout << "Terminating I/O pool..." << std::endl;
				ioPool->terminate();
			}

			if (diffusionPool) {
				std::cout << "Terminating diffusion pool..." << std::endl;
				diffusionPool->terminate();
			}

			std::cout << "All thread pools terminated immediately." << std::endl;
		}

		// Clear all task queues without terminating threads
		void clearAllQueues() {
			if (diffusionPool) diffusionPool->clearQueue();
			if (ioPool) ioPool->clearQueue();
			if (generalPool) generalPool->clearQueue();
			std::cout << "Cleared all thread pool queues." << std::endl;
		}

		// Graceful shutdown (if needed)
		void shutdownGracefully() {
			std::cout << "Shutting down ThreadPoolManager gracefully..." << std::endl;
			if (diffusionPool) diffusionPool->shutdownGracefully();
			if (ioPool) ioPool->shutdownGracefully();
			if (generalPool) generalPool->shutdownGracefully();
			std::cout << "All thread pools shut down gracefully." << std::endl;
		}

		// Delete copy and move operations for singleton
		ThreadPoolManager(const ThreadPoolManager&) = delete;
		ThreadPoolManager& operator=(const ThreadPoolManager&) = delete;
		ThreadPoolManager(ThreadPoolManager&&) = delete;
		ThreadPoolManager& operator=(ThreadPoolManager&&) = delete;

	private:
		// Private constructor for singleton
		ThreadPoolManager() {
			// Initialize pools with appropriate thread counts
			diffusionPool = std::make_unique<ThreadPool>(1);  // Single thread for diffusion
			ioPool = std::make_unique<ThreadPool>(2);         // 2 threads for I/O
			generalPool = std::make_unique<ThreadPool>(std::max(2u, std::thread::hardware_concurrency() / 2)); // Rest for general use

			std::cout << "ThreadPoolManager initialized with:" << std::endl;
			std::cout << "  Diffusion pool: " << diffusionPool->size() << " threads" << std::endl;
			std::cout << "  I/O pool: " << ioPool->size() << " threads" << std::endl;
			std::cout << "  General pool: " << generalPool->size() << " threads" << std::endl;
		}

		// Destructor - immediate termination
		~ThreadPoolManager() {
			terminateAll();
		}

		// Thread pools
		std::unique_ptr<ThreadPool> diffusionPool;
		std::unique_ptr<ThreadPool> ioPool;
		std::unique_ptr<ThreadPool> generalPool;
	};

} // namespace Utils