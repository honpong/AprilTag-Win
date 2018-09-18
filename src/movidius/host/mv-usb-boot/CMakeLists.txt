cmake_minimum_required(VERSION 3.2.2)

project(mv-usb-boot C)

add_library(mv_libusb
  crc32.c
  crc32.h
  dumphex.h
  dumphex.c
  rom_checksum.c
  rom_checksum.h
  usb_boot.c
  usb_common.h
  usb_libusb.c
  usb_boot.c
  usb_boot.h
)
set_target_properties(mv_libusb PROPERTIES C_STANDARD 99)
target_link_libraries(mv_libusb PUBLIC usb)
target_include_directories(mv_libusb PUBLIC .)

add_executable(${PROJECT_NAME}
  main.c
)
set_target_properties(${PROJECT_NAME} PROPERTIES C_STANDARD 99)
target_link_libraries(${PROJECT_NAME} mv_libusb)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
