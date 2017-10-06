#include "blis_buffers.h"

/* Buffer for A matrix */
float ALIGNED(16) a_buff[A_BUFF_SIZE];
/* Buffer for B matrix */
float ALIGNED(16) b_buff[2][B_BUFF_SIZE];
/* Buffer for C matrix */
float ALIGNED(16) c_buff[3][C_BUFF_SIZE];
/* Temporary C buffer for edge cases. */
float ALIGNED(16) ct[MR * NR];

