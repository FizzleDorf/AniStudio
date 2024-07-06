# Manually specify the path to the GLEW library and include directories
set(GLEW_LIBRARY "${LIBRARY_DIR}/glew32s.lib")
set(GLEW_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/includes")

# Manually specify the path to the GLFW3 library and include directories
set(GLFW3_LIBRARY "${LIBRARY_DIR}/glfw3.lib")
set(GLFW3_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/includes")

# Include directories for external libraries
target_include_directories(${PROJECT_NAME} PRIVATE
    "${GLEW_INCLUDE_DIR}"
    "${GLFW3_INCLUDE_DIR}"
    "${CMAKE_SOURCE_DIR}/external"  # Include directory for external libraries, source files, and headers
    "${CMAKE_SOURCE_DIR}/external/src"
    "${CMAKE_SOURCE_DIR}/external/libs"
    "${CMAKE_SOURCE_DIR}/external/includes"
)

# Link GLFW3 and GLEW libraries
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
    ${GLFW3_LIBRARY} 
    ${GLEW_LIBRARY}
)

# Add submodules
# add_subdirectory(${CMAKE_SOURCE_DIR}/external/imgui-node-editor)
# add_subdirectory(${CMAKE_SOURCE_DIR}/external/imgui)
