project(Balls)

file(GLOB Balls_SRC
    "*.cpp"
)

# These are used to create visual studio folders.
source_group(Balls FILES ${Balls_SRC})

include_directories(${BaluRender_SOURCE_DIR}/Include)
include_directories(${BaluLib_SOURCE_DIR}/Include)

if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=gnu++0x")
endif()
find_package(GLEW REQUIRED)
if (GLEW_FOUND)
    include_directories(${GLEW_INCLUDE_DIRS})
    link_libraries(${GLEW_LIBRARIES}/Release/Win32/glew32.lib)
endif()

find_package(DevIL REQUIRED)
if (IL_FOUND)
    include_directories(${IL_INCLUDE_DIR})
    link_libraries(${IL_LIBRARIES}/DevIL.lib)
	link_libraries(${IL_LIBRARIES}/ILU.lib)
	link_libraries(${IL_LIBRARIES}/ILUT.lib)
endif()

add_executable(Balls WIN32
	${Balls_SRC}
)

target_link_libraries ( Balls
	BaluLib
	BaluRender
	glu32 opengl32
) 

