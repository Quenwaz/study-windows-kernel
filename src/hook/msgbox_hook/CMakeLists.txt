BuildTarget(TRUE WIN32)
target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32 _DEBUG _WINDOWS)
target_link_libraries(${PROJECT_NAME}    api_interception)