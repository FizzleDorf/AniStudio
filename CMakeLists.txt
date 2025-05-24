cmake_minimum_required(VERSION 3.12)

# Build options
option(BUILD_ANISTUDIO "Build the AniStudio application" ON)
option(BUILD_PLUGINS "Build AniStudio plugins" ON)

set(PLUGINS_DIR "${CMAKE_BINARY_DIR}/plugins")
    file(MAKE_DIRECTORY ${PLUGINS_DIR})

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS OFF)

if (WIN32)
    add_definitions(-DNOMINMAX)
endif()

# Get Conan dependencies
include(${CMAKE_BINARY_DIR}/conan/conan_toolchain.cmake OPTIONAL RESULT_VARIABLE CONAN_TOOLCHAIN)
if(NOT CONAN_TOOLCHAIN)
    message(STATUS "Conan toolchain not found, installing dependencies...")
    
    # First, check if Conan is available
    execute_process(
        COMMAND conan --version
        RESULT_VARIABLE CONAN_CHECK
        OUTPUT_VARIABLE CONAN_VERSION
        ERROR_VARIABLE CONAN_ERROR
    )
    
    if(NOT CONAN_CHECK EQUAL 0)
        message(FATAL_ERROR 
            "Conan is not available. Please install it with:\n"
            "pip install conan\n"
            "Error: ${CONAN_ERROR}"
        )
    endif()
    
    message(STATUS "Found Conan: ${CONAN_VERSION}")
    
    # Run Conan install with detailed output
    execute_process(
        COMMAND conan install "${CMAKE_SOURCE_DIR}" 
                --output-folder="${CMAKE_BINARY_DIR}" 
                --build=missing
        RESULT_VARIABLE CONAN_RESULT
        OUTPUT_VARIABLE CONAN_OUTPUT
        ERROR_VARIABLE CONAN_ERROR
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
    
    if(NOT CONAN_RESULT EQUAL 0)
        message(FATAL_ERROR 
            "Failed to install Conan dependencies\n"
            "Output: ${CONAN_OUTPUT}\n"
            "Error: ${CONAN_ERROR}"
        )
    endif()
    
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan/conan_toolchain.cmake")
        message(FATAL_ERROR 
            "Conan install succeeded but toolchain file not found.\n"
            "Expected: ${CMAKE_BINARY_DIR}/conan/conan_toolchain.cmake\n"
            "Conan output: ${CONAN_OUTPUT}"
        )
    endif()
    
    include(${CMAKE_BINARY_DIR}/conan/conan_toolchain.cmake)
endif()

set(CMAKE_VERBOSE_MAKEFILE ON)

project(AniStudio CXX)

if(BUILD_ANISTUDIO)
	# Create data directories in build folder and copy defaults
	set(DATA_DIR "${CMAKE_BINARY_DIR}/data")
	set(DEFAULTS_DIR "${DATA_DIR}/defaults")
	file(MAKE_DIRECTORY ${DATA_DIR})
	file(MAKE_DIRECTORY ${DEFAULTS_DIR})
	
	# Copy the data/defaults directory and its contents
	file(COPY "${CMAKE_SOURCE_DIR}/data/defaults/" 
	     DESTINATION "${DEFAULTS_DIR}"
	     PATTERN "*")
	
	message(STATUS "Copied data/defaults to ${DEFAULTS_DIR}")

    # Required packages for AniStudio
    find_package(Vulkan REQUIRED)
    find_package(OpenCV REQUIRED)
    find_package(glew REQUIRED)
    find_package(glfw3 REQUIRED)
    find_package(ZLIB REQUIRED)
    find_package(PNG REQUIRED)
    find_package(exiv2 CONFIG REQUIRED)
	find_package(FFmpeg REQUIRED)

    # Set PCH file
    set(PCH_FILE "${CMAKE_SOURCE_DIR}/AniStudio/pch.h")

    # Define the shader source directory
    set(SHADER_DIR "${CMAKE_SOURCE_DIR}/Anistudio/shaders")

    # Copy the shaders to the build directory
    file(COPY ${SHADER_DIR}/basic.vert ${SHADER_DIR}/basic.frag DESTINATION ${CMAKE_BINARY_DIR}/shaders)

    # External libraries
    add_subdirectory(external/stable-diffusion.cpp)
    add_subdirectory(external/imgui)
    add_subdirectory(external/imgui_nodes)
    add_subdirectory(external/ImNodeFlow)
    add_subdirectory(external/stb_image)
    add_subdirectory(external/imgui_filedialog)
    add_subdirectory(external/ImJSchema)
    add_subdirectory(external/ImGuizmo)

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

    # Find all source files
    file(GLOB GUI_SOURCES AniStudio/GUI/*.cpp)
    file(GLOB SYSTEM_SOURCES AniStudio/ECS/Systems/*.cpp)
    file(GLOB UTILS_SOURCES AniStudio/utils/*.cpp)

    # Add main executable
    add_executable(${PROJECT_NAME}
        AniStudio/main.cpp
        AniStudio/pch.cpp
        AniStudio/Engine/Engine.cpp
        AniStudio/Events/Events.cpp
        AniStudio/Timer/Timer.cpp
		AniStudio/Plugins/PluginManager.cpp
        ${GUI_SOURCES}
        ${SYSTEM_SOURCES}
        ${UTILS_SOURCES}
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE FALSE)
    link_directories(${CONAN_LIB_DIRS})

    # Link libraries
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

    target_precompile_headers(${PROJECT_NAME} PRIVATE ${PCH_FILE})

	# Create an interface library for the core ECS functionality
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
    
    # Export ECS symbols for plugins
    if(WIN32)
        target_compile_definitions(${PROJECT_NAME} PRIVATE 
            ANI_CORE_API=__declspec\(dllexport\)
        )
        target_compile_definitions(AniStudioCore INTERFACE
            ANI_CORE_API=__declspec\(dllimport\)
        )
    else()
        target_compile_definitions(${PROJECT_NAME} PRIVATE ANI_CORE_API=)
        target_compile_definitions(AniStudioCore INTERFACE ANI_CORE_API=)
    endif()


endif()

# Build plugins if enabled
if(BUILD_PLUGINS)
    # Add API definition for plugin exports
    if(WIN32)
        add_definitions(-DPLUGIN_API=__declspec\(dllexport\))
    else()
        add_definitions(-DPLUGIN_API=)
    endif()
    
    # Only include headers needed for plugins
    if(NOT BUILD_ANISTUDIO)
        add_subdirectory(external/stb_image)
        add_subdirectory(external/stable-diffusion.cpp)
        
        # Create the core interface in standalone plugin mode
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
            target_compile_definitions(AniStudioCore INTERFACE
                ANI_CORE_API=__declspec\(dllimport\)
            )
        else()
            target_compile_definitions(AniStudioCore INTERFACE ANI_CORE_API=)
        endif()
    endif()
    
    add_subdirectory(plugins)
endif()