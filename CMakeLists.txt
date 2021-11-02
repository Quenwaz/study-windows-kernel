cmake_minimum_required (VERSION 3.18)

project (study-windows-kernel  LANGUAGES C CXX)

include (InstallRequiredSystemLibraries)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# include(CTest)
# enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake_modules)
option(OPT1 "description" ON)

message(STATUS "TEST: IMPORTED_CONFIGURATIONS =${IMPORTED_CONFIGURATIONS}")

include(utils)
include(compile_template)

GetSubDirectory(SubDirList ${CMAKE_CURRENT_LIST_DIR}/study_windows_kernel)
foreach(child ${SubDirList})
    add_subdirectory(study_windows_kernel/${child})
endforeach()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# ADD_CUSTOM_COMMAND(
#           TARGET targetname 
#           POST_BUILD
#           COMMAND cp ./someobject  ../
# ) -