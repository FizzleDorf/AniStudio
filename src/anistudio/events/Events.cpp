#include "Events.hpp"
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <iostream>

namespace ANI {

    // PIMPL implementation
    struct Events::Impl {
        struct EventData {
            std::vector<EventCallback> simpleCallbacks;
            std::vector<EventCallbackWithData> dataCallbacks;
        };

        struct QueuedEvent {
            std::string eventName;
            bool hasData = false;
            std::any data;
        };

        std::unordered_map<std::string, EventData> eventHandlers;
        std::vector<QueuedEvent> eventQueue;
    };

    Events& Events::Ref() {
        static Events instance;
        return instance;
    }

    Events::Events() : pImpl(std::make_unique<Impl>()) {
        std::cout << "[Events] Created instance: " << this << std::endl;
    }

    Events::~Events() {
        ClearAllEvents();
    }

    void Events::RegisterEvent(const std::string& eventName, EventCallback callback) {
        pImpl->eventHandlers[eventName].simpleCallbacks.push_back(callback);
        std::cout << "[Events] Registered callback for: " << eventName << std::endl;
    }

    void Events::RegisterEventWithData(const std::string& eventName, EventCallbackWithData callback) {
        pImpl->eventHandlers[eventName].dataCallbacks.push_back(callback);
        std::cout << "[Events] Registered data callback for: " << eventName << std::endl;
    }

    void Events::QueueEvent(const std::string& eventName) {
        Impl::QueuedEvent event;
        event.eventName = eventName;
        event.hasData = false;
        pImpl->eventQueue.push_back(event);
        std::cout << "[Events] Queued event: " << eventName << std::endl;
    }

    void Events::QueueEventWithDataImpl(const std::string& eventName, const std::any& data) {
        Impl::QueuedEvent event;
        event.eventName = eventName;
        event.hasData = true;
        event.data = data;
        pImpl->eventQueue.push_back(event);
        std::cout << "[Events] Queued event with data: " << eventName << std::endl;
    }

    void Events::Poll() {
        if (pImpl->eventQueue.empty()) return;

        for (const auto& queuedEvent : pImpl->eventQueue) {
            auto it = pImpl->eventHandlers.find(queuedEvent.eventName);
            if (it != pImpl->eventHandlers.end()) {
                const Impl::EventData& eventData = it->second;

                for (const auto& callback : eventData.simpleCallbacks) {
                    if (callback) callback();
                }

                if (queuedEvent.hasData) {
                    for (const auto& callback : eventData.dataCallbacks) {
                        if (callback) callback(queuedEvent.data);
                    }
                }

                std::cout << "[Events] Processed: " << queuedEvent.eventName << std::endl;
            }
        }

        pImpl->eventQueue.clear();
    }

    void Events::UnregisterEvent(const std::string& eventName) {
        auto it = pImpl->eventHandlers.find(eventName);
        if (it != pImpl->eventHandlers.end()) {
            pImpl->eventHandlers.erase(it);
            std::cout << "[Events] Unregistered: " << eventName << std::endl;
        }
    }

    void Events::UnregisterEvent(const std::string& eventName, EventCallback callback) {
        auto it = pImpl->eventHandlers.find(eventName);
        if (it != pImpl->eventHandlers.end()) {
            auto& callbacks = it->second.simpleCallbacks;
            callbacks.erase(
                std::remove_if(callbacks.begin(), callbacks.end(),
                    [&callback](const EventCallback& cb) {
                        return cb.target_type() == callback.target_type();
                    }),
                callbacks.end()
            );

            if (callbacks.empty() && it->second.dataCallbacks.empty()) {
                pImpl->eventHandlers.erase(it);
            }
        }
    }

    void Events::UnregisterEventWithData(const std::string& eventName, EventCallbackWithData callback) {
        auto it = pImpl->eventHandlers.find(eventName);
        if (it != pImpl->eventHandlers.end()) {
            auto& callbacks = it->second.dataCallbacks;
            callbacks.erase(
                std::remove_if(callbacks.begin(), callbacks.end(),
                    [&callback](const EventCallbackWithData& cb) {
                        return cb.target_type() == callback.target_type();
                    }),
                callbacks.end()
            );

            if (it->second.simpleCallbacks.empty() && callbacks.empty()) {
                pImpl->eventHandlers.erase(it);
            }
        }
    }

    void Events::UnregisterAllEventsForPlugin(const std::string& pluginName) {
        std::string prefix = "Plugin_" + pluginName + "_";
        auto it = pImpl->eventHandlers.begin();
        while (it != pImpl->eventHandlers.end()) {
            if (it->first.find(prefix) == 0) {
                it = pImpl->eventHandlers.erase(it);
            }
            else {
                ++it;
            }
        }
        std::cout << "[Events] Unregistered plugin: " << pluginName << std::endl;
    }

    void Events::ClearAllEvents() {
        pImpl->eventHandlers.clear();
        pImpl->eventQueue.clear();
        std::cout << "[Events] Cleared all events" << std::endl;
    }

} // namespace ANI