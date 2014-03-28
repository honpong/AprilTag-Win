// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#ifndef __MATRIX_2_H
#define __MATRIX_2_H

#include "cor_types.h"
#include <assert.h>
#include "vec4.h"

class matrix {
protected:
    v_intrinsic *storage;
    
public:
    int rows;
    int cols;
    int stride;
    int maxrows;
    f_t *data;

    /*    class const_index {
    public:
        const matrix &m;
        const int i, j, rows, cols;

    const_index(const matrix &mat, const int a, const int b, const int c, const int d): m(mat), i(a), j(b), rows(c), cols(d) {
            assert(i < m.rows && j < m.cols && i + rows <= m.rows && j + cols <= m.cols);
        }

        operator v4() const {
            v4 v;
            assert(rows == 1 || cols == 1);
            assert(rows <= 4 && cols <= 4);
            int el = 0;
            for(int ix = i; ix < i + rows; ++ix) {
                for(int jx = j; jx < j + cols; ++jx) {
                    v[el++] = m(ix, jx);
                }
            }
            return v;
        }

        operator m4() const {
            m4 res;
            assert(rows <= 4 && cols <= 4);
            for(int ix = 0; ix < rows; ++ix) {
                for(int jx = 0; jx < cols; ++jx) {
                    res[ix][jx] = m(i+ix, j+jx);
                }
            }
            return res;
        }
    };

    class index {
    public:
        matrix &m;
        const int i, j, rows, cols;

        index(matrix &mat, const int a, const int b, const int c, const int d): m(mat), i(a), j(b), rows(c), cols(d) {
            assert(i < m.rows && j < m.cols && i + rows <= m.rows && j + cols <= m.cols);
        }

        index &operator=(const v4 &v) {
            assert(rows == 1 || cols == 1);
            assert(rows <= 4 && cols <= 4);
            int el = 0;
            for(int ix = i; ix < i + rows; ++ix) {
                for(int jx = j; jx < j + cols; ++jx) {
                    m(ix, jx) = v[el++];
                }
            }
            return *this;
        }
  
        index &operator=(const_index &other) {
            assert(rows == other.rows && cols == other.cols);
            for(int ix = 0; ix < rows; ++ix) {
                for(int jx = 0; jx < cols; ++jx) {
                    m(i+ix, j+jx) = other.m(other.i+ix, other.j+jx);
                }
            }
            return *this;
        }

        index &operator=(index &other) {
            assert(rows == other.rows && cols == other.cols);
            for(int ix = 0; ix < rows; ++ix) {
                for(int jx = 0; jx < cols; ++jx) {
                    m(i+ix, j+jx) = other.m(other.i+ix, other.j+jx);
                }
            }
            return *this;
        }

        index &operator=(const m4 &other) {
            assert(rows <= 4 && cols <= 4);
            for(int ix = 0; ix < rows; ++ix) {
                for(int jx = 0; jx < cols; ++jx) {
                    m(i+ix, j+jx) = other[ix][jx];
                }
            }
            return *this;
            }
            };*/

    void clear() { memset(data, 0, rows * stride * sizeof(f_t)); }
    void resize(const int c) {assert(c <= stride && rows == 1); cols = c; }
    void resize(const int r, const int c) { assert(c <= stride && r <= maxrows); rows = r; cols = c; }
#ifdef DEBUG
    f_t &operator[] (const int i) { assert(i >= 0 && i < cols && rows == 1); return data[i]; }
    const f_t &operator[] (const int i) const { assert(i >= 0 && i < cols && rows == 1); return data[i]; }
    f_t &operator() (const int i, const int j) { assert(i >= 0 && j >= 0 && i < rows && j < cols); return data[i * stride + j]; }
    const f_t &operator() (const int i, const int j) const { assert(i >= 0 && j >= 0 && i < rows && j < cols); return data[i * stride + j]; }
#else
    inline f_t &operator[] (const int i) { return data[i]; }
    inline const f_t &operator[] (const int i) const { return data[i]; }
    inline f_t &operator() (const int i, const int j) { return data[i * stride + j]; }
    inline const f_t &operator() (const int i, const int j) const { return data[i * stride + j]; }
#endif

//index operator() (const int ix, const int jx, const int r, const int c) { return index(*this, ix, jx, r, c); }
//const const_index operator() (const int ix, const int jx, const int r, const int c) const { return const_index(*this, ix, jx, r, c); }
    
 matrix(f_t *d, const int r, const int c, const int mr, const int mc): storage(NULL), rows(r), cols(c), stride(mc), maxrows(mr), data(d) {}
 matrix(f_t *d, const int r, const int c): storage(NULL), rows(r), cols(c), stride(c), maxrows(r), data(d) {}
 matrix(f_t *d, const int size): storage(NULL), rows(1), cols(size), stride(size), maxrows(1), data(d) {}
 matrix(): storage(NULL), rows(0), cols(0), stride(0), maxrows(0), data(NULL) {}
 matrix(matrix &other, const int startrow, const int startcol, const int rows, const int cols): storage(NULL), rows(rows), cols(cols), data(&other(startrow, startcol)), stride(other.stride), maxrows(rows) {}
    
  matrix(const int nrows, const int ncols): storage(new v_intrinsic[nrows * ((ncols+3)/4)]), rows(nrows), cols(ncols), stride(((ncols+3)/4)*4), maxrows(nrows), data((f_t *)storage) { }

    ~matrix() { if(storage) delete [] storage; }
    bool is_symmetric(f_t eps) const;
    void print() const;
    void print_high() const;
    void print_diag() const;
};

matrix &matrix_dereference(matrix *m);

void matrix_product(matrix &res, const matrix &A, const matrix &B, bool trans1 = false, bool trans2 = false, const f_t dst_scale = 0.0, const f_t scale = 1.0);
bool matrix_svd(matrix &A, matrix &U, matrix &S, matrix &Vt);
bool matrix_invert(matrix &m);
void matrix_transpose(matrix &dst, const matrix &src);
bool matrix_cholesky(matrix &A);
f_t matrix_check_condition(matrix &A);
bool matrix_is_symmetric(matrix &m);
bool matrix_solve(matrix &A, matrix &B);
bool matrix_solve_svd(matrix &A, matrix &B);
bool matrix_solve_syt(matrix &A, matrix &B);
bool matrix_solve_extra(matrix &A, matrix &B);
bool matrix_solve_refine(matrix &A, matrix &B);

bool test_posdef(const matrix &m);

#ifdef DEBUG
//#define TEST_POSDEF
#endif

#endif
