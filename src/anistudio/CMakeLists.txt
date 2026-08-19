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

if(NOT TARGET ImNodeFlow)
    add_subdirectory("${EXTERNAL_DIR}/ImNodeFlow" "${CMAKE_BINARY_DIR}/_external/ImNodeFlow")
endif()

if(NOT TARGET ImGuizmo)
    add_subdirectory("${EXTERNAL_DIR}/ImGuizmo" "${CMAKE_BINARY_DIR}/_external/ImGuizmo")
endif()

set(NFD_AVAILABLE FALSE)
if(EXISTS "${EXTERNAL_DIR}/nativefiledialog-extended/CMakeLists.txt" AND NOT TARGET nfd)
    add_subdirectory("${EXTERNAL_DIR}/nativefiledialog-extended" "${CMAKE_BINARY_DIR}/_external/nativefiledialog-extended")
    set(NFD_AVAILABLE TRUE)
    target_compile_definitions(nfd PRIVATE NFD_OVERRIDE_RECENT_WITH_DEFAULT)
elseif(TARGET nfd)
    set(NFD_AVAILABLE TRUE)
    target_compile_definitions(nfd PRIVATE NFD_OVERRIDE_RECENT_WITH_DEFAULT)
endif()

set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_SAVE})

file(GLOB_RECURSE ANISTUDIO_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
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
    else()
        set_target_properties(AniStudioCore PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
            PREFIX ""
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
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui/project>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui/settings>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gui/media>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils/states>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils/rendering>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils/ui_schema>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils/widgets>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/events>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/plugins>
    "${EXTERNAL_DIR}/glm"
    "${EXTERNAL_DIR}/nlohmann_json"
    "${EXTERNAL_DIR}/imgui"
    "${EXTERNAL_DIR}/imgui/backends"
    "${EXTERNAL_DIR}/ImNodeFlow/include"
    "${EXTERNAL_DIR}/ImGuizmo"
    "${EXTERNAL_DIR}/imgui_markdown"
    "${EXTERNAL_DIR}/stb"
    $<BUILD_INTERFACE:${EXTERNAL_DIR}/nfd/src/include>
)

target_link_libraries(AniStudioCore PUBLIC
    AniEngineCore
    glfw
    GLEW::GLEW
    exiv2::exiv2
)

target_link_libraries(AniStudioCore PRIVATE
    ImGui
    ImNodeFlow
    ImGuizmo
    ZLIB::ZLIB
    PNG::PNG
)

if(WIN32)
    target_link_libraries(AniStudioCore PRIVATE ole32 shell32 uuid)
endif()

if(LINUX)
    target_link_libraries(AniStudioCore PRIVATE gtk-3 gdk-3 X11)
endif()

if(NFD_AVAILABLE)
    target_link_libraries(AniStudioCore PRIVATE nfd)
    target_compile_definitions(AniStudioCore PUBLIC NFD_AVAILABLE)
endif()

target_sources(AniStudioCore PRIVATE "${EXTERNAL_DIR}/stb_image/stb_image.cpp")

set(IMGUI_COLOR_TEXT_EDIT_AVAILABLE FALSE)
if(EXISTS "${EXTERNAL_DIR}/ImGuiColorTextEdit/TextEditor.h")
    set(IMGUI_COLOR_TEXT_EDIT_AVAILABLE TRUE)
    target_include_directories(AniStudioCore PUBLIC "${EXTERNAL_DIR}/ImGuiColorTextEdit")
    target_sources(AniStudioCore PRIVATE "${EXTERNAL_DIR}/ImGuiColorTextEdit/TextEditor.cpp")
    target_compile_definitions(AniStudioCore PUBLIC IMGUI_COLOR_TEXT_EDIT_AVAILABLE)
    message(STATUS "ImGuiColorTextEdit found and added")
else()
    message(WARNING "ImGuiColorTextEdit not found at ${EXTERNAL_DIR}/ImGuiColorTextEdit - text editor features will be limited")
endif()

target_compile_definitions(AniStudioCore PUBLIC
    GLEW_STATIC
    IMGUI_DEFINE_MATH_OPERATORS
    IMGUI_NODES_AVAILABLE
    IMNODEFLOW_AVAILABLE
    IMGUIZMO_AVAILABLE
    NODE_EDITOR_AVAILABLE
    IMSEQUENCER_AVAILABLE
)

add_library(AniStudio::AniStudioCore ALIAS AniStudioCore)

message(STATUS "AniStudio: BUILD_SHARED_LIBS = ${BUILD_SHARED_LIBS}")
if(BUILD_SHARED_LIBS)
    message(STATUS "AniStudio: Building as SHARED library (.dll/.so)")
    message(STATUS "AniStudio: Output -> ${CMAKE_BINARY_DIR}/lib")
else()
    message(STATUS "AniStudio: Building as STATIC library (.lib/.a)")
    message(STATUS "AniStudio: Output -> ${CMAKE_BINARY_DIR}/lib")
endif()