#pragma once

#define USB_VENDOR_ID 0x040E
#define USB_PRODUCT_ID 0xF63B
// all control packets are sent with this request type
#define USB_bmRequestType_VSC_HOST_TO_DEVICE 0x40

// on the MCCI (Myriad) device, endpoint numbering starts at 0
#define ENDPOINT_DEV_IN 0
#define ENDPOINT_DEV_OUT 0
#define ENDPOINT_DEV_MSGS_IN 1
#define ENDPOINT_DEV_MSGS_OUT 1
#define ENDPOINT_DEV_INT_OUT 2

// in libusb, endpoint numbering starts at 1 and has a 0x80 when the
// direction is into the host
#define ENDPOINT_HOST_IN (ENDPOINT_DEV_OUT + 1 + 0x80)
#define ENDPOINT_HOST_OUT (ENDPOINT_DEV_IN + 1)
#define ENDPOINT_HOST_MSGS_IN (ENDPOINT_DEV_MSGS_OUT + 1 + 0x80)
#define ENDPOINT_HOST_MSGS_OUT (ENDPOINT_DEV_MSGS_IN + 1)
#define ENDPOINT_HOST_INT_IN (ENDPOINT_DEV_INT_OUT + 1 + 0x80)

enum CONTROL_MESSAGES
{
    CONTROL_MESSAGE_RESET = 0,
    CONTROL_MESSAGE_START = 1,
    CONTROL_MESSAGE_STOP = 2,
    CONTROL_MESSAGE_USB_RESET = 0x10
};

enum REPLAY_FLAGS
{
    REPLAY_SYNC     = 0x0,
    REPLAY_REALTIME = 0x1
};

enum SIXDOF_FLAGS
{
    SIXDOF_FLAG_MULTI_THREADED = 0,
    SIXDOF_FLAG_SINGLE_THREADED
};
