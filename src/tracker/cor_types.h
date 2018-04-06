// Written by Eagle Jones
// Copyright (c) 2012-2015, RealityCap
// All rights reserved.
//

#ifndef __COR_TYPES_H
#define __COR_TYPES_H

#include <stdint.h>
#include <stdbool.h>

//#define F_T_IS_DOUBLE
#include <float.h>
#ifdef F_T_IS_DOUBLE
typedef double f_t;
#define F_T_EPS DBL_EPSILON
#else
typedef float f_t;
#define F_T_EPS FLT_EPSILON
#endif

//need infinity for eg, to show if a feature is dropped
#include <math.h>

#endif
