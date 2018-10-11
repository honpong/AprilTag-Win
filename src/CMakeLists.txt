if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU|Intel")
   add_compile_options(-Wall -Wextra)
   add_compile_options(-Wunreachable-code  -Wunused-macros)
   if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
      add_compile_options(-Wdeprecated -Wswitch-enum)
   endif()
   #add_compile_options(-Wweak-vtables -Wcast-align -Wextra-semi -Wdouble-promotion) # noisy, but should be considered in the future
   add_compile_options(-Wno-missing-field-initializers -Wno-unused-parameter) # TODO: components of -Wextra that are not clean yet
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang") # not supported by gcc
   add_compile_options(-Wconditional-uninitialized -Wcovered-switch-default -Wunreachable-code-break -Wmissing-variable-declarations)
   if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
      add_compile_options(-Wglobal-constructors)
   endif()
endif()

file(GLOB TRACKERI_HEADERS tracker/*.h)
set(TRACKER_SOURCES
    tracker/sensor_fusion_queue.cpp
    tracker/rotation_vector.cpp
    tracker/matrix.cpp
    tracker/transformation_cov.cpp
    tracker/estimate_horn.cpp
    tracker/estimate_epnp.cpp
    tracker/estimate_fundamental.cpp
    tracker/estimate_3d_point.cpp
    tracker/filter.cpp
    tracker/state_vision.cpp
    tracker/state_motion.cpp
    tracker/observation.cpp
    tracker/calibration.cpp
    tracker/calibration_json.cpp
    tracker/sensor_fusion.cpp
    tracker/capture.cpp
    tracker/replay.cpp
    tracker/mapper.cpp
    tracker/remapper.cpp
    tracker/bstream.cpp
    tracker/map_loader.cpp
    tracker/file_stream.cpp
    tracker/replay_device.cpp
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
set_source_files_properties(tracker/state_vision.cpp PROPERTIES COMPILE_DEFINITIONS _ENABLE_EXTENDED_ALIGNED_STORAGE)

add_library(trackeri STATIC ${TRACKER_SOURCES})

add_library(tracker SHARED
    tracker/rc_tracker.h
    tracker/calibration_tm2.cpp
    tracker/rc_tracker.cpp)
target_link_libraries(tracker PRIVATE trackeri)
target_include_directories(tracker PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/tracker> $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)
set_property(TARGET tracker trackeri PROPERTY CXX_STANDARD 14)

target_include_directories(trackeri PUBLIC tracker)

find_package(Threads REQUIRED)
find_package(Eigen3 REQUIRED)
target_link_libraries(trackeri PUBLIC Threads::Threads Eigen3::Eigen)

target_include_directories(trackeri SYSTEM PUBLIC
    ../ThirdParty/rapidjson/include
    ../ThirdParty/spdlog/include
)
target_compile_definitions(trackeri PUBLIC SPDLOG_NO_DATETIME SPDLOG_NO_THREAD_ID)

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
    message(STATUS "MKL not found; specify -DMKLROOT=")

    find_package(lapacke QUIET)
    find_package(blis QUIET)
    if (APPLE)
        find_library(ACCELERATE Accelerate)
    endif()

    if (TARGET blas)
        target_link_libraries(trackeri PRIVATE blas)
        target_compile_options(trackeri PUBLIC -DEIGEN_USE_BLAS)
    elseif(ACCELERATE)
        message(STATUS "Using Apple's Accelerate for BLAS.  Use -DACCELERATE= to disable.")
        target_compile_options(trackeri PUBLIC -DEIGEN_USE_BLAS)
        target_link_libraries(trackeri PUBLIC ${ACCELERATE})
      elseif(TARGET blis)
        message(STATUS "Using BLIS for BLAS.")
        target_link_libraries(trackeri PRIVATE blis)
        target_compile_options(trackeri PUBLIC -DHAVE_BLIS)
    else()
        message(WARNING "Neither MKL, BLIS nor Accelerate not found, using pure Eigen for BLAS.")
        if (LINUX)
            message(STATUS "If you want BLAS on linux, try 'apt-get install libblas-dev'")
        endif()
    endif()

    if(TARGET lapacke)
        message(STATUS "MKL not found, using LAPACKe")
        target_link_libraries(trackeri PRIVATE lapacke)
        target_compile_options(trackeri PUBLIC -DEIGEN_USE_LAPACKE_STRICT)
    else()
      message(WARNING "Neither MKL nor LAPACKe found, using pure Eigen for LAPACK.")
      if (LINUX)
          message(STATUS "Try apt-get install liblapack-dev liblapacke-dev.")
      endif()
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
    message(FATAL_ERROR "install git AND run `git fetch --tags` OR pass -DRC_BUILD=<build-string>")
  endif()
endif()
target_compile_definitions(tracker PRIVATE RC_BUILD="${RC_BUILD}")
install(TARGETS tracker EXPORT tracker LIBRARY DESTINATION lib ARCHIVE DESTINATION lib COMPONENT tracker RUNTIME DESTINATION bin COMPONENT tracker INCLUDES DESTINATION include)
install(FILES tracker/rc_tracker.h DESTINATION include COMPONENT tracker)
install(EXPORT tracker NAMESPACE DESTINATION cmake EXPORT_LINK_INTERFACE_LIBRARIES FILE trackerConfig.cmake)

if(ENABLE_QR)
    file(GLOB_RECURSE LIBZXING_FILES
        "../ThirdParty/libzxing-cpp/zxing/*.cpp"
        "../ThirdParty/libzxing-cpp/bigint/*.cpp"
    )
    add_library(libzxing STATIC ${LIBZXING_FILES})
    target_compile_definitions(libzxing PRIVATE NO_ICONV)
    target_include_directories(libzxing
      PUBLIC ../ThirdParty/libzxing-cpp
    )

    target_link_libraries(trackeri PRIVATE libzxing)
endif()

find_package(Boost)
if(NOT Boost_FOUND)
    message(STATUS "Boost not found; stereo disabled")
    set(ENABLE_STEREO False)
endif()

if(ENABLE_STEREO)
    add_subdirectory(../ThirdParty/qhull qhull)

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
set_target_properties(trackeri PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(ENABLE_STEREO)
    foreach(f alldai.cpp graph.cpp regiongraph.cpp emalg.cpp bipgraph.cpp evidence.cpp io.cpp trwbp.cpp bp.cpp exactinf.cpp daialg.cpp properties.cpp util.cpp bp_dual.cpp exceptions.cpp varset.cpp factor.cpp weightedgraph.cpp clustergraph.cpp factorgraph.cpp dag.cpp)
      list(APPEND DAI_FILES "../ThirdParty/libDAI-0.3.1/src/${f}")
    endforeach(f)
    add_library(DAI STATIC ${DAI_FILES})
    target_include_directories(DAI PUBLIC
      ../ThirdParty/libDAI-0.3.1/include
      "${Boost_INCLUDE_DIRS}"
    )
    # -DMACOSX forces libDAI to include boost using <boost/tr1/*>
    target_compile_options(DAI PUBLIC -DMACOSX -DDAI_WITH_BP -DDAI_WITH_TRWBP)
endif() # WIN32

if(CMAKE_BUILD_TYPE MATCHES "Debug|RelWithDebInfo")
    target_compile_options(trackeri PRIVATE -DDEBUG=1)
    target_compile_options(tracker PRIVATE -DDEBUG=1)
endif()

find_package(OpenCV QUIET)
if(ENABLE_RELOCALIZATION_DEBUG AND TARGET opencv_core
   AND TARGET opencv_highgui AND TARGET opencv_imgcodecs AND TARGET opencv_imgproc)
    target_sources(trackeri PRIVATE tracker/debug/visual_debug.cpp tracker/debug/visual_debug.h)
    target_compile_options(trackeri PRIVATE -DRELOCALIZATION_DEBUG)
    target_link_libraries(trackeri PRIVATE opencv_core opencv_highgui opencv_imgcodecs opencv_imgproc)
endif()

add_subdirectory(feature feature)
target_link_libraries(trackeri PUBLIC feature)
add_subdirectory(vocabulary vocabulary)
target_link_libraries(trackeri PUBLIC vocabulary)

set(DBOW2_FILES
    ../ThirdParty/DBoW2/include/DBoW2/TemplatedVocabulary.h
#   ../ThirdParty/DBoW2/src/train.cpp
)
add_library(dbow2 INTERFACE)
target_include_directories(dbow2 INTERFACE
    ../ThirdParty/DBoW2/include
)
target_link_libraries(trackeri PUBLIC dbow2)

add_subdirectory(brainstem brainstem)
