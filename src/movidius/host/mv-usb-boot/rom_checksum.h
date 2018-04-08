
#ifndef _ROM_CHECKSUM_H
#define _ROM_CHECKSUM_H

#include <stdint.h>
#include <stdlib.h>

extern uint16_t rom_checksum(uint16_t chk, const void *buf, size_t size);

#endif//_ROM_CHECKSUM_H
