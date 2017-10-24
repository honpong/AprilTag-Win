#ifndef _MSC_VER
__asm__(
#ifdef __APPLE__
    ".const_data\n"
#define _ "_"
#elif defined(MYRIAD2)
    ".section .ddr.data\n"
#define _ ""
#else
    ".section .rodata\n"
#define _ ""
#endif
    ".global " _"voc" "\n"
    _"voc:\n"
    ".incbin " "\"" VOC_DIR "voc.bin\"" "\n"
    ".global " _"voc_size" "\n"
    _"voc_size:\n"
    "1:\n"
    ".int 1b - " _"voc" "\n"
);
#endif
