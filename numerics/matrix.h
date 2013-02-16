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
 public:
    int rows;
    int cols;
    int stride;
    int maxrows;
    f_t *data;

    class const_index {
    public:
        const matrix &m;
        const int i, j, rows, cols;

    const_index(const matrix &mat, const int a, const int b, const int c, const int d): m(mat), i(a), j(b), rows(c), cols(d) {
            assert(i < m.rows && j < m.cols && i + rows < m.rows && j + cols < m.cols);
        }

        operator v4() {
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

        operator m4() {
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
            assert(i < m.rows && j < m.cols && i + rows < m.rows && j + cols < m.cols);
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
    };

    void resize(const int c) {assert(c <= stride && rows == 1); cols = c; }
    void resize(const int r, const int c) { assert(c <= stride && r <= maxrows); rows = r; cols = c; }
    f_t &operator[] (const int i) { assert(i < cols && rows == 1); return data[i]; }
    const f_t &operator[] (const int i) const { assert(i < cols && rows == 1); return data[i]; }
    f_t &operator() (const int i, const int j) { assert(i < rows && j < cols); return data[i * stride + j]; }
    const f_t &operator() (const int i, const int j) const { assert(i < rows && j < cols); return data[i * stride + j]; }

    index operator() (const int ix, const int jx, const int r, const int c) { return index(*this, ix, jx, r, c); }
    const const_index operator() (const int ix, const int jx, const int r, const int c) const { return const_index(*this, ix, jx, r, c); }
    
 matrix(f_t *d, const int r, const int c, const int mr, const int mc): rows(r), cols(c), stride(mc), maxrows(mr), data(d) {}
 matrix(f_t *d, const int r, const int c): rows(r), cols(c), stride(c), maxrows(r), data(d) {}
 matrix(f_t *d, const int size): rows(1), cols(size), stride(size), maxrows(1), data(d) {}
 matrix(): rows(0), cols(0), stride(0), maxrows(0), data(NULL) {}
    
    bool is_symmetric(f_t eps) const;
    void print() const;
    void print_high() const;
    void print_diag() const;
};

#define MAT_TEMP(name, nrows, ncols)                    \
    assert(ncols > 1 /*vectors are always rows!*/);              \
    v_intrinsic name##_data [((nrows+3)/4)*4 * ((ncols+3)/4)];   \
    matrix name((f_t*)name##_data, nrows, ncols, ((nrows + 3)/4)*4, ((ncols+3)/4)*4)

matrix &matrix_dereference(matrix *m);

void matrix_product(matrix &res, const matrix &A, const matrix &B, bool trans1 = false, bool trans2 = false, const f_t dst_scale = 0.0, const f_t scale = 1.0);
void matrix_svd(matrix &A, matrix &U, matrix &S, matrix &Vt);
void matrix_invert(matrix &m);
void matrix_transpose(matrix &dst, matrix &src);
void matrix_cholesky(matrix &A);
bool matrix_is_symmetric(matrix &m);
void matrix_solve(matrix &A, matrix &B);
void matrix_solve_svd(matrix &A, matrix &B);
void matrix_solve_syt(matrix &A, matrix &B);

#endif
