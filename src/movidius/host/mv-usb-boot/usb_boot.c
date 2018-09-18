// USB utility for use with Myriad2v2 ROM
// Very heavily modified from Sabre version of usb_boot
// Author: David Steinberg <david.steinberg@movidius.com>
// Copyright(C) 2015 Movidius Ltd.

#if defined(WIN32) && !defined(__CYGWIN32__)
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#include <direct.h>
#define mssleep(x)	Sleep(x)
#else
#include <unistd.h>
#define mssleep(x)	usleep((x) * 1000)
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <getopt.h>
#include <errno.h>
#include <ctype.h>
#include "highrestime.h"
#include "crc32.h"
#include "rom_checksum.h"
#include "dumphex.h"
#include "usb_common.h"
#include "usb_boot.h"

#define USB_BOOT_VERSION			"4.1"
#define DEFAULT_VID					0x03E7
#define DEFAULT_PID					0x2150		// Myriad2v2 ROM
#define DEFAULT_BUFLEN				1024
#define DEFAULT_WRITE_TIMEOUT		10000
#define DEFAULT_NULL_TIMEOUT		1000
#define DEFAULT_READ_TIMEOUT		1000
#define DEFAULT_INITIALREAD_TIMEOUT	-1			// if -1, use read_timeout
#define DEFAULT_CONNECT_TIMEOUT		100			// in 100ms units
#define DEFAULT_RECONNECT_TIMEOUT	100			// in 100ms units
#define DEFAULT_STDINBUFLEN			1024
#define DEFAULT_CHUNKSZ				1024

typedef enum _readback_mode {
	rmNormal,
	rmCrc32,
	rmRomChecksum,
	rmQuiet,
} readback_mode;

static void usage(char *argv0);
static void parse_args(int argc, char *argv[]);
static void reset_variables();
static usb_dev find_dev(int loud);
static usb_han open_dev(usb_dev dev);
static int wait_findopen(int as_reconnect, int timeout, usb_dev *dev_out, usb_han *han_out);
//static usb_dev wait_disconnect(int timeout);
static int wait_disconnect_withdev(usb_dev dev, int timeout);
static int readback_command(usb_han han, readback_mode mode);
static int send_null_packet(usb_han han);
static int send_stdin(usb_han han);
static int send_file(usb_han han, const char *fname);

static uint8_t *rx_buff;
static uint16_t vid = DEFAULT_VID;
static uint16_t pid = DEFAULT_PID;
//static int device_index = 0;
static char *device_address = NULL;
static int do_list_devices = 0;
static int use_guid = 0;
int ep_in = 0x81, ep_out = 0x01;
int ep_in_sz, ep_out_sz;
static unsigned long bulk_buflen = DEFAULT_BUFLEN;
static long stdin_buflen = DEFAULT_STDINBUFLEN;
unsigned int bulk_chunklen = DEFAULT_CHUNKSZ;
static int do_log = 0, always_null = 0, bin_data = 0;
static int wait_for_device = 0, loop_commands = 0, ignore_errors = 0;
static int quiet = 0, verbose = 0;
FILE *msgdev;
static int write_timeout = DEFAULT_WRITE_TIMEOUT, read_timeout = DEFAULT_READ_TIMEOUT;
static int initialread_timeout = DEFAULT_INITIALREAD_TIMEOUT;
static int null_timeout = DEFAULT_NULL_TIMEOUT;
static int connect_timeout = DEFAULT_CONNECT_TIMEOUT, reconnect_timeout = DEFAULT_RECONNECT_TIMEOUT;

static char last_open_dev_err[128];

