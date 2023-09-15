cmake_minimum_required(VERSION 3.25)

project(D64TOOL)

if(MSVC)
	add_definitions("/D_CRT_SECURE_NO_WARNINGS")
endif()

add_subdirectory("${PROJECT_SOURCE_DIR}/src")