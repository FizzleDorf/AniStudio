# Contributing to AniStudio

Thank you for your interest in contributing to AniStudio! This guide will help you understand our development workflow and contribution process.

## Development Workflow

### Branch Structure

- `main` - Production-ready code
- `dev` - Primary development branch
- `feature/*` - Feature branches
- `fix/*` - Bug fix branches
- `plugin/*` - Plugin development branches

### Making Changes

1. **Always branch from dev**:
```bash
git checkout dev
git pull origin dev
git checkout -b feature/your-feature-name
```

2. **Follow the commit message format**:
```
type(scope): description

[optional body]
```
Types: feat, fix, docs, style, refactor, test, chore
Example: `feat(plugin-system): add support for OpenGL shader plugins`

3. **Keep commits atomic and focused**

### Pull Request Process

1. Push your changes to a feature branch
2. Create a PR targeting the `dev` branch
3. Fill out the PR template with:
   - Description of changes
   - Related issues/tickets
   - Testing performed
   - Screenshots/videos if applicable
4. Request review from relevant team members
5. Address review feedback
6. Squash commits if requested
7. Merge only after approval

## Plugin Development

### Plugin-First Development

For new features:
1. First develop as a plugin
2. Test and iterate rapidly
3. Once stable, propose integration into core if appropriate

### Plugin Structure

```cpp
class MyPlugin : public Plugin::IPlugin {
public:
    const char* GetName() const override { return "MyPlugin"; }
    Version GetVersion() const override { return {1,0,0}; }
    
    bool OnLoad(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr) override {
        // Initialize plugin
        return true;
    }
    
    bool OnStart() override {
        // Start plugin functionality
        return true;
    }
    
    void OnStop() override {
        // Cleanup
    }
    
    void OnUpdate(float dt) override {
        // Update logic
    }
};

PLUGIN_API PLUGIN_EXPORT IPlugin* CreatePlugin() {
    return new MyPlugin();
}

PLUGIN_API PLUGIN_EXPORT void DestroyPlugin(IPlugin* plugin) {
    delete plugin;
}
```

### Plugin Development Guidelines

1. **Modularity**: Each plugin should have a single clear purpose
2. **Components**: Use ECS components for state management
3. **Systems**: Implement systems for logic/behavior
4. **Views**: Create custom views for UI elements
5. **Dependencies**: Clearly specify plugin dependencies
6. **Documentation**: Include usage examples and API documentation

### Testing Plugins

1. Build plugin:
```bash
mkdir build && cd build
cmake .. -DBUILD_PLUGINS=ON
make
```

2. Place in plugins directory
3. Test in AniStudio with plugin manager
4. Document any issues or limitations

## Code Style

- Follow existing code style
- Use consistent naming conventions
- Add comments for complex logic
- Include documentation for public APIs

## Core vs Plugin Features

Consider these factors when deciding between core integration and plugin:

### Keep as Plugin
- Experimental features
- Platform-specific functionality
- Optional workflows
- Third-party integrations
- User-specific customizations

### Consider Core Integration
- Critical functionality
- Performance-sensitive features
- Core workflow components
- Widely used features
- Base system extensions

## Review Process

1. Code Review
   - Functionality
   - Architecture
   - Performance
   - Security
   - Testing
   - Documentation

2. Testing Review
   - Unit tests
   - Integration tests
   - Performance testing
   - UI/UX testing

## Resources

- [Plugin API Documentation](docs/plugin-api.md)
- [Architecture Overview](docs/architecture.md)
- [Build Instructions](docs/building.md)

## Questions?

Open an issue with the `question` label for any contribution-related questions.
