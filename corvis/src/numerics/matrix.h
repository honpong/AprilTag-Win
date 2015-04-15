// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#ifndef __MATRIX_2_H
#define __MATRIX_2_H

#include "../cor/cor_types.h"
#include <assert.h>
#include <string.h>
#include "vec4.h"

class matrix {
protected:
    v_intrinsic *storage;
    
    int _rows;
    int _cols;
    int stride;
    int maxrows;
    f_t *data;

public:
    int get_stride() const { return stride; }
    int rows() const { return _rows; }
    int cols() const { return _cols; }
    void resize(const int c) {assert(c <= stride && _rows == 1); _cols = c; }
    void resize(const int r, const int c) { assert(c <= stride && r <= maxrows); _rows = r; _cols = c; }
#ifdef DEBUG
    f_t &operator[] (const int i) { assert(i >= 0 && i < _cols && _rows == 1); return data[i]; }
    const f_t &operator[] (const int i) const { assert(i >= 0 && i < _cols && _rows == 1); return data[i]; }
    f_t &operator() (const int i, const int j) { assert(i >= 0 && j >= 0 && i < _rows && j < _cols); return data[i * stride + j]; }
    const f_t &operator() (const int i, const int j) const { assert(i >= 0 && j >= 0 && i < _rows && j < _cols); return data[i * stride + j]; }
#else
    inline f_t &operator[] (const int i) { return data[i]; }
    inline const f_t &operator[] (const int i) const { return data[i]; }
    inline f_t &operator() (const int i, const int j) { return data[i * stride + j]; }
    inline const f_t &operator() (const int i, const int j) const { return data[i * stride + j]; }
#endif
    
 matrix(f_t *d, const int r, const int c, const int mr, const int mc): storage(NULL), _rows(r), _cols(c), stride(mc), maxrows(mr), data(d) {}
 matrix(f_t *d, const int r, const int c): storage(NULL), _rows(r), _cols(c), stride(c), maxrows(r), data(d) {}
 matrix(f_t *d, const int size): storage(NULL), _rows(1), _cols(size), stride(size), maxrows(1), data(d) {}
 matrix(): storage(NULL), _rows(0), _cols(0), stride(0), maxrows(0), data(NULL) {}
 matrix(matrix &other, const int startrow, const int startcol, const int rows, const int cols): storage(NULL), _rows(rows), _cols(cols), stride(other.stride), maxrows(rows), data(&other(startrow, startcol)) {}
    
  matrix(const int nrows, const int ncols): storage(new v_intrinsic[nrows * ((ncols+3)/4)]), _rows(nrows), _cols(ncols), stride(((ncols+3)/4)*4), maxrows(nrows), data((f_t *)storage) { }

    matrix(const matrix &other): storage(new v_intrinsic[other._rows * ((other._cols+3)/4)]), _rows(other._rows), _cols(other._cols), stride(((other._cols+3)/4)*4), maxrows(other._rows), data((f_t *)storage)
    { *this = other; }
    
    matrix &operator=(const matrix &other)
    {
        assert(_rows == other._rows && _cols == other._cols);
        for(int i = 0; i < _rows; ++i) {
            memcpy(data + i * stride, other.data + i * other.stride, _cols * sizeof(f_t));
        }
        return *this;
    }
    
    ~matrix() { if(storage) delete [] storage; }

    bool is_symmetric(f_t eps) const;
    void print() const;
    void print_high() const;
    void print_diag() const;
    
    
    
    friend void matrix_product(matrix &res, const matrix &A, const matrix &B, bool trans1, bool trans2, const f_t dst_scale, const f_t scale);
    friend bool matrix_svd(matrix &A, matrix &U, matrix &S, matrix &Vt);
    friend bool matrix_invert(matrix &m);
    friend void matrix_transpose(matrix &dst, const matrix &src);
    friend void matrix_negate(matrix & A);
    friend bool matrix_cholesky(matrix &A);
    friend void test_cholesky(matrix &A);
    friend f_t matrix_check_condition(matrix &A);
    friend bool matrix_is_symmetric(matrix &m);
    friend bool matrix_solve(matrix &A, matrix &B);
    friend bool matrix_solve_svd(matrix &A, matrix &B);
    friend bool matrix_solve_syt(matrix &A, matrix &B);
    friend bool matrix_solve_extra(matrix &A, matrix &B);
    friend bool matrix_solve_refine(matrix &A, matrix &B);
    friend f_t matrix_3x3_determinant(const matrix & A);
    
    friend bool test_posdef(const matrix &m);
    
    friend class covariance;
};

matrix &matrix_dereference(matrix *m);

void matrix_product(matrix &res, const matrix &A, const matrix &B, bool trans1 = false, bool trans2 = false, const f_t dst_scale = 0.0, const f_t scale = 1.0);
bool matrix_svd(matrix &A, matrix &U, matrix &S, matrix &Vt);
bool matrix_invert(matrix &m);
void matrix_transpose(matrix &dst, const matrix &src);
void matrix_negate(matrix & A);
bool matrix_cholesky(matrix &A);
f_t matrix_check_condition(matrix &A);
bool matrix_is_symmetric(matrix &m);
bool matrix_solve(matrix &A, matrix &B);
bool matrix_solve_svd(matrix &A, matrix &B);
bool matrix_solve_syt(matrix &A, matrix &B);
bool matrix_solve_extra(matrix &A, matrix &B);
bool matrix_solve_refine(matrix &A, matrix &B);
f_t matrix_3x3_determinant(const matrix & A);

bool test_posdef(const matrix &m);

#ifdef DEBUG
//#define TEST_POSDEF
#endif

#endif
