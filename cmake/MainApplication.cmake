#============================================================================
# cmake/MainApplication.cmake - Main Application
#============================================================================

message(STATUS "Building Main Application...")

if(NOT BUILD_ANISTUDIO)
    message(STATUS "BUILD_ANISTUDIO is OFF - skipping main application")
    set(MAIN_APP_FOUND FALSE CACHE INTERNAL "")
    return()
endif()

if(NOT TARGET AniStudioCore)
    message(STATUS "Main application requires AniStudioCore - skipping")
    set(MAIN_APP_FOUND FALSE CACHE INTERNAL "")
    return()
endif()

# Application sources
set(APP_SOURCES
    src/main.cpp
    src/pch.cpp
    src/engine/Engine.cpp
    src/timer/Timer.cpp
)

# Base plugins
file(GLOB BASE_PLUGIN_SOURCES src/base_plugins/*.cpp)
list(APPEND APP_SOURCES ${BASE_PLUGIN_SOURCES})

# Check required files exist
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/src/main.cpp")
    message(STATUS "main.cpp not found - skipping main application")
    set(MAIN_APP_FOUND FALSE CACHE INTERNAL "")
    return()
endif()

# Create executable
add_executable(AniStudio ${APP_SOURCES})

# Link to core libraries
target_link_libraries(AniStudio PRIVATE AniStudio::AniStudioCore)
target_link_libraries(AniStudio PRIVATE AniStudio::AniEngineCore)

# Link to plugin system if available
if(TARGET PluginInterface)
    target_link_libraries(AniStudio PRIVATE AniStudio::PluginInterface)
    target_compile_definitions(AniStudio PRIVATE PLUGIN_SYSTEM_AVAILABLE)
endif()

# Link external dependencies
target_link_libraries(AniStudio PRIVATE
    ${CONAN_LIBS}
    stable-diffusion
    ImGui
    ImGui_Nodes
    ImNodeFlow
    ImJSchema
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
    pybind11::embed
)

if(ZEP_AVAILABLE)
    target_link_libraries(AniStudio PRIVATE Zep::Zep)
    target_compile_definitions(AniStudio PRIVATE ZEP_AVAILABLE)
endif()

if(SD_CUDA)
    target_compile_definitions(AniStudio PRIVATE SD_USE_CUDA)
    target_link_libraries(AniStudio PRIVATE CUDA::cudart)
endif()

# Platform-specific
if(WIN32)
    target_compile_definitions(AniStudio PRIVATE "ANI_CORE_API=__declspec(dllexport)")
else()
    target_compile_definitions(AniStudio PRIVATE "ANI_CORE_API=")
endif()

# Properties
set_target_properties(AniStudio PROPERTIES
    WIN32_EXECUTABLE FALSE
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

target_compile_definitions(AniStudio PRIVATE GLEW_STATIC IMGUI_DEFINE_MATH_OPERATORS)

# Precompiled headers
target_precompile_headers(AniStudio PRIVATE src/pch.h)

# Visual Studio settings
if(MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT AniStudio)
    set_target_properties(AniStudio PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()

set(MAIN_APP_FOUND TRUE CACHE INTERNAL "")
message(STATUS "Main application configured successfully")