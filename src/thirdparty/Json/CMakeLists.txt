cmake_minimum_required(VERSION 3.7.0 FATAL_ERROR)

set(PROJECT_NAME json3)
project(${PROJECT_NAME})

add_library(${PROJECT_NAME} STATIC 
	src/jsoncpp.cpp 
	include/json/json.h)

target_include_directories(${PROJECT_NAME} PUBLIC include)

set_target_properties(${PROJECT_NAME} PROPERTIES 
	FOLDER "3rd Party" 
	CXX_STANDARD 14
)