int usb_boot(int argc, char *argv[]) {
	int exitcode = 1;
	int argind;
	usb_dev dev = USB_DEV_NONE;
	usb_han han = USB_HAN_NONE;

	reset_variables();
	parse_args(argc, argv);
	if(do_list_devices) {
		usb_init();
		usb_list_devices(vid, pid);
		return 0;
	}
	if(argc == optind)
		usage(argv[0]);
	argc -= optind;
	argv += optind;

	msgdev = bin_data ? stderr : stdout;
	if(initialread_timeout == -1)
		initialread_timeout = read_timeout;
	if(connect_timeout == 0)
		connect_timeout = -1;
	if(reconnect_timeout == 0)
		reconnect_timeout = -1;

	if(usb_init())
		return 1;
	usb_set_msgfile(msgdev);
	usb_set_verbose(verbose);
	usb_set_ignoreerrors(ignore_errors);

	bulk_buflen *= 1024;
	if((rx_buff = malloc(bulk_buflen)) == NULL) {
		perror("rx_buff");
		goto exit_err;
	}
	stdin_buflen *= 1024;
	bulk_chunklen *= 1024;

	if(!wait_findopen(0, wait_for_device ? -1 : connect_timeout, &dev, &han))
		goto exit_err;

	do {
		for(argind=0; argind < argc; argind++) {
			if(!strcmp(argv[argind], ":re")) {
				usb_close_device(han);
				usb_free_device(dev);
				if(!wait_findopen(1, reconnect_timeout, &dev, &han))
					goto exit_err;
			} else if(!strcmp(argv[argind], ":rb") ||
					  !strcmp(argv[argind], ":rbc") ||
					  !strcmp(argv[argind], ":rbr") ||
					  !strcmp(argv[argind], ":rbq")) {
				readback_mode mode = rmNormal;
				switch(argv[argind][3]) {
					case 'c':
						mode = rmCrc32;
						break;
					case 'r':
						mode = rmRomChecksum;
						break;
					case 'q':
						mode = rmQuiet;
						break;
				}
				if(readback_command(han, mode))
					goto exit_err;
			} else if(!strcmp(argv[argind], ":null") || !strcmp(argv[argind], ":nul")) {
				if(send_null_packet(han))
					goto exit_err;
			} else if(!strcmp(argv[argind], "-")) {
				if(send_stdin(han))
					goto exit_err;
			} else {
				if(send_file(han, argv[argind]))
					goto exit_err;
			}
		}
		if(loop_commands) {
			usb_close_device(han);
			han = USB_HAN_NONE;
			wait_disconnect_withdev(dev, -1);
			usb_free_device(dev);
			dev = USB_DEV_NONE;
			//wait_disconnect(-1);
			if(!wait_findopen(1, -1, &dev, &han))
				goto exit_err;
		}
	} while(loop_commands);

	exitcode = 0;
exit_err:
	if(rx_buff != NULL)
		free(rx_buff);
	if(han != USB_HAN_NONE) {
		usb_close_device(han);
	}
	if(dev != USB_DEV_NONE)
		usb_free_device(dev);
	usb_shutdown();
	return exitcode;
}

static void usage(char *argv0) {
	printf(
		"Usage: %s <options> <command list>\n\n"
		"Options:\n"
		"  -v <vid>    Specify VID (default: 0x%04x)\n"
		"  -p <pid>    Specify PID (default: 0x%04x)\n"
		"%s"
		"  -I <addr>   Address of the device to use as listed by -S\n"
		"  -S          List all devices that match the VID/PID\n"
		"  -s <size>   Specify maxmium readback size in KiB (default: %u KiB)\n"
		"  -n          Always write an empty packet after writing a file\n"
		"  -l          Display hexdump of all data sent to device\n"
		"  -b          Output readback data directly, do not display as hex\n"
		"              Note that if enabled, messages will be output to stderr\n"
		"  -t <ms>     Set timeout for bulk reads (device to host) (default is %ums)\n"
		"  -T <ms>     Set timeout for bulk writes (host to device) (default is %ums)\n"
		"  -i <ms>     Set different timeout for initial bulk read (device to host)\n"
		"  -N <ms>     Set timeout for empty packet writes (default is %ums)\n"
		"  -B <size>   Specify stdin buffer size in KiB (default: %u KiB)\n"
		"  -x <kb>     Specify maximum transfer chunk in KiB (default: %u KiB)\n"
		"  -a          Waits for device before starting\n"
		"  -A          The same as -a, but when finished, waits for disconnection\n"
		"              and reconnection, then re-executes command list\n"
		"  -c <val>    Sets first connect timeout in 200ms units (0 for no timeout)\n"
		"              Default is %u (%u ms)\n"
		"  -r <val>    Sets reconnect timeout in 200ms units (0 for no timeout)\n"
		"              Default is %u (%u ms)\n"
		"  -q          Do not output any messages (except errors)\n"
		"  -L          Verbose mode (loud, twice to enable debug)\n"
		"  -E          Ignore errors (where possible)\n"
		"  -V          Output version number and exit\n"
		"  -h          Output help\n\n"
		"Command list:\n"
		"  <filename>  Writes the specified file to the device\n"
		"  -           Writes contents of stdin to the device\n"
		"  :null       Write an empty packet to the device\n"
		"  :rb         Reads back data from the device and output the data\n"
		"              to stdout in hex or binary format (see -b)\n"
		"  :rbq        Reads back data with no data output\n"
		"  :rbc        Reads back data and outputs a CRC32\n"
		"  :rbr        Reads back data and outputs a ROM-style checksum\n"
		"  :re         Close and re-open the device (waiting if necessary)\n",
		argv0,
		DEFAULT_VID, DEFAULT_PID,
		usb_can_find_by_guid() ?
			"  -g          Find device by GUID instead of VID/PID\n" : "",
		DEFAULT_BUFLEN,
		DEFAULT_READ_TIMEOUT, DEFAULT_WRITE_TIMEOUT, DEFAULT_NULL_TIMEOUT,
		DEFAULT_STDINBUFLEN, DEFAULT_CHUNKSZ,
		DEFAULT_CONNECT_TIMEOUT, DEFAULT_CONNECT_TIMEOUT * 100,
		DEFAULT_RECONNECT_TIMEOUT, DEFAULT_RECONNECT_TIMEOUT * 100);
	exit(0);
}

