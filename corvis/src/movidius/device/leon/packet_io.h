#pragma once

#include "packet.h"

#include "tm2_outcall.h"
#include "usb_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

bool packet_io_write(packet_t * packet);
bool packet_io_read(packet_t ** packet);
void packet_io_free(packet_t * packet);
bool packet_io_unblock_read();

#ifdef __cplusplus
}
#endif
