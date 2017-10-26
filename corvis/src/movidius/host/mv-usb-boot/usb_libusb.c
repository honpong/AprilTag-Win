
#if (defined(WIN32) || defined(_WIN64)) && !defined(__CYGWIN32__)
#pragma comment(lib, "libusb-1.0.lib")
#undef interface
#define _CRT_SECURE_NO_WARNINGS
#define __objidl_h__
#include <wtypes.h>
#endif
#include <stdint.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include <errno.h>
#include <string.h>

typedef libusb_device *usb_dev;

struct ep_info {
	uint8_t ep;
	size_t sz;
};

struct _usb_han {
	libusb_device_handle *han;
	struct ep_info eps[2];
};
typedef struct _usb_han *usb_han;

#define _USB_SOURCE
#include "usb_common.h"

static FILE *msgfile = NULL;
static int verbose = 0, ignore_errors = 0;
static int last_bulk_result = 0;

int usb_init(void) {
	return libusb_init(NULL);
}

void usb_shutdown(void) {
	libusb_exit(NULL);
}

int usb_can_find_by_guid(void) {
	return 0;
}

static const char *gen_addr(libusb_device *dev) {
	static char buff[4 * 7];		// '255-' x 7 (also gives us nul-terminator for last entry)
	uint8_t pnums[7];
	int pnum_cnt, i;
	char *p;

	pnum_cnt = libusb_get_port_numbers(dev, pnums, 7);
	if (pnum_cnt == LIBUSB_ERROR_OVERFLOW) {
		// shouldn't happen!
		strcpy(buff, "<error>");
		return buff;
	}
	p = buff;
	for (i = 0; i<pnum_cnt - 1; i++)
		p += sprintf(p, "%u-", pnums[i]);
	sprintf(p, "%u", pnums[i]);
	return buff;
}

void usb_list_devices(uint16_t vid, uint16_t pid) {
	libusb_device **devs;
	libusb_device *dev;
	struct libusb_device_descriptor desc;
	size_t i;
	int res;
	if((res = libusb_get_device_list(NULL, &devs)) < 0) {
		fprintf(stderr, "Unable to get USB device list: %s\n", libusb_strerror(res));
		return;
	}
	i = 0;
	while((dev = devs[i++]) != NULL) {
		if((res = libusb_get_device_descriptor(dev, &desc)) < 0) {
			fprintf(stderr, "Unable to get USB device descriptor: %s\n", libusb_strerror(res));
			continue;
		}
		if((desc.idVendor == vid) && (desc.idProduct == pid)) {
			printf("Address: %-7s - VID/PID %04x:%04x\n", gen_addr(dev), desc.idVendor, desc.idProduct);
		}
	}
	libusb_free_device_list(devs, 1);
}

usb_dev usb_find_device(uint16_t vid, uint16_t pid, const char *addr, int loud) {
	libusb_device **devs;
	libusb_device *dev, *resdev = NULL;
	struct libusb_device_descriptor desc;
	size_t i;
	int res;
	const char *caddr;
	if((res = libusb_get_device_list(NULL, &devs)) < 0) {
		fprintf(stderr, "Unable to get USB device list: %s\n", libusb_strerror(res));
		return NULL;
	}
	i = 0;
	while((dev = devs[i++]) != NULL) {
		if((res = libusb_get_device_descriptor(dev, &desc)) < 0) {
			fprintf(stderr, "Unable to get USB device descriptor: %s\n", libusb_strerror(res));
			if(!ignore_errors)
				break;
		}
		if(verbose && loud)
			fprintf(msgfile, "Vendor/Product ID: %04x:%04x\n", desc.idVendor, desc.idProduct);
		if((resdev == NULL) && (desc.idVendor == vid) && (desc.idProduct == pid)) {
			caddr = gen_addr(dev);
			if((addr == NULL) || !strcmp(caddr, addr)) {
				if(verbose)
					fprintf(msgfile, "Found device with VID/PID %04x:%04x , address %s\n", vid, pid, caddr);
				resdev = dev;
				if(!(verbose && loud))
					break;
			}
		}
	}
	if(resdev != NULL)
		libusb_ref_device(resdev);
	libusb_free_device_list(devs, 1);
	return resdev;
}

usb_dev usb_find_device_by_guid(int loud) {
	return USB_DEV_NONE;
}

int usb_check_connected(usb_dev dev) {
	libusb_device **devs;
	libusb_device *fdev;
	int i, found = 0;
	if(libusb_get_device_list(NULL, &devs) < 0)
		return 0;
	i = 0;
	while((fdev = devs[i++]) != NULL) {
		if(fdev == dev) {
			found = 1;
			break;
		}
	}
	libusb_free_device_list(devs, 1);
	return found;
}

