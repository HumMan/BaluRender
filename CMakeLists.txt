cmake_minimum_required(VERSION 2.8)

project(BaluRender)

file(GLOB_RECURSE BaluRender_SRC
    "Source/*.h"
    "Source/*.cpp"
)

#if(UNIX)
#    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=gnu++0x")
#endif()

if(UNIX)
    SET(CMAKE_CXX_FLAGS "-g -std=c++11 -w -pedantic")
endif()

add_definitions(-DILUT_USE_OPENGL)

add_subdirectory("glfw")

include_directories("glfw/include")

include_directories("Include")
include_directories("easyloggingpp/src")

if (NOT TARGET BaluLib)
	add_subdirectory("BaluLib")
	add_subdirectory("RayCollisionTest")
	add_subdirectory("Balls")
endif()

include_directories(${BaluLib_SOURCE_DIR}/Include)

find_package(OpenGL REQUIRED)

find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})

find_package(DevIL REQUIRED)
include_directories(${IL_INCLUDE_DIR})

add_library(BaluRender SHARED ${BaluRender_SRC}
	)
	
add_definitions(-DBALURENDER_DLL_EXPORT)	
	
target_link_libraries ( BaluRender
	BaluLib
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES}
	${IL_LIBRARIES}
	${ILU_LIBRARIES}
	${ILUT_LIBRARIES}
) 
