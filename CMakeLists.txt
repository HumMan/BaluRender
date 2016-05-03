cmake_minimum_required(VERSION 2.8)

project(BaluRender)

file(GLOB_RECURSE BaluRender_SRC
    "Source/*.h"
    "Source/*.cpp"
)

if(UNIX)
    SET(CMAKE_CXX_FLAGS "-g -std=c++11 -w -pedantic")
endif()

add_definitions(-DILUT_USE_OPENGL)

add_subdirectory("glfw")

include_directories("Include")
include_directories("easyloggingpp/src")

add_subdirectory(Balls)
add_subdirectory(RayCollisionTest)
add_subdirectory(BaluLib)
include_directories(${BaluLib_SOURCE_DIR}/Include)

find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})

find_package(DevIL REQUIRED)
include_directories(${IL_INCLUDE_DIR})

find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})

add_library(BaluRender ${BaluRender_SRC})
