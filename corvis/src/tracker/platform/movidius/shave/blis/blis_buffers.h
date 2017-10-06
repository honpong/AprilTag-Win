#ifndef __BUFFERS_H__
#define __BUFFERS_H__

#include <mv_types.h>
#include "blis_defines.h"

#define MR              4
#define NR              4
#define PACKMR          4
#define PACKNR          4

#define A_BUFF_SIZE (4 * MAX_K * MAX_M_ITER)
#define B_BUFF_SIZE (4 * MAX_K)
#define C_BUFF_SIZE (4 *     4 * MAX_M_ITER)

extern float ALIGNED(16) a_buff[A_BUFF_SIZE];
extern float ALIGNED(16) b_buff[2][B_BUFF_SIZE];
extern float ALIGNED(16) c_buff[3][C_BUFF_SIZE];

/* Temporary C buffer for edge cases. */
extern float ALIGNED(16) ct[MR * NR];

#endif /*__BUFFERS_H__*/
