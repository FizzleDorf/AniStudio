#pragma once

#include "ThreadPool.hpp"
#include "SDcppUtils.hpp"

namespace ECS {

	// Base class for SDCPP tasks
	class SDCPPTask : public Utils::Task {
	public:
		SDCPPTask(const nlohmann::json& metadata, std::string fullPath)
			: metadata(metadata), fullPath(fullPath) {}

		virtual ~SDCPPTask() = default;

	protected:
		nlohmann::json metadata;
		std::string fullPath;
	};

	// Task for text-to-image generation
	class InferenceTask : public SDCPPTask {
	public:
		InferenceTask(const nlohmann::json& metadata, std::string fullPath)
			: SDCPPTask(metadata, fullPath) {} // Fix: Initialize base class properly

		void execute() override {
			if (isCancelled()) return;
			try {
				bool success = Utils::SDCPPUtils::RunInference(metadata, fullPath);
				if (!success) {
					throw std::runtime_error("Inference task failed");
				}

				markDone();
			}
			catch (const std::exception& e) {
				throw std::runtime_error("Exception in inference task");
				markDone();
			}
		}
	};

	// Task for model conversion to GGUF format
	class ConvertTask : public SDCPPTask {
	public:
		ConvertTask(const nlohmann::json& metadata)
			: SDCPPTask(metadata, "") {}

		void execute() override {
			if (isCancelled()) return;
			try {
				bool success = Utils::SDCPPUtils::ConvertToGGUF(metadata);
				if (!success) {
					throw std::runtime_error("Conversion task failed");
				}

				markDone();
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in conversion task: " << e.what() << std::endl;
				markDone();
			}
		}
	};

	// Task for image-to-image generation
	class Img2ImgTask : public SDCPPTask {
	public:
		Img2ImgTask(const nlohmann::json& metadata, std::string fullPath)
			: SDCPPTask(metadata, fullPath) {}

		void execute() override {
			if (isCancelled()) return;
			try {
				// Call the utility function to handle img2img
				bool success = Utils::SDCPPUtils::RunImg2Img(metadata, fullPath);
				if (!success) {
					throw std::runtime_error("Img2img task failed");
				}

				markDone();
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in img2img task: " << e.what() << std::endl;
				markDone();
			}
		}
	};

	// Task for upscaling images
	class UpscalingTask : public SDCPPTask {
	public:
		UpscalingTask(const nlohmann::json& metadata, std::string fullPath)
			: SDCPPTask(metadata, fullPath) {}

		void execute() override {
			if (isCancelled()) return;

			try {
				// Call the utility function to handle upscaling
				bool success = Utils::SDCPPUtils::RunUpscaling(metadata, fullPath);
				if (!success) {
					throw std::runtime_error("Upscaling task failed");
				}

				markDone();
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in upscaling task: " << e.what() << std::endl;
				markDone();
			}
		}
	};

} // namespace ECS