static void parse_args(int argc, char *argv[]) {
	int i;
	char *p;
	optind = 1;
	static char opt_list[] = "v:p:gI:Ss:nlbt:T:i:N:B:x:aAc:r:qLEVh";
	while((i = getopt(argc, argv, opt_list)) != -1) {
		switch(i) {
			case 'v':
				if(sscanf(optarg, "%hi", &vid) != 1)
					usage(argv[0]);
				if(vid == 0)
					vid = DEFAULT_VID;
				break;
			case 'p':
				if(sscanf(optarg, "%hi", &pid) != 1)
					usage(argv[0]);
				if(pid == 0)
					pid = DEFAULT_PID;
				break;
			case 'g':
				if(!usb_can_find_by_guid())
					usage(argv[0]);
				use_guid = 1;
				break;
			case 'I':
				//if(sscanf(optarg, "%i", &device_index) != 1)
				//	usage(argv[0]);
				device_address = optarg;
				break;
			case 'S':
				do_list_devices = 1;
				break;
			case 's':
				if(sscanf(optarg, "%li", &bulk_buflen) != 1)
					usage(argv[0]);
				break;
			case 'n':
				always_null = 1;
				break;
			case 'l':
				do_log = 1;
				break;
			case 'b':
				bin_data = 1;
				break;
			case 't':
				if(sscanf(optarg, "%i", &read_timeout) != 1)
					usage(argv[0]);
				break;
			case 'T':
				if(sscanf(optarg, "%i", &write_timeout) != 1)
					usage(argv[0]);
				break;
			case 'i':
				if(sscanf(optarg, "%i", &initialread_timeout) != 1)
					usage(argv[0]);
				break;
			case 'N':
				if(sscanf(optarg, "%i", &null_timeout) != 1)
					usage(argv[0]);
				break;
			case 'B':
				if(sscanf(optarg, "%li", &stdin_buflen) != 1)
					usage(argv[0]);
				break;
			case 'x':
				if(sscanf(optarg, "%i", &bulk_chunklen) != 1)
					usage(argv[0]);
				break;
			case 'a':
				wait_for_device = 1;
				break;
			case 'A':
				wait_for_device = loop_commands = 1;
				break;
			case 'c':
				if(sscanf(optarg, "%i", &connect_timeout) != 1)
					usage(argv[0]);
				break;
			case 'r':
				if(sscanf(optarg, "%i", &reconnect_timeout) != 1)
					usage(argv[0]);
				break;
			case 'q':
				quiet = 1;
				break;
			case 'L':
				verbose++;
				break;
			case 'E':
				ignore_errors = 1;
				break;
			case 'h':
				usage(argv[0]);
			case 'V':
				printf("usb_boot V%s\n", USB_BOOT_VERSION);
				exit(0);
			case '?':
				if((p = strchr(opt_list, optopt)) && (p[1] == ':'))
					fprintf(stderr, "Option -%c requires an argument\n", optopt);
				else if(isprint(optopt))
					fprintf(stderr, "Unknown option '-%c'\n", optopt);
				else
					fprintf(stderr, "Unknown option character '\\x%x'\n", optopt);
				usage(argv[0]);
			default:
				usage(argv[0]);
		}
	}
}

