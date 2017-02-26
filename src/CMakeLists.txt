cmake_minimum_required(VERSION 3.7.0 FATAL_ERROR)

project(shapefit C CXX)

# SDL (Security Development Lifecycle) Requirements
set(CMAKE_POSITION_INDEPENDENT_CODE True)
if (MSVC)
  add_compile_options(/sdl)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Intel|GNU|Clang")
  if (NOT APPLE) # not supported by Apple's linker
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -z relro -z now -z noexecstack -pie")
  endif()
  add_definitions(-D_FORTIFY_SOURCE=2)
  add_compile_options(-ffunction-sections)
  add_compile_options(-fstack-protector-strong)
  add_compile_options(-Wformat -Wformat-security)
  add_compile_options(-fvisibility=hidden)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fvisibility-inlines-hidden>)
endif ()

if(MSVC)
  add_definitions(-D_USE_MATH_DEFINES) # we expect M_PI in math.h / cmath
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)

foreach(flag_var
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
  if(${flag_var} MATCHES "/MD")
    string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
  endif()
  if(${flag_var} MATCHES "/MDd")
    string(REGEX REPLACE "/MDd" "/MTd" ${flag_var} "${${flag_var}}")
  endif()
endforeach(flag_var)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:atlthunk.lib /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:msvcrtd.lib")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcpmt.lib")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:libcmtd.lib")

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:atlthunk.lib /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:msvcrtd.lib")
set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcpmt.lib")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:libcmtd.lib")
endif()

macro(target_link_libraries_copied target)
  target_link_libraries(${target} ${ARGN})
  if (MSVC)
    foreach(__lib__ ${ARGN})
      if (NOT __lib__ MATCHES "^(PRIVATE|PUBLIC|INTERFACE)$")
        add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${__lib__}>" "$<TARGET_FILE_DIR:${target}>")
        set_target_properties(${target} PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "$<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_NAME:${__lib__}>") # FIXME: Generator Expressions not supported here
      endif()
    endforeach()
  endif()
endmacro()

############################################################################################################
option(BUILD_SAMPLE_APP "Build Sample App" ON)
SET(THIRDPARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)
SET(EIGEN_DIR ${THIRDPARTY_DIR}/eigen)
SET(LIBREALSENSE_VER "v2.3.3")
SET(LIBREALSENSE_DIR ${THIRDPARTY_DIR}/LibRealSense/Windows/${LIBREALSENSE_VER})
SET(LIBREALSENSE_LIB realsense)
SET(JSON_SRC_DIR ${THIRDPARTY_DIR}/Json/src)
SET(JSON_INC_DIR ${THIRDPARTY_DIR}/Json/include)
SET(JSON_SRC "${JSON_SRC_DIR}/jsoncpp.cpp")
SET(OpenCV_STATIC ON)
find_package(OPENCV REQUIRED)
find_package(OpenGL REQUIRED)

if(WIN32)
    add_subdirectory(${THIRDPARTY_DIR}/glfw)
    #list(APPEND DEPENDENCIES glfw3)
else()
    # Find glfw header
    find_path(GLFW_INCLUDE_DIR NAMES GLFW/glfw3.h
        PATHS /usr/X11R6/include
              /usr/include/X11
              /opt/graphics/OpenGL/include
              /opt/graphics/OpenGL/contrib/libglfw
              /usr/local/include
              /usr/include/GL
              /usr/include
    )
    # Find glfw library
    find_library(GLFW_LIBRARIES NAMES glfw glfw3
            PATHS /usr/lib64
                  /usr/lib
                  /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
                  /usr/local/lib64
                  /usr/local/lib
                  /usr/local/lib/${CMAKE_LIBRARY_ARCHITECTURE}
                  /usr/X11R6/lib
    )
    #list(APPEND DEPENDENCIES m ${GLFW_LIBRARIES} ${LIBUSB1_LIBRARIES})
    #include_directories(${GLFW_INCLUDE_DIR})
endif()


link_directories(${LIBREALSENSE_DIR}/lib/${CMAKE_VS_PLATFORM_NAME})

############################################################################################################
FILE(GLOB SHAPEFIT_API_SRC "${CMAKE_CURRENT_SOURCE_DIR}/api/*.cpp")
FILE(GLOB SHAPEFIT_API_HDR "${CMAKE_CURRENT_SOURCE_DIR}/api/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/api/inc/*")
add_library(shapefit_api SHARED 
    ${SHAPEFIT_API_SRC}
    ${SHAPEFIT_API_HDR})
set_property(TARGET shapefit_api PROPERTY CXX_STANDARD 14)
include_directories(
    ${OPENCV_INCLUDE_DIR} 
    ${EIGEN_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/api/inc)
target_link_libraries(shapefit_api PRIVATE ${OpenCV_LIBS})

#############################################################################################################
FILE(GLOB SHAPEFIT_APP_SRC "${CMAKE_CURRENT_SOURCE_DIR}/app/*.cpp")
FILE(GLOB SHAPEFIT_APP_HDR "${CMAKE_CURRENT_SOURCE_DIR}/app/*.hpp")
add_executable(shapefit_app 
    ${SHAPEFIT_APP_SRC}
	${SHAPEFIT_APP_HDR}
    ${JSON_SRC})
set_property(TARGET shapefit_app PROPERTY CXX_STANDARD 14)
include_directories(
    ${OPENCV_INCLUDE_DIR} 
    ${EIGEN_DIR} 
    ${LIBREALSENSE_DIR}/include
    ${JSON_INC_DIR})
target_link_libraries(shapefit_app PUBLIC shapefit_api PRIVATE ${OpenCV_LIBS} ${LIBREALSENSE_LIB} ${OPENGL_LIBRARIES} glfw3)
add_custom_command(TARGET shapefit_app POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:shapefit_api>" "$<TARGET_FILE_DIR:shapefit_app>")

IF(WIN32)
set_target_properties(shapefit_app PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(TargetDir) ")
ENDIF()