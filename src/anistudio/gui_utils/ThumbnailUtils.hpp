///*
//		d8888          d8b  .d8888b.  888                  888 d8b
//	   d88888          Y8P d88P  Y88b 888                  888 Y8P
//	  d88P888              Y88b.      888                  888
//	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
//	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
//   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
//  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
// d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"
//
// * This file is part of AniStudio.
// * Copyright (C) 2025 FizzleDorf (AnimAnon)
// *
// * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
// * and a commercial license. You may choose to use it under either license.
// *
// * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
// * For commercial license information, please contact legal@kframe.ai.
// */
//
//#pragma once
//
//#include <pch.h>
//#include <GL/glew.h>
//#include <ImGuiFileDialog.h>
//#include <iostream>
//
//namespace Utils {
//
//	/**
//	 * @brief Utility class for managing ImGuiFileDialog thumbnails with OpenGL
//	 *
//	 * This class provides static methods to initialize and manage GPU thumbnails
//	 * for the ImGuiFileDialog library. It handles OpenGL texture creation and
//	 * destruction for thumbnail display.
//	 */
//	class ThumbnailUtils {
//	private:
//		static inline bool thumbnailsInitialized = false;
//
//	public:
//		/**
//		 * @brief Initialize thumbnail support for ImGuiFileDialog
//		 *
//		 * This function sets up the thumbnail callbacks for ImGuiFileDialog to handle
//		 * OpenGL texture creation and destruction. Should be called once during
//		 * application initialization after OpenGL context is ready.
//		 *
//		 * @return true if thumbnails were successfully initialized, false otherwise
//		 */
//		static bool InitializeThumbnails() {
//			if (thumbnailsInitialized) {
//				std::cout << "[ThumbnailUtils] Thumbnails already initialized" << std::endl;
//				return true;
//			}
//
//			try {
//				std::cout << "[ThumbnailUtils] Initializing file dialog thumbnails..." << std::endl;
//
//				// Set up create thumbnail callback
//				ImGuiFileDialog::Instance()->SetCreateThumbnailCallback(
//					[](IGFD_Thumbnail_Info* thumbnailInfo) {
//					CreateThumbnailTexture(thumbnailInfo);
//				}
//				);
//
//				// Set up destroy thumbnail callback
//				ImGuiFileDialog::Instance()->SetDestroyThumbnailCallback(
//					[](IGFD_Thumbnail_Info* thumbnailInfo) {
//					DestroyThumbnailTexture(thumbnailInfo);
//				}
//				);
//
//				thumbnailsInitialized = true;
//				std::cout << "[ThumbnailUtils] File dialog thumbnails initialized successfully!" << std::endl;
//				return true;
//			}
//			catch (const std::exception& e) {
//				std::cerr << "[ThumbnailUtils] Failed to initialize thumbnails: " << e.what() << std::endl;
//				return false;
//			}
//		}
//
//		/**
//		 * @brief Manage GPU thumbnails - call this every frame
//		 *
//		 * This function should be called every frame in your render loop to handle
//		 * GPU texture creation and destruction for thumbnails. It processes any
//		 * pending thumbnail operations.
//		 */
//		static void ManageGPUThumbnails() {
//			if (thumbnailsInitialized) {
//				ImGuiFileDialog::Instance()->ManageGPUThumbnails();
//			}
//		}
//
//		/**
//		 * @brief Check if thumbnails are initialized
//		 *
//		 * @return true if thumbnails have been initialized, false otherwise
//		 */
//		static bool AreThumbnailsInitialized() {
//			return thumbnailsInitialized;
//		}
//
//		/**
//		 * @brief Cleanup thumbnail resources
//		 *
//		 * Should be called during application shutdown to properly cleanup
//		 * any remaining thumbnail resources.
//		 */
//		static void Cleanup() {
//			if (thumbnailsInitialized) {
//				std::cout << "[ThumbnailUtils] Cleaning up thumbnail resources..." << std::endl;
//				// Note: ImGuiFileDialog will handle cleanup of its own resources
//				thumbnailsInitialized = false;
//			}
//		}
//
//	private:
//		/**
//		 * @brief Create OpenGL texture from thumbnail data
//		 *
//		 * @param thumbnailInfo Pointer to thumbnail information structure
//		 */
//		static void CreateThumbnailTexture(IGFD_Thumbnail_Info* thumbnailInfo) {
//			if (!thumbnailInfo || !thumbnailInfo->isReadyToUpload || !thumbnailInfo->textureFileDatas) {
//				return;
//			}
//
//			try {
//				GLuint textureId = 0;
//				glGenTextures(1, &textureId);
//
//				if (textureId == 0) {
//					std::cerr << "[ThumbnailUtils] Failed to generate OpenGL texture" << std::endl;
//					return;
//				}
//
//				thumbnailInfo->textureID = reinterpret_cast<void*>(static_cast<uintptr_t>(textureId));
//
//				glBindTexture(GL_TEXTURE_2D, textureId);
//
//				// Set texture parameters for optimal thumbnail display
//				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//
//				// Upload texture data
//				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
//					static_cast<GLsizei>(thumbnailInfo->textureWidth),
//					static_cast<GLsizei>(thumbnailInfo->textureHeight),
//					0, GL_RGBA, GL_UNSIGNED_BYTE,
//					thumbnailInfo->textureFileDatas);
//
//				// Check for OpenGL errors
//				GLenum error = glGetError();
//				if (error != GL_NO_ERROR) {
//					std::cerr << "[ThumbnailUtils] OpenGL error creating texture: " << error << std::endl;
//					glDeleteTextures(1, &textureId);
//					thumbnailInfo->textureID = nullptr;
//					return;
//				}
//
//				glFinish();
//				glBindTexture(GL_TEXTURE_2D, 0);
//
//				// Clean up the CPU data
//				delete[] thumbnailInfo->textureFileDatas;
//				thumbnailInfo->textureFileDatas = nullptr;
//				thumbnailInfo->isReadyToUpload = false;
//				thumbnailInfo->isReadyToDisplay = true;
//
//				// Debug output
//				// std::cout << "[ThumbnailUtils] Created thumbnail texture: " << textureId 
//				//           << " (" << thumbnailInfo->textureWidth << "x" << thumbnailInfo->textureHeight << ")" << std::endl;
//			}
//			catch (const std::exception& e) {
//				std::cerr << "[ThumbnailUtils] Exception creating thumbnail texture: " << e.what() << std::endl;
//				if (thumbnailInfo->textureFileDatas) {
//					delete[] thumbnailInfo->textureFileDatas;
//					thumbnailInfo->textureFileDatas = nullptr;
//				}
//				thumbnailInfo->textureID = nullptr;
//				thumbnailInfo->isReadyToUpload = false;
//				thumbnailInfo->isReadyToDisplay = false;
//			}
//		}
//
//		/**
//		 * @brief Destroy OpenGL texture for thumbnail
//		 *
//		 * @param thumbnailInfo Pointer to thumbnail information structure
//		 */
//		static void DestroyThumbnailTexture(IGFD_Thumbnail_Info* thumbnailInfo) {
//			if (!thumbnailInfo || !thumbnailInfo->textureID) {
//				return;
//			}
//
//			try {
//				GLuint textureId = static_cast<GLuint>(reinterpret_cast<uintptr_t>(thumbnailInfo->textureID));
//
//				if (textureId != 0) {
//					glDeleteTextures(1, &textureId);
//					glFinish();
//
//					// Debug output
//					// std::cout << "[ThumbnailUtils] Destroyed thumbnail texture: " << textureId << std::endl;
//				}
//
//				thumbnailInfo->textureID = nullptr;
//				thumbnailInfo->isReadyToDisplay = false;
//			}
//			catch (const std::exception& e) {
//				std::cerr << "[ThumbnailUtils] Exception destroying thumbnail texture: " << e.what() << std::endl;
//			}
//		}
//	};
//
//} // namespace Utils