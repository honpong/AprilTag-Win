#pragma once

#include <memory>

#include "packet.h"
#include "usb_definitions.h"

typedef std::unique_ptr<packet_t, decltype(&free)> rc_packet_t;

bool usb_init();
void usb_shutdown();
bool usb_send_control(uint8_t control_message, uint16_t value, uint16_t index,
                      unsigned int timeout);
bool usb_write_packet(const rc_packet_t & packet);
rc_packet_t usb_read_packet();

bool usb_read_interrupt_packet(rc_packet_t &fixed_pkt);
