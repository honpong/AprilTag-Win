cmake_minimum_required(VERSION 3.1)
project(APRILTAG_LIBRARY)

include_directories(common)

file(GLOB lib_srcs *.c common/*.c)
file(GLOB lib_hdrs *.h common/*.h common/sys/*.h)

if(UNIX)
  find_package (Threads REQUIRED)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
if(CMAKE_COMPILER_IS_GNUCXX AND UNIX)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

add_library(APRILTAG_LIBRARY ${lib_srcs} ${lib_hdrs})

if(WIN32 AND MSVC)
  target_link_libraries (APRILTAG_LIBRARY PTHREADS_LIBRARY)
else()
  target_link_libraries( APRILTAG_LIBRARY ${CMAKE_THREAD_LIBS_INIT} )
endif()

target_include_directories (APRILTAG_LIBRARY PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

return()




project(apriltag)

find_package(OpenCV QUIET)

include_directories(.)
aux_source_directory(common COMMON_SRC)
aux_source_directory(. APRILTAG_SRCS)

# Library
add_library(${PROJECT_NAME} SHARED ${APRILTAG_SRCS} ${COMMON_SRC})
if (MSVC)
    # FindThreads will not find pthread.h with MSVC
    # winmm is necessary for __imp_timeGetTime
    find_library(PTHREAD_LIBRARIES NAMES pthreads)
    target_link_libraries(${PROJECT_NAME} ${PTHREAD_LIBRARIES} winmm)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} Threads::Threads m)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION 3 VERSION 3.1.0)

include(GNUInstallDirs)
target_include_directories(${PROJECT_NAME} PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/common>"
    "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>/apriltag")

set_target_properties(${PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_INSTALL_LIBDIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_INSTALL_LIBDIR}
)

# install library
install(TARGETS ${PROJECT_NAME} EXPORT apriltag
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# install header file hierarchy
file(GLOB_RECURSE HEADER_FILES RELATIVE ${CMAKE_SOURCE_DIR} *.h)
foreach(HEADER ${HEADER_FILES})
    string(REGEX MATCH "(.*)[/\\]" DIR ${HEADER})
    install(FILES ${HEADER} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/${DIR})
endforeach()

# export library
install(EXPORT apriltag
    DESTINATION share/apriltag/cmake
    FILE apriltagConfig.cmake
)

FILE(READ apriltag.pc.in PKGC)
STRING(REGEX REPLACE "^prefix=" "prefix=${CMAKE_INSTALL_PREFIX}" PKGC_CONF "${PKGC}" )
FILE(WRITE ${PROJECT_BINARY_DIR}/apriltag.pc ${PKGC_CONF})
install(FILES "${PROJECT_BINARY_DIR}/apriltag.pc" DESTINATION "lib/pkgconfig/")

# Examples
# apriltag_demo
add_executable(apriltag_demo example/apriltag_demo.c)
target_link_libraries(apriltag_demo apriltag)

# opencv_demo
if(OpenCV_FOUND)
    add_executable(opencv_demo example/opencv_demo.cc)
    target_link_libraries(opencv_demo apriltag ${OpenCV_LIBRARIES})
    install(TARGETS opencv_demo RUNTIME DESTINATION bin)
endif(OpenCV_FOUND)

# install example programs
install(TARGETS apriltag_demo RUNTIME DESTINATION bin)

