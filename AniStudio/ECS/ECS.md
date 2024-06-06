# Entity Component System (ECS)
This is the main design pattern for managing the render engines and game logic. This design pattern is not object oriented like other object component systems. Instead, this pattern is data oriented design that leverages components as structures of data, entities as a vectors of components and the systems run the processes depending on what components make up an entity. This makes the runtime experience dynamic by allowing the user to customize entities by adding, removing and/or replacing components to create unique implementations without having to code.

## Components
Components are the building blocks of entities in the ECS design pattern. They represent the data associated with specific aspects of an entity, such as position, velocity, or other entity parameters. Components are implemented as simple data structures containing relevant attributes or properties.

## Entity
Entities are essentially containers for components. They are typically represented as unique identifiers or handles and serve as a way to group related components together. Entities themselves do not contain any logic; instead, they are composed of components that define their behavior and characteristics.

## System
Systems are responsible for implementing the logic that operates on entities and their components. Each system focuses on a specific aspect of the game or application, such as rendering, physics, or input handling. Systems iterate over entities that contain the necessary components for the system to operate on, performing computations or updates as needed. This separation of concerns allows for better organization and scalability of code in large and complex projects.
