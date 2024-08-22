BuildTarget(TRUE WIN32)
# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:${PROJECT_NAME},WIN32_EXECUTABLE> $<TARGET_FILE_DIR:${PROJECT_NAME}>
#   COMMAND_EXPAND_LISTS
#   )
# target_link_libraries(${PROJECT_NAME} test_lib)
target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32 _DEBUG _WINDOWS UNICODE _UNICODE)
target_include_directories(${PROJECT_NAME} PRIVATE E:/DevEnvs/cpp/WTL10_10320_Release/Include E:/DevEnvs/cpp/wil-1.0.240803.1/include)
# set_target_properties()

message(STATUS "PROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}")