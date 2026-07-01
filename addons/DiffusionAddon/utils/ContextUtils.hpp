#pragma once

#include "stable-diffusion.h"
#include "pch.h"
#include "FilePathService.hpp"
#include "sdcpp_utils\SDContextUtil.hpp"
#include <iostream>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <memory>
#include <future>
#include <queue>

namespace Utils {

	struct SDContextCacheEntry {
		std::string cacheKey;
		sd_ctx_t* context;
		nlohmann::json metadata;
		std::chrono::steady_clock::time_point lastUsed;
		size_t useCount;
		bool isInUse;
		bool isLoading;
		std::future<sd_ctx_t*> loadingFuture;

		std::string modelPath;
		std::string vaePath;
		std::string clipLPath;
		std::string clipGPath;
		std::string clipVisionPath;
		std::string t5xxlPath;
		std::string llmPath;
		std::string llmVisionPath;
		std::string diffusionModelPath;
		std::string highNoiseModelPath;
		std::string taesdPath;
		std::string controlnetPath;
		std::string photoMakerPath;
		std::string tensorTypeRules;

		SDContextCacheEntry() : context(nullptr), useCount(0), isInUse(false), isLoading(false) {}
		~SDContextCacheEntry() {
			if (context) {
				free_sd_ctx(context);
				context = nullptr;
			}
		}
	};

	class SDContextManager {
	private:
		static inline std::unordered_map<std::string, std::shared_ptr<SDContextCacheEntry>> contextCache;
		static inline std::mutex cacheMutex;
		static inline size_t MAX_CACHE_SIZE = 3;
		static inline size_t totalContextsCreated = 0;
		static inline size_t totalContextsFailed = 0;

	public:
		static std::string GenerateCacheKey(const nlohmann::json& metadata) {
			try {
				std::string key;

				if (metadata.contains("components") && metadata["components"].is_array()) {
					for (const auto& comp : metadata["components"]) {
						if (comp.contains("Checkpoint")) {
							auto model = comp["Checkpoint"];
							if (model.contains("modelPath") && !model["modelPath"].is_null() && !model["modelPath"].get<std::string>().empty()) {
								key += model["modelPath"].get<std::string>();
							}
							else if (model.contains("modelName") && !model["modelName"].is_null() && !model["modelName"].get<std::string>().empty()) {
								key += model["modelName"].get<std::string>();
							}
						}

						if (comp.contains("Vae")) {
							auto vae = comp["Vae"];
							if (vae.contains("modelPath") && !vae["modelPath"].is_null() && !vae["modelPath"].get<std::string>().empty()) {
								key += "|vae:" + vae["modelPath"].get<std::string>();
							}
							else if (vae.contains("modelName") && !vae["modelName"].is_null() && !vae["modelName"].get<std::string>().empty()) {
								key += "|vae:" + vae["modelName"].get<std::string>();
							}
						}

						if (comp.contains("ClipL")) {
							auto clipL = comp["ClipL"];
							if (clipL.contains("modelPath") && !clipL["modelPath"].is_null() && !clipL["modelPath"].get<std::string>().empty()) {
								key += "|clipL:" + clipL["modelPath"].get<std::string>();
							}
							else if (clipL.contains("modelName") && !clipL["modelName"].is_null() && !clipL["modelName"].get<std::string>().empty()) {
								key += "|clipL:" + clipL["modelName"].get<std::string>();
							}
						}

						if (comp.contains("ClipG")) {
							auto clipG = comp["ClipG"];
							if (clipG.contains("modelPath") && !clipG["modelPath"].is_null() && !clipG["modelPath"].get<std::string>().empty()) {
								key += "|clipG:" + clipG["modelPath"].get<std::string>();
							}
							else if (clipG.contains("modelName") && !clipG["modelName"].is_null() && !clipG["modelName"].get<std::string>().empty()) {
								key += "|clipG:" + clipG["modelName"].get<std::string>();
							}
						}

						if (comp.contains("LlmEncoder")) {
							auto llm = comp["LlmEncoder"];
							if (llm.contains("modelPath") && !llm["modelPath"].is_null() && !llm["modelPath"].get<std::string>().empty()) {
								key += "|llm:" + llm["modelPath"].get<std::string>();
							}
							else if (llm.contains("modelName") && !llm["modelName"].is_null() && !llm["modelName"].get<std::string>().empty()) {
								key += "|llm:" + llm["modelName"].get<std::string>();
							}
						}

						if (comp.contains("LlmVision")) {
							auto llmVision = comp["LlmVision"];
							if (llmVision.contains("modelPath") && !llmVision["modelPath"].is_null() && !llmVision["modelPath"].get<std::string>().empty()) {
								key += "|llmVision:" + llmVision["modelPath"].get<std::string>();
							}
							else if (llmVision.contains("modelName") && !llmVision["modelName"].is_null() && !llmVision["modelName"].get<std::string>().empty()) {
								key += "|llmVision:" + llmVision["modelName"].get<std::string>();
							}
						}
					}
				}

				if (metadata.contains("components")) {
					for (const auto& comp : metadata["components"]) {
						if (comp.contains("Sampler")) {
							auto sampler = comp["Sampler"];
							if (sampler.contains("n_threads")) {
								key += "|threads:" + std::to_string(sampler["n_threads"].get<int>());
							}
							if (sampler.contains("current_type_method")) {
								key += "|wtype:" + std::to_string(sampler["current_type_method"].get<int>());
							}
						}
					}
				}

				if (key.empty()) {
					key = std::to_string(std::hash<std::string>{}(metadata.dump()));
				}

				return key;
			}
			catch (const std::exception& e) {
				std::cerr << "Error generating cache key: " << e.what() << std::endl;
				return "error_key";
			}
		}

