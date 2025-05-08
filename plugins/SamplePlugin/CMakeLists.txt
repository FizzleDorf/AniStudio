cmake_minimum_required(VERSION 3.12)

# Set the plugin name
set(PLUGIN_NAME SamplePlugin)

# Collect sources
set(SOURCES
    SamplePlugin.cpp
)

# Use the add_plugin macro defined in the parent CMakeLists.txt
add_plugin(${PLUGIN_NAME} ${SOURCES})

# Add GLFW dependency explicitly for glfwGetTime() used in the animation
target_link_libraries(${PLUGIN_NAME} PRIVATE
    glfw
)

# Additional compiler definitions if needed
target_compile_definitions(${PLUGIN_NAME} PRIVATE
    PLUGIN_VERSION="1.0.0"
)

# Output plugin information
message(STATUS "Configured plugin: ${PLUGIN_NAME}")