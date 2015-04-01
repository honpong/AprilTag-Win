// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#ifndef __vec4_H
#define __vec4_H

#ifdef __APPLE__
#include <Accelerate/Accelerate.h>
#endif
extern "C" {
#ifndef __APPLE__
#include <xmmintrin.h>
#endif
#include "../cor/cor_types.h"
#include "stdio.h"
}

#include <iostream>

class v4 {
 public:
    //initializers
 v4(): data((v_intrinsic){ 0., 0., 0., 0.}) {}
 v4(const v_intrinsic &other): data(other) {}
 v4(const f_t other[4]): data((v_intrinsic) {other[0], other[1], other[2], other[3]}) {}
 v4(const f_t other0, const f_t other1, const f_t other2, const f_t other3): data((v_intrinsic) { other0, other1, other2, other3 }) {}
 v4(const f_t other): data((v_intrinsic) {other, other, other, other}) {}
#ifdef __APPLE__
#ifdef TARGET_OS_IPHONE
    v4(const vFloat &other): data((v_intrinsic){other[0], other[1], other[2], other[3]}) {}
#endif
    operator vFloat() { return (vFloat){(float)(*this)[0], (float)(*this)[1], (float)(*this)[2], (float)(*this)[3]}; }
#endif
    v4 & operator=(const v_intrinsic &other) { data = other; return *this; }
    //member access
    f_t & operator[](const int i) { return ((f_t *)&data)[i]; }
    const f_t & operator[](const int i) const { return ((f_t *)&data)[i]; }

    f_t absmax() const {
        f_t max = fabs((*this)[0]) > fabs((*this)[1]) ? fabs((*this)[0]) : fabs((*this)[1]);
        max = max > fabs((*this)[2]) ? max : fabs((*this)[2]);
        return max;
    }

    v_intrinsic data;
};

//v4 math
static inline v4 operator*(const v4 &a, const f_t other) { return v4(a.data * v4(other).data); }
static inline v4 operator*(const v4 &a, const v4 &other) { return v4(a.data * other.data); }
static inline v4 operator/(const v4 &a, const v4 &other) { return v4(a.data / other.data); }
static inline v4 operator+(const v4 &a, const v4 &other) { return v4(a.data + other.data); }
static inline v4 operator-(const v4 &a, const v4 &other) { return v4(a.data - other.data); }
static inline v4 &operator*=(v4 &a, const f_t other) { a.data *= v4(other).data; return a; }
static inline v4 &operator*=(v4 &a, const v4 &other) { a.data *= other.data; return a; }
static inline v4 &operator/=(v4 &a, const v4 &other) { a.data /= other.data; return a; }
static inline v4 &operator+=(v4 &a, const v4 &other) { a.data += other.data; return a; }
static inline v4 &operator-=(v4 &a, const v4 &other) { a.data -= other.data; return a; }
static inline f_t sum(const v4 &v) { return v[0] + v[1] + v[2] + v[3]; }
static inline f_t norm(const v4 &v) { return sqrt(sum(v*v)); }
static inline v4 normalize(const v4 x) { return x / norm(x); }
static inline v4 v4_sqrt(const v4 &v) { return v4(sqrt(v[0]), sqrt(v[1]), sqrt(v[2]), sqrt(v[3])); }
static inline v4 operator-(const v4 &v) { return v4(-v.data); }
static inline bool operator==(const v4 &a, const v4 &b) { return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3]; }
static inline std::ostream& operator<<(std::ostream &stream, const v4 &v)
{
    return stream  << "(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
}