		static bool CanReuseContext(const nlohmann::json& cachedMetadata,
			const nlohmann::json& newMetadata) {
			try {
				auto getModelPaths = [](const nlohmann::json& metadata) -> std::vector<std::string> {
					std::vector<std::string> paths;
					if (metadata.contains("components") && metadata["components"].is_array()) {
						for (const auto& comp : metadata["components"]) {
							if (comp.contains("Checkpoint")) {
								auto model = comp["Checkpoint"];
								if (model.contains("modelPath") && !model["modelPath"].is_null() && !model["modelPath"].get<std::string>().empty()) {
									paths.push_back(model["modelPath"].get<std::string>());
								}
							}
							if (comp.contains("Vae")) {
								auto vae = comp["Vae"];
								if (vae.contains("modelPath") && !vae["modelPath"].is_null() && !vae["modelPath"].get<std::string>().empty()) {
									paths.push_back(vae["modelPath"].get<std::string>());
								}
							}
							if (comp.contains("ClipL")) {
								auto clipL = comp["ClipL"];
								if (clipL.contains("modelPath") && !clipL["modelPath"].is_null() && !clipL["modelPath"].get<std::string>().empty()) {
									paths.push_back(clipL["modelPath"].get<std::string>());
								}
							}
							if (comp.contains("ClipG")) {
								auto clipG = comp["ClipG"];
								if (clipG.contains("modelPath") && !clipG["modelPath"].is_null() && !clipG["modelPath"].get<std::string>().empty()) {
									paths.push_back(clipG["modelPath"].get<std::string>());
								}
							}
							if (comp.contains("LlmEncoder")) {
								auto llm = comp["LlmEncoder"];
								if (llm.contains("modelPath") && !llm["modelPath"].is_null() && !llm["modelPath"].get<std::string>().empty()) {
									paths.push_back(llm["modelPath"].get<std::string>());
								}
							}
							if (comp.contains("LlmVision")) {
								auto llmVision = comp["LlmVision"];
								if (llmVision.contains("modelPath") && !llmVision["modelPath"].is_null() && !llmVision["modelPath"].get<std::string>().empty()) {
									paths.push_back(llmVision["modelPath"].get<std::string>());
								}
							}
						}
					}
					return paths;
					};

				auto cachedPaths = getModelPaths(cachedMetadata);
				auto newPaths = getModelPaths(newMetadata);

				if (cachedPaths.size() != newPaths.size()) {
					return false;
				}

				for (size_t i = 0; i < cachedPaths.size(); ++i) {
					if (cachedPaths[i] != newPaths[i]) {
						return false;
					}
				}

				auto getSamplerSettings = [](const nlohmann::json& metadata) -> std::pair<int, int> {
					int n_threads = std::thread::hardware_concurrency();
					int wtype = 0;

					if (metadata.contains("components")) {
						for (const auto& comp : metadata["components"]) {
							if (comp.contains("Sampler")) {
								auto sampler = comp["Sampler"];
								if (sampler.contains("n_threads")) {
									n_threads = sampler["n_threads"].get<int>();
								}
								if (sampler.contains("current_type_method")) {
									wtype = sampler["current_type_method"].get<int>();
								}
							}
						}
					}
					return { n_threads, wtype };
					};

				auto cachedSettings = getSamplerSettings(cachedMetadata);
				auto newSettings = getSamplerSettings(newMetadata);

				return (cachedSettings == newSettings);
			}
			catch (const std::exception& e) {
				std::cerr << "Error checking context reuse: " << e.what() << std::endl;
				return false;
			}
		}