usb_han usb_open_device(usb_dev dev, char *err_string_buff) {
	struct libusb_config_descriptor *cdesc;
	const struct libusb_interface_descriptor *ifdesc;
	libusb_device_handle *han = NULL;
	int res;
	usb_han uhan = USB_HAN_NONE;
	int i;

	if((res = libusb_open(dev, &han)) < 0) {
		sprintf(err_string_buff, "cannot open device: %s\n", libusb_strerror(res));
		goto exit_err;
	}
	if((res = libusb_set_configuration(han, 1)) < 0) {
		sprintf(err_string_buff, "setting config 1 failed: %s\n", libusb_strerror(res));
		goto exit_err;
	}

	if((res = libusb_claim_interface(han, 0)) < 0) {
		sprintf(err_string_buff, "claiming interface 0 failed: %s\n", libusb_strerror(res));
		goto exit_err;
	}

	if((res = libusb_get_config_descriptor(dev, 0, &cdesc)) < 0) {
		sprintf(err_string_buff, "Unable to get USB config descriptor: %s\n", libusb_strerror(res));
		goto exit_err;
	}

	uhan = calloc(1, sizeof(*uhan));
	if(uhan == NULL) {
		sprintf(err_string_buff, "malloc: %s", strerror(errno));
		goto exit_err;
	}
	uhan->han = han;

	ifdesc = cdesc->interface->altsetting;
	for(i=0; i<ifdesc->bNumEndpoints; i++) {
		if(verbose) {
			fprintf(msgfile, "Found EP 0x%02x : max packet size is %u bytes\n",
				ifdesc->endpoint[i].bEndpointAddress, ifdesc->endpoint[i].wMaxPacketSize);
		}
		if((ifdesc->endpoint[i].bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) != LIBUSB_TRANSFER_TYPE_BULK)
			continue;
		int ind = (ifdesc->endpoint[i].bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) ? USB_DIR_IN : USB_DIR_OUT;
		uhan->eps[ind].ep = ifdesc->endpoint[i].bEndpointAddress;
		uhan->eps[ind].sz = ifdesc->endpoint[i].wMaxPacketSize;
	}
	if(uhan->eps[USB_DIR_IN].ep == 0) {
		sprintf(err_string_buff, "Unable to find BULK IN endpoint\n");
		goto exit_err;
	}
	if(uhan->eps[USB_DIR_OUT].ep == 0) {
		sprintf(err_string_buff, "Unable to find BULK OUT endpoint\n");
		goto exit_err;
	}
	if(uhan->eps[USB_DIR_IN].sz == 0) {
		sprintf(err_string_buff, "Unable to find BULK IN endpoint size\n");
		goto exit_err;
	}
	if(uhan->eps[USB_DIR_OUT].sz == 0) {
		sprintf(err_string_buff, "Unable to find BULK OUT endpoint size\n");
		goto exit_err;
	}
	return uhan;
exit_err:
	if(han != NULL)
		libusb_close(han);
	if(uhan != NULL)
		free(uhan);
	return USB_HAN_NONE;
}

uint8_t usb_get_bulk_endpoint(usb_han han, int dir) {
	if((han == NULL) || ((dir != USB_DIR_OUT) && (dir != USB_DIR_IN)))
		return 0;
	return han->eps[dir].ep;
}

size_t usb_get_endpoint_size(usb_han han, uint8_t ep) {
	if(han == NULL)
		return 0;
	if(han->eps[USB_DIR_OUT].ep == ep)
		return han->eps[USB_DIR_OUT].sz;
	if(han->eps[USB_DIR_IN].ep == ep)
		return han->eps[USB_DIR_IN].sz;
	return 0;
}

int usb_bulk_write(usb_han han, uint8_t ep, const void *buffer, size_t sz, size_t *wrote_bytes, int timeout_ms) {
	int wb = 0;
	int res;
	if(wrote_bytes != NULL)
		*wrote_bytes = 0;
	if(han == NULL)
		return USB_ERR_INVALID;
	if(ep != han->eps[USB_DIR_OUT].ep)
		return USB_ERR_INVALID;
	res = libusb_bulk_transfer(han->han, ep, (void*)buffer, (int)sz, &wb, timeout_ms);
	last_bulk_result = res;
	if(res == LIBUSB_ERROR_TIMEOUT)
		return USB_ERR_TIMEOUT;
	if(res != 0)
		return USB_ERR_FAILED;
	if(wrote_bytes != NULL)
		*wrote_bytes = wb;
	return USB_ERR_NONE;
}

int usb_bulk_read(usb_han han, uint8_t ep, void *buffer, size_t sz, size_t *read_bytes, int timeout_ms) {
	int rb = 0;
	int res;
	if(read_bytes != NULL)
		*read_bytes = 0;
	if(han == NULL)
		return USB_ERR_INVALID;
	if(ep != han->eps[USB_DIR_IN].ep)
		return USB_ERR_INVALID;
	if(sz == 0)
		return USB_ERR_NONE;
	res = libusb_bulk_transfer(han->han, ep, buffer, (int)sz, &rb, timeout_ms);
	last_bulk_result = res;
	if(res == LIBUSB_ERROR_TIMEOUT)
		return USB_ERR_TIMEOUT;
	if(res != 0)
		return USB_ERR_FAILED;
	if(read_bytes != NULL)
		*read_bytes = rb;
	return USB_ERR_NONE;
}

void usb_free_device(usb_dev dev) {
	if(dev != NULL)
		libusb_unref_device(dev);
}

void usb_close_device(usb_han han) {
	if(han == NULL)
		return;
	libusb_release_interface(han->han, 0);
	libusb_close(han->han);
	free(han);
}

const char *usb_last_bulk_errmsg(void) {
	return libusb_strerror(last_bulk_result);
}

void usb_set_msgfile(FILE *file) {
	msgfile = file;
}

void usb_set_verbose(int value) {
	verbose = value;
	if(verbose >= 2)
		libusb_set_debug(NULL, 3);
	else
		libusb_set_debug(NULL, 0);
}

void usb_set_ignoreerrors(int value) {
	ignore_errors = value;
}

