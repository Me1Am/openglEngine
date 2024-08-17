# Setup CMake
#set(CMAKE_COLOR_MAKEFILE ON)
set(CMAKE_VERBOSE_MAKEFILE ON)
#set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_CXX_STANDARD 17)

cmake_minimum_required(VERSION 3.20)

# Project Setup
project(openglEngine VERSION 1.0)

set(SOURCES 
	"src/main.cpp"
	"src/Collision.cpp"

	"src/include/Window.hpp"
	"src/include/UI.hpp"
	"src/include/Mesh.hpp"
	"src/include/Model.hpp"
	"src/include/PhysicsDrawer.hpp"
	"src/include/PhysicsEngine.hpp"
	"src/include/ObjectHandler.hpp"
	"src/include/FileHandler.hpp"
	"src/include/Collision.h"
	"src/include/Heightmap.hpp"
	"src/include/GameObject.hpp"
	"src/include/StaticBody.hpp"

	"src/include/shader/BaseShader.hpp"
	"src/include/shader/TextShader.hpp"
	"src/include/shader/CubeShader.hpp"

	"src/include/ecs/Node.hpp"
	"src/include/ecs/Global.hpp"
	"src/include/ecs/Systems.hpp"
	"src/include/ecs/Components.hpp"
)

if(WIN32)
	set(CMAKE_CXX_COMPILER "C:/Users/JTK6759/Documents/apps/msys64/ucrt64/bin/g++.exe")
	set(COMPILE_FLAGS="-g -Wall -Dmain=SDL_main")

	add_link_options("-lmingw32" "-lSDL2" "-lSDL2main")
	add_executable(openglEngine)

	target_include_directories(openglEngine PRIVATE 
		"C:/Users/JTK6759/Documents/apps/msys64/ucrt64/include/freetype2/"
		"C:/Users/JTK6759/Documents/apps/msys64/ucrt64/include/bullet/"
	)
	target_compile_definitions(openglEngine PRIVATE DEBUG WIN32)
	target_link_libraries(openglEngine PRIVATE SDL2 SOIL opengl32 glew32 freeglut assimp freetype BulletDynamics BulletCollision LinearMath BulletWorldImporter)
else()
	set(CMAKE_CXX_COMPILER "/usr/bin/g++")
	set(COMPILE_FLAGS="-g -Wall")
	
	add_executable(openglEngine)
	
	# If your distro used freetype2 for freetype 2.x
	if(NOT EXISTS "/usr/include/freetype/")
		target_include_directories(openglEngine PRIVATE "/usr/include/freetype2/")
	endif()
	target_include_directories(openglEngine PRIVATE "/usr/include/bullet/")
	target_compile_definitions(openglEngine PRIVATE DEBUG)
	target_link_libraries(openglEngine PRIVATE SDL2 GL GLEW SOIL assimp freetype BulletDynamics BulletCollision LinearMath BulletWorldImporter)
endif()

target_sources(openglEngine PRIVATE ${SOURCES})
target_include_directories(openglEngine PRIVATE "src/" "src/include/")
