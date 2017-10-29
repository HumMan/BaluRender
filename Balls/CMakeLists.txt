project(Balls)

file(GLOB Balls_SRC
    "*.cpp"
	${GLFW_SOURCE_DIR}/deps/tinycthread.c
)

source_group(Balls FILES ${Balls_SRC})
find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})
include_directories(../BaluLib/Include)

add_executable(Balls
	${Balls_SRC}
)

target_link_libraries ( Balls
	BaluLib
	BaluRender
	glfw
	GL GLEW IL ILU ILUT ${FREETYPE_LIBRARIES}
) 
