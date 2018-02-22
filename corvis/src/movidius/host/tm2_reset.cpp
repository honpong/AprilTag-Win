#include "usb_host.h"

int main(int c, char *v[]) {
    return usb_init() && usb_send_control(CONTROL_MESSAGE_USB_RESET, 0, 0, 0) ? 0 : 1;
}
