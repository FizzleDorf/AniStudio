# External/CMakeLists.txt

# GLFW
set(GLFW_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/external/includes/GLFW")
set(GLFW_LIBRARIES "${CMAKE_SOURCE_DIR}/external/libs/glfw3.lib")

# GLEW
set(GLEW_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/external/includes/GLEW")
set(GLEW_LIBRARIES "${CMAKE_SOURCE_DIR}/external/libs/glew32s.lib")

add_subdirectory(imgui)

# Print out paths for debugging purposes
message(STATUS "GLFW Include Dir: ${GLFW_INCLUDE_DIRS}")
message(STATUS "GLFW Libraries: ${GLFW_LIBRARIES}")
message(STATUS "GLEW Include Dir: ${GLEW_INCLUDE_DIRS}")
message(STATUS "GLEW Libraries: ${GLEW_LIBRARIES}")
