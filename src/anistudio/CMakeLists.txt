cmake_minimum_required(VERSION 3.12)
project(AniStudio VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/../../external")
else()
    set(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/external")
endif()

set(BUILD_SHARED_LIBS_SAVE ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)

if(NOT TARGET ImGui)
    add_subdirectory("${EXTERNAL_DIR}/imgui" "${CMAKE_BINARY_DIR}/_external/imgui")
endif()

if(NOT TARGET ImGuiFileDialog)
    add_subdirectory("${EXTERNAL_DIR}/imgui_filedialog" "${CMAKE_BINARY_DIR}/_external/imgui_filedialog")
endif()

if(NOT TARGET ImNodeFlow)
    add_subdirectory("${EXTERNAL_DIR}/ImNodeFlow" "${CMAKE_BINARY_DIR}/_external/ImNodeFlow")
endif()

if(NOT TARGET ImGuizmo)
    add_subdirectory("${EXTERNAL_DIR}/ImGuizmo" "${CMAKE_BINARY_DIR}/_external/ImGuizmo")
endif()

set(ZEP_AVAILABLE FALSE)

# Only check for Zep if you explicitly want it
if(DEFINED ANISTUDIO_ENABLE_ZEP AND ANISTUDIO_ENABLE_ZEP)
    if(EXISTS "${EXTERNAL_DIR}/zep/CMakeLists.txt" AND NOT TARGET Zep)
        set(BUILD_IMGUI OFF CACHE BOOL "" FORCE)
        set(BUILD_QT OFF CACHE BOOL "" FORCE)
        set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(BUILD_DEMOS OFF CACHE BOOL "" FORCE)
        
        # Disable vcpkg if you don't have it
        if(NOT EXISTS "${EXTERNAL_DIR}/zep/vcpkg/scripts/buildsystems/vcpkg.cmake")
            message(STATUS "Zep vcpkg not found, using system packages")
            # You might need to set variables to disable vcpkg usage in Zep
        endif()
        
        add_subdirectory("${EXTERNAL_DIR}/zep" "${CMAKE_BINARY_DIR}/_external/zep")
        set(ZEP_AVAILABLE TRUE)
    elseif(TARGET Zep)
        set(ZEP_AVAILABLE TRUE)
    endif()
else()
    message(STATUS "Zep not enabled (ANISTUDIO_ENABLE_ZEP=${ANISTUDIO_ENABLE_ZEP})")
endif()

set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_SAVE})

file(GLOB_RECURSE ANISTUDIO_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/gui/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/gui_utils/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/events/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/plugins/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/AniStudio.cpp"
)

add_library(AniStudioCore ${ANISTUDIO_SOURCES})

if(BUILD_SHARED_LIBS)
    target_compile_definitions(AniStudioCore PRIVATE BUILDING_ANISTUDIO)
    if(WIN32)
        set_target_properties(AniStudioCore PROPERTIES 
            WINDOWS_EXPORT_ALL_SYMBOLS ON
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        )
    endif()
else()
    target_compile_definitions(AniStudioCore PRIVATE BUILDING_ANISTUDIO)
endif()

target_include_directories(AniStudioCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/ecs/components
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/ecs/systems
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
    "${EXTERNAL_DIR}/imgui"
    "${EXTERNAL_DIR}/imgui/backends"
    "${EXTERNAL_DIR}/imgui_filedialog"
    "${EXTERNAL_DIR}/ImNodeFlow/include"
    "${EXTERNAL_DIR}/ImGuizmo"
    "${EXTERNAL_DIR}/imgui_markdown"
    "${EXTERNAL_DIR}/zep/include"
    "${EXTERNAL_DIR}/stb"
)

target_link_libraries(AniStudioCore PUBLIC
    AniEngineCore
    glfw
    GLEW::GLEW
    exiv2::exiv2
)

target_link_libraries(AniStudioCore PRIVATE
    ImGui
    ImGuiFileDialog
    ImNodeFlow
    ImGuizmo
)

if(ZEP_AVAILABLE AND TARGET Zep::Zep)
    target_link_libraries(AniStudioCore PRIVATE Zep::Zep)
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

add_library(AniStudio::AniStudioCore ALIAS AniStudioCore)

message(STATUS "AniStudio: BUILD_SHARED_LIBS = ${BUILD_SHARED_LIBS}")
if(BUILD_SHARED_LIBS)
    message(STATUS "AniStudio: Building as SHARED library (.dll)")
    message(STATUS "AniStudio: Output -> ${CMAKE_BINARY_DIR}/lib")
else()
    message(STATUS "AniStudio: Building as STATIC library (.lib)")
    message(STATUS "AniStudio: Output -> ${CMAKE_BINARY_DIR}/lib")
endif()