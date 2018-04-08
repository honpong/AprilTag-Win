#include "usb_host.h"

int main(int c, char *v[]) {
    if (usb_init())
        return usb_send_control(CONTROL_MESSAGE_USB_RESET, 0, 0, 0) ? 0 : 1;
    else if (usb_init(0x8087, 0x0AF3))
        return usb_send_control(CONTROL_MESSAGE_USB_RESET, 0, 0, 0) ? 0 : 0; // TM2 FW fails no matter what. :(
    else
        return 0;
}