static inline f_t dot(const v4 &a, const v4 &b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static inline v4 cross(const v4 &a, const v4 &b) {
    return v4(a[1] * b[2] - a[2] * b[1],
              a[2] * b[0] - a[0] * b[2],
              a[0] * b[1] - a[1] * b[0],
              0);
}

class stdev_vector
{
public:
    v4 sum, mean, M2;
    f_t max;
    v4 variance, stdev;
    uint64_t count;
    stdev_vector(): sum(0.), mean(0.), M2(0.), max(0.), variance(0.), stdev(0.), count(0) {}
    void data(const v4 &x) {
        ++count;
        v4 delta = x - mean;
        mean = mean + delta / count;
        M2 = M2 + delta * (x - mean);
        if(norm(x) > max) max = norm(x);
        variance = M2 / (count - 1);
        stdev = v4(sqrt(variance[0]), sqrt(variance[1]), sqrt(variance[2]), sqrt(variance[3]));
    }
};

static inline std::ostream& operator<<(std::ostream &stream, const stdev_vector &v)
{
    return stream << "mean is: " << v.mean << ", stdev is: " << v.stdev << ", max is: " << v.max << std::endl;
}

class stdev_scalar
{
public:
    double sum, mean, M2;
    f_t max;
    f_t variance, stdev;
    uint64_t count;
    stdev_scalar(): sum(0.), mean(0.), M2(0.), max(0.), variance(0.), stdev(0.), count(0) {}
    void data(const f_t &x) {
        ++count;
        double delta = x - mean;
        mean = mean + delta / count;
        M2 = M2 + delta * (x - mean);
        if(x > max) max = x;
        variance = M2 / (count - 1);
        stdev = sqrt(variance);
    }
};

static inline std::ostream& operator<<(std::ostream &stream, const stdev_scalar &s)
{
    return stream << "mean is: " << s.mean << ", stdev is: " << s.stdev << ", max is: " << s.max << std::endl;
}

class v4_lowpass {
public:
    v4 filtered;
    f_t constant;
    v4_lowpass(f_t rate, f_t cutoff) { constant  = 1. / (1. + rate/cutoff); }
    v4 sample(const v4 &data) { return filtered = filtered * (1. - constant) + data * constant; }
};

static inline v4 relative_rotation(const v4 &first, const v4 &second)
{
    v4 rv = cross(first/norm(first), second/norm(second));
    f_t sina = norm(rv);
    return rv / sina * asin(sina);
}

class m4 {
 public:
    //v4 & operator[](const int i) { return data[i]; }
    //const v4 & operator[](const int i) const { return data[i]; }
    const f_t& operator()(const int i, const int j) const { return data[i][j]; }
    f_t& operator()(const int i, const int j) { return data[i][j]; }
    v4 col(const int c) const { return v4( (v_intrinsic) { data[0][c], data[1][c], data[2][c], data[3][c] }); }
    
    v4& row(const int i) { return data[i]; }
    const v4& row(const int i) const { return data[i]; }

    v4 data[4];
};

static inline bool operator==(const m4 &a, const m4 &b)
{
    bool res = true;
    for(int i = 0; i < 4; ++i) res &= (a.row(i) == b.row(i));
    return res;
}

static inline std::ostream& operator<<(std::ostream &stream, const m4 &m)
{
    return stream  << "(" << m.row(0) << "\n " << m.row(1) << "\n " << m.row(2) << "\n " << m.row(3) << ")";
}

static inline m4 operator*(const m4 &a, const f_t s)  {
    return (m4) { { a.row(0) * s, a.row(1) * s, a.row(2) * s, a.row(3) * s }};
}

static inline m4 operator*(const f_t s, const m4 &a)  {
    return (m4) { { a.row(0) * s, a.row(1) * s, a.row(2) * s, a.row(3) * s }};
}

static inline m4 operator*(const m4 &b, const m4 &other) {
    m4 a;
    for(int j = 0; j < 4; ++j) {
        const v4 col = other.col(j);
        for(int i = 0; i < 4; ++i) {
            a(i, j) = sum(b.row(i) * col);
        }
    }
    return a;
}

static inline v4 operator*(const m4 &a, const v4 &other) {
    return v4( sum(a.row(0) * other),
               sum(a.row(1) * other),
               sum(a.row(2) * other),
               sum(a.row(3) * other));
}

static inline m4 operator+(const m4 &a, const m4 &c) {
    return (m4) { {
            a.row(0) + c.row(0),
            a.row(1) + c.row(1),
            a.row(2) + c.row(2),
            a.row(3) + c.row(3) }
    };
}

static inline m4 operator-(const m4 &a, const m4 &c) {
    return (m4) { {
            a.row(0) - c.row(0),
            a.row(1) - c.row(1),
            a.row(2) - c.row(2),
            a.row(3) - c.row(3) }
    };
}


extern m4 const m4_identity;

static inline f_t norm(const m4 &m) { return sqrt(sum(m.row(0)*m.row(0) + m.row(1)*m.row(1) + m.row(2)*m.row(2) + m.row(3)*m.row(3))); }

static inline m4 operator-(const m4 &m) {
    return (m4) { {
                -m.data[0],
                -m.data[1],
                -m.data[2],
                -m.data[3] }
    };
}

class m4v4 {
 public:
    m4 & operator[](const int i) { return data[i]; }
    const m4 & operator[](const int i) const { return data[i]; }

    m4v4 operator+(const m4v4 &c) const {
        return (m4v4) { { data[0] + c.data[0], data[1] + c.data[1], data[2] + c.data[2], data[3] + c.data[3] } };
    }
    m4v4 operator*(const f_t c) const {
        return (m4v4) { { data[0] * c, data[1] * c, data[2] * c, data[3] * c } };
    }

    m4 data[4];
};

static inline std::ostream& operator<<(std::ostream &stream, const m4v4 &v)
{
    return stream << "[" <<
    v.data[0] << ",\n" <<
    v.data[1] << ",\n" <<
    v.data[2] << ",\n" <<
    v.data[3] << "]\n";
}

static inline bool operator==(const m4v4 &a, const m4v4 &b)
{
    bool res = true;
    for(int i = 0; i < 4; ++i) res &= (a[i] == b[i]);
    return res;
}

class v4m4 {
 public:
    m4 & operator[](const int i) { return data[i]; }
    const m4 & operator[](const int i) const { return data[i]; }
    v4m4 operator+(const v4m4 &c) const {
        return (v4m4) { { data[0] + c.data[0], data[1] + c.data[1], data[2] + c.data[2], data[3] + c.data[3] } };
    }
    v4m4 operator*(const f_t c) const {
        return (v4m4) { { data[0] * c, data[1] * c, data[2] * c, data[3] * c } };
    }

    m4 data[4];
};

static inline std::ostream& operator<<(std::ostream &stream, const v4m4 &v)
{
    return stream << "[" <<
    v.data[0] << ",\n" <<
    v.data[1] << ",\n" <<
    v.data[2] << ",\n" <<
    v.data[3] << "]\n";
}

extern m4v4 const skew3_jacobian;
extern v4m4 const invskew3_jacobian;

class m4m4 {
 public:
    v4m4 & operator[](const int i) {return data[i]; }
    const v4m4 & operator[](const int i) const {return data[i]; }

    v4m4 data[4];
};

inline static m4 outer_product(const v4 &b, const v4 &c)
{
    return (m4) {{ b * c[0], b * c[1], b * c[2], b * c[3] }};
}

inline static m4v4 outer_product(const v4 &b, const m4 &c)
{
    return (m4v4) {{ outer_product(b, c.row(0)), outer_product(b, c.row(1)), outer_product(b, c.row(2)), outer_product(b, c.row(3)) }};
}

inline static v4m4 outer_product(const m4 &b, const v4 &c)
{
    return (v4m4) {{ b * c[0], b * c[1], b * c[2], b * c[3] }};
}

inline static m4 transpose(const m4 &b)
{
    return (m4) {{ b.col(0), b.col(1), b.col(2), b.col(3) }};
}

inline static m4v4 transpose(const m4v4 &b)
{
    return (m4v4) {{
            { {b[0].row(0), b[1].row(0), b[2].row(0), b[3].row(0)} },
                { {b[0].row(1), b[1].row(1), b[2].row(1), b[3].row(1)} },
                    { {b[0].row(2), b[1].row(2), b[2].row(2), b[3].row(2)} },
                        { {b[0].row(3), b[1].row(3), b[2].row(3), b[3].row(3)} }}};
}

//a[i][j][:] = vecsum(b[i][...][:] * (scalar->vec)c[...][j])
inline static m4v4 operator*(const m4v4 &b, const m4 &c)
{
    m4v4 a;
    for(int j = 0; j < 4; ++j) {
        v4
            t0(c(0, j)),
            t1(c(1, j)),
            t2(c(2, j)),
            t3(c(3, j));
        for(int i = 0; i < 4; ++i) {
            a[i].row(j) =
                b[i].row(0) * t0 +
                b[i].row(1) * t1 +
                b[i].row(2) * t2 +
                b[i].row(3) * t3;
        }
    }
    return a;
}

//a[i][j][:] = vecsum((scalar->vec)b[i][...] * c[...][j])
inline static m4v4 operator*(const m4 &b, const m4v4 &c)
{
    m4v4 a;
    for(int i = 0; i < 4; ++i) {
        v4
            t0(b(i, 0)),
            t1(b(i, 1)),
            t2(b(i, 2)),
            t3(b(i, 3));
        for(int j = 0; j < 4; ++j) {
            a[i].row(j) =
                t0 * c[0].row(j) +
                t1 * c[1].row(j) +
                t2 * c[2].row(j) +
                t3 * c[3].row(j);
        }
    }
    return a;
}

//a_k,... = sum_i(sum_j(b[k][i][j] * c[i][j][...]))
inline static m4 operator*(const v4m4 &b, const m4v4 &c)
{
    m4 a;
    for(int k = 0; k < 4; ++k) {
        for(int i = 0; i < 4; ++i) {
            for(int j = 0; j < 4; ++j) {
                a.row(k) += c[i].row(j) * b[k](i, j);
            }
        }
    }
    return a;
}

//a[i][:] = vecsum((scalar->vec)b[...] * c[i][...][:])
/*inline static m4 operator*(const v4 &b, const m4v4 &c)
{
    m4 a;
    v4
        t0(b[0]),
        t1(b[1]),
        t2(b[2]),
        t3(b[3]);
    
    for(int i = 0; i < 4; ++i) {
        a[i] = 
            t0 * c[0][i] +
            t1 * c[1][i] +
            t2 * c[2][i] +
            t3 * c[3][i];
    }
    return a;
}*/

//right-multiply row vector * matrix -> same as matrix^T * (col vector)
inline static v4 operator*(const v4 &b, const m4 &c)
{
    return
        c.row(0) * b[0] +
        c.row(1) * b[1] +
        c.row(2) * b[2] +
        c.row(3) * b[3];
}


//a[i] = vecsum(c[i][...][j] * b[...])
//inline static void m4v4_v4_dot(m4 *a, m4v4 *b, v4 *c)
//{
//}

//a[i][:] = vecsum((scalar->vec)b[...] * c[i][...][:])
inline static m4 operator*(const m4v4 &b, const v4 &c)
{
    m4 a;
    v4
        t0(c[0]),
        t1(c[1]),
        t2(c[2]),
        t3(c[3]);
    
    for(int i = 0; i < 4; ++i) {
        a.row(i) =
            b[i].row(0) * t0 +
            b[i].row(1) * t1 +
            b[i].row(2) * t2 +
            b[i].row(3) * t3;
    }
    return a;
}

inline static m4 apply_jacobian_m4v4(const m4v4 &b, const v4 &c)
{
    m4 a;
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            a(i, j) = sum(b[i].row(j) * c);
        }
    }
    return a;
}

