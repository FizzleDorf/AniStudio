// AniStudioCallbacksUtil.hpp - Updated with AVMasterSystem only
#pragma once

#include "EntityManager.hpp"
#include "Events.hpp"
#include "TextureSystem.hpp"
#include "ImageComponent.hpp"
#include "VideoComponent.hpp"
#include "AudioComponent.hpp"
#include "TextureComponent.hpp"
#include "AVMasterSystem.hpp"
#include <iostream>
#include <cstdlib>

namespace ANI {

    inline void RegisterCoreCallbacks(ECS::EntityManager& entityMgr) {
        std::cout << "[AniStudioCallbacks] Registering core callbacks..." << std::endl;

        auto textureSystem = entityMgr.GetSystem<ECS::TextureSystem>();
        auto imageSystem = entityMgr.GetSystem<ECS::ImageSystem>();
        auto avMaster = entityMgr.GetSystem<ECS::AVMasterSystem>();

        if (!textureSystem) {
            std::cerr << "[AniStudioCallbacks] ERROR: TextureSystem not found!" << std::endl;
            return;
        }

        if (!avMaster) {
            std::cerr << "[AniStudioCallbacks] ERROR: AVMasterSystem not found!" << std::endl;
            return;
        }

        // Register AVMasterSystem frame callback for video display
        avMaster->RegisterFrameCallback(
            [textureSystem, &entityMgr](ECS::EntityID entity, const uint8_t* data, int width, int height) {
                if (!textureSystem) return;

                if (!entityMgr.IsEntityValid(entity)) return;

                if (!entityMgr.HasComponent<ECS::TextureComponent>(entity)) {
                    entityMgr.AddComponent<ECS::TextureComponent>(entity);
                }

                size_t dataSize = static_cast<size_t>(width) * height * 4;
                unsigned char* copy = static_cast<unsigned char*>(malloc(dataSize));
                if (copy) {
                    memcpy(copy, data, dataSize);
                    textureSystem->QueueVideoTextureCreation(entity, copy, width, height, 4);
                }
            }
        );
        std::cout << "[AniStudioCallbacks] Frame callback registered" << std::endl;

        // Register AVMasterSystem audio callback
        avMaster->RegisterAudioCallback(
            [](ECS::EntityID entity, const float* data,
                size_t numSamples, int channels, int sampleRate, double pts) {
                    // Audio data is ready for playback
                    (void)entity;
                    (void)data;
                    (void)numSamples;
                    (void)channels;
                    (void)sampleRate;
                    (void)pts;
            }
        );
        std::cout << "[AniStudioCallbacks] Audio callback registered" << std::endl;

        // Register AVMasterSystem end callback
        avMaster->RegisterEndCallback(
            [](ECS::EntityID entity) {
                std::cout << "[AniStudioCallbacks] Playback ended for entity " << entity << std::endl;
                ANI::Events::Ref().QueueEventWithData("PlaybackEnded", entity);
            }
        );
        std::cout << "[AniStudioCallbacks] End callback registered" << std::endl;

        // Register AVMasterSystem error callback
        avMaster->RegisterErrorCallback(
            [](ECS::EntityID entity, const std::string& error) {
                std::cerr << "[AniStudioCallbacks] Media error for entity " << entity
                    << ": " << error << std::endl;
                ANI::Events::Ref().QueueEventWithData("MediaError", entity);
            }
        );
        std::cout << "[AniStudioCallbacks] Error callback registered" << std::endl;

        // ImageSystem callbacks (if available)
        if (imageSystem) {
            imageSystem->RegisterImageAddedCallback(
                [textureSystem, &entityMgr](ECS::EntityID entityID) {
                    if (!entityMgr.IsEntityValid(entityID)) return;

                    if (entityMgr.HasComponent<ECS::ImageComponent>(entityID)) {
                        auto& imageComp = entityMgr.GetComponent<ECS::ImageComponent>(entityID);
                        if (imageComp.imageData) {
                            textureSystem->QueueTextureCreation(
                                entityID,
                                imageComp.imageData,
                                imageComp.width,
                                imageComp.height,
                                imageComp.channels
                            );
                            ANI::Events::Ref().QueueEventWithData("ImageLoaded", entityID);
                        }
                    }
                }
            );

            imageSystem->RegisterImageRemovedCallback(
                [textureSystem](ECS::EntityID entityID) {
                    textureSystem->RemoveTexture(entityID);
                    ANI::Events::Ref().QueueEventWithData("ImageRemoved", entityID);
                }
            );
            std::cout << "[AniStudioCallbacks] ImageSystem callbacks registered" << std::endl;
        }

        std::cout << "[AniStudioCallbacks] All core callbacks registered successfully." << std::endl;
    }

    // Helper function to connect AVMasterSystem to AudioOutputSystem
    inline void ConnectAudioOutput(ECS::EntityManager& entityMgr) {
        auto avMaster = entityMgr.GetSystem<ECS::AVMasterSystem>();
        auto audioOutput = entityMgr.GetSystem<ECS::AudioOutputSystem>();

        if (!avMaster) {
            std::cerr << "[AniStudioCallbacks] ERROR: AVMasterSystem not found!" << std::endl;
            return;
        }

        if (!audioOutput) {
            std::cerr << "[AniStudioCallbacks] ERROR: AudioOutputSystem not found!" << std::endl;
            return;
        }

        // Register audio callback directly to AudioOutputSystem
        avMaster->RegisterAudioCallback(
            [audioOutput](ECS::EntityID entity, const float* data,
                size_t numSamples, int channels, int sampleRate, double pts) {
                    if (audioOutput && audioOutput->IsValid()) {
                        audioOutput->FeedAudioData(entity, data, numSamples,
                            channels, sampleRate, pts);
                    }
            }
        );
        std::cout << "[AniStudioCallbacks] AVMasterSystem connected to AudioOutputSystem" << std::endl;
    }

    // Helper function to connect AVMasterSystem to TextureSystem for thumbnails
    inline void ConnectVideoTexture(ECS::EntityManager& entityMgr) {
        auto avMaster = entityMgr.GetSystem<ECS::AVMasterSystem>();
        auto textureSystem = entityMgr.GetSystem<ECS::TextureSystem>();

        if (!avMaster) {
            std::cerr << "[AniStudioCallbacks] ERROR: AVMasterSystem not found!" << std::endl;
            return;
        }

        if (!textureSystem) {
            std::cerr << "[AniStudioCallbacks] ERROR: TextureSystem not found!" << std::endl;
            return;
        }

        // Register frame callback for texture creation
        avMaster->RegisterFrameCallback(
            [textureSystem, &entityMgr](ECS::EntityID entity, const uint8_t* data, int width, int height) {
                if (!textureSystem) return;

                if (!entityMgr.IsEntityValid(entity)) return;

                if (!entityMgr.HasComponent<ECS::TextureComponent>(entity)) {
                    entityMgr.AddComponent<ECS::TextureComponent>(entity);
                }

                size_t dataSize = static_cast<size_t>(width) * height * 4;
                unsigned char* copy = static_cast<unsigned char*>(malloc(dataSize));
                if (copy) {
                    memcpy(copy, data, dataSize);
                    textureSystem->QueueVideoTextureCreation(entity, copy, width, height, 4);
                }
            }
        );
        std::cout << "[AniStudioCallbacks] AVMasterSystem connected to TextureSystem" << std::endl;
    }

} // namespace ANI