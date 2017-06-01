#ifndef USB_H
#define USB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int usbInit();
int usbStop();

int startReplay();
void stopReplay();

#ifdef __cplusplus
}
#endif

#endif  //USB_H
