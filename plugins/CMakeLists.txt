# Scan for all plugin directories and add them as subdirectories
file(GLOB PLUGIN_DIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)

foreach(plugin ${PLUGIN_DIRS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${plugin})
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${plugin}/CMakeLists.txt)
            message(STATUS "Found plugin: ${plugin}")
            add_subdirectory(${plugin})
        endif()
    endif()
endforeach()