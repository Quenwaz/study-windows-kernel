cmake_minimum_required (VERSION 3.18)



GetCurrentFolderName(CURRENT_FOLDER)
project (${CURRENT_FOLDER}  LANGUAGES C CXX)


aux_source_directory(. DIR_SRCS)

add_executable(${PROJECT_NAME} ${DIR_SRCS})