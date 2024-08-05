cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(sbox LANGUAGES CXX)
add_definitions(-DFMT_HEADER_ONLY)
add_executable(sbox src/main.cpp)

add_subdirectory(external/glfw)
target_link_libraries(sbox glfw)

add_subdirectory(external/vk-bootstrap)
target_link_libraries(sbox vk-bootstrap)

target_include_directories(sbox PRIVATE external/vk-bootstrap/src)
target_include_directories(sbox PRIVATE src)