		static sd_ctx_t* CreateNewContextInternal(const nlohmann::json& metadata,
			std::shared_ptr<SDContextCacheEntry> entry) {
			try {
				sd_ctx_params_t ctx_params;

				if (!ParseContextParams(metadata, ctx_params,
					entry->modelPath, entry->vaePath, entry->clipLPath, entry->clipGPath,
					entry->clipVisionPath, entry->t5xxlPath, entry->llmPath, entry->llmVisionPath,
					entry->diffusionModelPath, entry->highNoiseModelPath, entry->taesdPath,
					entry->controlnetPath, entry->photoMakerPath, entry->tensorTypeRules)) {
					std::cerr << "ERROR: Failed to parse context parameters!" << std::endl;
					return nullptr;
				}

				std::cout << "DEBUG: Creating SD context with paths:" << std::endl;
				std::cout << "  model_path: " << (ctx_params.model_path ? ctx_params.model_path : "(empty)") << std::endl;
				std::cout << "  vae_path: " << (ctx_params.vae_path ? ctx_params.vae_path : "(empty)") << std::endl;
				std::cout << "  clip_l_path: " << (ctx_params.clip_l_path ? ctx_params.clip_l_path : "(empty)") << std::endl;
				std::cout << "  clip_g_path: " << (ctx_params.clip_g_path ? ctx_params.clip_g_path : "(empty)") << std::endl;
				std::cout << "  llm_path: " << (ctx_params.llm_path ? ctx_params.llm_path : "(empty)") << std::endl;
				std::cout << "  llm_vision_path: " << (ctx_params.llm_vision_path ? ctx_params.llm_vision_path : "(empty)") << std::endl;

				sd_ctx_t* ctx = new_sd_ctx(&ctx_params);

				if (!ctx) {
					std::cerr << "ERROR: Failed to create SD context!" << std::endl;
					return nullptr;
				}

				std::cout << "DEBUG: SD context created successfully!" << std::endl;
				return ctx;
			}
			catch (const std::exception& e) {
				std::cerr << "Error creating SD context: " << e.what() << std::endl;
				return nullptr;
			}
		}

		static std::future<sd_ctx_t*> CreateNewContextAsync(std::shared_ptr<SDContextCacheEntry> entry) {
			return std::async(std::launch::async, [entry]() -> sd_ctx_t* {
				return CreateNewContextInternal(entry->metadata, entry);
				});
		}

	public:
		static sd_ctx_t* CreateNewContext(const nlohmann::json& metadata) {
			auto entry = std::make_shared<SDContextCacheEntry>();
			entry->metadata = metadata;
			return CreateNewContextInternal(metadata, entry);
		}

