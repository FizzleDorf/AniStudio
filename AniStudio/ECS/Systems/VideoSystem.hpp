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

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "VideoComponent.hpp"
#include "ThreadPool.hpp"
#include "ImageUtils.hpp"
#include <GL/glew.h>
#include <opencv2/opencv.hpp>
#include <memory>
#include <functional>
#include <queue>
#include <mutex>
#include <chrono>

namespace ECS {

	class VideoSystem : public BaseSystem {
	public:
		using VideoCallback = std::function<void(EntityID)>;

		VideoSystem(EntityManager& entityMgr)
			: BaseSystem(entityMgr), lastFrameTime(std::chrono::high_resolution_clock::now()) {
			sysName = "VideoSystem";
			AddComponentSignature<VideoComponent>();

			// Initialize FFmpeg - these functions are deprecated in newer FFmpeg versions
			// but we're keeping them for compatibility with older versions
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
			av_register_all();
			avcodec_register_all();
#endif
		}

		~VideoSystem() override {
			for (auto entity : entities) {
				if (mgr.HasComponent<VideoComponent>(entity)) {
					auto& videoComp = mgr.GetComponent<VideoComponent>(entity);
					UnloadVideo(videoComp);
				}
			}
		}

		void Start() override {
			for (auto entity : entities) {
				if (mgr.HasComponent<VideoComponent>(entity)) {
					auto& videoComp = mgr.GetComponent<VideoComponent>(entity);
					LoadVideo(videoComp);
				}
			}
		}

		void Update(float deltaT) override {
			auto currentTime = std::chrono::high_resolution_clock::now();
			float actualDeltaT = std::chrono::duration<float>(currentTime - lastFrameTime).count();
			lastFrameTime = currentTime;

			for (auto entity : entities) {
				if (mgr.HasComponent<VideoComponent>(entity)) {
					auto& videoComp = mgr.GetComponent<VideoComponent>(entity);

					UpdateVideoPlayback(videoComp, actualDeltaT);

					if (videoComp.needsTextureUpdate) {
						UpdateTexture(videoComp);
						videoComp.needsTextureUpdate = false;
					}
				}
			}
		}

		bool GetNextFrame(VideoComponent& videoComp) {
			if (!videoComp.formatCtx) {
				return false;
			}

			AVFrame* frame = av_frame_alloc();
			AVPacket packet;
			av_init_packet(&packet);
			packet.data = NULL;
			packet.size = 0;

			int frameFinished = 0;
			bool gotFrame = false;

			// Keep reading packets until we get a video frame
			while (av_read_frame(videoComp.formatCtx, &packet) >= 0) {
				if (packet.stream_index == videoComp.videoStreamIdx) {
					// Decode video frame
					int ret = avcodec_send_packet(videoComp.codecCtx, &packet);
					if (ret < 0) {
						break;
					}

					ret = avcodec_receive_frame(videoComp.codecCtx, frame);
					if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
						av_packet_unref(&packet);
						continue;
					}
					else if (ret < 0) {
						break;
					}

					// Successfully decoded a frame
					ConvertFrameToRGB(videoComp, frame);
					videoComp.currentFrame++;

					// Handle looping
					if (videoComp.currentFrame >= videoComp.frameCount) {
						videoComp.currentFrame = 0;
						SeekToFrame(videoComp, 0);
					}

					gotFrame = true;
					break;
				}
				av_packet_unref(&packet);
			}

			av_frame_free(&frame);
			return gotFrame;
		}

