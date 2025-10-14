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
#define ANISTUDIO_API
#endif

namespace ANI {

	class ANISTUDIO_API Events {  // <-- ADD EXPORT MACRO
	public:
		using EventCallback = std::function<void()>;
		using EventCallbackWithData = std::function<void(const std::any&)>;

		~Events();
		Events(const Events &) = delete;
		Events &operator=(const Events &) = delete;

		// Inline singleton - must be in header to work correctly across DLL boundary
		static Events &Ref() {
			static Events instance;
			return instance;
		}

		// Export all public methods
		void RegisterEvent(const std::string& eventName, EventCallback callback);
		void RegisterEventWithData(const std::string& eventName, EventCallbackWithData callback);
		void QueueEvent(const std::string& eventName);

		// Template method - must be in header
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

} // namespace ANI