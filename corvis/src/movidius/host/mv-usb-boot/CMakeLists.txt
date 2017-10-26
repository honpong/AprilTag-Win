cmake_minimum_required(VERSION 3.2.2)

project(mv-usb-boot C)

add_executable(${PROJECT_NAME}
  crc32.c
  crc32.h
  dumphex.h
  dumphex.c
  rom_checksum.c
  rom_checksum.h
  usb_boot.c
  usb_common.h
  usb_libusb.c
)
set_target_properties(${PROJECT_NAME} PROPERTIES C_STANDARD 99)
target_link_libraries(${PROJECT_NAME} usb)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
