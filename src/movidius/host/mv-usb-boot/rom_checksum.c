
#include "rom_checksum.h"

uint16_t rom_checksum(uint16_t chk, const void *buf, size_t size) {
	const uint8_t *p;
	uint32_t cchk = chk;		// the ROM does this with a uint32_t so we will
	p = buf;
	while (size--) {
		cchk = ((cchk >> 15) & 1) | ((cchk << 1) & 0x0fff7);
		cchk += *p++;
	}

	return (uint16_t)cchk;
}
