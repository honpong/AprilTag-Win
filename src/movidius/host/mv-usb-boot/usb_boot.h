#ifndef _USB_BOOT_H
#define _USB_BOOT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#  define MV_USB_BOOT_API __declspec(dllimport)
#else
#  define MV_USB_BOOT_API __attribute__ ((visibility("default")))
#endif

MV_USB_BOOT_API int load_fw(const char* fw_file);

#ifdef __cplusplus
}
#endif

#endif