static usb_dev find_dev(int loud) {
	usb_dev dev;
	if(use_guid)
		dev = usb_find_device_by_guid(loud);
	else
		dev = usb_find_device(vid, pid, device_address, loud);
	return dev;
}

static usb_han open_dev(usb_dev dev) {
	usb_han han;

	han = usb_open_device(dev, last_open_dev_err);
	if(han == USB_HAN_NONE)
		return USB_HAN_NONE;
	ep_in = usb_get_bulk_endpoint(han, USB_DIR_IN);
	ep_out = usb_get_bulk_endpoint(han, USB_DIR_OUT);
	ep_in_sz = usb_get_endpoint_size(han, ep_in);
	ep_out_sz = usb_get_endpoint_size(han, ep_out);
	return han;
}

// timeout: -1 = no (infinite) timeout, 0 = must happen immediately
static int wait_findopen(int as_reconnect, int timeout, usb_dev *dev_out, usb_han *han_out) {
	int i;
	usb_dev dev = NULL;
	usb_han han = NULL;
	mssleep(100);
	*dev_out = USB_DEV_NONE;
	*han_out = USB_HAN_NONE;

	if(verbose) {
		// I know about switch(), but for some reason -1 is picked up correctly
		if(timeout == -1) {
			fprintf(msgdev, "Starting wait for %sconnect, no timeout\n", as_reconnect ? "re" : "");
		} else if(timeout == 0) {
			fprintf(msgdev, "Trying to %sconnect\n", as_reconnect ? "re" : "");
		} else {
			fprintf(msgdev, "Starting wait for %sconnect with %ums timeout\n", as_reconnect ? "re" : "", timeout * 200);
		}
	}
	last_open_dev_err[0] = 0;
	i = 0;
	while(1) {
		if((dev = find_dev(0)) == USB_DEV_NONE) {
			if(!quiet && (i == 0) && !verbose)
				fprintf(msgdev, "Waiting for device\n");
		} else {
			if((han = open_dev(dev)) != USB_HAN_NONE)
				break;
			usb_free_device(dev);
			dev = USB_DEV_NONE;
		}
		if((timeout != -1) && ((timeout == 0) || (i++ == timeout))) {
			if(last_open_dev_err[0])
				fprintf(stderr, "%s", last_open_dev_err);
			fprintf(stderr, "error: device not found!\n");
			return 0;
		} else if(timeout <= 0)
			i = 1;
		mssleep(100);
	}
	if(verbose)
		fprintf(msgdev, "Found and opened device%s\n", as_reconnect ? " after reconnect" : "");
	*dev_out = dev;
	*han_out = han;
	return 1;
}

#if 0
// timeout: -1 = no (infinite) timeout, 0 = must happen immediately
static usb_dev wait_disconnect(int timeout) {
	int i;
	usb_dev dev;
	mssleep(100);

	if(verbose) {
		if(timeout != -1)
			fprintf(msgdev, "Starting wait for disconnect, no timeout\n");
		else
			fprintf(msgdev, "Starting wait for disconnect with %ums timeout\n", timeout * 200);
	}
	i = 0;
	do {
		if((dev = find_dev(0)) != USB_DEV_NONE) {
			usb_free_device(dev);
			if(!quiet && (i == 0))
				fprintf(msgdev, "Waiting for disconnect\n");
			if((timeout != -1) && ((timeout == 0) || (i++ == timeout))) {
				fprintf(stderr, "error: device still connected!\n");
				if(ignore_errors)
					return USB_DEV_NONE;
				return dev;
			} else if(timeout <= 0)
				i = 1;
			mssleep(100);
		}
	} while(dev != USB_DEV_NONE);
	if(verbose)
		fprintf(msgdev, "Device disconnected\n");
	return USB_DEV_NONE;
}
#endif

