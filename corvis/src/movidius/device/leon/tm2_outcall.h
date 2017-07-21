#pragma once

#include "usbpump_vsc2app.h"

#ifdef __cplusplus
extern "C" {
#endif

int  usb_init();
bool usb_blocking_read(uint32_t endpoint, uint8_t * buffer, uint32_t size);
bool usb_blocking_write(uint32_t endpoint, uint8_t * buffer, uint32_t size);
void usb_nonblocking_write(uint32_t endpoint, uint8_t * buffer, uint32_t size,
                           void (*callback)(void * handle));

#ifdef __cplusplus
}
#endif
