project(RayCollisionTest)

file(GLOB RayCollisionTest_SRC
    "*.cpp"
	${GLFW_SOURCE_DIR}/deps/tinycthread.c
)

source_group(RayCollisionTest FILES ${RayCollisionTest_SRC})
find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})
include_directories(../BaluLib/Include)

add_executable(RayCollisionTest
	${RayCollisionTest_SRC}
)

target_link_libraries ( RayCollisionTest
	BaluLib
	BaluRender
	glfw
	GL GLEW IL ILU ILUT ${FREETYPE_LIBRARIES}
) 
