project(RayCollisionTest)

file(GLOB RayCollisionTest_SRC
    "*.cpp"
	${GLFW_SOURCE_DIR}/deps/tinycthread.c
)

source_group(RayCollisionTest FILES ${RayCollisionTest_SRC})
include_directories(../BaluLib/Include)

add_executable(RayCollisionTest
	${RayCollisionTest_SRC}
)

target_link_libraries ( RayCollisionTest
	BaluLib
	BaluRender
	glfw
) 
