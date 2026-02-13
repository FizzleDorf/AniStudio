#include "Events.hpp"
#include <algorithm>

namespace ANI {
    Events& Events::Ref() {
        static Events instance;
        return instance;
    }

    Events::Events() {
        std::cout << "[Events] Created instance: " << this << std::endl;
    }

    Events::~Events() {
        ClearAllEvents();
    }

    void Events::RegisterEvent(const std::string& eventName, EventCallback callback) {
        eventHandlers[eventName].simpleCallbacks.push_back(callback);
        std::cout << "[Events] Registered callback for: " << eventName << std::endl;
    }

    void Events::RegisterEventWithData(const std::string& eventName, EventCallbackWithData callback) {
        eventHandlers[eventName].dataCallbacks.push_back(callback);
        std::cout << "[Events] Registered data callback for: " << eventName << std::endl;
    }

    void Events::QueueEvent(const std::string& eventName) {
        QueuedEvent event;
        event.eventName = eventName;
        event.hasData = false;
        eventQueue.push_back(event);
        std::cout << "[Events] Queued event: " << eventName << std::endl;
    }

    void Events::Poll() {
        if (eventQueue.empty()) return;

        for (const auto& queuedEvent : eventQueue) {
            auto it = eventHandlers.find(queuedEvent.eventName);
            if (it != eventHandlers.end()) {
                const EventData& eventData = it->second;
                
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

        eventQueue.clear();
    }

    void Events::UnregisterEvent(const std::string& eventName) {
        auto it = eventHandlers.find(eventName);
        if (it != eventHandlers.end()) {
            eventHandlers.erase(it);
            std::cout << "[Events] Unregistered: " << eventName << std::endl;
        }
    }

    void Events::UnregisterEvent(const std::string& eventName, EventCallback callback) {
        auto it = eventHandlers.find(eventName);
        if (it != eventHandlers.end()) {
            auto& callbacks = it->second.simpleCallbacks;
            callbacks.erase(
                std::remove_if(callbacks.begin(), callbacks.end(),
                    [&callback](const EventCallback& cb) {
                        return cb.target_type() == callback.target_type();
                    }),
                callbacks.end()
            );

            if (callbacks.empty() && it->second.dataCallbacks.empty()) {
                eventHandlers.erase(it);
            }
        }
    }

    void Events::UnregisterEventWithData(const std::string& eventName, EventCallbackWithData callback) {
        auto it = eventHandlers.find(eventName);
        if (it != eventHandlers.end()) {
            auto& callbacks = it->second.dataCallbacks;
            callbacks.erase(
                std::remove_if(callbacks.begin(), callbacks.end(),
                    [&callback](const EventCallbackWithData& cb) {
                        return cb.target_type() == callback.target_type();
                    }),
                callbacks.end()
            );

            if (it->second.simpleCallbacks.empty() && callbacks.empty()) {
                eventHandlers.erase(it);
            }
        }
    }

    void Events::UnregisterAllEventsForPlugin(const std::string& pluginName) {
        std::string prefix = "Plugin_" + pluginName + "_";
        auto it = eventHandlers.begin();
        while (it != eventHandlers.end()) {
            if (it->first.find(prefix) == 0) {
                it = eventHandlers.erase(it);
            } else {
                ++it;
            }
        }
        std::cout << "[Events] Unregistered plugin: " << pluginName << std::endl;
    }

    void Events::ClearAllEvents() {
        eventHandlers.clear();
        eventQueue.clear();
        std::cout << "[Events] Cleared all events" << std::endl;
    }
}