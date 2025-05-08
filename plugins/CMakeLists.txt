cmake_minimum_required(VERSION 3.12)

# Common settings for all plugins
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Define plugin build macro
macro(add_plugin name)
    # Add a library target
    add_library(${name} SHARED ${ARGN})
    
    # Link against AniStudio core
    target_link_libraries(${name} PRIVATE 
        AniStudioCore
        glfw
        GLEW::GLEW
        ImGui
    )
    
    # Set output directory
    set_target_properties(${name} PROPERTIES 
        LIBRARY_OUTPUT_DIRECTORY "${PLUGINS_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${PLUGINS_DIR}"
    )
    
    # Define platform-specific output name
    if(WIN32)
        set_target_properties(${name} PROPERTIES SUFFIX ".dll")
    else()
        set_target_properties(${name} PROPERTIES 
            SUFFIX ".so"
            PREFIX "lib"
        )
    endif()
endmacro()

# Auto-discover and include all subdirectories with a CMakeLists.txt file
file(GLOB plugin_dirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)
foreach(plugin ${plugin_dirs})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${plugin})
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${plugin}/CMakeLists.txt)
            message(STATUS "Found plugin: ${plugin}")
            add_subdirectory(${plugin})
        endif()
    endif()
endforeach()