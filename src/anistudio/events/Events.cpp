#include "Events.hpp"

namespace ANI {

	Events::Events() {}

	Events::~Events() {
		ClearAllEvents();
	}

	void Events::RegisterEvent(const std::string& eventName, EventCallback callback) {
		eventHandlers[eventName].simpleCallbacks.push_back(callback);
		std::cout << "[Events] Registered simple callback for event: " << eventName << std::endl;
	}

	void Events::RegisterEventWithData(const std::string& eventName, EventCallbackWithData callback) {
		eventHandlers[eventName].dataCallbacks.push_back(callback);
		std::cout << "[Events] Registered data callback for event: " << eventName << std::endl;
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

		// Process all events in the queue
		for (const auto& queuedEvent : eventQueue) {
			auto it = eventHandlers.find(queuedEvent.eventName);
			if (it != eventHandlers.end()) {
				const EventData& eventData = it->second;

				try {
					// Process simple callbacks
					for (const auto& callback : eventData.simpleCallbacks) {
						callback();
					}

					// Process data callbacks if we have data
					if (queuedEvent.hasData) {
						for (const auto& callback : eventData.dataCallbacks) {
							callback(queuedEvent.data);
						}
					}

					std::cout << "[Events] Processed event: " << queuedEvent.eventName
						<< " (callbacks: " << eventData.simpleCallbacks.size() + eventData.dataCallbacks.size()
						<< ")" << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[Events] Exception processing event "
						<< queuedEvent.eventName << ": " << e.what() << std::endl;
				}
			}
			else {
				std::cout << "[Events] No handlers registered for event: "
					<< queuedEvent.eventName << std::endl;
			}
		}

		// Clear the processed events
		eventQueue.clear();
	}

	void Events::ClearAllEvents() {
		eventHandlers.clear();
		eventQueue.clear();
		std::cout << "[Events] Cleared all event handlers" << std::endl;
	}

} // namespace ANI