		static sd_ctx_t* GetOrCreateContext(const nlohmann::json& metadata) {
			std::string cacheKey = GenerateCacheKey(metadata);

			{
				std::lock_guard<std::mutex> lock(cacheMutex);

				auto it = contextCache.find(cacheKey);
				if (it != contextCache.end()) {
					auto& entry = it->second;

					if (entry->isLoading) {
						if (entry->loadingFuture.valid()) {
							auto status = entry->loadingFuture.wait_for(std::chrono::milliseconds(0));
							if (status == std::future_status::ready) {
								try {
									entry->context = entry->loadingFuture.get();
									entry->isLoading = false;

									if (!entry->context) {
										std::cout << "DEBUG: Async context loading failed for: " << cacheKey << std::endl;
										contextCache.erase(it);
										totalContextsFailed++;
										return nullptr;
									}

									std::cout << "DEBUG: Async context loading completed for: " << cacheKey << std::endl;
								}
								catch (const std::exception& e) {
									std::cerr << "Error getting async context result: " << e.what() << std::endl;
									contextCache.erase(it);
									totalContextsFailed++;
									return nullptr;
								}
							}
							else {
								std::cout << "DEBUG: Context is still loading: " << cacheKey << std::endl;
								entry->isInUse = true;
								return nullptr;
							}
						}
					}

					if (entry->context) {
						if (CanReuseContext(entry->metadata, metadata)) {
							entry->lastUsed = std::chrono::steady_clock::now();
							entry->useCount++;
							entry->isInUse = true;
							std::cout << "DEBUG: Reusing cached SD context: " << cacheKey
								<< " (use count: " << entry->useCount << ")" << std::endl;
							return entry->context;
						}
						else {
							std::cout << "DEBUG: Metadata differs, removing old cache entry: " << cacheKey << std::endl;
							contextCache.erase(it);
						}
					}
				}
			}

			std::shared_ptr<SDContextCacheEntry> newEntry;
			{
				std::lock_guard<std::mutex> lock(cacheMutex);

				if (contextCache.size() >= MAX_CACHE_SIZE) {
					auto oldest = contextCache.begin();
					auto oldestTime = oldest->second->lastUsed;

					for (auto it = contextCache.begin(); it != contextCache.end(); ++it) {
						if (it->second->lastUsed < oldestTime && !it->second->isInUse && !it->second->isLoading) {
							oldest = it;
							oldestTime = it->second->lastUsed;
						}
					}

					if (oldest != contextCache.end() && !oldest->second->isInUse && !oldest->second->isLoading) {
						std::cout << "DEBUG: Removing LRU cache entry: " << oldest->first << std::endl;
						contextCache.erase(oldest);
					}
				}

				newEntry = std::make_shared<SDContextCacheEntry>();
				newEntry->cacheKey = cacheKey;
				newEntry->metadata = metadata;
				newEntry->lastUsed = std::chrono::steady_clock::now();
				newEntry->useCount = 1;
				newEntry->isInUse = true;
				newEntry->isLoading = true;

				newEntry->loadingFuture = CreateNewContextAsync(newEntry);

				contextCache[cacheKey] = newEntry;
				totalContextsCreated++;

				std::cout << "DEBUG: Started async loading for SD context: " << cacheKey
					<< " (total created: " << totalContextsCreated << ")" << std::endl;
			}

			return nullptr;
		}

		static bool IsContextLoading(const nlohmann::json& metadata) {
			std::string cacheKey = GenerateCacheKey(metadata);
			std::lock_guard<std::mutex> lock(cacheMutex);

			auto it = contextCache.find(cacheKey);
			if (it != contextCache.end()) {
				return it->second->isLoading;
			}
			return false;
		}

		static sd_ctx_t* TryGetLoadedContext(const nlohmann::json& metadata) {
			std::string cacheKey = GenerateCacheKey(metadata);
			std::lock_guard<std::mutex> lock(cacheMutex);

			auto it = contextCache.find(cacheKey);
			if (it != contextCache.end() && it->second->isLoading) {
				auto& entry = it->second;
				if (entry->loadingFuture.valid()) {
					auto status = entry->loadingFuture.wait_for(std::chrono::milliseconds(0));
					if (status == std::future_status::ready) {
						try {
							entry->context = entry->loadingFuture.get();
							entry->isLoading = false;

							if (!entry->context) {
								std::cout << "DEBUG: Async context loading failed for: " << cacheKey << std::endl;
								contextCache.erase(it);
								totalContextsFailed++;
								return nullptr;
							}

							entry->isInUse = true;
							std::cout << "DEBUG: Async context loading completed for: " << cacheKey << std::endl;
							return entry->context;
						}
						catch (const std::exception& e) {
							std::cerr << "Error getting async context result: " << e.what() << std::endl;
							contextCache.erase(it);
							totalContextsFailed++;
							return nullptr;
						}
					}
				}
			}
			return nullptr;
		}

