#pragma once

#include <string>
#include <GL/glew.h>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <iostream>
#include <mutex>

namespace Utils {

	class VideoUtils {
	public:
		// Load a single frame from a video file at the specified time
		static unsigned char* LoadVideoFrame(const std::string& filePath, double timeInSeconds,
			int& width, int& height, int& channels,
			double* actualTime = nullptr) {
			try {
				// Open video file
				cv::VideoCapture cap(filePath);
				if (!cap.isOpened()) {
					std::cerr << "Could not open video file: " << filePath << std::endl;
					return nullptr;
				}

				// Get video properties
				width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
				height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
				channels = 4; // RGBA

				// Calculate frame position
				double fps = cap.get(cv::CAP_PROP_FPS);
				double frameCount = cap.get(cv::CAP_PROP_FRAME_COUNT);
				double durationInSeconds = frameCount / fps;

				// Clamp time to video duration
				double clampedTime = std::min(timeInSeconds, durationInSeconds);

				// Seek to the requested time position
				int frameNumber = static_cast<int>(clampedTime * fps);
				cap.set(cv::CAP_PROP_POS_FRAMES, frameNumber);

				// Read the frame
				cv::Mat frame;
				if (!cap.read(frame)) {
					// If failed, try seeking to the last frame
					cap.set(cv::CAP_PROP_POS_FRAMES, frameCount - 1);
					if (!cap.read(frame)) {
						std::cerr << "Failed to read frame at time: " << timeInSeconds << " seconds" << std::endl;
						return nullptr;
					}

					// Update actual time
					if (actualTime) {
						*actualTime = (frameCount - 1) / fps;
					}
				}
				else {
					// Update actual time
					if (actualTime) {
						*actualTime = frameNumber / fps;
					}
				}

				// Convert frame to RGBA (OpenCV uses BGR by default)
				cv::Mat frameRGBA;
				cv::cvtColor(frame, frameRGBA, cv::COLOR_BGR2RGBA);

				// Allocate memory and copy data
				size_t dataSize = width * height * 4;
				unsigned char* frameData = static_cast<unsigned char*>(malloc(dataSize));
				if (!frameData) {
					std::cerr << "Failed to allocate memory for frame data" << std::endl;
					return nullptr;
				}

				// Copy the data
				memcpy(frameData, frameRGBA.data, dataSize);

				return frameData;
			}
			catch (const cv::Exception& e) {
				std::cerr << "OpenCV exception when loading frame: " << e.what() << std::endl;
				return nullptr;
			}
			catch (const std::exception& e) {
				std::cerr << "Exception when loading frame: " << e.what() << std::endl;
				return nullptr;
			}
		}

		// Get video information without loading frames
		static bool GetVideoInfo(const std::string& filePath, int& width, int& height,
			double& duration, double& frameRate) {
			try {
				cv::VideoCapture cap(filePath);
				if (!cap.isOpened()) {
					std::cerr << "Could not open video file: " << filePath << std::endl;
					return false;
				}

				// Get video properties
				width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
				height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
				frameRate = cap.get(cv::CAP_PROP_FPS);

				// Calculate duration
				double frameCount = cap.get(cv::CAP_PROP_FRAME_COUNT);
				duration = frameCount / frameRate;

				// Error checking
				if (width <= 0 || height <= 0 || frameRate <= 0 || duration <= 0) {
					std::cerr << "Invalid video properties for: " << filePath << std::endl;
					return false;
				}

				return true;
			}
			catch (const cv::Exception& e) {
				std::cerr << "OpenCV exception when getting video info: " << e.what() << std::endl;
				return false;
			}
			catch (const std::exception& e) {
				std::cerr << "Exception when getting video info: " << e.what() << std::endl;
				return false;
			}
		}

		// Generate OpenGL texture from frame data - MUST be called from main thread
		static GLuint GenerateTextureFromVideoFrame(unsigned char* data, int width, int height) {
			if (!data || width <= 0 || height <= 0) {
				return 0;
			}

			try {
				GLuint textureID;
				glGenTextures(1, &textureID);
				glBindTexture(GL_TEXTURE_2D, textureID);

				// Set texture parameters
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				// Upload texture data
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

				// Unbind texture
				glBindTexture(GL_TEXTURE_2D, 0);

				return textureID;
			}
			catch (const std::exception& e) {
				std::cerr << "Exception generating texture: " << e.what() << std::endl;
				return 0;
			}
		}

		// Delete OpenGL texture - MUST be called from main thread
		static void DeleteTexture(GLuint& textureID) {
			if (textureID == 0) {
				return;
			}

			try {
				glDeleteTextures(1, &textureID);
				textureID = 0;
			}
			catch (const std::exception& e) {
				std::cerr << "Exception deleting texture: " << e.what() << std::endl;
			}
		}

		// Free video frame data
		static void FreeVideoFrameData(unsigned char* data) {
			if (data) {
				free(data);
			}
		}

