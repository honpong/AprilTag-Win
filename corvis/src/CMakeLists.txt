file(GLOB TRACKERI_HEADERS tracker/*.h)
set(TRACKER_SOURCES
    tracker/sensor_fusion_queue.cpp
    tracker/rotation_vector.cpp
    tracker/matrix.cpp
    tracker/kalman.cpp
    tracker/transformation.cpp
    tracker/filter.cpp
    tracker/state_vision.cpp
    tracker/state_motion.cpp
    tracker/observation.cpp
    tracker/fast_detector/fast_9.cpp
    tracker/scaled_mask.cpp
    tracker/calibration.cpp
    tracker/calibration_json.cpp
    tracker/sensor_fusion.cpp
    tracker/capture.cpp
    tracker/replay.cpp
    tracker/feature_descriptor.cpp
    tracker/mapper.cpp
    tracker/dictionary.cpp
    tracker/fast_tracker.cpp
    tracker/remapper.cpp
    ${TRACKERI_HEADERS}
)
if(WIN32)
    set(TRACKER_SOURCES ${TRACKER_SOURCES}
      tracker/platform/win/sensor_clock.cpp
    )
else()
    set(TRACKER_SOURCES ${TRACKER_SOURCES}
      tracker/platform/shell/sensor_clock.cpp
    )
endif()
if(ENABLE_QR)
    set(TRACKER_SOURCES ${TRACKER_SOURCES}
        tracker/homography.cpp
        tracker/qr.cpp
    )
endif()

set_source_files_properties(tracker/remapper.cpp PROPERTIES COMPILE_DEFINITIONS_DEBUG _SCL_SECURE_NO_WARNINGS)

add_library(trackeri STATIC ${TRACKER_SOURCES})

add_library(tracker SHARED
    tracker/rc_tracker.h
    tracker/rc_tracker.cpp)
target_link_libraries(tracker PRIVATE trackeri)
target_include_directories(tracker PUBLIC tracker)
set_property(TARGET tracker trackeri PROPERTY CXX_STANDARD 14)

target_include_directories(trackeri PUBLIC tracker)

find_package(Threads REQUIRED)
target_link_libraries(trackeri PUBLIC Threads::Threads)

target_include_directories(trackeri SYSTEM PUBLIC
    ../../ThirdParty/rapidjson/include
    ../../ThirdParty/spdlog/include
    ../../ThirdParty/eigen
)
target_compile_definitions(trackeri PUBLIC SPDLOG_NO_DATETIME SPDLOG_NO_THREAD_ID)

target_link_libraries(trackeri PRIVATE vlfeat)

function(find_intel_root TYPE) # [REQUIRED]
    set(VAR ${TYPE}ROOT)
    string(TOLOWER ${TYPE} type)
    if (NOT ${VAR} AND "${${VAR}}" STREQUAL "") # Allow False to disable
        file(GLOB INTEL_LOCS $ENV{${VAR}}
            "/opt/intel/${type}"
            "C:/Program Files (x86)/IntelSWTools/compilers_and_libraries/windows/${type}"
        )
        list(LENGTH INTEL_LOCS SUPPORTED)
        if (NOT SUPPORTED)
            # ARGV1 is option argument REQUIRED
            # If ARGV1 is not set message() will be of default type
            # "Important information" intead of a fatal error.
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

find_intel_root(MKL REQUIRED)
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
    target_include_directories(trackeri SYSTEM PUBLIC "${MKLROOT}/include")
    if (ANDROID)
    target_link_libraries(trackeri PRIVATE -Wl,--whole-archive)
    target_link_libraries(trackeri PRIVATE "${MKLROOT}/lib/${INTEL_ARCH}${CMAKE_STATIC_LIBRARY_PREFIX}mkl_compat${CMAKE_STATIC_LIBRARY_SUFFIX}")
    target_link_libraries(trackeri PRIVATE -Wl,--no-whole-archive)
    endif (ANDROID)

    if (CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_link_libraries(trackeri PRIVATE -Wl,--start-group)
    endif ()
    target_link_libraries(trackeri PRIVATE "${MKLROOT}/lib/${INTEL_ARCH}${CMAKE_STATIC_LIBRARY_PREFIX}mkl_intel${MKL_INTEL_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")
    target_link_libraries(trackeri PRIVATE "${MKLROOT}/lib/${INTEL_ARCH}${CMAKE_STATIC_LIBRARY_PREFIX}mkl_core${CMAKE_STATIC_LIBRARY_SUFFIX}")
    target_link_libraries(trackeri PRIVATE "${MKLROOT}/lib/${INTEL_ARCH}${CMAKE_STATIC_LIBRARY_PREFIX}mkl_sequential${CMAKE_STATIC_LIBRARY_SUFFIX}")
    if (CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_link_libraries(trackeri PRIVATE -Wl,--end-group)
    endif ()
    if (NOT ANDROID AND (CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU OR CMAKE_CXX_COMPILER_ID STREQUAL Intel))
      target_link_libraries(trackeri PRIVATE -ldl -lpthread)
    endif()
    target_compile_options(trackeri PUBLIC -DEIGEN_USE_MKL_ALL)
else()
    find_package(lapacke)
    if(TARGET lapacke)
        message(STATUS "MKL not found, using LAPACKe")
        target_link_libraries(trackeri PRIVATE lapacke)
        target_compile_options(trackeri PUBLIC -DEIGEN_USE_LAPACKE_STRICT -DEIGEN_USE_BLAS)
    else()
        message(WARNING "MKL and LAPACKe not found, using pure Eigen.")
        message(STATUS "If you want MKL, specify -DMKLROOT=")
        message(STATUS "If you want LAPACKe on linux, try 'apt-get install libblas-dev liblapack-dev liblapacke-dev'")
    endif()
endif()

find_intel_root(IPP)
if (IPPROOT)
    target_compile_definitions(trackeri PRIVATE HAVE_IPP)
    target_include_directories(trackeri SYSTEM PUBLIC "${IPPROOT}/include")
    target_link_libraries(trackeri PRIVATE
        "${IPPROOT}/lib/${INTEL_ARCH}${CMAKE_STATIC_LIBRARY_PREFIX}ippcv${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${IPPROOT}/lib/${INTEL_ARCH}${CMAKE_STATIC_LIBRARY_PREFIX}ippi${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${IPPROOT}/lib/${INTEL_ARCH}${CMAKE_STATIC_LIBRARY_PREFIX}ipps${CMAKE_STATIC_LIBRARY_SUFFIX}"
       #"${IPPROOT}/lib/${INTEL_ARCH}${CMAKE_STATIC_LIBRARY_PREFIX}ippvm${CMAKE_STATIC_LIBRARY_SUFFIX}"
        "${IPPROOT}/lib/${INTEL_ARCH}${CMAKE_STATIC_LIBRARY_PREFIX}ippcore${CMAKE_STATIC_LIBRARY_SUFFIX}"
    )
endif()

if (RC_VERSION)
  set(RC_VERSION ${RC_VERSION} CACHE STRING "RC tracker Release Numeric Version")
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
install(FILES tracker/rc_tracker.h DESTINATION include COMPONENT tracker)

if(ENABLE_QR)
    file(GLOB_RECURSE LIBZXING_FILES
        "../../ThirdParty/libzxing-cpp/zxing/*.cpp"
        "../../ThirdParty/libzxing-cpp/bigint/*.cpp"
    )
    add_library(libzxing STATIC ${LIBZXING_FILES})
    target_compile_definitions(libzxing PRIVATE NO_ICONV)
    target_include_directories(libzxing
      PUBLIC ../../ThirdParty/libzxing-cpp
    )

    target_link_libraries(trackeri PRIVATE libzxing)
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

    target_link_libraries(stereo PUBLIC trackeri PRIVATE DAI qhull vlfeat)
    target_include_directories(stereo PUBLIC stereo)
    set_property(TARGET stereo PROPERTY CXX_STANDARD 14)
endif()

set(LIBVLFEAT_FILES
   ../../ThirdParty/vlfeat-0.9.18/vl/generic.c
   ../../ThirdParty/vlfeat-0.9.18/vl/host.c
   ../../ThirdParty/vlfeat-0.9.18/vl/gmm.c
   ../../ThirdParty/vlfeat-0.9.18/vl/mathop.c
   ../../ThirdParty/vlfeat-0.9.18/vl/random.c
   ../../ThirdParty/vlfeat-0.9.18/vl/vlad.c
   ../../ThirdParty/vlfeat-0.9.18/vl/kmeans.c
   ../../ThirdParty/vlfeat-0.9.18/vl/kdtree.c
   ../../ThirdParty/vlfeat-0.9.18/vl/liop.c
   ../../ThirdParty/vlfeat-0.9.18/vl/imopv.c
   $<$<BOOL:${ENABLE_STEREO}>:
   ../../ThirdParty/vlfeat-0.9.18/vl/sift.c
   >
)
add_library(vlfeat STATIC ${LIBVLFEAT_FILES})
target_include_directories(vlfeat INTERFACE ../../ThirdParty/vlfeat-0.9.18/vl)
target_compile_options(vlfeat PRIVATE $<$<BOOL:${MSVC}>:/wd4244>)
set_target_properties(trackeri vlfeat PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(ENABLE_STEREO)
    foreach(f alldai.cpp graph.cpp regiongraph.cpp emalg.cpp bipgraph.cpp evidence.cpp io.cpp trwbp.cpp bp.cpp exactinf.cpp daialg.cpp properties.cpp util.cpp bp_dual.cpp exceptions.cpp varset.cpp factor.cpp weightedgraph.cpp clustergraph.cpp factorgraph.cpp dag.cpp)
      list(APPEND DAI_FILES "../../ThirdParty/libDAI-0.3.1/src/${f}")
    endforeach(f)
    add_library(DAI STATIC ${DAI_FILES})
    target_include_directories(DAI PUBLIC
      ../../ThirdParty/libDAI-0.3.1/include
      "${Boost_INCLUDE_DIRS}"
    )
    # -DMACOSX forces libDAI to include boost using <boost/tr1/*>
    target_compile_options(DAI PUBLIC -DMACOSX -DDAI_WITH_BP -DDAI_WITH_TRWBP)
endif() # WIN32

if(CMAKE_BUILD_TYPE MATCHES "Debug" OR CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    target_compile_options(trackeri PRIVATE -DDEBUG=1)
endif()
