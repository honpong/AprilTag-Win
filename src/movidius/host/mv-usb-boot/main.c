// USB utility wrapper for use with Myriad2v2 ROM
#include "usb_common.h"
#include "usb_boot.h"
int main(int argc, char *argv[]) {
    return usb_boot(argc, argv);
}