// timeout: -1 = no (infinite) timeout, 0 = must happen immediately
// returns 1 if device is still not disconnected after timeout
static int wait_disconnect_withdev(usb_dev dev, int timeout) {
	int i;
	usb_han han;
	mssleep(100);

	if(verbose) {
		if(timeout != -1)
			fprintf(msgdev, "Starting wait for disconnect, no timeout\n");
		else
			fprintf(msgdev, "Starting wait for disconnect with %ums timeout\n", timeout * 200);
	}
	i = 0;
	do {
		if((han = usb_open_device(dev, last_open_dev_err)) != USB_HAN_NONE) {
			usb_close_device(han);
			if(!quiet && (i == 0))
				fprintf(msgdev, "Waiting for disconnect\n");
			if((timeout != -1) && ((timeout == 0) || (i++ == timeout))) {
				fprintf(stderr, "error: device still connected!\n");
				if(ignore_errors)
					return 0;
				return 1;
			} else if(timeout <= 0)
				i = 1;
			mssleep(100);
		}
	} while(han != USB_HAN_NONE);
	if(verbose)
		fprintf(msgdev, "Device disconnected\n");
	return 0;
}

static int readback_command(usb_han han, readback_mode mode) {
	uint8_t *p;
	size_t trb, rb;
	int res;
	double elapsedTime;
	highres_time_t t1, t2;

	p = rx_buff;
	trb = 0;
	elapsedTime = 0;
	if(verbose)
		fprintf(msgdev, "Starting readback\n");
	while(trb < bulk_buflen) {
		highres_gettime(&t1);
		rb = 0;
		res = usb_bulk_read(han, ep_in, p, ep_in_sz, &rb, (trb == 0) ? initialread_timeout : read_timeout);
		if((res == USB_ERR_TIMEOUT) && (trb != 0))
			break;
		highres_gettime(&t2);
		if(res < 0) {
			fprintf(stderr, "bulk read: %s\n", usb_last_bulk_errmsg());
			if(ignore_errors)
				break;
			return 1;
		}
		elapsedTime += highres_elapsed_ms(&t1, &t2);
		trb += rb;
		p += rb;
	}
	if(trb != 0) {
		if(!quiet) {
			double MBpS =
				((double)trb / 1048576.) /
				(elapsedTime * 0.001);
			fprintf(msgdev, "Successfully read %d bytes of data in %lf ms (%lf MB/s)\n", (int)trb, elapsedTime, MBpS);
		}
		switch(mode) {
			case rmNormal:
				if(bin_data)
					fwrite(rx_buff, 1, trb, stdout);
				else
					dumphex(stdout, rx_buff, trb);
				break;
			case rmCrc32:
				fprintf(msgdev, "CRC: 0x%08x\n", crc32(0, rx_buff, trb));
				break;
			case rmRomChecksum:
				fprintf(msgdev, "ROM Checksum: 0x%08x\n", rom_checksum(0, rx_buff, trb));
				break;
			case rmQuiet:
				break;
		}
	} else {
		if(!quiet)
			fprintf(msgdev, "No data read\n");
	}
	return 0;
}

static int send_null_packet(usb_han han) {
	uint8_t tx_buf[1] = {0};
	size_t wb;
	int res;
	if(!quiet)
		fprintf(msgdev, "Writing empty packet\n");
	if((res = usb_bulk_write(han, ep_out, tx_buf, 0, &wb, null_timeout)) < 0) {
		fprintf(stderr, "bulk write: %s\n", usb_last_bulk_errmsg());
		if(!ignore_errors)
			return 1;
	}
	return 0;
}

static int send_stdin(usb_han han) {
	uint8_t *tx_buf, *p;
	size_t rb, wb, wbr;
	int res;
	int filesize;
	double elapsedTime;
	highres_time_t t1, t2;

	if(!(tx_buf = malloc(stdin_buflen))) {
		perror("buffer");
		return 1;
	}

	if(!quiet)
		fprintf(msgdev, "Performing bulk write from stdin...\n");
	filesize = 0;
	elapsedTime = 0;
#if defined(WIN32) && !defined(__CYGWIN32__)
	_setmode(_fileno(stdin), _O_BINARY);
#endif
	while((rb = fread(tx_buf, 1, stdin_buflen, stdin)) > 0) {
		if(do_log)
			dumphex(msgdev, tx_buf, rb);
		filesize += rb;
		p = tx_buf;
		while(rb != 0) {
			wb = rb;
			if(wb > bulk_chunklen)
				wb = bulk_chunklen;
			highres_gettime(&t1);
			res = usb_bulk_write(han, ep_out, p, wb, &wbr, write_timeout);
			if((res < 0) || (wbr != wb)) {
				fprintf(stderr, "bulk write: %s (previously transfered %d, current transfer %zu/%zu)\n", usb_last_bulk_errmsg(), filesize, wbr, wb);
				if(ignore_errors)
					break;
				return 1;
			}
			highres_gettime(&t2);
			elapsedTime += highres_elapsed_ms(&t1, &t2);
			p += wb;
			rb -= wb;
		}
	}
	if(!quiet) {
		double MBpS =
			((double)filesize / 1048576.) /
			(elapsedTime * 0.001);
		fprintf(msgdev, "Successfully sent %d bytes of data in %lf ms (%lf MB/s)\n", filesize, elapsedTime, MBpS);
	}
	free(tx_buf);
	if(always_null)
		usb_bulk_write(han, ep_out, tx_buf, 0, &wb, null_timeout);
	return 0;
}

