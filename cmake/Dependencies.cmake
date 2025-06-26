# ============================================================================
# Dependencies.cmake - External Library Management
# ============================================================================

# Option to force rebuild dependencies
option(REBUILD_DEPS "Force rebuild of all dependencies" OFF)

# Include Conan toolchain - EXACT same as your working CMakeLists.txt
include(${CMAKE_BINARY_DIR}/conan/conan_toolchain.cmake OPTIONAL RESULT_VARIABLE CONAN_TOOLCHAIN)
if(NOT CONAN_TOOLCHAIN OR REBUILD_DEPS)
    if(REBUILD_DEPS)
        message(STATUS "REBUILD_DEPS=ON - Forcing dependency rebuild...")
        if(EXISTS "${CMAKE_BINARY_DIR}/conan")
            file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/conan")
        endif()
    else()
        message(STATUS "Conan toolchain not found, installing dependencies...")
    endif()
    
    find_program(CONAN_CMD conan)
    if(NOT CONAN_CMD)
        message(FATAL_ERROR "Conan not found! Please install it first: pip install conan")
    endif()
    
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)

    execute_process(
        COMMAND ${CONAN_CMD} install "${CMAKE_SOURCE_DIR}" 
                --output-folder="${CMAKE_BINARY_DIR}" 
                --build=missing
        RESULT_VARIABLE CONAN_RESULT
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
    
    if(NOT CONAN_RESULT EQUAL 0)
        message(FATAL_ERROR "Conan install failed!")
    endif()
    
    include(${CMAKE_BINARY_DIR}/conan/conan_toolchain.cmake)
endif()

# Find required packages - EXACT same as your working CMakeLists.txt
find_package(Vulkan REQUIRED)
find_package(OpenCV REQUIRED)
find_package(glew REQUIRED)
find_package(glfw3 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(exiv2 CONFIG REQUIRED)
find_package(FFmpeg REQUIRED)
find_package(pybind11 REQUIRED)

include_directories(${GLFW_INCLUDE_DIRS})

# Add external dependencies - EXACT same as your working CMakeLists.txt
add_subdirectory(external/stable-diffusion.cpp)
add_subdirectory(external/imgui)
add_subdirectory(external/imgui_nodes)
add_subdirectory(external/ImNodeFlow)
add_subdirectory(external/imgui_filedialog)
add_subdirectory(external/ImJSchema)
add_subdirectory(external/ImGuizmo)

# Only build Zep if needed - EXACT same as your working CMakeLists.txt
set(BUILD_IMGUI OFF CACHE BOOL "Build ImGui Demo" FORCE)
set(BUILD_QT OFF CACHE BOOL "Build Qt Demo" FORCE) 
set(BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
set(ZEP_FEATURE_CPP_FILE_SYSTEM ON CACHE BOOL "Enable cpp file system" FORCE)

if(EXISTS "${CMAKE_SOURCE_DIR}/external/zep/CMakeLists.txt")
    add_subdirectory(external/zep)
    set(ZEP_AVAILABLE TRUE)
    message(STATUS "Zep text editor found and will be built")
else()
    message(WARNING "Zep not found at external/zep - text editor features will be limited")
    set(ZEP_AVAILABLE FALSE)
endif()

message(STATUS "Dependencies configured successfully")