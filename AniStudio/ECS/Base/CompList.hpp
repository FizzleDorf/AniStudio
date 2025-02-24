#pragma once

#include "Types.hpp"
#include "pch.h"

namespace ECS {

// ICompList is the base interface for all component lists in the ECS.
// It provides a common interface for managing components, such as erasing components by entity ID.
class ICompList {
public:
    ICompList() = default;          // Default constructor.
    virtual ~ICompList() = default; // Virtual destructor for proper cleanup.

    // Virtual function to erase a component associated with a specific entity.
    // Derived classes can override this to provide specific behavior.
    virtual void Erase(const EntityID entity) {}
};

// CompList is a templated class that manages a list of components of a specific type.
// It inherits from ICompList to provide a common interface for component management.
template <typename T>
class CompList : public ICompList {
public:
    CompList() = default;  // Default constructor.
    ~CompList() = default; // Default destructor.

    // Inserts a component into the list if it doesn't already exist.
    void Insert(const T &component) {
        // Check if a component with the same entity ID already exists in the list.
        auto comp = std::find_if(data.begin(), data.end(), [&](const T &c) { return c.GetID() == component.GetID(); });
        if (comp == data.end()) {
            // If the component doesn't exist, add it to the list.
            data.push_back(component);
            std::cout << "Component added! ID: " << component.GetID() << ", Type ID: " << CompType<T>() << std::endl;
        } else {
            // If the component already exists, log a message.
            std::cout << "Component already Exists! ID: " << component.GetID() << std::endl;
        }
    }

    // Retrieves a component associated with a specific entity.
    T &Get(const EntityID entity) {
        // Find the component with the given entity ID.
        auto comp = std::find_if(data.begin(), data.end(), [&](const T &c) { return c.GetID() == entity; });
        // Assert that the component exists. If not, the program will terminate with an error.
        assert(comp != data.end() && "Component doesn't exist!");
        return *comp; // Return the found component.
    }

    // Erases a component associated with a specific entity.
    void Erase(const EntityID entity) override final {
        // Find the component with the given entity ID.
        auto comp = std::find_if(data.begin(), data.end(), [&](const T &c) { return c.GetID() == entity; });
        if (comp != data.end()) {
            // If the component exists, erase it from the list.
            data.erase(comp);
            std::cout << "Component Erased! ID: " << entity << ", Type ID: " << CompType<T>() << std::endl;
        } else {
            // If the component doesn't exist, log a message.
            std::cout << "No Entity Found with ID: " << entity << ", Type ID: " << CompType<T>() << std::endl;
        }
    }

    // Vector to store the components of type T.
    std::vector<T> data;
};

} // namespace ECS