# Define the directories for GLEW and GLFW headers and libraries
set(GLFW_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/external/includes/GLFW")
set(GLFW_LIBRARIES "${CMAKE_SOURCE_DIR}/external/libs/glfw3.lib")

set(GLEW_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/external/includes/GLEW")
set(GLEW_LIBRARIES "${CMAKE_SOURCE_DIR}/external/libs/glew32.lib")

# Optionally, if using Vulkan, set the Vulkan directories
# set(VULKAN_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/external/includes/Vulkan")
# set(VULKAN_LIBRARIES "${CMAKE_SOURCE_DIR}/external/libs/vulkan-1.lib")

# Print out paths for debugging purposes
message(STATUS "GLFW Include Dir: ${GLFW_INCLUDE_DIRS}")
message(STATUS "GLFW Libraries: ${GLFW_LIBRARIES}")
message(STATUS "GLEW Include Dir: ${GLEW_INCLUDE_DIRS}")
message(STATUS "GLEW Libraries: ${GLEW_LIBRARIES}")
# message(STATUS "Vulkan Include Dir: ${VULKAN_INCLUDE_DIRS}")
# message(STATUS "Vulkan Libraries: ${VULKAN_LIBRARIES}")
