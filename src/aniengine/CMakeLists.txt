cmake_minimum_required(VERSION 3.12)
project(AniEngine VERSION 1.0.0 LANGUAGES CXX)

# Build options
option(ANIENGINE_BUILD_SHARED "Build AniEngine as shared library" OFF)
option(ANIENGINE_ENABLE_CUDA "Enable CUDA support" OFF)
option(ANIENGINE_BUILD_EXAMPLES "Build AniEngine examples" OFF)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform specific settings
if(WIN32)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# CUDA setup
if(ANIENGINE_ENABLE_CUDA)
    enable_language(CUDA)
    set(CUDA_ARCHITECTURES "89" CACHE STRING "CUDA architectures")
    add_definitions(-DSD_USE_CUDA)
    find_package(CUDAToolkit REQUIRED)
endif()

# Find dependencies if building standalone
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    # Standalone build - find all dependencies
    include(${CMAKE_BINARY_DIR}/conan/conan_toolchain.cmake OPTIONAL)
    
    find_package(Vulkan REQUIRED)
    find_package(OpenCV REQUIRED)
    find_package(glew REQUIRED)
    find_package(glfw3 REQUIRED)
    find_package(ZLIB REQUIRED)
    find_package(PNG REQUIRED)
    find_package(FFmpeg REQUIRED)
    find_package(pybind11 REQUIRED)
    find_package(semver REQUIRED)
    
    # Add ImGui subdirectory
    if(EXISTS "${CMAKE_SOURCE_DIR}/../../external/imgui")
        add_subdirectory(${CMAKE_SOURCE_DIR}/../../external/imgui ${CMAKE_BINARY_DIR}/imgui)
    else()
        message(FATAL_ERROR "ImGui not found. Required for AniEngine.")
    endif()
    
    # Add external dependencies when building standalone
    if(EXISTS "${CMAKE_SOURCE_DIR}/../../external/stable-diffusion.cpp")
        add_subdirectory(${CMAKE_SOURCE_DIR}/../../external/stable-diffusion.cpp ${CMAKE_BINARY_DIR}/stable-diffusion)
    else()
        message(FATAL_ERROR "stable-diffusion.cpp not found. Required for AniEngine.")
    endif()
else()
    # Parent project build - add external dependencies here to avoid rebuilding
    if(NOT TARGET stable-diffusion)
        add_subdirectory(${CMAKE_SOURCE_DIR}/external/stable-diffusion.cpp ${CMAKE_BINARY_DIR}/external/stable-diffusion)
    endif()
    if(NOT TARGET ImGui)
        add_subdirectory(${CMAKE_SOURCE_DIR}/external/imgui ${CMAKE_BINARY_DIR}/external/imgui)
    endif()
endif()

# Collect source files
file(GLOB_RECURSE ANIENGINE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ecs/*.cpp"
)

# Add the main AniEngine implementation
list(APPEND ANIENGINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/AniEngine.cpp")

# Create the library
if(ANIENGINE_BUILD_SHARED)
    add_library(AniEngineCore SHARED ${ANIENGINE_SOURCES})
    target_compile_definitions(AniEngineCore PRIVATE ANI_ENGINE_EXPORTS)
    target_compile_definitions(AniEngineCore PUBLIC ANI_ENGINE_API=__declspec\(dllimport\))
else()
    add_library(AniEngineCore STATIC ${ANIENGINE_SOURCES})
    target_compile_definitions(AniEngineCore PUBLIC ANI_ENGINE_API=)
endif()

# Set library properties
set_target_properties(AniEngineCore PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Include directories
target_include_directories(AniEngineCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	#ecs base
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs/base>
	# systems
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs/systems>
	# components
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs/components>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs/components/SDCPPComponents>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs/components/ImageComponents>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs/components/VideoComponents>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ecs/components/ScriptComponents>
	# utils
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils/defaults>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils/media>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils/scripts>
	# interface
    $<INSTALL_INTERFACE:include/aniengine>
)

# Add root include directory for public headers
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    # Standalone build - include directory is at ../../include
    target_include_directories(AniEngineCore PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/../../include>
    )
else()
    # Parent project build - include directory is at root/include
    target_include_directories(AniEngineCore PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    )
endif()

# Plugin interface include
target_include_directories(AniEngineCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../aniplugins>
    $<INSTALL_INTERFACE:include/aniplugins>
)

# External includes (when building standalone)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    target_include_directories(AniEngineCore PUBLIC
        ${CMAKE_SOURCE_DIR}/../../external/glm
        ${CMAKE_SOURCE_DIR}/../../external/stable-diffusion.cpp
        ${CMAKE_SOURCE_DIR}/../../external/nlohmann_json
        ${CMAKE_SOURCE_DIR}/../../external/cr/cr
        ${CMAKE_SOURCE_DIR}/../../external/imgui
        ${CMAKE_SOURCE_DIR}/../../external/imgui/backends
    )
else()
    # Parent project includes
    target_include_directories(AniEngineCore PUBLIC
        ${CMAKE_SOURCE_DIR}/external/glm
        ${CMAKE_SOURCE_DIR}/external/stable-diffusion.cpp
        ${CMAKE_SOURCE_DIR}/external/nlohmann_json
        ${CMAKE_SOURCE_DIR}/external/cr/cr
        ${CMAKE_SOURCE_DIR}/external/imgui
        ${CMAKE_SOURCE_DIR}/external/imgui/backends
    )
endif()

# Link libraries
target_link_libraries(AniEngineCore PUBLIC
    opencv::opencv
    ZLIB::ZLIB
    PNG::PNG
    Vulkan::Vulkan
    GLEW::GLEW
    glfw
    ffmpeg::avcodec
    ffmpeg::avformat
    ffmpeg::avutil
    ffmpeg::swscale
    pybind11::embed
    semver::semver
    stable-diffusion
    ImGui
)

# CUDA support
if(ANIENGINE_ENABLE_CUDA)
    target_compile_definitions(AniEngineCore PUBLIC SD_USE_CUDA)
    target_link_libraries(AniEngineCore PUBLIC CUDA::cudart)
endif()

# Compile definitions
target_compile_definitions(AniEngineCore PUBLIC
    GLEW_STATIC
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# Install configuration
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    include(GNUInstallDirs)
    
    # Install the library
    install(TARGETS AniEngineCore
        EXPORT AniEngineCoreTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    
    # Install headers
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/aniengine
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
    
    # Install plugin headers
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../aniplugins/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/aniplugins
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
    
    # Install public headers from include directory
    if(EXISTS ${CMAKE_SOURCE_DIR}/../../include)
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/../../include/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
        )
    endif()
    
    # Create and install config files
    include(CMakePackageConfigHelpers)
    
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/AniEngineCoreConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/AniEngineCoreConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AniEngineCore
    )
    
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/AniEngineCoreConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/AniEngineCoreConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/AniEngineCoreConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AniEngineCore
    )
    
    install(EXPORT AniEngineCoreTargets
        FILE AniEngineCoreTargets.cmake
        NAMESPACE AniEngineCore::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AniEngineCore
    )
endif()

# Examples (optional)
if(ANIENGINE_BUILD_EXAMPLES AND CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    add_subdirectory(examples)
endif()

message(STATUS "AniEngine configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Library type: ${BUILD_SHARED_LIBS}")
message(STATUS "  CUDA support: ${ANIENGINE_ENABLE_CUDA}")
message(STATUS "  Examples: ${ANIENGINE_BUILD_EXAMPLES}")