		// Save a video frame as an image
		static bool SaveVideoFrameAsImage(const std::string& videoPath, const std::string& imagePath,
			double timeInSeconds) {
			try {
				int width, height, channels;
				unsigned char* frameData = LoadVideoFrame(videoPath, timeInSeconds, width, height, channels);

				if (!frameData) {
					return false;
				}

				// Create directories if they don't exist
				std::filesystem::path outputDir = std::filesystem::path(imagePath).parent_path();
				if (!outputDir.empty() && !std::filesystem::exists(outputDir)) {
					std::filesystem::create_directories(outputDir);
				}

				// Save image using OpenCV
				cv::Mat frameRGBA(height, width, CV_8UC4, frameData);

				// Convert to BGR for saving with OpenCV
				cv::Mat frameBGR;
				cv::cvtColor(frameRGBA, frameBGR, cv::COLOR_RGBA2BGR);

				bool success = cv::imwrite(imagePath, frameBGR);

				// Free allocated memory
				FreeVideoFrameData(frameData);

				return success;
			}
			catch (const std::exception& e) {
				std::cerr << "Exception saving video frame: " << e.what() << std::endl;
				return false;
			}
		}

		// Export a clip from video file
		static bool ExportVideoClip(const std::string& inputPath, const std::string& outputPath,
			double startTime, double endTime, int width = 0, int height = 0,
			double frameRate = 0.0, const std::string& codecFourCC = "mp4v", int bitrate = 8000000) {
			try {
				// Open input video file
				cv::VideoCapture cap(inputPath);
				if (!cap.isOpened()) {
					std::cerr << "Could not open input video file: " << inputPath << std::endl;
					return false;
				}

				// Get source properties
				int srcWidth = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
				int srcHeight = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
				double srcFps = cap.get(cv::CAP_PROP_FPS);
				double srcDuration = cap.get(cv::CAP_PROP_FRAME_COUNT) / srcFps;

				// Use source values if custom values not specified
				int outWidth = (width > 0) ? width : srcWidth;
				int outHeight = (height > 0) ? height : srcHeight;
				double outFps = (frameRate > 0.0) ? frameRate : srcFps;

				// Validate time range
				double validStart = std::max(0.0, std::min(startTime, srcDuration));
				double validEnd = std::max(validStart, std::min(endTime, srcDuration));

				// Calculate frame positions
				int startFrame = static_cast<int>(validStart * srcFps);
				int endFrame = static_cast<int>(validEnd * srcFps);
				int frameCount = endFrame - startFrame;

				if (frameCount <= 0) {
					std::cerr << "Invalid time range for export: " << validStart << " to " << validEnd << std::endl;
					return false;
				}

				// Create output directory if needed
				std::filesystem::path outputDir = std::filesystem::path(outputPath).parent_path();
				if (!outputDir.empty() && !std::filesystem::exists(outputDir)) {
					std::filesystem::create_directories(outputDir);
				}

				// Set codec
				int fourcc;
				if (codecFourCC.length() == 4) {
					fourcc = cv::VideoWriter::fourcc(
						codecFourCC[0], codecFourCC[1], codecFourCC[2], codecFourCC[3]);
				}
				else {
					// Default to mp4v if invalid codec provided
					fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
				}

				// Create video writer
				cv::VideoWriter writer(outputPath, fourcc, outFps, cv::Size(outWidth, outHeight));

				if (!writer.isOpened()) {
					std::cerr << "Failed to create output video file: " << outputPath << std::endl;
					return false;
				}

				// Seek to start frame
				cap.set(cv::CAP_PROP_POS_FRAMES, startFrame);

				// Process frames
				cv::Mat frame, resizedFrame;
				int processedFrames = 0;

				for (int i = 0; i < frameCount; i++) {
					if (!cap.read(frame)) {
						break;
					}

					// Resize if needed
					if (outWidth != srcWidth || outHeight != srcHeight) {
						cv::resize(frame, resizedFrame, cv::Size(outWidth, outHeight));
						writer.write(resizedFrame);
					}
					else {
						writer.write(frame);
					}

					processedFrames++;

					// Report progress every 10%
					if (i % (frameCount / 10 + 1) == 0) {
						int percent = static_cast<int>(static_cast<double>(i) / frameCount * 100);
						std::cout << "Exporting video: " << percent << "% complete" << std::endl;
					}
				}

				// Release resources
				writer.release();
				cap.release();

				std::cout << "Exported " << processedFrames << " frames to " << outputPath << std::endl;
				return (processedFrames > 0);
			}
			catch (const cv::Exception& e) {
				std::cerr << "OpenCV exception when exporting video: " << e.what() << std::endl;
				return false;
			}
			catch (const std::exception& e) {
				std::cerr << "Exception when exporting video: " << e.what() << std::endl;
				return false;
			}
		}
	};

} // namespace Utils