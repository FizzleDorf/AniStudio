#pragma once

#include "stable-diffusion.h"
#include "pch.h"
#include "FilePathService.hpp"
#include <iostream>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <memory>
#include <future>
#include <queue>

namespace Utils {

	// Cache entry with metadata and context
	struct SDContextCacheEntry {
		std::string cacheKey;
		sd_ctx_t* context;
		nlohmann::json metadata;
		std::chrono::steady_clock::time_point lastUsed;
		size_t useCount;
		bool isInUse;
		bool isLoading; // NEW: Track if context is currently loading
		std::future<sd_ctx_t*> loadingFuture; // NEW: Future for async loading

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
		// INLINE static members (C++17+) - no separate .cpp file needed
		static inline std::unordered_map<std::string, std::shared_ptr<SDContextCacheEntry>> contextCache;
		static inline std::mutex cacheMutex;
		static inline size_t MAX_CACHE_SIZE = 3; // Maximum number of cached contexts (made non-const)
		static inline std::atomic<size_t> totalContextsCreated{ 0 };
		static inline std::atomic<size_t> totalContextsFailed{ 0 };

	public:
		// Generate a cache key from metadata
		static std::string GenerateCacheKey(const nlohmann::json& metadata) {
			try {
				std::string key;

				// Extract key components from metadata
				if (metadata.contains("components") && metadata["components"].is_array()) {
					for (const auto& comp : metadata["components"]) {
						// Model paths are the most important for context uniqueness
						if (comp.contains("Checkpoint")) {
							auto model = comp["Checkpoint"];
							if (model.contains("modelPath") && !model["modelPath"].get<std::string>().empty()) {
								key += model["modelPath"].get<std::string>();
							}
							else if (model.contains("modelName") && !model["modelName"].get<std::string>().empty()) {
								key += model["modelName"].get<std::string>();
							}
						}

						// Include VAE if present
						if (comp.contains("Vae")) {
							auto vae = comp["Vae"];
							if (vae.contains("modelPath") && !vae["modelPath"].get<std::string>().empty()) {
								key += "|vae:" + vae["modelPath"].get<std::string>();
							}
							else if (vae.contains("modelName") && !vae["modelName"].get<std::string>().empty()) {
								key += "|vae:" + vae["modelName"].get<std::string>();
							}
						}

						// Include CLIP if present
						if (comp.contains("ClipL")) {
							auto clipL = comp["ClipL"];
							if (clipL.contains("modelPath") && !clipL["modelPath"].get<std::string>().empty()) {
								key += "|clipL:" + clipL["modelPath"].get<std::string>();
							}
							else if (clipL.contains("modelName") && !clipL["modelName"].get<std::string>().empty()) {
								key += "|clipL:" + clipL["modelName"].get<std::string>();
							}
						}

						if (comp.contains("ClipG")) {
							auto clipG = comp["ClipG"];
							if (clipG.contains("modelPath") && !clipG["modelPath"].get<std::string>().empty()) {
								key += "|clipG:" + clipG["modelPath"].get<std::string>();
							}
							else if (clipG.contains("modelName") && !clipG["modelName"].get<std::string>().empty()) {
								key += "|clipG:" + clipG["modelName"].get<std::string>();
							}
						}
					}
				}

				// Add sampler settings that affect context
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

				// If key is empty, generate a hash of the entire metadata
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

		// Check if metadata is similar enough to reuse context
		static bool CanReuseContext(const nlohmann::json& cachedMetadata,
			const nlohmann::json& newMetadata) {
			try {
				// Check if the core model paths are the same
				auto getModelPaths = [](const nlohmann::json& metadata) -> std::vector<std::string> {
					std::vector<std::string> paths;
					if (metadata.contains("components") && metadata["components"].is_array()) {
						for (const auto& comp : metadata["components"]) {
							if (comp.contains("Checkpoint")) {
								auto model = comp["Checkpoint"];
								if (model.contains("modelPath") && !model["modelPath"].get<std::string>().empty()) {
									paths.push_back(model["modelPath"].get<std::string>());
								}
							}
							if (comp.contains("Vae")) {
								auto vae = comp["Vae"];
								if (vae.contains("modelPath") && !vae["modelPath"].get<std::string>().empty()) {
									paths.push_back(vae["modelPath"].get<std::string>());
								}
							}
							if (comp.contains("ClipL")) {
								auto clipL = comp["ClipL"];
								if (clipL.contains("modelPath") && !clipL["modelPath"].get<std::string>().empty()) {
									paths.push_back(clipL["modelPath"].get<std::string>());
								}
							}
							if (comp.contains("ClipG")) {
								auto clipG = comp["ClipG"];
								if (clipG.contains("modelPath") && !clipG["modelPath"].get<std::string>().empty()) {
									paths.push_back(clipG["modelPath"].get<std::string>());
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

				// Check key sampler settings
				auto getSamplerSettings = [](const nlohmann::json& metadata) -> std::pair<int, int> {
					int n_threads = std::thread::hardware_concurrency();
					int wtype = 0; // SD_TYPE_F32

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

		// Create new context from metadata (private helper, now async)
		static sd_ctx_t* CreateNewContextInternal(const nlohmann::json& metadata) {
			try {
				std::string modelPath = "";
				std::string clipLPath = "";
				std::string clipGPath = "";
				std::string clipVisionPath = "";
				std::string t5xxlPath = "";
				std::string llmPath = "";
				std::string llmVisionPath = "";
				std::string diffusionModelPath = "";
				std::string highNoiseModelPath = "";
				std::string vaePath = "";
				std::string taesdPath = "";
				std::string controlnetPath = "";
				std::string photoMakerPath = "";
				std::string tensorTypeRules = "";

				sd_ctx_params_t ctx_params;
				sd_ctx_params_init(&ctx_params); // Initialize with defaults

				if (metadata.contains("components") && metadata["components"].is_array()) {
					for (const auto &comp : metadata["components"]) {
						if (comp.contains("Checkpoint")) {
							auto model = comp["Checkpoint"];
							if (model.contains("modelPath") && !model["modelPath"].get<std::string>().empty())
								modelPath = model["modelPath"].get<std::string>();
							else if (model.contains("modelName") && !model["modelName"].get<std::string>().empty())
								modelPath = FilePathService::GetPath("Checkpoint") + "/" + model["modelName"].get<std::string>();

							std::cout << "DEBUG: Model path set to: " << modelPath << std::endl;
						}

						if (comp.contains("ClipL")) {
							auto clipL = comp["ClipL"];
							if (clipL.contains("modelPath") && !clipL["modelPath"].get<std::string>().empty())
								clipLPath = clipL["modelPath"].get<std::string>();
							else if (clipL.contains("modelName") && !clipL["modelName"].get<std::string>().empty())
								clipLPath = FilePathService::GetPath("Encoder") + "/" + clipL["modelName"].get<std::string>();
						}

						if (comp.contains("ClipG")) {
							auto clipG = comp["ClipG"];
							if (clipG.contains("modelPath") && !clipG["modelPath"].get<std::string>().empty())
								clipGPath = clipG["modelPath"].get<std::string>();
							else if (clipG.contains("modelName") && !clipG["modelName"].get<std::string>().empty())
								clipGPath = FilePathService::GetPath("Encoder") + "/" + clipG["modelName"].get<std::string>();
						}

						if (comp.contains("ClipVision")) {
							auto clipVision = comp["ClipVision"];
							if (clipVision.contains("modelPath") && !clipVision["modelPath"].get<std::string>().empty())
								clipVisionPath = clipVision["modelPath"].get<std::string>();
							else if (clipVision.contains("modelName") && !clipVision["modelName"].get<std::string>().empty())
								clipVisionPath = FilePathService::GetPath("Encoder") + "/" + clipVision["modelName"].get<std::string>();
						}

						if (comp.contains("T5XXL")) {
							auto t5xxl = comp["T5XXL"];
							if (t5xxl.contains("modelPath") && !t5xxl["modelPath"].get<std::string>().empty())
								t5xxlPath = t5xxl["modelPath"].get<std::string>();
							else if (t5xxl.contains("modelName") && !t5xxl["modelName"].get<std::string>().empty())
								t5xxlPath = FilePathService::GetPath("Encoder") + "/" + t5xxl["modelName"].get<std::string>();
						}

						if (comp.contains("LLM")) {
							auto llm = comp["LLM"];
							if (llm.contains("modelPath") && !llm["modelPath"].get<std::string>().empty())
								llmPath = llm["modelPath"].get<std::string>();
							else if (llm.contains("modelName") && !llm["modelName"].get<std::string>().empty())
								llmPath = FilePathService::GetPath("Encoder") + "/" + llm["modelName"].get<std::string>();
						}

						if (comp.contains("LLMVision")) {
							auto llmVision = comp["LLMVision"];
							if (llmVision.contains("modelPath") && !llmVision["modelPath"].get<std::string>().empty())
								llmVisionPath = llmVision["modelPath"].get<std::string>();
							else if (llmVision.contains("modelName") && !llmVision["modelName"].get<std::string>().empty())
								llmVisionPath = FilePathService::GetPath("Encoder") + "/" + llmVision["modelName"].get<std::string>();
						}

						if (comp.contains("DiffusionModel")) {
							auto diffusion = comp["DiffusionModel"];
							if (diffusion.contains("modelPath") && !diffusion["modelPath"].get<std::string>().empty())
								diffusionModelPath = diffusion["modelPath"].get<std::string>();
							else if (diffusion.contains("modelName") && !diffusion["modelName"].get<std::string>().empty())
								diffusionModelPath = FilePathService::GetPath("Unet") + "/" + diffusion["modelName"].get<std::string>();
						}

						if (comp.contains("HighNoiseDiffusionModel")) {
							auto highNoise = comp["HighNoiseDiffusionModel"];
							if (highNoise.contains("modelPath") && !highNoise["modelPath"].get<std::string>().empty())
								highNoiseModelPath = highNoise["modelPath"].get<std::string>();
							else if (highNoise.contains("modelName") && !highNoise["modelName"].get<std::string>().empty())
								highNoiseModelPath = FilePathService::GetPath("Unet") + "/" + highNoise["modelName"].get<std::string>();
						}

						if (comp.contains("Vae")) {
							auto vae = comp["Vae"];
							if (vae.contains("modelPath") && !vae["modelPath"].get<std::string>().empty())
								vaePath = vae["modelPath"].get<std::string>();
							else if (vae.contains("modelName") && !vae["modelName"].get<std::string>().empty())
								vaePath = FilePathService::GetPath("Vae") + "/" + vae["modelName"].get<std::string>();
							if (vae.contains("vae_decode_only"))
								ctx_params.vae_decode_only = vae["vae_decode_only"].get<bool>();
							if (vae.contains("keep_vae_on_cpu"))
								ctx_params.keep_vae_on_cpu = vae["keep_vae_on_cpu"].get<bool>();
						}

						if (comp.contains("Taesd")) {
							auto taesd = comp["Taesd"];
							if (taesd.contains("modelPath") && !taesd["modelPath"].get<std::string>().empty())
								taesdPath = taesd["modelPath"].get<std::string>();
							else if (taesd.contains("modelName") && !taesd["modelName"].get<std::string>().empty())
								taesdPath = FilePathService::GetPath("Vae") + "/" + taesd["modelName"].get<std::string>();
						}

						if (comp.contains("Controlnet")) {
							auto controlnet = comp["Controlnet"];
							if (controlnet.contains("modelPath") && !controlnet["modelPath"].get<std::string>().empty())
								controlnetPath = controlnet["modelPath"].get<std::string>();
							else if (controlnet.contains("modelName") && !controlnet["modelName"].get<std::string>().empty())
								controlnetPath = FilePathService::GetPath("ControlNet") + "/" + controlnet["modelName"].get<std::string>();

							if (controlnet.contains("keep_control_net_on_cpu"))
								ctx_params.keep_control_net_on_cpu = controlnet["keep_control_net_on_cpu"].get<bool>();
						}

						if (comp.contains("PhotoMaker") || comp.contains("StackedIdEmbed")) {
							auto pm = comp.contains("PhotoMaker") ? comp["PhotoMaker"] : comp["StackedIdEmbed"];
							if (pm.contains("modelPath") && !pm["modelPath"].get<std::string>().empty())
								photoMakerPath = pm["modelPath"].get<std::string>();
							else if (pm.contains("modelName") && !pm["modelName"].get<std::string>().empty())
								photoMakerPath = FilePathService::GetPath("Embed") + "/" + pm["modelName"].get<std::string>();
						}

						if (comp.contains("Sampler")) {
							auto sampler = comp["Sampler"];
							if (sampler.contains("n_threads"))
								ctx_params.n_threads = sampler["n_threads"].get<int>();
							if (sampler.contains("free_params_immediately"))
								ctx_params.free_params_immediately = sampler["free_params_immediately"].get<bool>();
							if (sampler.contains("offload_params_to_cpu"))
								ctx_params.offload_params_to_cpu = sampler["offload_params_to_cpu"].get<bool>();
							if (sampler.contains("keep_clip_on_cpu"))
								ctx_params.keep_clip_on_cpu = sampler["keep_clip_on_cpu"].get<bool>();
							if (sampler.contains("diffusion_flash_attn"))
								ctx_params.diffusion_flash_attn = sampler["diffusion_flash_attn"].get<bool>();
							if (sampler.contains("diffusion_conv_direct"))
								ctx_params.diffusion_conv_direct = sampler["diffusion_conv_direct"].get<bool>();
							if (sampler.contains("vae_conv_direct"))
								ctx_params.vae_conv_direct = sampler["vae_conv_direct"].get<bool>();
							if (sampler.contains("current_type_method"))
								ctx_params.wtype = static_cast<sd_type_t>(sampler["current_type_method"].get<int>());
							if (sampler.contains("current_prediction_type"))
								ctx_params.prediction = static_cast<prediction_t>(sampler["current_prediction_type"].get<int>());

							if (sampler.contains("tensor_type_rules") && !sampler["tensor_type_rules"].get<std::string>().empty())
								tensorTypeRules = sampler["tensor_type_rules"].get<std::string>();
						}

						if (comp.contains("Latent")) {
							auto latent = comp["Latent"];
							if (latent.contains("current_rng_type"))
								ctx_params.rng_type = static_cast<rng_type_t>(latent["current_rng_type"].get<int>());
							if (latent.contains("sampler_rng_type"))
								ctx_params.sampler_rng_type = static_cast<rng_type_t>(latent["sampler_rng_type"].get<int>());
						}

						if (comp.contains("VideoParams")) {
							auto videoParams = comp["VideoParams"];
							if (videoParams.contains("flow_shift"))
								ctx_params.flow_shift = videoParams["flow_shift"].get<float>();
						}

						if (comp.contains("Chroma")) {
							auto chroma = comp["Chroma"];
							if (chroma.contains("use_dit_mask"))
								ctx_params.chroma_use_dit_mask = chroma["use_dit_mask"].get<bool>();
							if (chroma.contains("use_t5_mask"))
								ctx_params.chroma_use_t5_mask = chroma["use_t5_mask"].get<bool>();
							if (chroma.contains("t5_mask_pad"))
								ctx_params.chroma_t5_mask_pad = chroma["t5_mask_pad"].get<int>();
						}
					}
				}

				std::cout << "DEBUG: Creating SD context with paths:" << std::endl;
				std::cout << "  model_path: " << (modelPath.empty() ? "(empty)" : modelPath) << std::endl;
				std::cout << "  vae_path: " << (vaePath.empty() ? "(empty)" : vaePath) << std::endl;
				std::cout << "  clip_l_path: " << (clipLPath.empty() ? "(empty)" : clipLPath) << std::endl;
				std::cout << "  clip_g_path: " << (clipGPath.empty() ? "(empty)" : clipGPath) << std::endl;

				ctx_params.model_path = modelPath.empty() ? nullptr : modelPath.c_str();
				ctx_params.clip_l_path = clipLPath.empty() ? nullptr : clipLPath.c_str();
				ctx_params.clip_g_path = clipGPath.empty() ? nullptr : clipGPath.c_str();
				ctx_params.clip_vision_path = clipVisionPath.empty() ? nullptr : clipVisionPath.c_str();
				ctx_params.t5xxl_path = t5xxlPath.empty() ? nullptr : t5xxlPath.c_str();
				ctx_params.llm_path = llmPath.empty() ? nullptr : llmPath.c_str();
				ctx_params.llm_vision_path = llmVisionPath.empty() ? nullptr : llmVisionPath.c_str();
				ctx_params.diffusion_model_path = diffusionModelPath.empty() ? nullptr : diffusionModelPath.c_str();
				ctx_params.high_noise_diffusion_model_path = highNoiseModelPath.empty() ? nullptr : highNoiseModelPath.c_str();
				ctx_params.vae_path = vaePath.empty() ? nullptr : vaePath.c_str();
				ctx_params.taesd_path = taesdPath.empty() ? nullptr : taesdPath.c_str();
				ctx_params.control_net_path = controlnetPath.empty() ? nullptr : controlnetPath.c_str();
				ctx_params.photo_maker_path = photoMakerPath.empty() ? nullptr : photoMakerPath.c_str();
				ctx_params.tensor_type_rules = tensorTypeRules.empty() ? nullptr : tensorTypeRules.c_str();

				sd_ctx_t* ctx = new_sd_ctx(&ctx_params);

				if (!ctx) {
					std::cerr << "ERROR: Failed to create SD context!" << std::endl;
					return nullptr;
				}

				std::cout << "DEBUG: SD context created successfully!" << std::endl;
				return ctx;
			}
			catch (const std::exception &e) {
				std::cerr << "Error creating SD context: " << e.what() << std::endl;
				return nullptr;
			}
		}

		// Async function to create context
		static std::future<sd_ctx_t*> CreateNewContextAsync(const nlohmann::json& metadata) {
			return std::async(std::launch::async, [metadata]() -> sd_ctx_t* {
				return CreateNewContextInternal(metadata);
			});
		}

	public:
		// Create new context from metadata (public interface)
		static sd_ctx_t* CreateNewContext(const nlohmann::json& metadata) {
			return CreateNewContextInternal(metadata);
		}

		// Get or create context - UPDATED for async loading
		static sd_ctx_t* GetOrCreateContext(const nlohmann::json& metadata) {
			std::string cacheKey = GenerateCacheKey(metadata);

			{
				std::lock_guard<std::mutex> lock(cacheMutex);

				// Check if we have a cached context
				auto it = contextCache.find(cacheKey);
				if (it != contextCache.end()) {
					auto& entry = it->second;

					// Check if context is currently loading
					if (entry->isLoading) {
						// Check if loading is complete
						if (entry->loadingFuture.valid()) {
							auto status = entry->loadingFuture.wait_for(std::chrono::milliseconds(0));
							if (status == std::future_status::ready) {
								try {
									// Get the loaded context
									entry->context = entry->loadingFuture.get();
									entry->isLoading = false;

									if (!entry->context) {
										// Loading failed, remove the entry
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
								// Still loading
								std::cout << "DEBUG: Context is still loading: " << cacheKey << std::endl;
								entry->isInUse = true;
								return nullptr; // Return null to indicate loading in progress
							}
						}
					}

					// Check if we have a valid context now
					if (entry->context) {
						// Check if metadata is similar enough to reuse
						if (CanReuseContext(entry->metadata, metadata)) {
							entry->lastUsed = std::chrono::steady_clock::now();
							entry->useCount++;
							entry->isInUse = true;
							std::cout << "DEBUG: Reusing cached SD context: " << cacheKey
								<< " (use count: " << entry->useCount << ")" << std::endl;
							return entry->context;
						}
						else {
							// Metadata differs, remove old entry
							std::cout << "DEBUG: Metadata differs, removing old cache entry: " << cacheKey << std::endl;
							contextCache.erase(it);
						}
					}
				}
			}

			// Create new context entry and start async loading
			std::shared_ptr<SDContextCacheEntry> newEntry;
			{
				std::lock_guard<std::mutex> lock(cacheMutex);

				// Clean up least recently used if cache is full
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

				// Create new entry
				newEntry = std::make_shared<SDContextCacheEntry>();
				newEntry->cacheKey = cacheKey;
				newEntry->metadata = metadata;
				newEntry->lastUsed = std::chrono::steady_clock::now();
				newEntry->useCount = 1;
				newEntry->isInUse = true;
				newEntry->isLoading = true;

				// Start async loading
				newEntry->loadingFuture = CreateNewContextAsync(metadata);

				contextCache[cacheKey] = newEntry;
				totalContextsCreated++;

				std::cout << "DEBUG: Started async loading for SD context: " << cacheKey
					<< " (total created: " << totalContextsCreated << ")" << std::endl;
			}

			// Return nullptr to indicate loading in progress
			return nullptr;
		}

		// Check if a context is currently loading for given metadata
		static bool IsContextLoading(const nlohmann::json& metadata) {
			std::string cacheKey = GenerateCacheKey(metadata);
			std::lock_guard<std::mutex> lock(cacheMutex);

			auto it = contextCache.find(cacheKey);
			if (it != contextCache.end()) {
				return it->second->isLoading;
			}
			return false;
		}

		// Check if loading is complete and get context if ready
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
								// Loading failed
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

		// Release context back to cache
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

			// Context not found in cache, free it directly
			std::cerr << "WARNING: Context not found in cache, freeing directly" << std::endl;
			free_sd_ctx(context);
		}

		// Force free a specific context
		static void ForceFreeContext(sd_ctx_t* context) {
			std::lock_guard<std::mutex> lock(cacheMutex);

			for (auto it = contextCache.begin(); it != contextCache.end(); ++it) {
				if (it->second->context == context) {
					std::cout << "DEBUG: Force freeing context: " << it->first << std::endl;
					contextCache.erase(it);
					return;
				}
			}

			// Context not in cache, free directly
			free_sd_ctx(context);
		}

		// Clear all cached contexts
		static void ClearAllContexts() {
			std::lock_guard<std::mutex> lock(cacheMutex);

			size_t count = contextCache.size();
			contextCache.clear();
			std::cout << "DEBUG: Cleared all cached contexts (" << count << " contexts)" << std::endl;
		}

		// Get cache statistics
		static std::tuple<size_t, size_t, size_t> GetCacheStats() {
			std::lock_guard<std::mutex> lock(cacheMutex);

			size_t totalCached = contextCache.size();
			size_t inUse = 0;
			size_t available = 0;

			for (const auto& entry : contextCache) {
				if (entry.second->isInUse) {
					inUse++;
				}
				else if (!entry.second->isLoading) {
					available++;
				}
			}

			return { totalCached, inUse, available };
		}

		// Get loading statistics
		static std::tuple<size_t, size_t> GetLoadingStats() {
			std::lock_guard<std::mutex> lock(cacheMutex);

			size_t loadingCount = 0;
			for (const auto& entry : contextCache) {
				if (entry.second->isLoading) {
					loadingCount++;
				}
			}

			return { loadingCount, totalContextsFailed };
		}

		// List all cached contexts
		static void ListCachedContexts() {
			std::lock_guard<std::mutex> lock(cacheMutex);

			std::cout << "=== Cached SD Contexts ===" << std::endl;
			std::cout << "Total contexts: " << contextCache.size() << std::endl;
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

				// Show model paths
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

		// ================================================
		// NEW: Model Management API
		// ================================================

		// Unload specific model by path
		static void UnloadSpecificModel(const std::string& modelPath) {
			std::lock_guard<std::mutex> lock(cacheMutex);

			std::cout << "DEBUG: Looking for contexts using model: " << modelPath << std::endl;

			// Find all cache entries using this model
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
									model["modelPath"].get<std::string>().find(modelPath) != std::string::npos) {
									usesModel = true;
									break;
								}
								if (model.contains("modelName") &&
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

			// Remove the contexts
			for (const auto& key : toRemove) {
				contextCache.erase(key);
				std::cout << "DEBUG: Removed context: " << key << std::endl;
			}
		}

		// Unload all models
		static void UnloadAllModels() {
			ClearAllContexts();
		}

		// Set maximum number of models to cache
		static void SetMaxCacheSize(size_t size) {
			std::lock_guard<std::mutex> lock(cacheMutex);
			MAX_CACHE_SIZE = size;
			std::cout << "DEBUG: Max cache size set to: " << MAX_CACHE_SIZE << std::endl;
		}

		// Get maximum cache size
		static size_t GetMaxCacheSize() {
			std::lock_guard<std::mutex> lock(cacheMutex);
			return MAX_CACHE_SIZE;
		}

		// Force unload when new model is requested (default behavior)
		static void ForceUnloadForNewModel(const nlohmann::json& newMetadata) {
			std::lock_guard<std::mutex> lock(cacheMutex);

			if (contextCache.size() >= 1) { // If we have at least one model loaded
				// Extract model path from new metadata
				std::string newModelPath = "";
				if (newMetadata.contains("components") && newMetadata["components"].is_array()) {
					for (const auto& comp : newMetadata["components"]) {
						if (comp.contains("Checkpoint")) {
							auto model = comp["Checkpoint"];
							if (model.contains("modelPath") && !model["modelPath"].get<std::string>().empty()) {
								newModelPath = model["modelPath"].get<std::string>();
							}
							break;
						}
					}
				}

				if (!newModelPath.empty()) {
					// Find and remove all contexts that don't use the new model
					for (auto it = contextCache.begin(); it != contextCache.end();) {
						bool usesNewModel = false;

						if (it->second->metadata.contains("components") &&
							it->second->metadata["components"].is_array()) {
							for (const auto& comp : it->second->metadata["components"]) {
								if (comp.contains("Checkpoint")) {
									auto model = comp["Checkpoint"];
									if (model.contains("modelPath") &&
										model["modelPath"].get<std::string>() == newModelPath) {
										usesNewModel = true;
										break;
									}
								}
							}
						}

						if (!usesNewModel) {
							std::cout << "DEBUG: Force unloading model for new request: " << it->first << std::endl;
							it = contextCache.erase(it);
						}
						else {
							++it;
						}
					}
				}
			}
		}

		// Check if a specific model is currently loaded
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
									model["modelPath"].get<std::string>().find(modelPath) != std::string::npos) {
									return true;
								}
								if (model.contains("modelName") &&
									model["modelName"].get<std::string>().find(modelPath) != std::string::npos) {
									return true;
								}
							}
						}
					}
				}
				catch (const std::exception& e) {
					// Skip errors
				}
			}

			return false;
		}

		// Get list of all loaded models
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
								if (model.contains("modelPath") && !model["modelPath"].get<std::string>().empty()) {
									modelInfo = model["modelPath"].get<std::string>();
								}
								else if (model.contains("modelName") && !model["modelName"].get<std::string>().empty()) {
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
					// Skip errors
				}
			}

			return models;
		}
	};

	// Legacy function for backward compatibility
	inline sd_ctx_t* InitializeStableDiffusionContext(const nlohmann::json& metadata) {
		return SDContextManager::GetOrCreateContext(metadata);
	}

} // namespace Utils