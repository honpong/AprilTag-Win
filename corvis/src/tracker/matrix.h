// Copyright (c) 2008-2012, Eagle Jones
// Copyright (c) 2012-2015 RealityCap, Inc
// All rights reserved.
//

#ifndef __MATRIX_2_H
#define __MATRIX_2_H

#include "cor_types.h"
#include <assert.h>
#include <string.h>
#include "vec4.h"
#include <cstddef>

//typedef Eigen::Matrix<f_t, Eigen::Dynamic, Eigen::Dynamic> matrixtype;
//typedef Eigen::Map<matrixtype, Eigen::Aligned, Eigen::OuterStride<Eigen::Dynamic>> matrix_map;

class matrix {
protected:
    f_t *storage;
    
    int _rows;
    int _cols;
    int stride;
    int maxrows;
    f_t *data;

public:
    f_t * Data() { return data; }
    const f_t * Data() const { return data; }
    int Maxrows() { return maxrows; }
    int get_stride() const { return stride; }
    int rows() const { return _rows; }
    int cols() const { return _cols; }
    matrix row(const int r) { return matrix(data + r * stride, _cols); }
    void resize(const int size) {assert(size <= stride && _rows == 1); _cols = size; }
    void resize(const int r, const int c) { assert(c <= stride && r <= maxrows); _rows = r; _cols = c; }
    f_t &operator[] (const int i)       { assert(i >= 0 && i < _cols && _rows == 1); return data[i]; }
    f_t &operator[] (const int i) const { assert(i >= 0 && i < _cols && _rows == 1); return data[i]; }
    f_t &operator() (const int i, const int j)       { assert(i >= 0 && j >= 0 && i < _rows && j < _cols); return data[i * stride + j]; }
    f_t &operator() (const int i, const int j) const { assert(i >= 0 && j >= 0 && i < _rows && j < _cols); return data[i * stride + j]; }
    struct row_segment {
        const matrix &m;
        const int r, c, s;
        row_segment(const matrix &m_, int r_, int c_, int s_) : m(m_), r(r_), c(c_), s(s_) {
            assert(0 <= r   && r   <  m._rows &&
                   0 <= c   && c   <= m._cols &&
                   0 <= c+s && c+s <= m._cols);
        }
        const row_segment &operator=(f_t n) const {
            for (size_t i=0; i<s; i++) m.data[r*m.stride+c+i] = n;
            return *this;
        }
        const row_segment &operator=(const row_segment &o) const {
            assert(s == o.s);
            std::memmove(&  m.data[r   *   m.stride +   c],
                         &o.m.data[o.r * o.m.stride + o.c], o.s * sizeof(*o.m.data));
            return *this;
        }
    };
    row_segment row_segment(int row, int col, int size) const { return {*this, row, col, size}; }

 template<int R, int C>
 matrix(f_t (&d)[R][C]) : storage(NULL), _rows(0), _cols(0), stride(C), maxrows(R), data(&d[0][0]) {}
 template<int S>
 matrix(f_t (&d)[S]) : storage(NULL), _rows(1), _cols(S), stride(S), maxrows(1), data(&d[0]) {}
 matrix(f_t *d, const int r, const int c, const int mr, const int mc): storage(NULL), _rows(r), _cols(c), stride(mc), maxrows(mr), data(d) {}
 matrix(f_t *d, const int r, const int c): storage(NULL), _rows(r), _cols(c), stride(c), maxrows(r), data(d) {}
 matrix(f_t *d, const int size): storage(NULL), _rows(1), _cols(size), stride(size), maxrows(1), data(d) {}
 matrix(): storage(NULL), _rows(0), _cols(0), stride(0), maxrows(0), data(NULL) {}
 matrix(matrix &other, const int startrow, const int startcol, const int rows, const int cols): storage(NULL), _rows(rows), _cols(cols), stride(other.stride), maxrows(other.maxrows-startrow), data(&other(startrow, startcol)) {}
    
  matrix(const int size): storage(new f_t[size]), _rows(1), _cols(size), stride(size), maxrows(1), data(storage) { }
  matrix(const int nrows, const int ncols): storage(new f_t[nrows * ncols]), _rows(nrows), _cols(ncols), stride(ncols), maxrows(nrows), data((f_t *)storage) { }

    ~matrix() { if(storage) delete [] storage; }

    typedef Eigen::Map<Eigen::Matrix<f_t, Eigen::Dynamic, Eigen::Dynamic>, Eigen::Unaligned, Eigen::OuterStride<>> Map;

    Map map() const { return Map { data, _rows, _cols, Eigen::OuterStride<>(stride) }; }

    void print() const;
    void print_high() const;
    void print_diag() const;
    bool identical(const matrix &other, f_t epsilon) const;

    friend void matrix_product(matrix &res, const matrix &A, const matrix &B, bool trans1, bool trans2, const f_t dst_scale, const f_t scale);
    friend f_t matrix_check_condition(matrix &A);
    friend bool matrix_solve(matrix &A, matrix &B);
    friend bool test_posdef(const matrix &m);

    friend class covariance;
    friend class state_motion;
};

void matrix_product(matrix &res, const matrix &A, const matrix &B, bool trans1 = false, bool trans2 = false, const f_t dst_scale = 0.0, const f_t scale = 1.0);
f_t matrix_check_condition(matrix &A);
bool matrix_solve(matrix &A, matrix &B);
bool test_posdef(const matrix &m);

#ifdef DEBUG
//#define TEST_POSDEF
#endif

#endif
