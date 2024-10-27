cmake_minimum_required(VERSION 3.20)

# Include Conan-generated cmake files
include(${CMAKE_SOURCE_DIR}/conan/build/generators/conan_toolchain.cmake OPTIONAL RESULT_VARIABLE CONAN_TOOLCHAIN_INCLUDED)
if(NOT CONAN_TOOLCHAIN_INCLUDED)
    message(FATAL_ERROR "The file conan_toolchain.cmake doesn't exist, you must run install first")
endif()

set(CMAKE_VERBOSE_MAKEFILE ON)

project(AniStudio CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)

# Set PCH file
set(PCH_FILE "${CMAKE_SOURCE_DIR}/AniStudio/pch.h")

find_package(Vulkan REQUIRED)
# Check if Vulkan was found
if (Vulkan_FOUND)
    message(STATUS "Found Vulkan, include: ${Vulkan_INCLUDE_DIR}, library: ${Vulkan_LIBRARY}")
else()
    message(FATAL_ERROR "Vulkan not found")
endif()

find_package(glfw3 REQUIRED)
# Check if glfw was found
if (glfw3_FOUND)
    message(STATUS "Found glfw3, include: ${glfw3_INCLUDE_DIR}, library: ${glfw3_LIBRARY}")
else()
    message(FATAL_ERROR "glfw3 not found")
endif()

find_package(OpenCV REQUIRED)
# Check if opencv was found
if (OpenCV_FOUND)
    message(STATUS "Found OpenCV, include: ${OpenCV_INCLUDE_DIRS}, library: ${OpenCV_LIBS}")
else()
    message(FATAL_ERROR "OpenCV not found")
endif()

# Set CURL options
# set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
# set(CURL_USE_LIBSSH2 OFF CACHE BOOL "" FORCE)
# set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
# set(HTTP_ONLY ON CACHE BOOL "" FORCE)
# set(CURL_ZLIB OFF CACHE BOOL "" FORCE) 

# set cpr options
# set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
# set(CPR_ENABLE_SSL OFF CACHE BOOL "" FORCE)
# set(CPR_ENABLE_SANITIZERS OFF CACHE BOOL "" FORCE)

# Define options for stable diffusion
# general
option(ANI_CUBLAS                     "sd: cuda backend" ON)
option(ANI_HIPBLAS                    "sd: rocm backend" OFF)
option(ANI_METAL                      "sd: metal backend" OFF)
option(ANI_VULKAN                     "sd: vulkan backend" OFF)
option(ANI_SYCL                       "sd: sycl backend" OFF)
option(ANI_FLASH_ATTN                 "sd: use flash attention for x4 less memory usage" ON)
option(ANI_FAST_SOFTMAX               "sd: x1.5 faster softmax, indeterministic (sometimes, same seed don't generate same image), cuda only" OFF)
option(ANI_BUILD_SHARED_LIBS          "sd: build shared libs" ON)
#option(SD_BUILD_SERVER               "sd: build server example"                           ON)

if(ANI_CUBLAS)
    message("-- Use CUBLAS as backend stable-diffusion")
    set(SD_CUDA ON)
endif()

if(ANI_METAL)
    message("-- Use Metal as backend stable-diffusion")
    set(SD_METAL ON)
endif()

if (ANI_VULKAN)
    message("-- Use Vulkan as backend stable-diffusion")
    set(SD_VULKAN ON)
endif ()

if (ANI_HIPBLAS)
    message("-- Use HIPBLAS as backend stable-diffusion")
    set(SD_HIPBLAS ON)
    if(ANI_FAST_SOFTMAX)
        set(SD_CUDA_FAST_SOFTMAX ON)
    endif()
endif ()

if(ANI_FLASH_ATTN)
    message("-- Use Flash Attention for memory optimization")
    set(SD_FLASH_ATTN ON)
endif()

# Add external libraries
add_subdirectory(external/imgui)
add_subdirectory(external/imgui_nodes)
# add_subdirectory(external/Vulkan-Hpp)
add_subdirectory(external/stb_image)
add_subdirectory(external/imgui_filedialog)
add_subdirectory(external/ImGuizmo)
add_subdirectory(external/sdcpp)

# Find GLSL validator
find_program(GLSL_VALIDATOR glslangValidator HINTS 
    /usr/bin 
    /usr/local/bin 
    $ENV{VULKAN_SDK}/Bin/ 
    $ENV{VULKAN_SDK}/Bin32/
)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/AniStudio
    ${CMAKE_SOURCE_DIR}/Engine
    ${CMAKE_SOURCE_DIR}/Events
    ${CMAKE_SOURCE_DIR}/Timer

    ${CMAKE_SOURCE_DIR}/AniStudio/Gui
    ${CMAKE_SOURCE_DIR}/AniStudio/utils
    ${CMAKE_SOURCE_DIR}/AniStudio/ECS
    ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Base
    ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Systems
    ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Components
    ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Components/SDCPPComponents
    ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Components/ImageComponents

    ${CMAKE_SOURCE_DIR}/AniStudio/backends

    ${CMAKE_SOURCE_DIR}/external/glm
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/external/imgui_filedialog
    ${CMAKE_SOURCE_DIR}/external/nlohmann_json
    ${CMAKE_SOURCE_DIR}/external/vma
    ${CMAKE_SOURCE_DIR}/external/vkbootstrap
    ${CMAKE_SOURCE_DIR}/external/imgui_nodes
    ${CMAKE_SOURCE_DIR}/external/immvision
    ${CMAKE_SOURCE_DIR}/external/Vulkan-Headers/include
    # ${CMAKE_SOURCE_DIR}/external/cpr/include

    ${glfw3_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    # ${ZLIB_INCLUDE_DIRS}
    ${Vulkan_INCLUDE_DIR}
)

# Preprocessor definitions
add_definitions(-DGLEW_STATIC)

# Find all source files
file(GLOB GUI_SOURCES AniStudio/Gui/*.cpp)

file(GLOB SYSTEM_SOURCES AniStudio/ECS/Systems/*.cpp)

# Add main executable
add_executable(${PROJECT_NAME}
    ${CMAKE_SOURCE_DIR}/AniStudio/main.cpp
    ${CMAKE_SOURCE_DIR}/AniStudio/pch.cpp

    ${CMAKE_SOURCE_DIR}/AniStudio/Engine/Engine.cpp
    ${CMAKE_SOURCE_DIR}/AniStudio/Events/Events.cpp
    ${CMAKE_SOURCE_DIR}/AniStudio/Timer/Timer.cpp

    ${CMAKE_SOURCE_DIR}/external/vkbootstrap/VkBootstrap.cpp

    ${CMAKE_SOURCE_DIR}/AniStudio/backends/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/AniStudio/backends/imgui_impl_vulkan.cpp

    ${CMAKE_SOURCE_DIR}/AniStudio/utils/Fonts.cpp

    ${CMAKE_SOURCE_DIR}/AniStudio/Gui/TestDiffuseView.cpp

    ${GUI_SOURCES}
    ${SYSTEM_SOURCES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE FALSE)

# Setup OpenCV
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(IMMVISION_FETCH_OPENCV ON)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/find_opencv.cmake)
immvision_find_opencv()

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ImGui
    ImGui_Nodes
    stb_image
    ImGuiFileDialog 
    ImGuizmo
    Vulkan::Vulkan
    # cpr
    # CURL::libcurl
    ${OpenCV_LIBS}
    # ZLIB::ZLIB
    ${glfw3_LIBRARIES}
    stable-diffusion
)

target_precompile_headers(${PROJECT_NAME} PRIVATE ${PCH_FILE})