inline static v4 apply_jacobian_v4m4(const v4m4 &b, const m4 &c)
{
    v4 a;
    for(int i = 0; i < 4; ++i) {
        a[i] = sum(b[i].row(0) * c.row(0) + b[i].row(1) * c.row(1) + b[i].row(2) * c.row(2) + b[i].row(3) * c.row(3));
    }
    return a;
}

inline static m4 diag(const v4 &b)
{ 
    return (m4) { {
            v4(b[0], 0., 0., 0.),
            v4(0., b[1], 0., 0.),
            v4(0., 0., b[2], 0.),
            v4(0., 0., 0., b[3]) }
    };
}

inline static v4 m4_diag(const m4 b)
{
    return v4(b(0, 0), b(1, 1), b(2, 2), b(3, 3));
}

static inline f_t trace3(const m4 R)
{ 
    return R(0, 0) + R(1, 1) + R(2, 2);
}

inline static m4 skew3(const v4 &v)
{
    m4 V;
    V(1, 2) = -(V(2, 1) = v[0]);
    V(2, 0) = -(V(0, 2) = v[1]);
    V(0, 1) = -(V(1, 0) = v[2]);
    return V;
}

inline static v4 invskew3(const m4 &V)
{
    return v4(.5 * (V(2, 1) - V(1, 2)),
              .5 * (V(0, 2) - V(2, 0)),
              .5 * (V(1, 0) - V(0, 1)),
              0);
}

inline static f_t determinant_minor(const m4 &m, const int a, const int b)
{
    return (m(1, a) * m(2, b) - m(1, b) * m(2, a));
}

inline static f_t determinant3(const m4 &m)
{
    return m(0, 0) * determinant_minor(m, 1, 2)
    - m(0, 1) * determinant_minor(m, 0, 2)
    + m(0, 2) * determinant_minor(m, 0, 1);
}

m4 rodrigues(const v4 W, m4v4 *dR_dW);

v4 integrate_angular_velocity(const v4 &W, const v4 &w);
void linearize_angular_integration(const v4 &W, const v4 &w, m4 &dW_dW, m4 &dW_dw);

/*
    a->v[i][j] = sum(b[i][:] * c[:][j])


    for(int k = 0; k < 4; ++k) {
        m4 col = m4v4_col(b, k);
        for(int j = 0; j < 4; ++j) {
            v4 col2 = m4_col(c, j);
            for(int i = 0; i < 4; ++i) {
                v4 s;
                s.v = col->v[i] * col2.v;
                a->f[i][j][k] = v4_sum(&s);
            }
        }
        return a;
    }
    }*/
#endif