		bool SeekToFrame(VideoComponent& videoComp, int frameIndex) {
			if (!videoComp.formatCtx || frameIndex < 0 || frameIndex >= videoComp.frameCount) {
				return false;
			}

			// Calculate timestamp for seeking
			int64_t timestamp = av_rescale_q(frameIndex,
				av_make_q(1, videoComp.fps),
				videoComp.formatCtx->streams[videoComp.videoStreamIdx]->time_base);

			// Seek to the nearest keyframe before the target
			if (av_seek_frame(videoComp.formatCtx, videoComp.videoStreamIdx,
				timestamp, AVSEEK_FLAG_BACKWARD) < 0) {
				std::cerr << "Error seeking to frame " << frameIndex << std::endl;
				return false;
			}

			avcodec_flush_buffers(videoComp.codecCtx);

			// We may need to decode several frames to reach our target
			AVFrame* frame = av_frame_alloc();
			AVPacket packet;
			av_init_packet(&packet);
			packet.data = NULL;
			packet.size = 0;

			int actualFrame = 0;
			bool foundTargetFrame = false;

			// Decode frames until we reach our target frame
			while (av_read_frame(videoComp.formatCtx, &packet) >= 0) {
				if (packet.stream_index == videoComp.videoStreamIdx) {
					int ret = avcodec_send_packet(videoComp.codecCtx, &packet);
					if (ret < 0) {
						av_packet_unref(&packet);
						continue;
					}

					ret = avcodec_receive_frame(videoComp.codecCtx, frame);
					if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
						av_packet_unref(&packet);
						continue;
					}
					else if (ret < 0) {
						av_packet_unref(&packet);
						break;
					}

					// We found a frame
					actualFrame++;

					// If we've reached or passed our target frame, convert and use it
					if (actualFrame >= frameIndex) {
						ConvertFrameToRGB(videoComp, frame);
						videoComp.currentFrame = frameIndex;
						foundTargetFrame = true;
						av_packet_unref(&packet);
						break;
					}
				}
				av_packet_unref(&packet);
			}

