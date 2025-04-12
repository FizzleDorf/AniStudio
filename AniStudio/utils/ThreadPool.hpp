/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license iformation, please contact legal@kframe.ai.
 */

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

	// Singleton ThreadPool Manager
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

		// Shutdown all pools gracefully
		void shutdown() {
			std::cout << "Shutting down ThreadPoolManager..." << std::endl;

			// Wait for all pools to complete their tasks
			diffusionPool->waitForTasks();
			ioPool->waitForTasks();
			generalPool->waitForTasks();

			std::cout << "All thread pools shut down successfully." << std::endl;
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
			generalPool = std::make_unique<ThreadPool>(std::max(2u, std::thread::hardware_concurrency()/2)); // Rest for general use

			std::cout << "ThreadPoolManager initialized with:" << std::endl;
			std::cout << "  Diffusion pool: " << diffusionPool->size() << " threads" << std::endl;
			std::cout << "  I/O pool: " << ioPool->size() << " threads" << std::endl;
			std::cout << "  General pool: " << generalPool->size() << " threads" << std::endl;
		}

		// Destructor
		~ThreadPoolManager() {
			shutdown();
		}

		// Thread pools
		std::unique_ptr<ThreadPool> diffusionPool;
		std::unique_ptr<ThreadPool> ioPool;
		std::unique_ptr<ThreadPool> generalPool;
	};

} // namespace Utils