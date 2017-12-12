cmake_minimum_required(VERSION 3.5)

project(synthetic_generator)

find_package(VTK 7.1.1 QUIET)
if (NOT VTK_FOUND)
    message(STATUS "VTK not found; disabling ${PROJECT_NAME}")
    return()
endif()
find_package(OpenCV REQUIRED COMPONENTS
    core
    imgproc
    imgcodecs
)
find_package(Eigen3 REQUIRED)
add_executable(${PROJECT_NAME} Utils.cpp ScenesObjects.cpp Scenarios.cpp SingleController.cpp TwoControllers.cpp Interactive.cpp Callbacks.cpp TrackerProxy.cpp main.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 14)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${VTK_INCLUDE_DIRS} PUBLIC .)
target_link_libraries(${PROJECT_NAME} PRIVATE tracker ${VTK_LIBRARIES} ${OpenCV_LIB_COMPONENTS} Eigen3::Eigen)
