#pragma once

#include <iostream>
#include <mutex>
#include <queue>
#include <vector>
#include "Types.hpp"

using namespace ECS;

struct InferenceQueue {
    mutable std::mutex mutex_;   // Mutable to allow locking in const methods
    std::queue<EntityID> queue_; // Queue to hold EntityID

    // Add an entity to the queue
    void Enqueue(EntityID entity) {
        std::lock_guard<std::mutex> lock(mutex_); // Protect concurrent access
        queue_.push(entity);
    }

    // Remove all entities from the queue and return them
    std::vector<EntityID> DequeueAll() {
        std::lock_guard<std::mutex> lock(mutex_); // Protect concurrent access
        std::vector<EntityID> entities;

        while (!queue_.empty()) {
            entities.push_back(queue_.front());
            queue_.pop();
        }

        return entities; // Return a vector of all dequeued entities
    }

    // Check if the queue is empty
    bool IsEmpty() const {
        std::lock_guard<std::mutex> lock(mutex_); // Protect concurrent access
        return queue_.empty();
    }
};
