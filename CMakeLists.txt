cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0079 NEW)

project(AniStudio VERSION 0.2.0)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

option(BUILD_ANIENGINE "Build AniEngine library" ON)
option(BUILD_ANISTUDIO "Build AniStudio library" ON)
option(BUILD_MAIN_APP "Build the main AniStudio application" ON)
option(BUILD_SHARED_LIBS "Build shared libraries instead of static" ON)

set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if(UNIX AND NOT APPLE)
    set(OpenGL_GL_PREFERENCE GLVND)
endif()

include(${CMAKE_BINARY_DIR}/conan/conan_toolchain.cmake OPTIONAL RESULT_VARIABLE CONAN_TOOLCHAIN)
if(NOT CONAN_TOOLCHAIN)
    message(STATUS "Conan toolchain not found, installing dependencies...")

    find_program(CONAN_CMD conan)
    if(NOT CONAN_CMD)
        message(FATAL_ERROR "Conan not found! Please install it first: pip install conan")
    endif()

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

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib)
endforeach()

set(PLUGINS_DIR "${CMAKE_BINARY_DIR}/plugins")
set(DATA_DIR "${CMAKE_BINARY_DIR}/data")
set(DEFAULTS_DIR "${DATA_DIR}/defaults")
set(SCRIPTS_DIR "${CMAKE_BINARY_DIR}/scripts")

file(MAKE_DIRECTORY ${PLUGINS_DIR})
file(MAKE_DIRECTORY ${DATA_DIR})
file(MAKE_DIRECTORY ${DEFAULTS_DIR})
file(MAKE_DIRECTORY ${SCRIPTS_DIR})

if(EXISTS "${CMAKE_SOURCE_DIR}/data/defaults")
    file(GLOB_RECURSE DEFAULT_FILES "${CMAKE_SOURCE_DIR}/data/defaults/*")
    foreach(DEFAULT_FILE ${DEFAULT_FILES})
        file(RELATIVE_PATH REL_PATH "${CMAKE_SOURCE_DIR}/data/defaults" "${DEFAULT_FILE}")
        set(DEST_FILE "${DEFAULTS_DIR}/${REL_PATH}")

        if(NOT EXISTS "${DEST_FILE}")
            get_filename_component(DEST_DIR "${DEST_FILE}" DIRECTORY)
            file(MAKE_DIRECTORY "${DEST_DIR}")
            file(COPY "${DEFAULT_FILE}" DESTINATION "${DEST_DIR}")
        endif()
    endforeach()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/scripts")
    file(GLOB_RECURSE SCRIPT_FILES "${CMAKE_SOURCE_DIR}/scripts/*")
    foreach(SCRIPT_FILE ${SCRIPT_FILES})
        file(RELATIVE_PATH REL_PATH "${CMAKE_SOURCE_DIR}/scripts" "${SCRIPT_FILE}")
        set(DEST_FILE "${SCRIPTS_DIR}/${REL_PATH}")

        get_filename_component(DEST_DIR "${DEST_FILE}" DIRECTORY)
        file(MAKE_DIRECTORY "${DEST_DIR}")
        file(COPY "${SCRIPT_FILE}" DESTINATION "${DEST_DIR}")
    endforeach()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/src/shaders")
    set(SHADER_DIR "${CMAKE_SOURCE_DIR}/src/shaders")
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

    file(GLOB SHADER_FILES "${SHADER_DIR}/*.vert" "${SHADER_DIR}/*.frag")

    if(SHADER_FILES)
        file(COPY ${SHADER_FILES} DESTINATION ${CMAKE_BINARY_DIR}/shaders)
        list(LENGTH SHADER_FILES SHADER_COUNT)
        message(STATUS "Copied ${SHADER_COUNT} shader file(s) to ${CMAKE_BINARY_DIR}/shaders")
    else()
        message(STATUS "No .vert or .frag files found in ${SHADER_DIR}")
    endif()
else()
    message(STATUS "Shader directory ${CMAKE_SOURCE_DIR}/src/shaders not found")
endif()

