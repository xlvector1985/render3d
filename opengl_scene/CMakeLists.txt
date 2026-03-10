cmake_minimum_required(VERSION 3.16)
project(opengl_scene LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)

add_executable(opengl_scene main.cpp)
target_link_libraries(opengl_scene PRIVATE glfw OpenGL::GL)
