cmake_minimum_required(VERSION 3.2.2)
project( slam_movidius_host )

find_package(Threads REQUIRED)
find_package(Eigen3 REQUIRED)

find_path(GETOPT_INC getopt.h)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(getopt "getopt.h not found" GETOPT_INC)
if (GETOPT_FOUND)
  add_library(getopt INTERFACE)
  target_include_directories(getopt INTERFACE ${GETOPT_INC})
endif()

if (NOT TARGET getopt)
  message(STATUS "slam_usb will not be built; requires getopt")
  return()
endif()

find_library(LIBUSB_LIB usb-1.0)
find_path(LIBUSB_INC libusb-1.0/libusb.h)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(usb "libusb not found; `brew install libusb` or `apt-get install libusb-1.0-0-dev`." LIBUSB_LIB LIBUSB_INC)
if (USB_FOUND AND NOT TARGET usb)
  add_library(usb INTERFACE)
  target_include_directories(usb INTERFACE ${LIBUSB_INC})
  target_link_libraries(usb INTERFACE ${LIBUSB_LIB})
endif()

if (NOT TARGET usb)
  message(STATUS "slam_usb will not be built; requires libusb")
  return()
endif()

add_subdirectory(mv-usb-boot)

add_library(slam_usb
   usb_host.cpp
   usb_host.h
   tm2_host_stream.cpp
   ../../tracker/replay.cpp
   ../../tracker/bstream.cpp
)

target_link_libraries(slam_usb PUBLIC usb getopt Threads::Threads Eigen3::Eigen mv_libusb)

target_include_directories(slam_usb PUBLIC
    ../common/                                  # for usb_definitions.h & get_pose.h
    ../../tracker/ # for packet.h
    .
)
set_property(TARGET slam_usb PROPERTY CXX_STANDARD 14)

add_executable(tm2_reset tm2_reset.cpp)
target_link_libraries(tm2_reset slam_usb)
set_property(TARGET tm2_reset PROPERTY CXX_STANDARD 14)