		static void ReleaseContext(sd_ctx_t* context) {
			std::lock_guard<std::mutex> lock(cacheMutex);

			for (auto& entry : contextCache) {
				if (entry.second->context == context) {
					entry.second->lastUsed = std::chrono::steady_clock::now();
					entry.second->isInUse = false;
					std::cout << "DEBUG: Released context: " << entry.first
						<< " (total cached: " << contextCache.size() << ")" << std::endl;
					return;
				}
			}

			std::cerr << "WARNING: Context not found in cache, freeing directly" << std::endl;
			free_sd_ctx(context);
		}

		static void ForceFreeContext(sd_ctx_t* context) {
			std::lock_guard<std::mutex> lock(cacheMutex);

			for (auto it = contextCache.begin(); it != contextCache.end(); ++it) {
				if (it->second->context == context) {
					std::cout << "DEBUG: Force freeing context: " << it->first << std::endl;
					contextCache.erase(it);
					return;
				}
			}

			free_sd_ctx(context);
		}

		static void ClearAllContexts() {
			std::lock_guard<std::mutex> lock(cacheMutex);

			size_t count = contextCache.size();
			contextCache.clear();
			std::cout << "DEBUG: Cleared all cached contexts (" << count << " contexts)" << std::endl;
		}

		static void GetCacheStats(size_t& totalCached, size_t& inUse, size_t& available) {
			std::lock_guard<std::mutex> lock(cacheMutex);

			totalCached = contextCache.size();
			inUse = 0;
			available = 0;

			for (const auto& entry : contextCache) {
				if (entry.second->isInUse) {
					inUse++;
				}
				else if (!entry.second->isLoading) {
					available++;
				}
			}
		}

		static void GetLoadingStats(size_t& loadingCount, size_t& failedCount) {
			std::lock_guard<std::mutex> lock(cacheMutex);

			loadingCount = 0;
			for (const auto& entry : contextCache) {
				if (entry.second->isLoading) {
					loadingCount++;
				}
			}

			failedCount = totalContextsFailed;
		}

		static void ListCachedContexts() {
			std::lock_guard<std::mutex> lock(cacheMutex);

			std::cout << "=== Cached SD Contexts ===" << std::endl;
			std::cout << "Total contexts: " << contextCache.size() << " (max: " << MAX_CACHE_SIZE << ")" << std::endl;
			std::cout << "Total created: " << totalContextsCreated << std::endl;
			std::cout << "Total failed: " << totalContextsFailed << std::endl;

			for (const auto& entry : contextCache) {
				auto& cacheEntry = entry.second;
				auto age = std::chrono::duration_cast<std::chrono::seconds>(
					std::chrono::steady_clock::now() - cacheEntry->lastUsed);

				std::cout << "  Key: " << entry.first << std::endl;
				std::cout << "    Use count: " << cacheEntry->useCount << std::endl;
				std::cout << "    In use: " << (cacheEntry->isInUse ? "yes" : "no") << std::endl;
				std::cout << "    Loading: " << (cacheEntry->isLoading ? "yes" : "no") << std::endl;
				std::cout << "    Age: " << age.count() << " seconds" << std::endl;

				if (cacheEntry->metadata.contains("components")) {
					for (const auto& comp : cacheEntry->metadata["components"]) {
						if (comp.contains("Checkpoint")) {
							auto model = comp["Checkpoint"];
							if (model.contains("modelPath")) {
								std::cout << "    Model: " << model["modelPath"].get<std::string>() << std::endl;
							}
						}
					}
				}
				std::cout << std::endl;
			}
			std::cout << "==========================" << std::endl;
		}

