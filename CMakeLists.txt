cmake_minimum_required (VERSION 3.18)

project (study-windows-kernel VERSION 0.1.0 LANGUAGES C CXX)

include (InstallRequiredSystemLibraries)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake_modules)

option(OPT1 "description" ON)

add_subdirectory(study_windows_kernel/dynamic_link_library)


message(STATUS "msg...")



set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# ADD_CUSTOM_COMMAND(
#           TARGET targetname 
#           POST_BUILD
#           COMMAND cp ./someobject  ../
# ) -