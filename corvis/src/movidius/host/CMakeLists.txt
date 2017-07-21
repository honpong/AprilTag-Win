cmake_minimum_required(VERSION 3.2.2)
project( slam_client )

add_library(slam_usb
   usb_host.cpp
   usb_host.h
)

target_link_libraries(slam_usb PUBLIC ${LIBUSB_1_LIBRARIES} -lusb-1.0
    -lpthread)
if(UNIX AND NOT APPLE)
    target_link_libraries(slam_usb PUBLIC -lrt)
endif(UNIX AND NOT APPLE)

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
