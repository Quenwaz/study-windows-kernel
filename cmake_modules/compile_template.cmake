

macro(BuildTarget build_to_exe)
    cmake_minimum_required (VERSION 3.18)

    GetCurrentFolderName(__CURRENT_FOLDER)
    set(TargetName ${__CURRENT_FOLDER})
    unset(__CURRENT_FOLDER)
    project (${TargetName} VERSION 0.1.0 LANGUAGES C CXX)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_STANDARD 17)
    message("---------------------Project:${TargetName}---------------------")
    message("Please use the variable PROJECT_NAME to set more compile or link options\n e.g.\n\ttarget_include_directories(${PROJECT_NAME} PRIVATE \${ffmpeg_INCLUDE_DIR} \${SDL_INCLUDE_DIR})")
    aux_source_directory(. SOURCES)
    file(GLOB_RECURSE RCFILES "${CMAKE_CURRENT_SOURCE_DIR}/*.rc")
    list(APPEND SOURCES ${RCFILES})

    message(STATUS "Sources:${SOURCES}")
    if (${build_to_exe})
        add_executable(${PROJECT_NAME} ${ARGN} ${SOURCES})
        message(STATUS "Generate executable program")
    else(${build_to_exe})
        add_library(${PROJECT_NAME} ${SOURCES})
        message(STATUS "Generate link library")
    endif(${build_to_exe})
    
    # target_include_directories(${PROJECT_NAME} PRIVATE ${ffmpeg_INCLUDE_DIR} ${SDL_INCLUDE_DIR})
    # target_link_libraries(${PROJECT_NAME} ${ffmpeg_LIBRARY} ${SDL_LIBRARY})
endmacro(BuildTarget)


