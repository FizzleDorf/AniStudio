#pragma once
#include "Types.hpp"
#include <imgui.h>
#include <string>
#include <variant>
#include <vector>

namespace ECS {

// Forward declaration of EntityManager to avoid circular dependencies.
class EntityManager;

// ComponentProperty struct defines the properties that can be exposed in the UI for a component.
struct ComponentProperty {
    // Enum defining the types of properties that can be exposed.
    enum class Type {
        Int,       // Integer property
        Float,     // Floating-point property
        String,    // String property
        Bool,      // Boolean property
        Image,     // Image property (likely for textures or icons)
        Vec2,      // 2D vector property
        Vec3,      // 3D vector property
        Vec4,      // 4D vector property
        Component, // Reference to another component
        Custom     // Custom property type for component-specific data
    };

    std::string name;        // Name of the property, displayed in the UI.
    Type type;               // Type of the property, as defined by the enum.
    std::string category;    // Category for organizing properties in the UI.
    bool isReadOnly = false; // Whether the property is read-only in the UI.

    // Struct defining limits, default values, and ranges for the property.
    struct Limits {
        // Variant to hold different types of property values.
        using PropertyValue = std::variant<int, float, std::string, bool, ImVec2,
                                           ImVec4, // Use ImVec4 for both Vec3 and Vec4
                                           ComponentTypeID>;

        PropertyValue defaultValue; // Default value for the property.
        float step = 0.1f;          // Step size for numeric properties (e.g., sliders).
        float min = 0.0f;           // Minimum value for numeric properties.
        float max = 1.0f;           // Maximum value for numeric properties.
    } limits;

    // For component references, this stores the type ID of the target component.
    ComponentTypeID targetType = 0;
};

// PinDescription struct defines connection points for nodes in a visual scripting or graph-based system.
struct PinDescription {
    std::string name;             // Name of the pin, displayed in the UI.
    ComponentProperty::Type type; // Type of data the pin can handle.
    std::string category;         // Category for organizing pins in the UI.
    bool isInput;                 // Whether the pin is an input (true) or output (false).
    bool isRequired;              // Whether the pin is required for the node to function.
};

// ComponentMetadata struct defines complete metadata for a component, including UI-related information.
struct ComponentMetadata {
    std::string name;        // Display name of the component.
    std::string category;    // Category for organizing components in the UI.
    std::string description; // Optional description or tooltip for the component.

    // UI-related properties.
    std::vector<ComponentProperty> properties; // List of editable properties for the component.
    std::vector<PinDescription> pins;          // List of connection points for the component.
    ImVec2 defaultSize = ImVec2(200, 150);     // Default size of the component's node in the UI.
    bool canBeNode = false;                    // Whether the component can be used as a node in a graph.
    bool showPreview = false;                  // Whether to show a preview of the component in the properties panel.
};

// IComponentType is the base interface for all components in the ECS.
class