static int send_file(usb_han han, const char *fname) {
	FILE *file;
	uint8_t *tx_buf, *p;
	int res;
	size_t wb, twb, wbr;
	unsigned long filesize;
	double elapsedTime;
	highres_time_t t1, t2;

	file = fopen(fname, "rb");
	if(file == NULL) {
		perror(fname);
		return 1;
	}

	fseek(file, 0, SEEK_END);
	filesize = ftell(file);
	rewind(file);

	if(!(tx_buf = malloc(filesize))) {
		perror("buffer");
		fclose(file);
		return 1;
	}

	if(fread(tx_buf, 1, filesize, file) != filesize) {
		perror(fname);
		fclose(file);
		free(tx_buf);
		return 1;
	}
	fclose(file);

	if(do_log)
		dumphex(msgdev, tx_buf, filesize);
	elapsedTime = 0;
	twb = 0;
	p = tx_buf;
	if(!quiet)
		fprintf(msgdev, "Performing bulk write of %lu byte%s from %s...\n", filesize, filesize == 1 ? "" : "s", fname);
	while(twb < filesize) {
		highres_gettime(&t1);
		wb = filesize - twb;
		if(wb > bulk_chunklen)
			wb = bulk_chunklen;
		wbr = 0;
		res = usb_bulk_write(han, ep_out, p, wb, &wbr, write_timeout);
		if((res == USB_ERR_TIMEOUT) && (twb != 0))
			break;
		highres_gettime(&t2);
		if((res < 0) || (wb != wbr)) {
			fprintf(stderr, "bulk write: %s (previously transfered %zu, current transfer %zu/%zu, left to transfer %lu)\n", usb_last_bulk_errmsg(), twb, wbr, wb, filesize - twb - wb);
			if(!ignore_errors) {
				free(tx_buf);
				return 1;
			}
		}
		elapsedTime += highres_elapsed_ms(&t1, &t2);
		twb += wbr;
		p += wbr;
	}
	if(!quiet) {
		double MBpS =
			((double)filesize / 1048576.) /
			(elapsedTime * 0.001);
		fprintf(msgdev, "Successfully sent %ld bytes of data in %lf ms (%lf MB/s)\n", filesize, elapsedTime, MBpS);
	}
	free(tx_buf);
	if ((always_null) || (filesize % 512))
		usb_bulk_write(han, ep_out, tx_buf, 0, &wb, null_timeout);
	return 0;
}

static void reset_variables() {
	device_address = NULL;
	do_list_devices = 0;
	use_guid = 0;
	ep_in = 0x81;
	ep_out = 0x01;
	ep_in_sz = 0;
	ep_out_sz = 0;
	bulk_buflen = DEFAULT_BUFLEN;
	stdin_buflen = DEFAULT_STDINBUFLEN;
	do_log = 0; always_null = 0; bin_data = 0;
	wait_for_device = 0;
	loop_commands = 0;
	ignore_errors = 0;
	quiet = 0;
	verbose = 0;
	msgdev = NULL;
	write_timeout = DEFAULT_WRITE_TIMEOUT;
	read_timeout = DEFAULT_READ_TIMEOUT;
	initialread_timeout = DEFAULT_INITIALREAD_TIMEOUT;
	null_timeout = DEFAULT_NULL_TIMEOUT;
	connect_timeout = DEFAULT_CONNECT_TIMEOUT;
	reconnect_timeout = DEFAULT_RECONNECT_TIMEOUT;
	memset(last_open_dev_err, 0, sizeof(last_open_dev_err));
}

int load_fw(const char* fw_file) {
	char *argv[] = { "load_fw", "-a", "-x 2", "-q", (char *)fw_file };
	return usb_boot(sizeof(argv) / sizeof(char *), argv);
}
