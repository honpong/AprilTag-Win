set(GLFW_INSTALL OFF CACHE BOOL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
add_subdirectory(glfw)
if(!WIN32)
    target_compile_options(glfw PRIVATE -Wno-deprecated-declarations)
endif()
target_include_directories(glfw INTERFACE glfw/include)

add_library(glad STATIC
  glfw/deps/glad.c
)
target_compile_options(glad PRIVATE $<$<C_COMPILER_ID:MSVC>:/wd4996>)
target_include_directories(glad
  PUBLIC glfw/deps
)
