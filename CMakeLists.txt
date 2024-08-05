cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# TODO: fix the ImGUI dependency and provide it as a library
set(IMGUI_SRC
    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_vulkan.cpp)
    
project(sbox LANGUAGES CXX)
add_definitions(-DFMT_HEADER_ONLY)
add_executable(sbox src/main.cpp ${IMGUI_SRC})
target_link_libraries(sbox vulkan)
target_include_directories(sbox PRIVATE external/)
target_include_directories(sbox PRIVATE external/imgui)

add_subdirectory(external/glfw)
target_link_libraries(sbox glfw)

add_subdirectory(external/vk-bootstrap)
target_link_libraries(sbox vk-bootstrap)

add_subdirectory(external/glm)
target_link_libraries(sbox glm)

target_include_directories(sbox PRIVATE external/vk-bootstrap/src)
target_include_directories(sbox PRIVATE external/glm)
target_include_directories(sbox PRIVATE src)
