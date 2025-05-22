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