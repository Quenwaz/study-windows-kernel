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


function(GetCurrentFolderName FolderName)
    # remove last end of "/"
    string(REGEX REPLACE "/$" "" CURRENT_FOLDER_ABSOLUTE ${CMAKE_CURRENT_SOURCE_DIR})
    # get current relative dir name and set target name
    string(REGEX REPLACE ".*/(.*)" "\\1" FolderName ${CURRENT_FOLDER_ABSOLUTE})
    set(${FolderName} ${FolderName} PARENT_SCOPE)
    unset(CURRENT_FOLDER_ABSOLUTE)
endfunction(GetCurrentFolderName)

add_subdirectory(study_windows_kernel/dynamic_link_library)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# ADD_CUSTOM_COMMAND(
#           TARGET targetname 
#           POST_BUILD
#           COMMAND cp ./someobject  ../
# ) -