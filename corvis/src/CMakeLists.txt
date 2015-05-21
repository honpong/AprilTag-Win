# add_definitions(-Wno-deprecated-declarations)

set(CMAKE_SKIP_INSTALL_RULES 1) # Avoids warning w/mgl-static and EXCLUDE_FROM_ALL

set(enable-rvalue ON CACHE BOOL "")
set(enable-lgpl ON CACHE BOOL "")
set(enable-png OFF CACHE BOOL "")
set(enable-openmp OFF CACHE BOOL "")
set(enable-pthread OFF CACHE BOOL "")
set(enable-opengl OFF CACHE BOOL "")
set(enable-mpi OFF CACHE BOOL "")
set(enable-zlib OFF CACHE BOOL "")
set(enable-all-widgets OFF CACHE BOOL "")
add_subdirectory(mathgl)
set_target_properties(mgl PROPERTIES MACOSX_RPATH ON)
target_include_directories(mgl INTERFACE mathgl/include $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/mathgl/include>)
set_target_properties(mgl_example mgl-static PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1 CMAKE_SKIP_INSTALL_RULES 1)
target_compile_options(mgl PUBLIC -Wno-deprecated-register)
target_compile_options(mgl PRIVATE -Wno-deprecated-declarations)

set(GLFW_INSTALL OFF CACHE BOOL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
add_subdirectory(glfw)
target_compile_options(glfw PRIVATE -Wno-deprecated-declarations)
target_include_directories(glfw INTERFACE glfw/include)

add_library(glad STATIC
  glfw/deps/glad.c
)
target_include_directories(glad
  PUBLIC glfw/deps
)

add_library(cor STATIC
  cor/sensor_fusion_queue.cpp
  cor/platform/shell/sensor_data.cpp
  cor/platform/shell/sensor_clock.cpp
)
target_compile_options(cor PUBLIC -std=c++14)

add_library(numerics STATIC
  numerics/rotation_vector.cpp
  numerics/vec4.cpp
  numerics/matrix.cpp
  numerics/kalman.cpp
)
target_link_libraries(numerics PUBLIC "-framework Accelerate")
target_compile_options(numerics PUBLIC -std=c++14)

add_library(filter STATIC
  filter/filter.cpp
  filter/state_vision.cpp
  filter/state_motion.cpp
  filter/observation.cpp
  filter/filter_setup.cpp
  filter/fast_detector/fast_9.cpp
  filter/scaled_mask.cpp
  filter/camera_control_interface_dummy.cpp
  filter/homography.cpp
  filter/qr.cpp
  filter/device_parameters.cpp
)
target_link_libraries(filter PRIVATE libzxing)
target_compile_options(filter PUBLIC -std=c++14)

add_library(vis STATIC
  vis/world_state.cpp
  vis/world_state_render.cpp
  vis/arcball.cpp
  vis/offscreen_render.cpp
  vis/gui.cpp
  vis/lodepng.cpp
  vis/gl_util.cpp
  vis/render.cpp
  vis/video_render.cpp
  )
target_link_libraries(vis PRIVATE glad glfw mgl)
target_link_libraries(vis PRIVATE ${GLFW_LIBRARIES})
target_compile_options(vis PUBLIC -std=c++14)

file(GLOB_RECURSE LIBZXING_FILES
    "libzxing-cpp/zxing/*.cpp"
    "libzxing-cpp/bigint/*.cpp"
)
add_library(libzxing STATIC ${LIBZXING_FILES})
target_compile_definitions(libzxing PRIVATE NO_ICONV)
target_include_directories(libzxing
  PUBLIC libzxing-cpp
)
