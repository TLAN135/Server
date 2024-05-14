cmake_minimum_required(VERSION 3.16.3)

project(NetworkServer)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
aux_source_directory(src SRC)

include_directories(include)

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries (${PROJECT_NAME} pthread) 