find_package(OpenCV REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(exiv2 CONFIG REQUIRED)
find_package(FFmpeg REQUIRED COMPONENTS avformat avutil swscale)
# find_package(PkgConfig REQUIRED)
find_package(pybind11 REQUIRED)

# Find X11 on Linux
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    if(NOT X11_FOUND)
        message(FATAL_ERROR "X11 development libraries not found!")
    endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/external/zep/CMakeLists.txt")
    set(ZEP_AVAILABLE TRUE)
    set(BUILD_DEMOS OFF CACHE BOOL "Build Zep demos" FORCE)
    set(BUILD_IMGUI OFF CACHE BOOL "Build ImGui Demo" FORCE)
    set(BUILD_QT OFF CACHE BOOL "Build Qt Demo" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
    message(STATUS "Zep text editor source found")
else()
    message(WARNING "Zep not found at external/zep - text editor features will be limited")
    set(ZEP_AVAILABLE FALSE)
endif()

set(ANIENGINE_BUILD_SHARED ${BUILD_SHARED_LIBS} CACHE BOOL "AniEngine shared" FORCE)
set(ANISTUDIO_BUILD_SHARED ${BUILD_SHARED_LIBS} CACHE BOOL "AniStudio shared" FORCE)
set(ANISTUDIO_ENABLE_ZEP ${ZEP_AVAILABLE} CACHE BOOL "AniStudio Zep" FORCE)

# Set up external dependencies as interface libraries to propagate usage requirements
add_library(AniEngineDeps INTERFACE)
target_link_libraries(AniEngineDeps INTERFACE
    ${OpenCV_LIBS}
    ZLIB::ZLIB
    PNG::PNG
    GLEW::GLEW
    glfw
    ffmpeg::avformat
    ffmpeg::avutil
    ffmpeg::swscale
    pybind11::embed
)

if(UNIX AND NOT APPLE)
    target_link_libraries(AniEngineDeps INTERFACE
        pthread
        dl
        rt
        ${X11_LIBRARIES}
    )
    target_include_directories(AniEngineDeps INTERFACE ${X11_INCLUDE_DIR})
endif()

target_compile_definitions(AniEngineDeps INTERFACE
    GLEW_STATIC
)

add_library(AniStudioDeps INTERFACE)
target_link_libraries(AniStudioDeps INTERFACE
    AniEngineDeps
    exiv2::exiv2
    ImGui
    ImGuiFileDialog
    ImNodeFlow
    ImGuizmo
)

if(UNIX AND NOT APPLE)
    target_link_libraries(AniStudioDeps INTERFACE
        pthread
        dl
        rt
        ${X11_LIBRARIES}
    )
endif()

target_compile_definitions(AniStudioDeps INTERFACE
    GLEW_STATIC
    IMGUI_DEFINE_MATH_OPERATORS
    IMGUI_FILEDIALOG_AVAILABLE
    IMGUI_NODES_AVAILABLE
    IMNODEFLOW_AVAILABLE
    IMGUIZMO_AVAILABLE
    NODE_EDITOR_AVAILABLE
    IMSEQUENCER_AVAILABLE
)

if(ZEP_AVAILABLE)
    target_link_libraries(AniStudioDeps INTERFACE Zep)
    target_compile_definitions(AniStudioDeps INTERFACE ZEP_AVAILABLE)
endif()

# Build AniEngine first
if(BUILD_ANIENGINE)
    add_subdirectory(src/aniengine)
    
    # Ensure AniEngineCore uses the dependencies interface
    target_link_libraries(AniEngineCore PUBLIC AniEngineDeps)
endif()

# Build AniStudio second (depends on AniEngine)
if(BUILD_ANISTUDIO)
    add_subdirectory(src/anistudio)
    
    # Ensure AniStudioCore uses the dependencies interface
    if(TARGET AniStudioCore)
        target_link_libraries(AniStudioCore PUBLIC AniEngineCore AniStudioDeps)
    endif()
endif()

# Build main application last
if(BUILD_MAIN_APP AND BUILD_ANISTUDIO)
    add_executable(${PROJECT_NAME}
        ${CMAKE_SOURCE_DIR}/src/main.cpp
        ${CMAKE_SOURCE_DIR}/src/pch.cpp
        ${CMAKE_SOURCE_DIR}/src/core/Core.cpp
        ${CMAKE_SOURCE_DIR}/src/timer/Timer.cpp
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE FALSE
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        PREFIX ""
    )

    target_compile_definitions(${PROJECT_NAME} PRIVATE
        GLEW_STATIC
        IMGUI_DEFINE_MATH_OPERATORS
    )

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_SOURCE_DIR}/src/timer
        ${CMAKE_SOURCE_DIR}/include
    )

    # Link only what's directly needed - AniStudioCore brings everything else
    target_link_libraries(${PROJECT_NAME} PRIVATE
        AniStudioCore
        glfw
        GLEW::GLEW
    )

    # Ensure proper symbol resolution on Linux
    if(UNIX AND NOT APPLE)
        target_link_options(${PROJECT_NAME} PRIVATE
            -Wl,--no-as-needed
            -Wl,--copy-dt-needed-entries
            -Wl,-rpath-link,${CMAKE_BINARY_DIR}/lib
        )
        
        # Also link X11 directly for the executable
        target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_LIBRARIES})
    endif()

    target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)

    message(STATUS "Main application will be built")
    message(STATUS "  Links against: AniStudioCore (brings all engine and studio dependencies)")
