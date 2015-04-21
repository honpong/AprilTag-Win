// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#ifndef __COR_TYPES_H
#define __COR_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int width, height;
} cor_size_t;

typedef struct {
    cor_size_t size;
    uint8_t *data;
} image_t;

//note use of unnamed union member: gcc allows us to access x and y directly.
//v is the vectorized version.

typedef struct {
    float x,y;
} feature_t;

typedef struct {
    float x, y, cx, cy, cxy;
} feature_covariance_t;

typedef struct {
    int size;
    feature_t *data;
} feature_vector_t;

typedef struct {
    int size;
    feature_covariance_t *data;
} feature_covariance_vector_t;

typedef struct {
    int size;
    float *data;
} point3d_vector_t;

typedef struct {
    int size;
    float *data;
} float_vector_t;

typedef struct {
    int size;
    uint8_t *data;
} char_vector_t;

typedef struct {
    int size;
    uint16_t *data;
} short_vector_t;

typedef struct {
    int size;
    uint64_t *data;
} uint64_vector_t;

#define F_T_IS_DOUBLE
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
