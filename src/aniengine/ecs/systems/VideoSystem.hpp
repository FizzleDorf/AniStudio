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
 * For commercial license information, please contact legal@kframe.ai.
 */

#pragma once
#include "BaseSystem.hpp"
#include "VideoComponent.hpp"
#include <vector>

namespace ECS {

	class VideoSystem : public BaseSystem {
	public:
		VideoSystem(EntityManager& entityMgr) : BaseSystem(entityMgr) {
			sysName = "VideoSystem";
			AddComponentSignature<VideoComponent>();
		}

		void Start() override {
			std::cout << "[VideoSystem] Started" << std::endl;
		}

		void Update(const float deltaT) override {
			for (auto entity : entities) {
				auto& videoComp = mgr.GetComponent<VideoComponent>(entity);

				if (videoComp.isPlaying) {
					videoComp.frameTime += deltaT * videoComp.playbackSpeed;

					// Check if we need to advance to the next frame
					if (videoComp.frameTime >= videoComp.frameDuration) {
						// Calculate how many frames to advance
						int framesToAdvance = static_cast<int>(videoComp.frameTime / videoComp.frameDuration);
						videoComp.frameTime -= framesToAdvance * videoComp.frameDuration;

						// Advance frames
						for (int i = 0; i < framesToAdvance; ++i) {
							if (!GetNextFrame(videoComp)) {
								if (videoComp.looping) {
									// Reset to beginning
									SeekToFrame(videoComp, 0);
								}
								else {
									// Stop playback
									videoComp.isPlaying = false;
								}
								break;
							}
						}
					}
				}
			}
		}

		void Destroy() override {
			std::cout << "[VideoSystem] Destroyed" << std::endl;
		}

		// Video loading and management methods - FIXED: Added missing methods
		bool SetVideo(EntityID entityID, const std::string& filePath) {
			if (!mgr.IsEntityValid(entityID) || !mgr.HasComponent<VideoComponent>(entityID)) {
				std::cerr << "[VideoSystem] Invalid entity or missing VideoComponent" << std::endl;
				return false;
			}

			auto& videoComp = mgr.GetComponent<VideoComponent>(entityID);
			return LoadVideo(videoComp, filePath);
		}

		bool RemoveVideo(EntityID entityID) {
			if (!mgr.IsEntityValid(entityID) || !mgr.HasComponent<VideoComponent>(entityID)) {
				return false;
			}

			auto& videoComp = mgr.GetComponent<VideoComponent>(entityID);
			CleanupVideo(videoComp);
			mgr.DestroyEntity(entityID);
			return true;
		}

		bool GetNextFrame(VideoComponent& videoComp) {
			if (!videoComp.formatCtx || !videoComp.codecCtx) {
				return false;
			}

			// FIXED: Replace deprecated av_init_packet with modern FFmpeg approach
			AVPacket* packet = av_packet_alloc();
			if (!packet) {
				std::cerr << "[VideoSystem] Failed to allocate packet" << std::endl;
				return false;
			}

			AVFrame* frame = av_frame_alloc();
			if (!frame) {
				std::cerr << "[VideoSystem] Failed to allocate frame" << std::endl;
				av_packet_free(&packet);
				return false;
			}

			bool frameDecoded = false;
			int ret;

			while ((ret = av_read_frame(videoComp.formatCtx, packet)) >= 0) {
				if (packet->stream_index == videoComp.videoStreamIdx) {
					ret = avcodec_send_packet(videoComp.codecCtx, packet);
					if (ret < 0) {
						std::cerr << "[VideoSystem] Error sending packet to decoder" << std::endl;
						break;
					}

					ret = avcodec_receive_frame(videoComp.codecCtx, frame);
					if (ret == 0) {
						// Successfully decoded a frame
						ConvertFrameToTexture(videoComp, frame);
						videoComp.currentFrame++;
						frameDecoded = true;
						break;
					}
					else if (ret == AVERROR(EAGAIN)) {
						// Need more input
						continue;
					}
					else if (ret == AVERROR_EOF) {
						// End of file
						break;
					}
					else {
						std::cerr << "[VideoSystem] Error receiving frame from decoder" << std::endl;
						break;
					}
				}
				av_packet_unref(packet);
			}

			av_frame_free(&frame);
			av_packet_free(&packet);

			return frameDecoded;
		}