endif()

# Function to add external plugins
function(add_external_plugin TARGET_NAME)
    cmake_parse_arguments(PLUGIN "" "" "SOURCES;LIBRARIES;INCLUDE_DIRS" ${ARGN})

    add_library(${TARGET_NAME} SHARED ${PLUGIN_SOURCES})

    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src/aniengine/plugins
        ${CMAKE_SOURCE_DIR}/src/anistudio/plugins
        ${CMAKE_SOURCE_DIR}/external/cr/cr
        ${PLUGIN_INCLUDE_DIRS}
    )

    target_link_libraries(${TARGET_NAME} PRIVATE
        AniEngineCore
        AniStudioCore
        ${PLUGIN_LIBRARIES}
    )

    if(UNIX AND NOT APPLE)
        target_link_libraries(${TARGET_NAME} PRIVATE
            pthread
            dl
        )
    endif()

    target_compile_definitions(${TARGET_NAME} PRIVATE
        PLUGIN_API=__declspec\(dllexport\)
        GLEW_STATIC
    )

    set_target_properties(${TARGET_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        RUNTIME_OUTPUT_DIRECTORY ${PLUGINS_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${PLUGINS_DIR}
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PLUGINS_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PLUGINS_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PLUGINS_DIR}
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${PLUGINS_DIR}
        LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PLUGINS_DIR}
        LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PLUGINS_DIR}
        LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PLUGINS_DIR}
        LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${PLUGINS_DIR}
    )

    message(STATUS "Added external plugin: ${TARGET_NAME}")
endfunction()

# Installation rules
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    include(GNUInstallDirs)

    if(BUILD_MAIN_APP AND TARGET ${PROJECT_NAME})
        install(TARGETS ${PROJECT_NAME}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()

    if(TARGET AniEngineCore)
        install(TARGETS AniEngineCore
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()

    if(TARGET AniStudioCore)
        install(TARGETS AniStudioCore
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()

    install(DIRECTORY ${DATA_DIR}/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}
        PATTERN "*.tmp" EXCLUDE
    )

    install(DIRECTORY ${SCRIPTS_DIR}/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/scripts
        PATTERN "*.tmp" EXCLUDE
    )

    if(EXISTS ${CMAKE_BINARY_DIR}/shaders)
        install(DIRECTORY ${CMAKE_BINARY_DIR}/shaders/
            DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/shaders
        )
    endif()

    install(DIRECTORY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}/plugins)
endif()

message(STATUS "")
message(STATUS "AniStudio build configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Shared libraries: ${BUILD_SHARED_LIBS}")
message(STATUS "  Runtime library: ${CMAKE_MSVC_RUNTIME_LIBRARY}")
message(STATUS "")
message(STATUS "Components:")
message(STATUS "  AniEngine: ${BUILD_ANIENGINE}")
message(STATUS "  AniStudio: ${BUILD_ANISTUDIO}")
message(STATUS "  Main App: ${BUILD_MAIN_APP}")
message(STATUS "")
message(STATUS "Output directories:")
message(STATUS "  Binaries: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "  Libraries: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
message(STATUS "  Plugins: ${PLUGINS_DIR}")
message(STATUS "")