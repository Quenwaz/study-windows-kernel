BuildTarget(TRUE WIN32)
# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:${PROJECT_NAME},WIN32_EXECUTABLE> $<TARGET_FILE_DIR:${PROJECT_NAME}>
#   COMMAND_EXPAND_LISTS
#   )
# target_link_libraries(${PROJECT_NAME} test_lib)
target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32 _DEBUG _WINDOWS)
# set_target_properties()
target_link_libraries(${PROJECT_NAME}  libscreenusetime)

add_custom_command(TARGET ${PROJECT_NAME} 
   POST_BUILD 
   COMMAND ${CMAKE_COMMAND} -E 
       copy_if_different  
        "$<TARGET_FILE_DIR:libscreenusetime>/$<TARGET_FILE_NAME:libscreenusetime>"  
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"  
)

message(STATUS "PROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}")