		bool SeekToFrame(VideoComponent& videoComp, int targetFrame) {
			if (!videoComp.formatCtx || !videoComp.codecCtx) {
				return false;
			}

			// Calculate timestamp for the target frame
			AVStream* videoStream = videoComp.formatCtx->streams[videoComp.videoStreamIdx];
			int64_t timestamp = av_rescale_q(targetFrame, av_inv_q(videoStream->avg_frame_rate), videoStream->time_base);

			// Seek to the timestamp
			int ret = av_seek_frame(videoComp.formatCtx, videoComp.videoStreamIdx, timestamp, AVSEEK_FLAG_BACKWARD);
			if (ret < 0) {
				std::cerr << "[VideoSystem] Failed to seek to frame " << targetFrame << std::endl;
				return false;
			}

			// Flush decoder buffers
			avcodec_flush_buffers(videoComp.codecCtx);

			// Read and decode frames until we reach the target frame
			// FIXED: Replace deprecated av_init_packet with modern FFmpeg approach
			AVPacket* packet = av_packet_alloc();
			if (!packet) {
				std::cerr << "[VideoSystem] Failed to allocate packet for seeking" << std::endl;
				return false;
			}

			AVFrame* frame = av_frame_alloc();
			if (!frame) {
				std::cerr << "[VideoSystem] Failed to allocate frame for seeking" << std::endl;
				av_packet_free(&packet);
				return false;
			}

			int currentFrame = 0;
			bool success = false;

			while ((ret = av_read_frame(videoComp.formatCtx, packet)) >= 0) {
				if (packet->stream_index == videoComp.videoStreamIdx) {
					ret = avcodec_send_packet(videoComp.codecCtx, packet);
					if (ret < 0) {
						std::cerr << "[VideoSystem] Error sending packet to decoder during seek" << std::endl;
						break;
					}

					while (ret >= 0) {
						ret = avcodec_receive_frame(videoComp.codecCtx, frame);
						if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
							break;
						}
						else if (ret < 0) {
							std::cerr << "[VideoSystem] Error receiving frame from decoder during seek" << std::endl;
							goto cleanup;
						}

						if (currentFrame == targetFrame) {
							ConvertFrameToTexture(videoComp, frame);
							videoComp.currentFrame = targetFrame;
							success = true;
							goto cleanup;
						}
						currentFrame++;
					}
				}
				av_packet_unref(packet);
			}

		cleanup:
			av_frame_free(&frame);
			av_packet_free(&packet);

			return success;
		}

		std::vector<EntityID> GetAllVideoEntities() const {
			std::vector<EntityID> videoEntities;
			for (auto entity : entities) {
				videoEntities.push_back(entity);
			}
			return videoEntities;
		}
		
		void RegisterVideoAddedCallback(std::function<void(EntityID)> callback) {
		    videoAddedCallbacks.push_back(callback);
		}

		void RegisterVideoRemovedCallback(std::function<void(EntityID)> callback) {
		    videoRemovedCallbacks.push_back(callback);
		}
		
	private:
		std::vector<std::function<void(EntityID)>> videoAddedCallbacks;
    	std::vector<std::function<void(EntityID)>> videoRemovedCallbacks;

		bool LoadVideo(VideoComponent& videoComp, const std::string& filePath) {
			// Clean up any existing video data
			CleanupVideo(videoComp);

			// Set basic properties
			std::filesystem::path path(filePath);
			videoComp.fileName = path.filename().string();
			videoComp.filePath = filePath;

			// Open video file
			videoComp.formatCtx = avformat_alloc_context();
			if (avformat_open_input(&videoComp.formatCtx, filePath.c_str(), nullptr, nullptr) < 0) {
				std::cerr << "[VideoSystem] Failed to open video file: " << filePath << std::endl;
				return false;
			}

			if (avformat_find_stream_info(videoComp.formatCtx, nullptr) < 0) {
				std::cerr << "[VideoSystem] Failed to find stream info" << std::endl;
				CleanupVideo(videoComp);
				return false;
			}

			// Find video stream
			videoComp.videoStreamIdx = -1;
			for (unsigned int i = 0; i < videoComp.formatCtx->nb_streams; i++) {
				if (videoComp.formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
					videoComp.videoStreamIdx = static_cast<int>(i);
					break;
				}
			}

			if (videoComp.videoStreamIdx == -1) {
				std::cerr << "[VideoSystem] No video stream found" << std::endl;
				CleanupVideo(videoComp);
				return false;
			}

			// Get codec
			AVCodecParameters* codecParams = videoComp.formatCtx->streams[videoComp.videoStreamIdx]->codecpar;
			const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
			if (!codec) {
				std::cerr << "[VideoSystem] Codec not found" << std::endl;
				CleanupVideo(videoComp);
				return false;
			}

			// Create codec context
			videoComp.codecCtx = avcodec_alloc_context3(codec);
			if (avcodec_parameters_to_context(videoComp.codecCtx, codecParams) < 0) {
				std::cerr << "[VideoSystem] Failed to copy codec parameters" << std::endl;
				CleanupVideo(videoComp);
				return false;
			}

			if (avcodec_open2(videoComp.codecCtx, codec, nullptr) < 0) {
				std::cerr << "[VideoSystem] Failed to open codec" << std::endl;
				CleanupVideo(videoComp);
				return false;
			}

			// Set video properties
			videoComp.width = videoComp.codecCtx->width;
			videoComp.height = videoComp.codecCtx->height;
			
			AVStream* videoStream = videoComp.formatCtx->streams[videoComp.videoStreamIdx];
			videoComp.fps = av_q2d(videoStream->avg_frame_rate);
			videoComp.frameDuration = 1.0f / static_cast<float>(videoComp.fps);
			videoComp.frameCount = static_cast<int>(videoStream->nb_frames);
			
			if (videoComp.frameCount <= 0) {
				// Estimate frame count from duration
				double duration = static_cast<double>(videoComp.formatCtx->duration) / AV_TIME_BASE;
				videoComp.frameCount = static_cast<int>(duration * videoComp.fps);
			}

			videoComp.currentFrame = 0;
			videoComp.isPlaying = false;

			std::cout << "[VideoSystem] Video loaded: " << filePath << std::endl;
			std::cout << "  Resolution: " << videoComp.width << "x" << videoComp.height << std::endl;
			std::cout << "  FPS: " << videoComp.fps << std::endl;
			std::cout << "  Frame count: " << videoComp.frameCount << std::endl;

			return true;
		}

