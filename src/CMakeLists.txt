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
  add_compile_options(/wd3199) #disable librealsense warning

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

############################################################################################################
add_subdirectory(api)

############################################################################################################
option(BUILD_SHAPEFIT_DEV_APP "Build Shapefit Development App" ON)
if(BUILD_SHAPEFIT_DEV_APP)

    find_package(OpenGL REQUIRED)
    set(SHAPEFIT_DEV_DEPENDENCIES shapefit-core ${OPENGL_LIBRARIES})
    
    if(WIN32)
        add_subdirectory(thirdparty/LibRealSense/librealsense)
        add_subdirectory(thirdparty/glfw)
        list(APPEND SHAPEFIT_DEV_DEPENDENCIES realsense glfw3)
    else()
        if(APPLE)
            add_definitions(-DAPPLE)
        else()
            add_subdirectory(thirdparty/LibRealSense/librealsense)
            list(APPEND SHAPEFIT_DEV_DEPENDENCIES realsense ${LIBUSB1_LIBRARIES})
        endif()

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
        list(APPEND SHAPEFIT_DEV_DEPENDENCIES m ${GLFW_LIBRARIES} shapefit-core)
        include_directories(${GLFW_INCLUDE_DIR})
    endif()
	set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
	set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    add_subdirectory(dev)

    if(MSVC)
        set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT shapefit-dev)
		set(CMAKE_CONFIGURATION_TYPES Release CACHE STRING "" FORCE)
    endif()
endif()
