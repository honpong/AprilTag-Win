// Common USB API

#ifndef _USB_COMMON_H
#define _USB_COMMON_H

#include <stdint.h>
#include <stdio.h>

#ifndef _USB_SOURCE
typedef void *usb_dev;
typedef void *usb_han;
#endif

#define USB_DIR_OUT		0
#define USB_DIR_IN		1

#define USB_DEV_NONE	NULL
#define USB_HAN_NONE	NULL

#define USB_ERR_NONE		0
#define USB_ERR_TIMEOUT		-1
#define USB_ERR_FAILED		-2
#define USB_ERR_INVALID		-3

extern int usb_init(void);
extern void usb_shutdown(void);
extern int usb_can_find_by_guid(void);
extern void usb_list_devices(uint16_t vid, uint16_t pid);
extern usb_dev usb_find_device(uint16_t vid, uint16_t pid, const char *addr, int loud);
extern usb_dev usb_find_device_by_guid(int loud);
extern int usb_check_connected(usb_dev dev);		// TODO: actually use this
extern usb_han usb_open_device(usb_dev dev, char *err_string_buff);
extern uint8_t usb_get_bulk_endpoint(usb_han han, int dir);
extern size_t usb_get_endpoint_size(usb_han han, uint8_t ep);
extern int usb_bulk_write(usb_han han, uint8_t ep, const void *buffer, size_t sz, size_t *wrote_bytes, int timeout_ms);
extern int usb_bulk_read(usb_han han, uint8_t ep, void *buffer, size_t sz, size_t *read_bytes, int timeout_ms);
extern void usb_free_device(usb_dev dev);
extern void usb_close_device(usb_han han);

extern const char *usb_last_bulk_errmsg(void);
extern void usb_set_msgfile(FILE *file);
extern void usb_set_verbose(int value);
extern void usb_set_ignoreerrors(int value);
extern int usb_boot(int argc, char *argv[]);

#endif//_USB_COMMON_H
