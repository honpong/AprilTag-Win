#pragma once

#include "packet.h"
#include "usb_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

void usb_init();
void usb_shutdown(int _unused);
bool usb_send_control(uint8_t control_message, uint16_t value, uint16_t index,
                      unsigned int timeout);
void usb_write_packet(packet_t * packet);
packet_t * usb_read_packet();  // Note, you need to free() the returned packet

packet_rc_pose_t usb_read_6dof();

#ifdef __cplusplus
}
#endif
