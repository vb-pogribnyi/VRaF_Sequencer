cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
set(PROJECT_NAME VRaF_Example)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

set (SOURCE_FILES main.cpp src/VRaFSequencer.cpp
		third_party/imgui/imgui.cpp
		third_party/imgui/imgui_widgets.cpp
		third_party/imgui/imgui_draw.cpp
		third_party/imgui/imgui_tables.cpp
		third_party/imgui/backends/imgui_impl_opengl3.cpp
		third_party/imgui/backends/imgui_impl_glfw.cpp)
set (HEADER_FILES)

# OpenGL
find_package(OpenGL REQUIRED)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/glfw)

include_directories(inc)
include_directories(third_party/glfw/include)
include_directories(third_party/glm)
include_directories(third_party/imgui)
include_directories(third_party/imgui/backends)

add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} glfw)
target_link_libraries(${PROJECT_NAME} opengl32)
