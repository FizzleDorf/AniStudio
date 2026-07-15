#pragma once
#include <functional>
#include <string>
#include <memory>
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
            QueueEventWithDataImpl(eventName, std::make_any<T>(data));
        }

        void Poll();
        void UnregisterEvent(const std::string& eventName);
        void UnregisterEvent(const std::string& eventName, EventCallback callback);
        void UnregisterEventWithData(const std::string& eventName, EventCallbackWithData callback);
        void UnregisterAllEventsForPlugin(const std::string& pluginName);
        void ClearAllEvents();

    private:
        Events();

        struct Impl;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
        std::unique_ptr<Impl> pImpl;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

        void QueueEventWithDataImpl(const std::string& eventName, const std::any& data);
    };
}