		void CleanupVideo(VideoComponent& videoComp) {
			if (videoComp.swsCtx) {
				sws_freeContext(videoComp.swsCtx);
				videoComp.swsCtx = nullptr;
			}

			if (videoComp.rgbFrame) {
				av_frame_free(&videoComp.rgbFrame);
			}

			if (videoComp.frameBuffer) {
				av_free(videoComp.frameBuffer);
				videoComp.frameBuffer = nullptr;
			}

			if (videoComp.codecCtx) {
				avcodec_free_context(&videoComp.codecCtx);
			}

			if (videoComp.formatCtx) {
				avformat_close_input(&videoComp.formatCtx);
			}

			if (videoComp.currentTexture != 0) {
				glDeleteTextures(1, &videoComp.currentTexture);
				videoComp.currentTexture = 0;
			}

			videoComp.currentFrameData.release();
			videoComp.videoStreamIdx = -1;
		}

		void ConvertFrameToTexture(VideoComponent& videoComp, AVFrame* frame) {
			if (!videoComp.swsCtx) {
				// Initialize sws context for format conversion
				videoComp.swsCtx = sws_getContext(
					frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
					frame->width, frame->height, AV_PIX_FMT_RGB24,
					SWS_BILINEAR, nullptr, nullptr, nullptr
				);

				if (!videoComp.swsCtx) {
					std::cerr << "[VideoSystem] Failed to create SWS context" << std::endl;
					return;
				}

				// Allocate RGB frame
				videoComp.rgbFrame = av_frame_alloc();
				if (!videoComp.rgbFrame) {
					std::cerr << "[VideoSystem] Failed to allocate RGB frame" << std::endl;
					return;
				}

				videoComp.rgbFrame->format = AV_PIX_FMT_RGB24;
				videoComp.rgbFrame->width = frame->width;
				videoComp.rgbFrame->height = frame->height;

				// FIXED: Declare ret variable
				int ret = av_frame_get_buffer(videoComp.rgbFrame, 32);
				if (ret < 0) {
					std::cerr << "[VideoSystem] Failed to allocate RGB frame buffer" << std::endl;
					return;
				}
			}

			// Convert frame to RGB
			sws_scale(videoComp.swsCtx, frame->data, frame->linesize, 0, frame->height,
				videoComp.rgbFrame->data, videoComp.rgbFrame->linesize);

			// Convert to OpenCV Mat for texture upload
			cv::Mat rgbMat(frame->height, frame->width, CV_8UC3, videoComp.rgbFrame->data[0], videoComp.rgbFrame->linesize[0]);
			videoComp.currentFrameData = rgbMat.clone();

			// Update OpenGL texture
			UpdateVideoTexture(videoComp);
		}

		void UpdateVideoTexture(VideoComponent& videoComp) {
			if (videoComp.currentFrameData.empty()) {
				return;
			}

			// Create texture if it doesn't exist
			if (videoComp.currentTexture == 0) {
				glGenTextures(1, &videoComp.currentTexture);
			}

			glBindTexture(GL_TEXTURE_2D, videoComp.currentTexture);

			// Set texture parameters
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			// Convert BGR to RGB if necessary
			cv::Mat rgbMat;
			if (videoComp.currentFrameData.channels() == 3) {
				cv::cvtColor(videoComp.currentFrameData, rgbMat, cv::COLOR_BGR2RGB);
			}
			else {
				rgbMat = videoComp.currentFrameData;
			}

			// Upload texture data
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, rgbMat.cols, rgbMat.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, rgbMat.data);

			glBindTexture(GL_TEXTURE_2D, 0);
		}
	};

} // namespace ECS