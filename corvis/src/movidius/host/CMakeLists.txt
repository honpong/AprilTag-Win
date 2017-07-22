cmake_minimum_required(VERSION 3.2.2)
project( slam_client )

add_library(slam_usb
   usb_host.cpp
   usb_host.h
)

find_package(Threads REQUIRED)

find_library(LIBUSB_LIB usb-1.0)
find_path(LIBUSB_INC libusb-1.0/libusb.h)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(usb DEFAULT_MSG LIBUSB_LIB LIBUSB_INC)
if (USB_FOUND AND NOT TARGET usb)
  add_library(usb INTERFACE)
  target_include_directories(usb INTERFACE ${LIBUSB_INC})
  target_link_libraries(usb INTERFACE ${LIBUSB_LIB})
endif()

if (NOT TARGET usb)
  message(STATUS "libusb not found; slam_usb and slam_client will not be built. `brew install libusb` or `apt-get install libusb-1.0-0-dev`.")
  return()
endif()

target_link_libraries(slam_usb PUBLIC ${LIBUSB_1_LIBRARIES} usb Threads::Threads)

target_include_directories(slam_usb PUBLIC
    ../common/                                  # for usb_definitions.h & get_pose.h
    ../../tracker/ # for packet.h
)
set_property(TARGET slam_usb PROPERTY CXX_STANDARD 14)

add_executable( slam_client
    slam_client.cpp
)
set_property(TARGET slam_client PROPERTY CXX_STANDARD 14)

target_link_libraries( slam_client slam_usb ${OpenCV_LIBS})
