#include "usb_host.h"
#include <libusb-1.0/libusb.h>
#include <signal.h>  // capture sigint
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // sleep
#include <iostream>
#include <thread>
#include <string.h>
#include <stream_object.h>
#include "packet.h"

#define USB_TIMEOUT 0

using namespace std;

static libusb_context *       context;
static libusb_device_handle * device_handle;

bool usb_write_packet(const rc_packet_t & packet)
{
    int r, bytes_written;
    int bytes_to_write = sizeof(packet_header_t);
    r                  = libusb_bulk_transfer(device_handle, ENDPOINT_HOST_OUT,
                             (uint8_t *)&packet->header, bytes_to_write,
                             &bytes_written, USB_TIMEOUT);
    if(r) {
        cerr << "bulk write header failed " << r << "\n";
        return false;
    }

    bytes_to_write = packet->header.bytes - sizeof(packet_header_t);
    if (bytes_to_write == 0)
        return true;
    r              = libusb_bulk_transfer(device_handle, ENDPOINT_HOST_OUT,
                             (uint8_t *)&packet->data, bytes_to_write,
                             &bytes_written, USB_TIMEOUT);
    if(r) {
        cerr << "bulk write packet failed " << r << "\n";
        return false;
    }
    return true;
}

rc_packet_t usb_read_packet()
{
    int             r = 0;
    packet_header_t packet_header;

    int bytes_to_read = sizeof(packet_header_t);
    int bytes_read;
    r = libusb_bulk_transfer(device_handle, ENDPOINT_HOST_IN,
                             (uint8_t *)&packet_header, bytes_to_read,
                             &bytes_read, USB_TIMEOUT);
    if(r) {
        cerr << "bulk transfer failed " << r << "\n";
        return { nullptr, free };
    }

    rc_packet_t packet((packet_t *)malloc(packet_header.bytes), free);
    packet->header = packet_header;

    uint64_t packet_bytes = packet->header.bytes - sizeof(packet_header_t);
    if(packet_bytes) {
        r = libusb_bulk_transfer(device_handle, ENDPOINT_HOST_IN,
                                 (uint8_t *)&packet->data, packet_bytes,
                                 &bytes_read, USB_TIMEOUT);
        if(r) {
            cerr << "bulk transfer failed " << r << "\n";
            return{ nullptr, free };
        }
    }
    return packet;
}

bool usb_send_control(uint8_t control_message, uint16_t value, uint16_t index,
                      unsigned int timeout = 0)
{
    int r = libusb_control_transfer(
            device_handle, USB_bmRequestType_VSC_HOST_TO_DEVICE,
            control_message, value, index, NULL, 0, timeout);
    if(r) {
        std::cerr << "Control transfer error" << r << "\n";
        return false;
    }
    return true;
}

void usb_shutdown()
{
    if (device_handle) {
        if (!usb_send_control(CONTROL_MESSAGE_STOP, 0, 0, 0)) {
            std::cerr << "Error: unable to stop replay cleanly\n";
        }
        libusb_reset_device(device_handle); 
        libusb_release_interface(device_handle, 0);
        libusb_close(device_handle);
        device_handle = NULL;
    }
    if (context) libusb_exit(context);
    context = NULL;
    std::this_thread::sleep_for(std::chrono::milliseconds(5000)); //wait for complete USB shutdown before potential bringup
}

void usb_reset()
{
    if (device_handle && !usb_send_control(CONTROL_MESSAGE_RESET, 0, 0, 500))
        std::cerr << "Error: unable to reset usb cleanly\n";
}

bool usb_init(int vendor_id, int product_id)
{
    libusb_init(&context);
#if LIBUSB_API_VERSION >= 0x01000106
    libusb_set_option(context, LIBUSB_OPTION_LOG_LEVEL, 3);
#else
    libusb_set_debug(context, 3);
#endif
    device_handle = libusb_open_device_with_vid_pid(context, vendor_id, product_id);
    bool is_failure = false;
    if (!device_handle) {
        std::cerr << "Error: failed to open a TM2 device ("<< std::hex << std::setw(4) << std::setfill('0') << vendor_id << ":" << std::setw(4) << std::setfill('0') << product_id << std::dec << ")\n";
        is_failure = true;
    }
    else {
        std::cout << "TM2 Found (" << std::hex << std::setw(4) << std::setfill('0') << vendor_id << ":" << std::setw(4) << std::setfill('0') << product_id << std::dec << ")\n";
        libusb_device* dev = libusb_get_device(device_handle);
        if (libusb_get_device_speed(dev) != LIBUSB_SPEED_SUPER) {
            std::cout << "Error: device is not connected via USB3\n";
            is_failure = true;
        }
        else if (libusb_claim_interface(device_handle, 0)) {
            std::cerr << "Error: failed to claim interface\n";
            is_failure = true;
        }
    }
    if (is_failure) {
        usb_shutdown();
        return false;
    }
    return true;
}

bool usb_read_interrupt_packet(rc_packet_t &fixed_pkt)
{
    int32_t bytes_read = 0;
    int r = libusb_interrupt_transfer(device_handle, ENDPOINT_HOST_INT_IN,
        (uint8_t *)fixed_pkt.get(), fixed_pkt->header.bytes, &bytes_read, USB_TIMEOUT);
    if(r || (bytes_read != (int32_t)fixed_pkt->header.bytes)) {
        cerr << "interrupt packet transfer failed " << r << " read " << bytes_read << " vs. expected " << fixed_pkt->header.bytes << "\n";
        return false;
    }
    return true;
}
