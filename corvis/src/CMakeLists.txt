set(CMAKE_SKIP_INSTALL_RULES 1) # Avoids warning w/mgl-static and EXCLUDE_FROM_ALL
if(ENABLE_MGL)
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
    set_target_properties(mgl_example mgl-static PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
    target_compile_options(mgl PUBLIC -Wno-deprecated-register)
    target_compile_options(mgl PRIVATE -Wno-deprecated-declarations)
    target_compile_options(mgl INTERFACE -Wno-return-type-c-linkage -Wno-parentheses-equality)
endif()

set(GLFW_INSTALL OFF CACHE BOOL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
add_subdirectory(glfw)
if(NOT WIN32)
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

set(COR_SOURCES cor/sensor_fusion_queue.cpp)
if(WIN32)
    set(COR_SOURCES ${COR_SOURCES}
      cor/platform/win/sensor_clock.cpp
    )
else()
    set(COR_SOURCES ${COR_SOURCES}
      cor/platform/shell/sensor_clock.cpp
    )
endif()
add_library(cor STATIC ${COR_SOURCES})
target_include_directories(cor PUBLIC cor)
set_property(TARGET cor PROPERTY CXX_STANDARD 14)
if(WIN32)
    target_include_directories(cor PRIVATE "$ENV{RSSDK_DIR}include")
endif()

add_library(numerics STATIC
  numerics/rotation_vector.cpp
  numerics/vec4.cpp
  numerics/matrix.cpp
  numerics/kalman.cpp
)
target_include_directories(numerics INTERFACE numerics)

set_property(TARGET numerics PROPERTY CXX_STANDARD 14)
if(WIN32)
  target_include_directories(numerics PRIVATE "$ENV{RSSDK_DIR}include")
endif()

if(WIN32 OR ANDROID)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(MKL_ARCH intel64)
      set(MKL_INTEL_SUFFIX _lp64)
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
      set(MKL_ARCH ia32)
      set(MKL_INTEL_SUFFIX _c)
    endif()
    file(GLOB MKLLOCS
        "C:/Program Files (x86)/Intel/Composer XE 2015/mkl"
        "C:/Program Files/Intel/Composer XE 2015/mkl"
    )
    list(GET MKLLOCS 0 MKLLOC)
    target_include_directories(numerics PRIVATE "${MKLLOC}/include")
    link_directories("$ENV{RSSDK_DIR}lib/$(PlatformName)")
    target_link_libraries(numerics PRIVATE "${MKLLOC}/lib/${MKL_ARCH}/${CMAKE_STATIC_LIBRARY_PREFIX}mkl_intel${MKL_INTEL_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")
    target_link_libraries(numerics PRIVATE "${MKLLOC}/lib/${MKL_ARCH}/${CMAKE_STATIC_LIBRARY_PREFIX}mkl_core${CMAKE_STATIC_LIBRARY_SUFFIX}")
    target_link_libraries(numerics PRIVATE "${MKLLOC}/lib/${MKL_ARCH}/${CMAKE_STATIC_LIBRARY_PREFIX}mkl_sequential${CMAKE_STATIC_LIBRARY_SUFFIX}")
else()
    target_link_libraries(numerics PUBLIC "-framework Accelerate")
endif()

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
  filter/calibration_json_store.cpp
  filter/calibration_defaults.cpp
  filter/sensor_fusion.cpp
  filter/capture.cpp
  filter/replay.cpp
)
target_include_directories(filter PUBLIC filter)
target_include_directories(filter PRIVATE rapidjson/include)
if(ENABLE_ZXING)
    target_link_libraries(filter PRIVATE libzxing)
endif()
set_property(TARGET filter PROPERTY CXX_STANDARD 14)

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
target_link_libraries(vis PRIVATE glad glfw)
if(ENABLE_MGL)
    target_link_libraries(vis PRIVATE mgl)
endif()
target_link_libraries(vis PRIVATE ${GLFW_LIBRARIES})
set_property(TARGET vis PROPERTY CXX_STANDARD 14)

if(ENABLE_STEREO)
    add_subdirectory(qhull)
    foreach(qvar qhull_TARGETS qhull_STATIC qhull_SHARED qhull_SHAREDP)
      get_directory_property(${qvar} DIRECTORY qhull DEFINITION ${qvar})
    endforeach(qvar)
    set_target_properties(${qhull_TARGETS} PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
    set_target_properties(${qhull_SHARED} ${qhull_SHAREDP} PROPERTIES MACOSX_RPATH ON) # Avoid pointless warning
    target_include_directories(${qhull_STATIC} PUBLIC qhull/src/libqhull)

    add_library(stereo STATIC
      stereo/stereo.cpp
      stereo/stereo_mesh.cpp
      stereo/camera.cpp
      stereo/fundamental.cpp
      stereo/stereo_features.cpp
    )
    target_link_libraries(stereo PRIVATE DAI ${qhull_STATIC} vlfeat)
    target_include_directories(stereo PUBLIC stereo)
    set_property(TARGET stereo PROPERTY CXX_STANDARD 14)

    file(GLOB LIBVLFEAT_FILES "vlfeat-0.9.18/vl/*.c")
    add_library(vlfeat STATIC ${LIBVLFEAT_FILES})
    target_include_directories(vlfeat INTERFACE vlfeat-0.9.18/vl)

    find_package(Boost REQUIRED)
    foreach(f alldai.cpp graph.cpp regiongraph.cpp emalg.cpp bipgraph.cpp evidence.cpp io.cpp trwbp.cpp bp.cpp exactinf.cpp daialg.cpp properties.cpp util.cpp bp_dual.cpp exceptions.cpp varset.cpp factor.cpp weightedgraph.cpp clustergraph.cpp factorgraph.cpp dag.cpp)
      list(APPEND DAI_FILES "libDAI-0.3.1/src/${f}")
    endforeach(f)
    add_library(DAI STATIC ${DAI_FILES})
    target_include_directories(DAI PUBLIC
      libDAI-0.3.1/include
      "${Boost_INCLUDE_DIRS}"
    )
    # -DMACOSX forces libDAI to include boost using <boost/tr1/*>
    target_compile_options(DAI PUBLIC -DMACOSX -DDAI_WITH_BP -DDAI_WITH_TRWBP)
endif() # WIN32

if(ENABLE_ZXING)
    file(GLOB_RECURSE LIBZXING_FILES
        "libzxing-cpp/zxing/*.cpp"
        "libzxing-cpp/bigint/*.cpp"
    )
    add_library(libzxing STATIC ${LIBZXING_FILES})
    target_compile_definitions(libzxing PRIVATE NO_ICONV)
    target_include_directories(libzxing
      PUBLIC libzxing-cpp
    )
endif()

if(NOT WIN32)
    target_compile_options(filter PRIVATE -DDEBUG=1)
    target_compile_options(cor PRIVATE -DDEBUG=1)
    target_compile_options(numerics PRIVATE -DDEBUG=1)
    target_compile_options(vis PRIVATE -DDEBUG=1)
endif()
