
function(GetCurrentFolderName FolderName)
    # remove last end of "/"
    string(REGEX REPLACE "/$" "" CURRENT_FOLDER_ABSOLUTE ${CMAKE_CURRENT_SOURCE_DIR})
    # get current relative dir name and set target name
    string(REGEX REPLACE ".*/(.*)" "\\1" __FolderName ${CURRENT_FOLDER_ABSOLUTE})
    set(${FolderName} ${__FolderName} PARENT_SCOPE)
endfunction(GetCurrentFolderName)


macro(GetSubDirectory result curdir)   
    file(GLOB children RELATIVE ${curdir} ${curdir}/*)
    set(dirlist "")
    if(${ARGN})
        list(FILTER children EXCLUDE REGEX ${ARGN})
    endif()
    foreach(child ${children})
        if(IS_DIRECTORY ${curdir}/${child})
            LIST(APPEND dirlist ${child})
        endif()
    endforeach()
    set(${result} ${dirlist})
    unset(dirlist)
    unset(children)
endmacro()
