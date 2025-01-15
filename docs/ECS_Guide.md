# AniStudio ECS System Guide

## EntityManager Usage

The EntityManager is the core of the ECS system, managing entities, components, and systems.

### Creating and Managing Entities

```cpp
// Create a new entity
EntityID entity = mgr.AddNewEntity();

// Destroy an entity and all its components
mgr.DestroyEntity(entity);

// Get all entities
std::vector<EntityID> entities = mgr.GetAllEntities();
```

### Working with Components

```cpp
// Adding components
mgr.AddComponent<ImageComponent>(entity);
mgr.AddComponent<LatentComponent>(entity, width, height); // With parameters

// Accessing components
auto& imageComp = mgr.GetComponent<ImageComponent>(entity);
imageComp.width = 512;

// Checking for components
if (mgr.HasComponent<ImageComponent>(entity)) {
    // Do something with the image component
}

// Removing components
mgr.RemoveComponent<ImageComponent>(entity);
```

### Working with Systems

```cpp
// Register a system
mgr.RegisterSystem<ImageSystem>();

// Get a system reference
auto imageSystem = mgr.GetSystem<ImageSystem>();
if (imageSystem) {
    imageSystem->ProcessImage(entity);
}

// Unregister a system
mgr.UnregisterSystem<ImageSystem>();
```

### Example: Complete Entity Setup

```cpp
// Creating a diffusion entity
EntityID entity = mgr.AddNewEntity();

// Add required components
mgr.AddComponent<ModelComponent>(entity);
mgr.AddComponent<LatentComponent>(entity);
mgr.AddComponent<PromptComponent>(entity);
mgr.AddComponent<SamplerComponent>(entity);
mgr.AddComponent<ImageComponent>(entity);

// Configure components
auto& latentComp = mgr.GetComponent<LatentComponent>(entity);
latentComp.latentWidth = 512;
latentComp.latentHeight = 512;

auto& promptComp = mgr.GetComponent<PromptComponent>(entity);
promptComp.posPrompt = "a beautiful landscape";
```

### Best Practices for EntityManager

1. **Component Access**:
   - Always use references when getting components to avoid copies
   - Check if entity has component before accessing
   - Keep component modifications localized to systems

2. **Entity Lifecycle**:
   - Clean up entities when no longer needed
   - Use RAII patterns when possible
   - Maintain clear ownership of entities

3. **System Management**:
   - Register systems at initialization
   - Keep system dependencies clear
   - Use appropriate system update order

## ViewManager Usage

The ViewManager handles UI views and their lifecycle in AniStudio.

### Creating and Managing Views

```cpp
// Create a new view
ViewID viewId = viewManager.AddNewView();

// Add a specific view type
viewManager.AddView<DiffusionView>(viewId);

// Access a view
auto& diffusionView = viewManager.GetView<DiffusionView>(viewId);

// Remove a view
viewManager.RemoveView<DiffusionView>(viewId);

// Destroy a view completely
viewManager.DestroyView(viewId);
```

### Custom View Implementation

```cpp
class CustomView : public BaseView {
public:
    CustomView() { 
        viewName = "Custom View"; 
    }

    void Init() override {
        // Initialize view resources
    }

    void Render() override {
        ImGui::Begin(viewName.c_str());
        // Render view contents using ImGui
        ImGui::End();
    }

    void HandleInput(int key, int action) override {
        // Handle input events
    }
};
```

### Best Practices for ViewManager

1. **View Organization**:
   - Group related views logically
   - Keep view dependencies minimal
   - Use clear naming conventions

2. **View Lifecycle**:
   - Initialize resources in Init()
   - Clean up in destructor
   - Handle view state properly

3. **ImGui Usage**:
   - Always pair Begin()/End() calls
   - Use unique IDs for widgets
   - Handle window flags appropriately

## Integration Example

Here's an example showing EntityManager and ViewManager working together:

```cpp
class DiffusionSystem : public BaseSystem {
public:
    DiffusionSystem(EntityManager& entityMgr) : BaseSystem(entityMgr) {
        sysName = "DiffusionSystem";
        AddComponentSignature<LatentComponent>();
        AddComponentSignature<PromptComponent>();
    }

    void Update(float deltaT) override {
        for (auto entity : entities) {
            auto& latentComp = mgr.GetComponent<LatentComponent>(entity);
            auto& promptComp = mgr.GetComponent<PromptComponent>(entity);
            // Process diffusion
        }
    }
};

class DiffusionView : public BaseView {
public:
    void Render() override {
        ImGui::Begin("Diffusion Control");
        
        if (ImGui::Button("Generate")) {
            EntityID entity = mgr.AddNewEntity();
            mgr.AddComponent<LatentComponent>(entity);
            mgr.AddComponent<PromptComponent>(entity);
            
            auto& promptComp = mgr.GetComponent<PromptComponent>(entity);
            promptComp.posPrompt = promptInput;
        }
        
        ImGui::End();
    }
private:
    std::string promptInput;
};
```

## Common Patterns

### Component Communication

```cpp
// Using events for component communication
Event event;
event.type = EventType::InferenceRequest;
event.entityID = entity;
Events::Ref().QueueEvent(event);
```

### System Dependencies

```cpp
// System ordering through component dependencies
class RenderSystem : public BaseSystem {
    void Start() override {
        // Ensure required systems exist
        assert(mgr.GetSystem<ImageSystem>());
    }
};
```

### View-Entity Interaction

```cpp
class ImageView : public BaseView {
    void RenderImage(EntityID entity) {
        if (mgr.HasComponent<ImageComponent>(entity)) {
            auto& imageComp = mgr.GetComponent<ImageComponent>(entity);
            // Render image using ImGui
        }
    }
};
```

## Troubleshooting

Common issues and solutions:

1. **Entity not found**:
   - Check entity ID validity
   - Verify entity hasn't been destroyed
   - Ensure proper entity lifecycle management

2. **Component access errors**:
   - Always check HasComponent() before access
   - Use reference returns for GetComponent()
   - Verify component registration

3. **System update issues**:
   - Check system registration
   - Verify component signatures
   - Ensure proper update order

4. **View rendering problems**:
   - Check ImGui Begin/End pairing
   - Verify view registration
   - Debug window flags and positioning

## Performance Considerations

1. **Entity Creation/Destruction**:
   - Batch entity operations when possible
   - Reuse entities instead of create/destroy cycles
   - Monitor entity count for performance impacts

2. **Component Access**:
   - Cache component references in tight loops
   - Use appropriate container types
   - Consider component data layout

3. **System Updates**:
   - Profile system update times
   - Optimize component iterations
   - Use appropriate update frequencies

4. **View Rendering**:
   - Minimize ImGui draws
   - Use efficient rendering patterns
   - Handle large data sets appropriately