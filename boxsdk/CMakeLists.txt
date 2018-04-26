cmake_minimum_required(VERSION 3.7.0 FATAL_ERROR)

function(assign_version_property VER_COMPONENT)
    file(STRINGS "./inc/rs_box_sdk.hpp" BOX_SDK_VERSION_${VER_COMPONENT} REGEX "#define RS2_BOX_SDK_${VER_COMPONENT}_VERSION")
    separate_arguments(BOX_SDK_VERSION_${VER_COMPONENT})
    list(GET BOX_SDK_VERSION_${VER_COMPONENT} -1 tmp)
    if (tmp LESS 0)
        message( FATAL_ERROR "Could not obtain valid box-sdk version ${VER_COMPONENT} component - actual value is ${tmp}" )
    endif()
    set(BOX_SDK_VERSION_${VER_COMPONENT} ${tmp} PARENT_SCOPE)
endfunction()

set(BOX_SDK_VERSION_MAJOR -1)
set(BOX_SDK_VERSION_MINOR -1)
set(BOX_SDK_VERSION_PATCH -1)
assign_version_property(MAJOR)
assign_version_property(MINOR)
assign_version_property(PATCH)
set(BOX_SDK_VERSION_STRING ${BOX_SDK_VERSION_MAJOR}.${BOX_SDK_VERSION_MINOR}.${BOX_SDK_VERSION_PATCH})
infoValue(BOX_SDK_VERSION_STRING)

set(BUILD_SHARED_LIB_ALGO OFF CACHE BOOL "" FORCE)
option(BUILD_SHARED_LIB_BOXSDK "Build Box SDK SHARED LIBRARY" ON)
if(BUILD_SHARED_LIB_BOXSDK)
    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
    add_library(box-sdk SHARED
        inc/rs_box_sdk.hpp
        rs_box_api.h
        rs_box_image.hpp
        rs_box_sdk.cpp
		rs_icon.cpp
        rs_icon.h
        rs_sf_pose_tracker.h
        rs_sf_pose_tracker.cpp)
else()
    add_library(box-sdk STATIC
        inc/rs_box_sdk.hpp
        rs_box_api.h
        rs_box_image.hpp
        rs_box_sdk.cpp
		rs_icon.cpp
        rs_icon.h
        rs_sf_pose_tracker.h
        rs_sf_pose_tracker.cpp)
endif()

find_package(libSP QUIET)
if(libSP_FOUND)
    add_definitions(-DlibSP_FOUND)
    set(libSP libSP)
endif()

target_include_directories(box-sdk 
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
            $<INSTALL_INTERFACE:include>
    PRIVATE ${LIBRS_LIBS} ${libSP})
    
target_link_libraries(box-sdk 
    PUBLIC  ${LIBRS_LIBS} ${libSP}
    PRIVATE ${ALGO_LIB})

add_executable(box-app
    rs-box-app.cpp
    rs-box-app.hpp
	rs_icon.cpp
    rs_icon.h)

target_link_libraries(box-app
    PRIVATE box-sdk ${LIBRS_LIBS} ${LIBRS_APP_LIBS})

set_property(TARGET box-sdk PROPERTY CXX_STANDARD 14)
set_property(TARGET box-app PROPERTY CXX_STANDARD 14)

if (BUILD_SHARED_LIB_BOXSDK)
    install(TARGETS box-sdk
        EXPORT box-sdkTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

    include(CMakePackageConfigHelpers)
    configure_package_config_file(../cmake/box-sdkConfig.cmake.in box-sdkConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/realsense2
        INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}
        PATH_VARS CMAKE_INSTALL_BINDIR CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_INCLUDEDIR)

    install(TARGETS box-app RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(DIRECTORY inc/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(EXPORT box-sdkTargets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/realsense2)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/box-sdkConfig.cmake" DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/realsense2)
    install(FILES ../cmake/my-box-appCMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/example RENAME CMakeLists.txt)
    install(FILES my-box-app.cpp DESTINATION ${CMAKE_INSTALL_PREFIX}/example)
    install(DIRECTORY ../doc DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()

if(MSVC)
    if(BUILD_SHARED_LIBS)
        list(APPEND BOX_SDK_DEPENDENCY "$<TARGET_FILE:realsense2>" "$<TARGET_LINKER_FILE:realsense2>")
    endif()
    if(libSP_FOUND)
        list(APPEND BOX_SDK_DEPENDENCY "$<TARGET_FILE:libSP>")
        install(FILES "$<TARGET_FILE:libSP>" DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
    if(BUILD_SHARED_LIB_BOXSDK)
        list(APPEND BOX_SDK_DEPENDENCY "$<TARGET_FILE:box-sdk>" "$<TARGET_LINKER_FILE:box-sdk>")
    endif()

    foreach(var IN LISTS BOX_SDK_DEPENDENCY)
        if(BUILD_SHARED_LIB_BOXSDK)
            add_custom_command(TARGET box-sdk POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${var} "$<TARGET_FILE_DIR:box-sdk>")
        else()
            add_custom_command(TARGET box-app POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${var} "$<TARGET_FILE_DIR:box-app>")
        endif()
    endforeach(var)
    set_target_properties(box-app PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(TargetDir) ")
endif()