		static void UnloadSpecificModel(const std::string& modelPath) {
			std::lock_guard<std::mutex> lock(cacheMutex);

			std::cout << "DEBUG: Looking for contexts using model: " << modelPath << std::endl;

			std::vector<std::string> toRemove;
			for (const auto& entry : contextCache) {
				try {
					bool usesModel = false;

					if (entry.second->metadata.contains("components") &&
						entry.second->metadata["components"].is_array()) {
						for (const auto& comp : entry.second->metadata["components"]) {
							if (comp.contains("Checkpoint")) {
								auto model = comp["Checkpoint"];
								if (model.contains("modelPath") &&
									!model["modelPath"].is_null() &&
									model["modelPath"].get<std::string>().find(modelPath) != std::string::npos) {
									usesModel = true;
									break;
								}
								if (model.contains("modelName") &&
									!model["modelName"].is_null() &&
									model["modelName"].get<std::string>().find(modelPath) != std::string::npos) {
									usesModel = true;
									break;
								}
							}
						}
					}

					if (usesModel) {
						std::cout << "DEBUG: Found context using model: " << entry.first << std::endl;
						toRemove.push_back(entry.first);
					}
				}
				catch (const std::exception& e) {
					std::cerr << "Error checking model usage: " << e.what() << std::endl;
				}
			}

			for (const auto& key : toRemove) {
				contextCache.erase(key);
				std::cout << "DEBUG: Removed context: " << key << std::endl;
			}
		}

		static void UnloadAllModels() {
			ClearAllContexts();
		}

		static void SetMaxCacheSize(size_t size) {
			std::lock_guard<std::mutex> lock(cacheMutex);
			MAX_CACHE_SIZE = size;
			std::cout << "DEBUG: Max cache size set to: " << MAX_CACHE_SIZE << std::endl;
		}

		static size_t GetMaxCacheSize() {
			std::lock_guard<std::mutex> lock(cacheMutex);
			return MAX_CACHE_SIZE;
		}

		static bool IsModelLoaded(const std::string& modelPath) {
			std::lock_guard<std::mutex> lock(cacheMutex);

			for (const auto& entry : contextCache) {
				try {
					if (entry.second->metadata.contains("components") &&
						entry.second->metadata["components"].is_array()) {
						for (const auto& comp : entry.second->metadata["components"]) {
							if (comp.contains("Checkpoint")) {
								auto model = comp["Checkpoint"];
								if (model.contains("modelPath") &&
									!model["modelPath"].is_null() &&
									model["modelPath"].get<std::string>().find(modelPath) != std::string::npos) {
									return true;
								}
								if (model.contains("modelName") &&
									!model["modelName"].is_null() &&
									model["modelName"].get<std::string>().find(modelPath) != std::string::npos) {
									return true;
								}
							}
						}
					}
				}
				catch (const std::exception& e) {
				}
			}

			return false;
		}

		static std::vector<std::string> GetLoadedModels() {
			std::lock_guard<std::mutex> lock(cacheMutex);
			std::vector<std::string> models;

			for (const auto& entry : contextCache) {
				try {
					if (entry.second->metadata.contains("components") &&
						entry.second->metadata["components"].is_array()) {
						for (const auto& comp : entry.second->metadata["components"]) {
							if (comp.contains("Checkpoint")) {
								auto model = comp["Checkpoint"];
								std::string modelInfo;
								if (model.contains("modelPath") && !model["modelPath"].is_null() && !model["modelPath"].get<std::string>().empty()) {
									modelInfo = model["modelPath"].get<std::string>();
								}
								else if (model.contains("modelName") && !model["modelName"].is_null() && !model["modelName"].get<std::string>().empty()) {
									modelInfo = model["modelName"].get<std::string>();
								}

								if (!modelInfo.empty()) {
									modelInfo += " [Cache: " + entry.first + "]";
									models.push_back(modelInfo);
								}
							}
						}
					}
				}
				catch (const std::exception& e) {
				}
			}

			return models;
		}
	};

	inline sd_ctx_t* InitializeStableDiffusionContext(const nlohmann::json& metadata) {
		return SDContextManager::GetOrCreateContext(metadata);
	}

}