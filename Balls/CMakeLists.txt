project(Balls)

file(GLOB Balls_SRC
    "*.cpp"
	${GLFW_SOURCE_DIR}/deps/tinycthread.c
)

source_group(Balls FILES ${Balls_SRC})
include_directories(../BaluLib/Include)

add_executable(Balls
	${Balls_SRC}
)

find_package(OpenGL REQUIRED)

target_link_libraries ( Balls
	BaluLib
	BaluRender
	glfw
) 
