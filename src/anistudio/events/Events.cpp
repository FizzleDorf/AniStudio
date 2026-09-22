#include "Events.hpp"
#include "Log.hpp"

#include <algorithm>
#include <unordered_map>
#include <vector>

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
        ANI_LOG_DEBUG("[Events] Created instance: %p", static_cast<const void*>(this));
    }

    Events::~Events() {
        ClearAllEvents();
    }

    void Events::RegisterEvent(const std::string& eventName, EventCallback callback) {
        pImpl->eventHandlers[eventName].simpleCallbacks.push_back(callback);
        ANI_LOG_DEBUG("[Events] Registered callback for: %s", eventName.c_str());
    }

    void Events::RegisterEventWithData(const std::string& eventName, EventCallbackWithData callback) {
        pImpl->eventHandlers[eventName].dataCallbacks.push_back(callback);
        ANI_LOG_DEBUG("[Events] Registered data callback for: %s", eventName.c_str());
    }

    void Events::QueueEvent(const std::string& eventName) {
        Impl::QueuedEvent event;
        event.eventName = eventName;
        event.hasData = false;
        pImpl->eventQueue.push_back(event);
        ANI_LOG_TRACE("[Events] Queued event: %s", eventName.c_str());
    }

    void Events::QueueEventWithDataImpl(const std::string& eventName, const std::any& data) {
        Impl::QueuedEvent event;
        event.eventName = eventName;
        event.hasData = true;
        event.data = data;
        pImpl->eventQueue.push_back(event);
        ANI_LOG_TRACE("[Events] Queued event with data: %s", eventName.c_str());
    }

    void Events::Poll() {
        if (pImpl->eventQueue.empty()) return;

        for (const auto& queuedEvent : pImpl->eventQueue) {
            auto it = pImpl->eventHandlers.find(queuedEvent.eventName);
            if (it == pImpl->eventHandlers.end()) {
                ANI_LOG_TRACE("[Events] No handler for queued event: %s", queuedEvent.eventName.c_str());
                continue;
            }

            const Impl::EventData& eventData = it->second;

            for (const auto& callback : eventData.simpleCallbacks) {
                if (!callback) continue;
                try {
                    callback();
                }
                catch (const std::exception& e) {
                    ANI_LOG_ERROR("[Events] Exception in callback for '%s': %s",
                        queuedEvent.eventName.c_str(), e.what());
                }
                catch (...) {
                    ANI_LOG_ERROR("[Events] Unknown exception in callback for '%s'",
                        queuedEvent.eventName.c_str());
                }
            }

            if (queuedEvent.hasData) {
                for (const auto& callback : eventData.dataCallbacks) {
                    if (!callback) continue;
                    try {
                        callback(queuedEvent.data);
                    }
                    catch (const std::exception& e) {
                        ANI_LOG_ERROR("[Events] Exception in data callback for '%s': %s",
                            queuedEvent.eventName.c_str(), e.what());
                    }
                    catch (...) {
                        ANI_LOG_ERROR("[Events] Unknown exception in data callback for '%s'",
                            queuedEvent.eventName.c_str());
                    }
                }
            }

            ANI_LOG_TRACE("[Events] Processed: %s", queuedEvent.eventName.c_str());
        }

        pImpl->eventQueue.clear();
    }

    void Events::UnregisterEvent(const std::string& eventName) {
        auto it = pImpl->eventHandlers.find(eventName);
        if (it == pImpl->eventHandlers.end()) {
            ANI_LOG_TRACE("[Events] UnregisterEvent: no handlers for '%s'", eventName.c_str());
            return;
        }
        pImpl->eventHandlers.erase(it);
        ANI_LOG_DEBUG("[Events] Unregistered: %s", eventName.c_str());
    }

    void Events::UnregisterEvent(const std::string& eventName, EventCallback callback) {
        auto it = pImpl->eventHandlers.find(eventName);
        if (it == pImpl->eventHandlers.end()) {
            ANI_LOG_TRACE("[Events] UnregisterEvent(callback): no handlers for '%s'",
                eventName.c_str());
            return;
        }

        auto& callbacks = it->second.simpleCallbacks;
        auto before = callbacks.size();
        callbacks.erase(
            std::remove_if(callbacks.begin(), callbacks.end(),
                [&callback](const EventCallback& cb) {
                    return cb.target_type() == callback.target_type();
                }),
            callbacks.end()
        );

        ANI_LOG_DEBUG("[Events] Unregistered callback(s) for '%s': removed %zu",
            eventName.c_str(), before - callbacks.size());

        if (callbacks.empty() && it->second.dataCallbacks.empty()) {
            pImpl->eventHandlers.erase(it);
        }
    }

    void Events::UnregisterEventWithData(const std::string& eventName, EventCallbackWithData callback) {
        auto it = pImpl->eventHandlers.find(eventName);
        if (it == pImpl->eventHandlers.end()) {
            ANI_LOG_TRACE("[Events] UnregisterEventWithData: no handlers for '%s'",
                eventName.c_str());
            return;
        }

        auto& callbacks = it->second.dataCallbacks;
        auto before = callbacks.size();
        callbacks.erase(
            std::remove_if(callbacks.begin(), callbacks.end(),
                [&callback](const EventCallbackWithData& cb) {
                    return cb.target_type() == callback.target_type();
                }),
            callbacks.end()
        );

        ANI_LOG_DEBUG("[Events] Unregistered data callback(s) for '%s': removed %zu",
            eventName.c_str(), before - callbacks.size());

        if (it->second.simpleCallbacks.empty() && callbacks.empty()) {
            pImpl->eventHandlers.erase(it);
        }
    }

    void Events::UnregisterAllEventsForPlugin(const std::string& pluginName) {
        std::string prefix = "Plugin_" + pluginName + "_";
        auto it = pImpl->eventHandlers.begin();
        size_t removed = 0;
        while (it != pImpl->eventHandlers.end()) {
            if (it->first.find(prefix) == 0) {
                it = pImpl->eventHandlers.erase(it);
                removed++;
            }
            else {
                ++it;
            }
        }
        ANI_LOG_INFO("[Events] Unregistered plugin '%s': removed %zu event handler(s)",
            pluginName.c_str(), removed);
    }

    void Events::ClearAllEvents() {
        pImpl->eventHandlers.clear();
        pImpl->eventQueue.clear();
        ANI_LOG_INFO("[Events] Cleared all events");
    }

} // namespace ANI