cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0091 NEW)

project(AniStudio)

if(SD_CUDA)
    enable_language(CUDA)
    set(CUDA_ARCHITECTURES "89" CACHE STRING "Override for RTX 4090" FORCE)
    add_definitions(-DSD_USE_CUDA)
    
    # Find CUDA toolkit
    find_package(CUDAToolkit REQUIRED)
    if(NOT CUDAToolkit_FOUND)
        message(FATAL_ERROR "CUDA toolkit not found!")
    endif()
endif()

option(BUILD_ANISTUDIO "Build the AniStudio application" ON)
option(BUILD_PLUGINS "Build AniStudio plugins" ON)

set(PLUGINS_DIR "${CMAKE_BINARY_DIR}/plugins")
file(MAKE_DIRECTORY ${PLUGINS_DIR})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Include Conan toolchain
include(${CMAKE_BINARY_DIR}/conan/conan_toolchain.cmake OPTIONAL RESULT_VARIABLE CONAN_TOOLCHAIN)
if(NOT CONAN_TOOLCHAIN)
    message(STATUS "Conan toolchain not found, installing dependencies...")
    
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

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Set binary output directories - ensures consistent placement across platforms
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin)

# For libraries
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)

# For static libraries and import libraries on Windows
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)


if(BUILD_ANISTUDIO)

    # Data directory setup
    set(DATA_DIR "${CMAKE_BINARY_DIR}/data")
    set(DEFAULTS_DIR "${DATA_DIR}/defaults")
    file(MAKE_DIRECTORY ${DATA_DIR})
    file(MAKE_DIRECTORY ${DEFAULTS_DIR})
    file(COPY "${CMAKE_SOURCE_DIR}/data/defaults/" DESTINATION "${DEFAULTS_DIR}")
    message(STATUS "Copied data/defaults to ${DEFAULTS_DIR}")

    # Find required packages
    find_package(Vulkan REQUIRED)
    find_package(OpenCV REQUIRED)
    find_package(glew REQUIRED)
    find_package(glfw3 REQUIRED)
    find_package(ZLIB REQUIRED)
    find_package(PNG REQUIRED)
    find_package(exiv2 CONFIG REQUIRED)
    find_package(FFmpeg REQUIRED)
	
	include_directories(${GLFW_INCLUDE_DIRS})

    # Shader files
    set(PCH_FILE "${CMAKE_SOURCE_DIR}/AniStudio/pch.h")
    set(SHADER_DIR "${CMAKE_SOURCE_DIR}/AniStudio/shaders")
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)
    file(COPY ${SHADER_DIR}/basic.vert ${SHADER_DIR}/basic.frag DESTINATION ${CMAKE_BINARY_DIR}/shaders)

    # Add external dependencies
    add_subdirectory(external/stable-diffusion.cpp)
    add_subdirectory(external/imgui)
    add_subdirectory(external/imgui_nodes)
    add_subdirectory(external/ImNodeFlow)
    add_subdirectory(external/stb_image)
    add_subdirectory(external/imgui_filedialog)
    add_subdirectory(external/ImJSchema)
    add_subdirectory(external/ImGuizmo)
	
	# For external/imgui directory
	target_include_directories(ImGui PRIVATE ${GLFW_INCLUDE_DIRS})
    add_compile_definitions(IMGUI_DEFINE_MATH_OPERATORS)

    # Include directories
    include_directories(
        ${CONAN_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/AniStudio
        ${CMAKE_SOURCE_DIR}/AniStudio/Plugins
        ${CMAKE_SOURCE_DIR}/AniStudio/Engine
        ${CMAKE_SOURCE_DIR}/AniStudio/Events
        ${CMAKE_SOURCE_DIR}/AniStudio/Timer
        ${CMAKE_SOURCE_DIR}/AniStudio/GUI
        ${CMAKE_SOURCE_DIR}/AniStudio/GUI/Base
        ${CMAKE_SOURCE_DIR}/AniStudio/utils
        ${CMAKE_SOURCE_DIR}/AniStudio/ECS
        ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Base
        ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Systems
        ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Components
        ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Components/SDCPPComponents
        ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Components/ImageComponents
        ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Components/VideoComponents
        ${CMAKE_SOURCE_DIR}/external/glm
        ${CMAKE_SOURCE_DIR}/external/stable-diffusion.cpp
        ${CMAKE_SOURCE_DIR}/external/imgui_filedialog
        ${CMAKE_SOURCE_DIR}/external/nlohmann_json
        ${CMAKE_SOURCE_DIR}/external/imgui_nodes
        ${CMAKE_SOURCE_DIR}/external/immvision
        ${Vulkan_INCLUDE_DIR}
    )

    # Source files
    file(GLOB_RECURSE GUI_SOURCES ${CMAKE_SOURCE_DIR}/AniStudio/Gui/*.cpp)
    file(GLOB_RECURSE SYSTEM_SOURCES ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Systems/*.cpp)
    file(GLOB_RECURSE UTILS_SOURCES ${CMAKE_SOURCE_DIR}/AniStudio/utils/*.cpp)

    message(STATUS "GUI_SOURCES found: ${GUI_SOURCES}")
    foreach(file ${GUI_SOURCES})
        message(STATUS "  Found GUI file: ${file}")
    endforeach()

    # Main executable
    add_executable(${PROJECT_NAME}
        ${CMAKE_SOURCE_DIR}/AniStudio/main.cpp
        ${CMAKE_SOURCE_DIR}/AniStudio/pch.cpp
        ${CMAKE_SOURCE_DIR}/AniStudio/Engine/Engine.cpp
        ${CMAKE_SOURCE_DIR}/AniStudio/Events/Events.cpp
        ${CMAKE_SOURCE_DIR}/AniStudio/Timer/Timer.cpp
        ${CMAKE_SOURCE_DIR}/AniStudio/Plugins/PluginManager.cpp
        ${GUI_SOURCES}
        ${SYSTEM_SOURCES}
        ${UTILS_SOURCES}
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE FALSE
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    # Link directories and libraries
    link_directories(${CONAN_LIB_DIRS})

	if(SD_CUDA)
		target_compile_definitions(${PROJECT_NAME} PRIVATE SD_USE_CUDA)
		target_link_libraries(${PROJECT_NAME} PRIVATE CUDA::cudart)
	endif()

    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${CONAN_LIBS}
        stable-diffusion
        ImGui
        ImGui_Nodes
        ImNodeFlow
        ImJSchema
        stb_image
        ImGuiFileDialog
        ImGuizmo
        opencv::opencv
        glfw
        GLEW::GLEW
        ZLIB::ZLIB
        PNG::PNG
        Vulkan::Vulkan
        exiv2::exiv2
        ffmpeg::avcodec
        ffmpeg::avformat
        ffmpeg::avutil
        ffmpeg::swscale
    )

    # Precompiled headers
    target_precompile_headers(${PROJECT_NAME} PRIVATE ${PCH_FILE})

    # Core library interface
    add_library(AniStudioCore INTERFACE)
    target_include_directories(AniStudioCore INTERFACE
        ${CMAKE_SOURCE_DIR}/AniStudio
        ${CMAKE_SOURCE_DIR}/AniStudio/ECS
        ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Base
        ${CMAKE_SOURCE_DIR}/AniStudio/GUI/Base
        ${CMAKE_SOURCE_DIR}/AniStudio/Plugins
        ${CMAKE_SOURCE_DIR}/external/imgui
        ${CMAKE_SOURCE_DIR}/external/nlohmann_json
    )
    
    if(WIN32)
        target_compile_definitions(${PROJECT_NAME} PRIVATE ANI_CORE_API=__declspec(dllexport))
        target_compile_definitions(AniStudioCore INTERFACE ANI_CORE_API=__declspec(dllimport))
    else()
        target_compile_definitions(${PROJECT_NAME} PRIVATE ANI_CORE_API=)
        target_compile_definitions(AniStudioCore INTERFACE ANI_CORE_API=)
    endif()
endif()

# Plugins configuration
if(BUILD_PLUGINS)
    if(WIN32)
        add_definitions(-DPLUGIN_API=__declspec(dllexport))
    else()
        add_definitions(-DPLUGIN_API=)
    endif()
    
    if(NOT BUILD_ANISTUDIO)
        add_subdirectory(external/stb_image)
        add_subdirectory(external/stable-diffusion.cpp)
        
        add_library(AniStudioCore INTERFACE)
        target_include_directories(AniStudioCore INTERFACE
            ${CMAKE_SOURCE_DIR}/AniStudio
            ${CMAKE_SOURCE_DIR}/AniStudio/ECS
            ${CMAKE_SOURCE_DIR}/AniStudio/ECS/Base
            ${CMAKE_SOURCE_DIR}/AniStudio/GUI/Base
            ${CMAKE_SOURCE_DIR}/AniStudio/Plugins
            ${CMAKE_SOURCE_DIR}/external/nlohmann_json
            ${CMAKE_SOURCE_DIR}/external/stb_image
            ${CMAKE_SOURCE_DIR}/external/imgui
        )
        if(WIN32)
            target_compile_definitions(AniStudioCore INTERFACE ANI_CORE_API=__declspec(dllimport))
        else()
            target_compile_definitions(AniStudioCore INTERFACE ANI_CORE_API=)
        endif()
    endif()
    
    add_subdirectory(plugins)
endif()