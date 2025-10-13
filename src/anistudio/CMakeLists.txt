cmake_minimum_required(VERSION 3.12)
project(AniStudio VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Get external dir
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/../../external")
else()
    set(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/external")
endif()

# Add external dependencies
if(NOT TARGET ImGui)
    add_subdirectory("${EXTERNAL_DIR}/imgui" "${CMAKE_BINARY_DIR}/imgui")
endif()

if(NOT TARGET ImGuiFileDialog)
    add_subdirectory("${EXTERNAL_DIR}/imgui_filedialog" "${CMAKE_BINARY_DIR}/imgui_filedialog")
endif()

if(NOT TARGET ImNodeFlow)
    add_subdirectory("${EXTERNAL_DIR}/ImNodeFlow" "${CMAKE_BINARY_DIR}/ImNodeFlow")
endif()

if(NOT TARGET ImGuizmo)
    add_subdirectory("${EXTERNAL_DIR}/ImGuizmo" "${CMAKE_BINARY_DIR}/ImGuizmo")
endif()

# Zep (optional)
set(ZEP_AVAILABLE FALSE)
if(EXISTS "${EXTERNAL_DIR}/zep/CMakeLists.txt")
    set(BUILD_IMGUI OFF CACHE BOOL "" FORCE)
    set(BUILD_QT OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory("${EXTERNAL_DIR}/zep" "${CMAKE_BINARY_DIR}/zep")
    set(ZEP_AVAILABLE TRUE)
endif()

# Collect sources
file(GLOB_RECURSE ANISTUDIO_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui_utils/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/events/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/plugins/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/AniStudio.cpp"
)

# Create library - uses BUILD_SHARED_LIBS automatically
add_library(AniStudioCore ${ANISTUDIO_SOURCES})

# CRITICAL FIX: Export symbols when building as DLL
if(BUILD_SHARED_LIBS)
    target_compile_definitions(AniStudioCore PRIVATE BUILDING_ANISTUDIO)
    # On Windows, export all symbols automatically
    if(WIN32)
        set_target_properties(AniStudioCore PROPERTIES
            WINDOWS_EXPORT_ALL_SYMBOLS ON
        )
    endif()
else()
    target_compile_definitions(AniStudioCore PRIVATE BUILDING_ANISTUDIO)
endif()

# Includes
target_include_directories(AniStudioCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui/base>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui/editors>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui/project>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui/settings>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui/tools>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui/viewers>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui_utils>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui_utils/states>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui_utils/rendering>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui_utils/ui_schema>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui_utils/widgets>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/events>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/plugins>
    "${EXTERNAL_DIR}/glm"
    "${EXTERNAL_DIR}/nlohmann_json"
    "${EXTERNAL_DIR}/imgui_filedialog"
    "${EXTERNAL_DIR}/ImNodeFlow/include"
    "${EXTERNAL_DIR}/ImGuizmo"
    "${EXTERNAL_DIR}/imgui_markdown"
    "${EXTERNAL_DIR}/zep/include"
    "${EXTERNAL_DIR}/stb"
)

# Link against AniEngineCore!
target_link_libraries(AniStudioCore PUBLIC
    AniEngineCore      # <-- CRITICAL: This provides ECS, EntityManager, etc.
    glfw
    GLEW::GLEW
    exiv2::exiv2
    ImGui
    ImGuiFileDialog
    ImNodeFlow
    ImGuizmo
)

if(ZEP_AVAILABLE AND TARGET Zep::Zep)
    target_link_libraries(AniStudioCore PUBLIC Zep::Zep)
    target_compile_definitions(AniStudioCore PUBLIC ZEP_AVAILABLE)
endif()

target_compile_definitions(AniStudioCore PUBLIC
    GLEW_STATIC
    IMGUI_DEFINE_MATH_OPERATORS
    IMGUI_FILEDIALOG_AVAILABLE
    IMGUI_NODES_AVAILABLE
    IMNODEFLOW_AVAILABLE
    IMGUIZMO_AVAILABLE
    NODE_EDITOR_AVAILABLE
    IMSEQUENCER_AVAILABLE
)

# Create an alias for cleaner target naming
add_library(AniStudio::AniStudioCore ALIAS AniStudioCore)

message(STATUS "AniStudio: BUILD_SHARED_LIBS = ${BUILD_SHARED_LIBS}")
if(BUILD_SHARED_LIBS)
    message(STATUS "AniStudio: Building as SHARED library (.dll) with symbol export")
else()
    message(STATUS "AniStudio: Building as STATIC library (.lib)")
endif()