			av_frame_free(&frame);
			return foundTargetFrame;
		}

		void RegisterVideoAddedCallback(const VideoCallback& callback) {
			videoAddedCallbacks.push_back(callback);
		}

		void RegisterVideoRemovedCallback(const VideoCallback& callback) {
			videoRemovedCallbacks.push_back(callback);
		}

		void SetVideo(const EntityID entity, const std::string& filePath) {
			if (mgr.HasComponent<VideoComponent>(entity)) {
				auto& videoComp = mgr.GetComponent<VideoComponent>(entity);

				if (videoComp.formatCtx) {
					UnloadVideo(videoComp);
				}

				videoComp.filePath = filePath;
				size_t lastSlash = filePath.find_last_of("/\\");
				videoComp.fileName = (lastSlash != std::string::npos) ?
					filePath.substr(lastSlash + 1) : filePath;

				LoadVideo(videoComp);
				NotifyVideoAdded(entity);
			}
		}

		void RemoveVideo(const EntityID entity) {
			if (mgr.HasComponent<VideoComponent>(entity)) {
				auto& videoComp = mgr.GetComponent<VideoComponent>(entity);
				UnloadVideo(videoComp);
				NotifyVideoRemoved(entity);
				mgr.DestroyEntity(entity);
			}
		}

		std::vector<EntityID> GetAllVideoEntities() const {
			std::vector<EntityID> result;
			for (auto entity : entities) {
				if (mgr.HasComponent<VideoComponent>(entity)) {
					result.push_back(entity);
				}
			}
			return result;
		}

	private:
		std::vector<VideoCallback> videoAddedCallbacks;
		std::vector<VideoCallback> videoRemovedCallbacks;
		std::mutex frameMutex;
		std::chrono::high_resolution_clock::time_point lastFrameTime;

		void ConvertFrameToRGB(VideoComponent& videoComp, AVFrame* frame) {
			std::lock_guard<std::mutex> lock(frameMutex);

			if (!videoComp.swsCtx) {
				videoComp.swsCtx = sws_getContext(
					frame->width, frame->height, (AVPixelFormat)frame->format,
					frame->width, frame->height, AV_PIX_FMT_RGB24,
					SWS_BILINEAR, nullptr, nullptr, nullptr);
			}

			// Allocate RGB frame if needed
			if (!videoComp.rgbFrame) {
				videoComp.rgbFrame = av_frame_alloc();
				int size = av_image_get_buffer_size(AV_PIX_FMT_RGB24,
					frame->width, frame->height, 1);
				videoComp.frameBuffer = (uint8_t*)av_malloc(size);
				av_image_fill_arrays(videoComp.rgbFrame->data, videoComp.rgbFrame->linesize,
					videoComp.frameBuffer, AV_PIX_FMT_RGB24,
					frame->width, frame->height, 1);
			}

			// Convert to RGB
			sws_scale(videoComp.swsCtx, frame->data, frame->linesize, 0,
				frame->height, videoComp.rgbFrame->data, videoComp.rgbFrame->linesize);

			// Create OpenCV Mat from frame data
			videoComp.currentFrameData.create(frame->height, frame->width, CV_8UC3);
			memcpy(videoComp.currentFrameData.data, videoComp.frameBuffer,
				frame->width * frame->height * 3);

			// Ensure data is continuous
			if (!videoComp.currentFrameData.isContinuous()) {
				videoComp.currentFrameData = videoComp.currentFrameData.clone();
			}

			videoComp.needsTextureUpdate = true;
		}

		void UpdateTexture(VideoComponent& videoComp) {
			// Delete existing texture if invalid
			if (videoComp.currentTexture != 0 && !glIsTexture(videoComp.currentTexture)) {
				glDeleteTextures(1, &videoComp.currentTexture);
				videoComp.currentTexture = 0;
			}

			// Create new texture if needed
			if (videoComp.currentTexture == 0) {
				glGenTextures(1, &videoComp.currentTexture);
				glBindTexture(GL_TEXTURE_2D, videoComp.currentTexture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

				// Allocate storage
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
					videoComp.width, videoComp.height,
					0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
			}

			// Update texture data
			glBindTexture(GL_TEXTURE_2D, videoComp.currentTexture);

			// Flip the image vertically (OpenGL expects 0,0 at bottom-left)
			cv::Mat flipped;
			cv::flip(videoComp.currentFrameData, flipped, 0);

			// Check if frame data matches texture dimensions
			if (flipped.cols != videoComp.width || flipped.rows != videoComp.height) {
				cv::resize(flipped, flipped, cv::Size(videoComp.width, videoComp.height));
			}

			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
				flipped.cols, flipped.rows,
				GL_RGB, GL_UNSIGNED_BYTE,
				flipped.data);

			GLenum err = glGetError();
			if (err != GL_NO_ERROR) {
				std::cerr << "OpenGL texture error after update: " << err << std::endl;
			}
		}

		void NotifyVideoAdded(EntityID entity) {
			for (const auto& callback : videoAddedCallbacks) {
				callback(entity);
			}
		}

		void NotifyVideoRemoved(EntityID entity) {
			for (const auto& callback : videoRemovedCallbacks) {
				callback(entity);
			}
		}

		void LoadVideo(VideoComponent& videoComp) {
			UnloadVideo(videoComp);

			if (avformat_open_input(&videoComp.formatCtx, videoComp.filePath.c_str(), nullptr, nullptr) != 0) {
				std::cerr << "Could not open video file: " << videoComp.filePath << std::endl;
				return;
			}

			if (avformat_find_stream_info(videoComp.formatCtx, nullptr) < 0) {
				std::cerr << "Could not find stream information" << std::endl;
				avformat_close_input(&videoComp.formatCtx);
				videoComp.formatCtx = nullptr;
				return;
			}

			videoComp.videoStreamIdx = -1;
			for (unsigned int i = 0; i < videoComp.formatCtx->nb_streams; i++) {
				if (videoComp.formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
					videoComp.videoStreamIdx = i;
					break;
				}
			}

			if (videoComp.videoStreamIdx == -1) {
				std::cerr << "No video stream found" << std::endl;
				avformat_close_input(&videoComp.formatCtx);
				videoComp.formatCtx = nullptr;
				return;
			}

			AVCodecParameters* codecPar = videoComp.formatCtx->streams[videoComp.videoStreamIdx]->codecpar;
			AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
			if (!codec) {
				std::cerr << "Unsupported codec" << std::endl;
				avformat_close_input(&videoComp.formatCtx);
				videoComp.formatCtx = nullptr;
				return;
			}

			videoComp.codecCtx = avcodec_alloc_context3(codec);
			if (avcodec_parameters_to_context(videoComp.codecCtx, codecPar) < 0) {
				std::cerr << "Could not copy codec context" << std::endl;
				avformat_close_input(&videoComp.formatCtx);
				videoComp.formatCtx = nullptr;
				return;
			}

			if (avcodec_open2(videoComp.codecCtx, codec, nullptr) < 0) {
				std::cerr << "Could not open codec" << std::endl;
				avcodec_free_context(&videoComp.codecCtx);
				avformat_close_input(&videoComp.formatCtx);
				videoComp.formatCtx = nullptr;
				videoComp.codecCtx = nullptr;
				return;
			}

			videoComp.width = codecPar->width;
			videoComp.height = codecPar->height;

			// Calculate FPS
			AVRational frame_rate = videoComp.formatCtx->streams[videoComp.videoStreamIdx]->avg_frame_rate;
			if (frame_rate.num == 0 || frame_rate.den == 0) {
				frame_rate = videoComp.formatCtx->streams[videoComp.videoStreamIdx]->r_frame_rate;
			}
			videoComp.fps = av_q2d(frame_rate);

			// Try to get frame count
			videoComp.frameCount = videoComp.formatCtx->streams[videoComp.videoStreamIdx]->nb_frames;
			if (videoComp.frameCount <= 0) {
				// Estimate frame count based on duration
				if (videoComp.formatCtx->duration != AV_NOPTS_VALUE) {
					videoComp.frameCount = (int)(videoComp.formatCtx->duration * videoComp.fps / AV_TIME_BASE);
				}
				else {
					// Default to some reasonable value if we can't determine
					videoComp.frameCount = 10000;
					std::cerr << "Warning: Could not determine frame count, using default" << std::endl;
				}
			}

			if (videoComp.width <= 0 || videoComp.height <= 0) {
				std::cerr << "Invalid video dimensions: " << videoComp.width << "x" << videoComp.height << std::endl;
				UnloadVideo(videoComp);
				return;
			}

			if (videoComp.fps <= 0) {
				videoComp.fps = 30.0;
				std::cerr << "Invalid FPS, using default: " << videoComp.fps << std::endl;
			}

			if (videoComp.frameCount <= 0) {
				std::cerr << "Couldn't determine frame count" << std::endl;
				UnloadVideo(videoComp);
				return;
			}

			// Reset playback state
			videoComp.currentFrame = 0;
			videoComp.isPlaying = false;
			videoComp.playbackSpeed = 1.0f;

			// Load the first frame
			SeekToFrame(videoComp, 0);

			std::cout << "Video loaded successfully:\n"
				<< "Path: " << videoComp.filePath << "\n"
				<< "Size: " << videoComp.width << "x" << videoComp.height << "\n"
				<< "FPS: " << videoComp.fps << "\n"
				<< "Frames: " << videoComp.frameCount << std::endl;
		}

		void UnloadVideo(VideoComponent& videoComp) {
			videoComp.ReleaseTexture();

			if (videoComp.swsCtx) {
				sws_freeContext(videoComp.swsCtx);
				videoComp.swsCtx = nullptr;
			}

			if (videoComp.rgbFrame) {
				av_frame_free(&videoComp.rgbFrame);
				videoComp.rgbFrame = nullptr;
			}

			if (videoComp.frameBuffer) {
				av_free(videoComp.frameBuffer);
				videoComp.frameBuffer = nullptr;
			}

			if (videoComp.codecCtx) {
				avcodec_close(videoComp.codecCtx);
				avcodec_free_context(&videoComp.codecCtx);
				videoComp.codecCtx = nullptr;
			}

			if (videoComp.formatCtx) {
				avformat_close_input(&videoComp.formatCtx);
				videoComp.formatCtx = nullptr;
			}

			videoComp.width = 0;
			videoComp.height = 0;
			videoComp.fps = 0.0;
			videoComp.frameCount = 0;
			videoComp.currentFrame = 0;
			videoComp.isPlaying = false;
			videoComp.videoStreamIdx = -1;
			videoComp.currentFrameData.release();
		}

		void UpdateVideoPlayback(VideoComponent& videoComp, float deltaTime) {
			if (!videoComp.isPlaying || !videoComp.formatCtx)
				return;

			static float frameAccumulator = 0.0f;
			frameAccumulator += deltaTime * videoComp.playbackSpeed;

			const float frameDuration = 1.0f / static_cast<float>(videoComp.fps);

			while (frameAccumulator >= frameDuration) {
				frameAccumulator -= frameDuration;

				// Get the next frame
				if (!GetNextFrame(videoComp)) {
					// If we can't get the next frame, try seeking to beginning
					if (videoComp.currentFrame >= videoComp.frameCount - 1) {
						SeekToFrame(videoComp, 0);
					}
					else {
						// If that fails too, just stop playback
						videoComp.isPlaying = false;
						std::cerr << "Failed to get next frame, stopping playback" << std::endl;
					}
					break;
				}
			}
		}
	};

} // namespace ECS