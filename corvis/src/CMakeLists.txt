if(ENABLE_VISGL)
set(GLFW_INSTALL OFF CACHE BOOL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
add_subdirectory(glfw)
if(NOT WIN32)
    target_compile_options(glfw PRIVATE $<$<CXX_COMPILER_ID:AppleClang>: -Wno-deprecated-declarations>)
endif()
target_include_directories(glfw INTERFACE glfw/include)

add_library(glad STATIC
  glfw/deps/glad.c
)
target_compile_options(glad PRIVATE $<$<C_COMPILER_ID:MSVC>:/wd4996>)
target_include_directories(glad
  PUBLIC glfw/deps
)
endif()

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
target_link_libraries(cor numerics)
target_include_directories(cor PUBLIC cor)
target_include_directories(cor PRIVATE filter)
set_property(TARGET cor PROPERTY CXX_STANDARD 14)

add_library(numerics STATIC
  numerics/rotation_vector.cpp
  numerics/matrix.cpp
  numerics/kalman.cpp
)
target_include_directories(numerics SYSTEM PUBLIC eigen)
target_include_directories(numerics PUBLIC numerics)
target_include_directories(numerics PRIVATE cor)

set_property(TARGET numerics PROPERTY CXX_STANDARD 14)

function(find_intel_root TYPE) # [REQUIRED]
    set(VAR ${TYPE}ROOT)
    string(TOLOWER ${TYPE} type)
    if (NOT ${VAR} AND "${${VAR}}" STREQUAL "") # Allow False to disable
        file(GLOB INTEL_LOCS $ENV{${VAR}}
            "/opt/intel/${type}"
            "C:/Program Files (x86)/Intel/Composer XE 2015/${type}"
            "C:/Program Files (x86)/IntelSWTools/compilers_and_libraries/windows/${type}"
            "C:/Program Files/Intel/Composer XE 2015/${type}"
        )
        list(LENGTH INTEL_LOCS SUPPORTED)
        if (NOT SUPPORTED)
            if (ARGV1)
                set(ARGV1 FATAL_ERROR)
            endif()
            message(${ARGV1} "${TYPE} not found: Pass -D${VAR}= to cmake or set environment variable ${VAR}=")
        else()
            list(GET INTEL_LOCS 0 ${VAR})
            message(STATUS "${TYPE} found at ${${VAR}}")
        endif()
    endif()
    set(${VAR} "${${VAR}}" CACHE STRING "${TYPE} root directory.")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      if (APPLE)
        set(INTEL_ARCH "")
      else()
        set(INTEL_ARCH intel64/)
      endif()
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
      set(INTEL_ARCH ia32/)
    endif()
    set(INTEL_ARCH ${INTEL_ARCH} CACHE STRING "Subdirectroy below intel/${TYPE}/lib/")
endfunction()

find_intel_root(MKL $<$<NOT:APPLE>:REQUIRED>)
if(MKLROOT)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(MKL_INTEL_SUFFIX _lp64)
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
      if (WIN32)
        set(MKL_INTEL_SUFFIX _c)
      elseif(ANDROID)
        set(MKL_INTEL_SUFFIX "")
      endif()
    endif()
    target_include_directories(numerics PRIVATE SYSTEM "${MKLROOT}/include")
    if (ANDROID)
    target_link_libraries(numerics PRIVATE -Wl,--whole-archive)
    target_link_libraries(numerics PRIVATE "${MKLROOT}/lib/${INTEL_ARCH}${CMAKE_STATIC_LIBRARY_PREFIX}mkl_compat${CMAKE_STATIC_LIBRARY_SUFFIX}")
    target_link_libraries(numerics PRIVATE -Wl,--no-whole-archive)
    endif (ANDROID)

    if (CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_link_libraries(numerics PRIVATE -Wl,--start-group)
    endif ()
    target_link_libraries(numerics PRIVATE "${MKLROOT}/lib/${INTEL_ARCH}${CMAKE_STATIC_LIBRARY_PREFIX}mkl_intel${MKL_INTEL_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")
    target_link_libraries(numerics PRIVATE "${MKLROOT}/lib/${INTEL_ARCH}${CMAKE_STATIC_LIBRARY_PREFIX}mkl_core${CMAKE_STATIC_LIBRARY_SUFFIX}")
    target_link_libraries(numerics PRIVATE "${MKLROOT}/lib/${INTEL_ARCH}${CMAKE_STATIC_LIBRARY_PREFIX}mkl_sequential${CMAKE_STATIC_LIBRARY_SUFFIX}")
    if (CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_link_libraries(numerics PRIVATE -Wl,--end-group)
    endif ()
    if (NOT ANDROID AND (CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU OR CMAKE_CXX_COMPILER_ID STREQUAL Intel))
      target_link_libraries(numerics PRIVATE -ldl -lpthread)
    endif()
    target_compile_options(numerics PRIVATE -DHAVE_MKL)
elseif(APPLE)
    target_link_libraries(numerics PUBLIC "-framework Accelerate")
    message(STATUS "MKL not found, using Accelerate")
else()
    find_package(lapacke)
    if (TARGET lapacke)
        message(STATUS "MKL not found, using LAPACKe")
        target_link_libraries(numerics PRIVATE lapacke)
    else()
        message(FATAL_ERROR "Specify -DMKLROOT=, or 'apt-get install libblas-dev liblapack-dev liblapacke-dev'")
    endif()
endif()

add_library(filter STATIC
  filter/filter.cpp
  filter/state_vision.cpp
  filter/state_motion.cpp
  filter/observation.cpp
  filter/fast_detector/fast_9.cpp
  filter/scaled_mask.cpp
  filter/calibration_keys.h
  filter/calibration.h
  filter/calibration.cpp
  filter/calibration_json.cpp
  filter/sensor_fusion.cpp
  filter/capture.cpp
  filter/replay.cpp
  filter/feature_descriptor.h
  filter/feature_descriptor.cpp
  filter/mapper.h
  filter/mapper.cpp
  filter/dictionary.h
  filter/dictionary.cpp
  filter/tracker.h
  filter/fast_tracker.h
  filter/fast_tracker.cpp
)
target_include_directories(filter PUBLIC filter)
target_include_directories(filter PRIVATE rapidjson/include)
target_include_directories(filter PUBLIC ../../ThirdParty/spdlog/include)
target_compile_definitions(filter PUBLIC SPDLOG_NO_DATETIME SPDLOG_NO_THREAD_ID)
target_link_libraries(filter PRIVATE vlfeat)
target_link_libraries(filter PUBLIC numerics cor)
set_property(TARGET filter PROPERTY CXX_STANDARD 14)

find_intel_root(IPP)
if (IPPROOT)
    target_compile_definitions(filter PRIVATE HAVE_IPP)
    target_include_directories(filter PUBLIC SYSTEM "${IPPROOT}/include")
    target_link_libraries(filter PRIVATE
        "${IPPROOT}/lib/${INTEL_ARCH}${CMAKE_STATIC_LIBRARY_PREFIX}ippcv${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${IPPROOT}/lib/${INTEL_ARCH}${CMAKE_STATIC_LIBRARY_PREFIX}ippi${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${IPPROOT}/lib/${INTEL_ARCH}${CMAKE_STATIC_LIBRARY_PREFIX}ipps${CMAKE_STATIC_LIBRARY_SUFFIX}"
       #"${IPPROOT}/lib/${INTEL_ARCH}${CMAKE_STATIC_LIBRARY_PREFIX}ippvm${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${IPPROOT}/lib/${INTEL_ARCH}${CMAKE_STATIC_LIBRARY_PREFIX}ippcore${CMAKE_STATIC_LIBRARY_SUFFIX}"
    )
endif()

add_library(tracker SHARED filter/rc_tracker.cpp)
set_target_properties(tracker PROPERTIES CXX_STANDARD 14)
target_link_libraries(tracker filter)
target_include_directories(tracker PUBLIC tracker)
if (RC_VERSION)
  set(RC_VERSION ${RC_VERSION} CACHE STRING "RC Tracker Release Numeric Version")
  target_compile_definitions(tracker PRIVATE RC_VERSION=${RC_VERSION})
endif()
if (NOT RC_BUILD)
  execute_process(COMMAND git describe --tags --match intel_v\* HEAD
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
    OUTPUT_VARIABLE RC_BUILD RESULT_VARIABLE GIT_RETURN OUTPUT_STRIP_TRAILING_WHITESPACE)
  if (NOT GIT_RETURN EQUAL 0)
    message(FATAL_ERROR "install git OR pass -DRC_BUILD=<build-string>")
  endif()
endif()
target_compile_definitions(tracker PRIVATE RC_BUILD="${RC_BUILD}")
install(TARGETS tracker LIBRARY DESTINATION lib ARCHIVE DESTINATION lib RUNTIME DESTINATION bin COMPONENT tracker)
install(FILES filter/rc_tracker.h DESTINATION include COMPONENT tracker)

if(ENABLE_QR)
add_library(qr STATIC
  filter/homography.cpp
  filter/qr.cpp
)
target_include_directories(qr PUBLIC filter)
target_link_libraries(qr PRIVATE filter libzxing)
set_property(TARGET qr PROPERTY CXX_STANDARD 14)
endif()

if(ENABLE_VISGL)
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
target_link_libraries(vis PRIVATE glad glfw PUBLIC filter)
if(ENABLE_MGL)
    add_subdirectory(../../ThirdParty/mathgl mathgl)
    target_compile_options(mgl PRIVATE $<$<BOOL:${MSVC}>:/wd4244 /wd4190 /wd4800>)
    target_link_libraries(vis PRIVATE mgl)
    target_compile_definitions(vis PRIVATE HAVE_MGL)
endif()
target_link_libraries(vis PRIVATE ${GLFW_LIBRARIES})
target_include_directories(vis PUBLIC vis)
set_property(TARGET vis PROPERTY CXX_STANDARD 14)
endif()

find_package(Boost)
if(NOT Boost_FOUND)
    message(STATUS "Boost not found; stereo disabled")
    set(ENABLE_STEREO False)
endif()

if(ENABLE_STEREO)
    add_subdirectory(../../ThirdParty/qhull qhull)

    add_library(stereo STATIC
      stereo/stereo.cpp
      stereo/stereo_mesh.cpp
      stereo/camera.cpp
      stereo/fundamental.cpp
      stereo/stereo_features.cpp
    )
    target_link_libraries(stereo PUBLIC filter PRIVATE DAI qhull vlfeat)
    target_include_directories(stereo PUBLIC stereo)
    set_property(TARGET stereo PROPERTY CXX_STANDARD 14)
endif()

set(LIBVLFEAT_FILES
   vlfeat-0.9.18/vl/generic.c
   vlfeat-0.9.18/vl/host.c
   vlfeat-0.9.18/vl/gmm.c
   vlfeat-0.9.18/vl/mathop.c
   vlfeat-0.9.18/vl/random.c
   vlfeat-0.9.18/vl/vlad.c
   vlfeat-0.9.18/vl/kmeans.c
   vlfeat-0.9.18/vl/kdtree.c
   vlfeat-0.9.18/vl/liop.c
   vlfeat-0.9.18/vl/imopv.c
)
add_library(vlfeat STATIC ${LIBVLFEAT_FILES})
target_include_directories(vlfeat INTERFACE vlfeat-0.9.18/vl)
target_compile_options(vlfeat PRIVATE $<$<BOOL:${MSVC}>:/wd4244>)
set_target_properties(filter cor numerics vlfeat PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(ENABLE_STEREO)
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

if(ENABLE_QR)
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

if(CMAKE_BUILD_TYPE MATCHES "Debug" OR CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    target_compile_options(filter PRIVATE -DDEBUG=1)
    if(ENABLE_QR)
    target_compile_options(qr PRIVATE -DDEBUG=1)
    endif()
    target_compile_options(cor PRIVATE -DDEBUG=1)
    target_compile_options(numerics PRIVATE -DDEBUG=1)
    if(ENABLE_VISGL)
    target_compile_options(vis PRIVATE -DDEBUG=1)
    endif()
endif()
