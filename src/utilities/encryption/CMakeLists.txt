BuildTarget(TRUE WIN32)
# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:${PROJECT_NAME},WIN32_EXECUTABLE> $<TARGET_FILE_DIR:${PROJECT_NAME}>
#   COMMAND_EXPAND_LISTS
#   )
# target_link_libraries(${PROJECT_NAME} test_lib)
set(OensslDir  $ENV{CppDevEnvs}/OpenSSL-Win64)
target_include_directories(${PROJECT_NAME} PRIVATE ${OensslDir}/include)
target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32 _DEBUG _WINDOWS _UNICODE UNICODE)
target_link_directories(${PROJECT_NAME} PRIVATE ${OensslDir}/lib/VC/x64/MD)
target_link_libraries(${PROJECT_NAME} PRIVATE libssl_static.lib libcrypto_static.lib Ws2_32.lib Crypt32.lib) 



