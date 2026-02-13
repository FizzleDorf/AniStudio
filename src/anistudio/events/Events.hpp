#pragma once
#include <functional>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <any>

#ifdef _WIN32
    #ifdef BUILDING_ANISTUDIO
        #define ANISTUDIO_API __declspec(dllexport)
    #else
        #define ANISTUDIO_API __declspec(dllimport)
    #endif
#else
    #define ANISTUDIO_API __attribute__((visibility("default")))
#endif

namespace ANI {
    class ANISTUDIO_API Events {
    public:
        using EventCallback = std::function<void()>;
        using EventCallbackWithData = std::function<void(const std::any&)>;

        ~Events();
        Events(const Events&) = delete;
        Events& operator=(const Events&) = delete;

        static Events& Ref();

        void RegisterEvent(const std::string& eventName, EventCallback callback);
        void RegisterEventWithData(const std::string& eventName, EventCallbackWithData callback);
        void QueueEvent(const std::string& eventName);
        
        template<typename T>
        void QueueEventWithData(const std::string& eventName, const T& data) {
            QueuedEvent event;
            event.eventName = eventName;
            event.hasData = true;
            event.data = std::make_any<T>(data);
            eventQueue.push_back(event);
            std::cout << "[Events] Queued event with data: " << eventName << std::endl;
        }

        void Poll();
        void UnregisterEvent(const std::string& eventName);
        void UnregisterEvent(const std::string& eventName, EventCallback callback);
        void UnregisterEventWithData(const std::string& eventName, EventCallbackWithData callback);
        void UnregisterAllEventsForPlugin(const std::string& pluginName);
        void ClearAllEvents();

    private:
        Events();

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
}