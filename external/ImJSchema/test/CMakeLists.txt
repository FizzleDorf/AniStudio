# Create a static library for Catch2s main so that we can reduce
# compiling time. Each unit test will link to this
cmake_minimum_required(VERSION 3.13)

find_package(Catch2 REQUIRED)

add_library(${PROJECT_NAME}-catchmain STATIC ${CMAKE_CURRENT_SOURCE_DIR}/catch-main.cpp)
target_include_directories(${PROJECT_NAME}-catchmain PUBLIC third_party)
target_compile_features(${PROJECT_NAME}-catchmain PUBLIC cxx_std_17)
target_link_libraries( ${PROJECT_NAME}-catchmain PUBLIC   catch2::catch2)

get_filename_component(folder_name ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" folder_name ${folder_name})




find_package(Threads REQUIRED)

file(GLOB files "unit-*.cpp")
foreach(file ${files})

    # Replace any files named test_XXXXX with XXXX
    get_filename_component(file_basename ${file} NAME_WE)
    string(REGEX REPLACE "unit-([^$]+)" "\\1" testcase ${file_basename})

    set(FILE_SUFFIX_NAME  ${testcase} )
    set(EXE_NAME     unit-${FILE_SUFFIX_NAME} )
    set(TEST_NAME    test-${FILE_SUFFIX_NAME} )
    set(SRC_NAME     unit-${FILE_SUFFIX_NAME}.cpp )

    set(exeCmd  ${EXE_NAME} )
    set(envVar  "" )


    add_executable( ${EXE_NAME}  ${SRC_NAME} )

    target_link_libraries( ${EXE_NAME}
                                PUBLIC
                                    ${UNIT_TEST_LINK_TARGETS}
                                    ${LINKED_TARGETS})

    target_link_libraries( ${EXE_NAME}
                                PRIVATE
                                    -lstdc++fs
                                    ${PROJECT_NAME}-catchmain
                                    Threads::Threads
                                    ImJSchema::ImJSchema
                                    nlohmann_json::nlohmann_json)


    target_compile_definitions(${EXE_NAME}
                               PRIVATE
                                   CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

    add_test(  NAME    ${TEST_NAME}
               COMMAND ${exeCmd}
            )
    set_tests_properties(${TEST_NAME}
                            PROPERTIES
                                ENVIRONMENT
                                    "${envVar}")



    message("--------------------------------------------------")
    message("Unit Test Added: ${TEST_NAME}")
    message("--------------------------------------------------")
    message("Souce File       : ${SRC_NAME}")
    message("Working Directory: ${CMAKE_CURRENT_BINARY_DIR}")
    message("Env Variables    : ${envVar}")
    message("Command          : ${exeCmd}")
    #set_coverage_flags(${EXE_NAME})
    #set_project_warnings(${EXE_NAME})
endforeach()

return()
