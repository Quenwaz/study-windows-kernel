cmake_minimum_required (VERSION 3.18)

project (study_windows_kernel  LANGUAGES C CXX)

include (InstallRequiredSystemLibraries)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# include(CTest)
# enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake_modules)
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/install)
option(OPT1 "description" ON)

message(STATUS "TEST: IMPORTED_CONFIGURATIONS =${IMPORTED_CONFIGURATIONS}")

include(utils)
include(compile_template)

include_directories(src)
# GetSubDirectory(SubDirList ${CMAKE_CURRENT_LIST_DIR}/src "common_header")
file(GLOB_RECURSE CMakeListFiles "${CMAKE_CURRENT_LIST_DIR}/src/CMakeLists.txt")
message(STATUS CMakeListFiles=${CMakeListFiles})
foreach(CMakeListFile ${CMakeListFiles})
    get_filename_component(CMakeListDir ${CMakeListFile} DIRECTORY)
    file(RELATIVE_PATH CMakeListDir ${CMAKE_CURRENT_LIST_DIR}/src ${CMakeListDir})
    # if(EXISTS ${CMakeListFile}/CMakeLists.txt)
    add_subdirectory(src/${CMakeListDir})
    # endif()
endforeach()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# ADD_CUSTOM_COMMAND(
#           TARGET targetname 
#           POST_BUILD
#           COMMAND cp ./someobject  ../
# ) -