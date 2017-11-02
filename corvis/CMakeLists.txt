cmake_minimum_required(VERSION 3.2.2 FATAL_ERROR)

project(corvis C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../cmake)

if (CMAKE_CXX_COMPILER_ID STREQUAL Clang AND EXISTS /usr/include/c++/v1)
  include_directories(SYSTEM BEFORE /usr/include/c++/v1)
  link_libraries(c++)
  list(APPEND CMAKE_REQUIRED_LIBRARIES c++)
endif()

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("int main(int c, char **v){return 0;}" UNWIND_UNNEEDED FAIL_REGEX Unwind)
if (NOT UNWIND_UNNEEDED)
  link_libraries(unwind)
endif()

# SDL (Security Development Lifecycle) Requirements
set(CMAKE_POSITION_INDEPENDENT_CODE True)
if (MSVC)
  add_compile_options(/sdl)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Intel|GNU|Clang")
  if (NOT APPLE) # not supported by Apple's linker
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -z relro -z now -z noexecstack -pie")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
  else()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-dead_strip")
  endif()
  add_definitions(-D_FORTIFY_SOURCE=2)
  add_compile_options(-ffunction-sections)
  add_compile_options(-fstack-protector-strong)
  add_compile_options(-Wformat -Wformat-security)
  add_compile_options(-fvisibility=hidden)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fvisibility-inlines-hidden>)
endif ()

if (MSVC)
  add_compile_options(-W2)
else()
  add_compile_options(-Wall)
endif()

if(MSVC)
  add_definitions(-D_USE_MATH_DEFINES) # we expect M_PI in math.h / cmath
endif()

find_package(alloy QUIET)
if (TARGET alloy)
  add_compile_options(-march=silvermont -mfpmath=sse)
elseif (WIN32)
elseif (ANDROID)
  # Match the options used by default in android-ndk-*/toolchains/*-clang3.6/setup.mk
  add_compile_options(-no-canonical-prefixes -funwind-tables -fno-omit-frame-pointer)
elseif (APPLE)
  add_compile_options(-march=native -mfpmath=sse)
  set(CMAKE_STRIP "/usr/bin/strip -ru")
endif ()

option(ENABLE_STEREO "Enable Stereo" OFF)
option(ENABLE_MEASURE "Enable Measure" ON)
option(ENABLE_MGL "Enable MathGL (used for plots in measure)" OFF)
option(ENABLE_QR "Enable QR Code localization" OFF)
option(ENABLE_SOURCES "Enable installation of source code examples" OFF)

# A few hints for the BOOST_ROOT to help find_package(Boost)
if (APPLE)
  execute_process(COMMAND brew --prefix OUTPUT_VARIABLE BREW_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
  file(GLOB BREW_PREFIX ${BREW_PREFIX} /opt/local)
  list(APPEND CMAKE_PREFIX_PATH ${BREW_PREFIX})
endif (APPLE)

add_subdirectory(src)

if (ENABLE_MEASURE)
find_package(OpenGL QUIET)
if (OPENGL_FOUND AND NOT ANDROID)
  set(GLFW_INSTALL OFF CACHE BOOL "")
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
  set(GLFW_)
  add_subdirectory(src/glfw)
  if(NOT WIN32)
    target_compile_options(glfw PRIVATE $<$<CXX_COMPILER_ID:AppleClang>: -Wno-deprecated-declarations>)
  endif()
  target_include_directories(glfw INTERFACE src/glfw/include)

  add_library(glad STATIC src/glfw/deps/glad.c)
  target_compile_options(glad PRIVATE $<$<C_COMPILER_ID:MSVC>:/wd4996>)
  target_include_directories(glad PUBLIC src/glfw/deps)
  set(HAVE_GLFW 1)
else()
  message(STATUS "No OpenGL found; measure's GUI disabled.")
endif()

add_executable(measure
  src/measure/measure.cpp
  src/measure/benchmark.cpp
  src/measure/for_each_file.cpp

  src/measure/world_state.cpp
  $<$<BOOL:${HAVE_GLFW}>:
    src/measure/world_state_render.cpp
    src/measure/arcball.cpp
    src/measure/offscreen_render.cpp
    src/measure/gui.cpp
    src/measure/lodepng.cpp
    src/measure/gl_util.cpp
    src/measure/render.cpp
    src/measure/video_render.cpp
  >
)
set_property(TARGET measure PROPERTY CXX_STANDARD 14)
install(TARGETS measure RUNTIME DESTINATION bin COMPONENT alloy)
if (HAVE_GLFW)
    target_compile_options(measure PRIVATE -DHAVE_GLFW=1)
    target_link_libraries(measure PRIVATE glad glfw ${GLFW_LIBRARIES})
endif()
if(CMAKE_BUILD_TYPE MATCHES "Debug" OR CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    target_compile_options(measure PRIVATE -DDEBUG=1)
endif()

target_link_libraries(measure PUBLIC tracker trackeri)
if(ENABLE_MGL)
    target_link_libraries(measure PRIVATE mgl)
endif()

target_link_libraries(measure PUBLIC trackeri)
if(ENABLE_MGL)
    add_subdirectory(../ThirdParty/mathgl mathgl)
    target_compile_options(mgl PRIVATE $<$<BOOL:${MSVC}>:/wd4244 /wd4190 /wd4800>)
    target_link_libraries(measure PRIVATE mgl)
    target_compile_definitions(measure PRIVATE HAVE_MGL)
endif()
target_include_directories(measure PUBLIC measure)
set_property(TARGET measure PROPERTY CXX_STANDARD 14)

endif()

add_subdirectory(src/rc_replay)

add_subdirectory(src/compare_poses)

add_subdirectory(src/create_vocabulary)

include(TargetLinkLibrariesCopied RESULT_VARIABLE TARGET_LINK_LIBRARIES_COPIED)
if(ENABLE_SOURCES)
install(FILES ${TARGET_LINK_LIBRARIES_COPIED} DESTINATION lib/cmake COMPONENT example-sources)
install(FILES
  src/rc_replay/CMakeLists.txt
  src/rc_replay/main.cpp
  src/rc_replay/rc_replay.h
  src/rc_replay/rc_replay.cpp
  DESTINATION src/rc_replay
  COMPONENT example-sources
)
endif()

add_subdirectory(src/movidius/host)

add_executable(alloy_merge src/tools/alloy_merge.cpp)
set_property(TARGET alloy_merge PROPERTY CXX_STANDARD 14)
target_link_libraries(alloy_merge tracker trackeri) # trackeri for private includes though we really link with tracker

find_package(alloy QUIET)
if (TARGET alloy)
    install(TARGETS alloy_merge RUNTIME DESTINATION bin COMPONENT alloy)
endif()

if(YES AND NOT ANDROID)
    enable_testing()
    add_subdirectory(test)
endif()

set(CPACK_PACKAGE_NAME "tracker")
set(CPACK_GENERATOR "ZIP")
if (NOT RC_VERSION)
    string(TIMESTAMP RC_VERSION "%Y-%m-%d")
endif()
set(CPACK_PACKAGE_VERSION ${RC_VERSION})
include(CPack)
