
#include <stdint.h>
#include "dumphex.h"

void dumphex(FILE *outf, const void *buff, size_t sz) {
	size_t i, col, cs, j;
	const uint8_t *p = (uint8_t*)buff;
	cs = col = 0;
	for(i=0; i<sz; i++) {
		if(col == 16) {
			if(col != 0) {
				for(j=0; j<((3*(16 - col))+4); j++)
					fprintf(outf, " ");
				for(j=0; j<col; j++)
					fprintf(outf, "%c", p[cs+j] < ' ' ? '.' : p[cs+j]);
			}
			fprintf(outf, "\n");
			cs = i;
			col = 0;
		}
		fprintf(outf, "%02x ", p[i]);
		col++;
	}
	if(col != 0) {
		for(j=0; j<((3*(16 - col))+4); j++)
			fprintf(outf, " ");
		for(j=0; j<col; j++)
			fprintf(outf, "%c", p[cs+j] < ' ' ? '.' : p[cs+j]);
		fprintf(outf, "\n");
	}
}

