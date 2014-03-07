// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#ifndef __STATE_H
#define __STATE_H

extern "C" {
#include "cor_types.h"
#include "packet.h"
}
#include "../numerics/vec4.h"
#include "../numerics/matrix.h"
#include "../numerics/quaternion.h"
#include "../numerics/rotation_vector.h"
#include "../numerics/covariance.h"

#include <vector>
#include <list>
#include <iostream>
using namespace std;

//minstatesize = base (38) + 2xref (12) + full group(40) + min group (6) = 96
#define MINSTATESIZE 96
#define MAXGROUPS 8

class state_node {
public:
    state_node(): dynamic(false) {}
    bool dynamic;
    static int statesize, maxstatesize;
    virtual void copy_state_to_array(matrix &state) = 0;
    virtual void copy_state_from_array(matrix &state) = 0;
    virtual int remap(int i, covariance &cov) = 0;
    virtual void reset() = 0;
    virtual void remove() = 0;
};

template<class T> class state_branch: public state_node {
protected:
    int dynamic_statesize;
public:
    //for some reason i need this typename qualifier, including without the typedef
    typedef typename list<T>::iterator iterator;

    void copy_state_to_array(matrix &state) {
        for(iterator j = children.begin(); j != children.end(); ++j) {
            (*j)->copy_state_to_array(state);
        }
    }

    virtual void copy_state_from_array(matrix &state) {
        for(iterator j = children.begin(); j != children.end(); ++j) {
            (*j)->copy_state_from_array(state);
        }
    }
    
    int remap(int i, covariance &cov) {
        int start = i;
        for(iterator j = children.begin(); j != children.end(); ++j)  {
            if((*j)->dynamic) i = (*j)->remap(i, cov);
        }
        dynamic_statesize = i - start;
        for(iterator j = children.begin(); j != children.end(); ++j) {
            if(!(*j)->dynamic) i = (*j)->remap(i, cov);
        }
        return i;
    }

    virtual void reset() {
        for(iterator j = children.begin(); j != children.end(); ++j) {
            (*j)->reset();
        }
    }
    
    virtual void remove()
    {
        for(iterator j = children.begin(); j != children.end(); j = children.erase(j)) {
            (*j)->remove();
        }
    }
    
    void remove_child(const T n)
    {
        children.remove(n);
        n->remove();
    }
    
    list<T> children;
};

class state_root: public state_branch<state_node *> {
public:
    state_root(covariance &c): cov(c) {}
    
    covariance &cov;

    int remap() {
#ifdef TEST_POSDEF
        if(cov.size() && !test_posdef(cov.cov)) fprintf(stderr, "not pos def at beginning of remap\n");
#endif
        statesize = state_branch<state_node *>::remap(0, cov);
        cov.remap(statesize);
#ifdef TEST_POSDEF
        if(!test_posdef(cov.cov)) {
            cov.cov.print();
            fprintf(stderr, "not pos def at end of remap\n");
            assert(0);
        }
#endif
        return statesize;
    }
    
    void reset() {
        cov.resize(0);
        state_branch<state_node *>::reset();
    }
};

template <class T, int _size> class state_leaf: public state_node {
 public:
    state_leaf(): index(-1) {}

    T v;
    
    covariance *cov;
    
    void set_process_noise(f_t x)
    {
        for(int i = 0; i < size; ++i) process_noise[i] = x;
    }
    
    void set_initial_variance(f_t x)
    {
        for(int i = 0; i < size; ++i) initial_variance[i] = x;
    }
    
    virtual f_t *raw_array() = 0;
    
    void copy_state_to_array(matrix &state) {
        for(int i = 0; i < size; ++i) state[index + i] = raw_array()[i];
    }
    
    virtual void copy_state_from_array(matrix &state) {
        for(int i = 0; i < size; ++i) raw_array()[i] = state[index + i];
    }
    
    int remap(int i, covariance &cov) {
        if(index < 0) {
            int temploc = cov.add(i, size);
            for(int j = 0; j < size; ++j) {
                cov(temploc+j, temploc+j) = initial_variance[j];
                cov.process_noise[temploc+j] = process_noise[j];
            }
        } else {
            cov.reindex(i, index, size);
        }
        index = i;
        this->cov = &cov;
        return i + size;
    }
    
    void reset_covariance(covariance &covariance_m) {
        for(int i = 0; i < size; ++i) {
            if(index >= 0) {
                for(int j = 0; j < covariance_m.size(); ++j) {
                    covariance_m(index + i, j) = covariance_m(j, index + i) = 0.;
                }
                covariance_m(index + i, index + i) = initial_variance[i];
            }
        }
    }
    
    static void resize_covariance(int i, int old_i, matrix &covariance_m, matrix &process_noise_m) {
        //fix everything that came before us
        for(int j = 0; j < i; ++j) {
            covariance_m(i, j) = covariance_m(old_i, j);
            covariance_m(j, i) = covariance_m(j, old_i);
        }
        //skip the gap
        //fix the diagonal
        covariance_m(i, i) = covariance_m(old_i, old_i);
        process_noise_m[i] = process_noise_m[old_i];
        //fix everything that will come after us
        for(int j = old_i + 1; j < covariance_m.rows; ++j) {
            covariance_m(i, j) = covariance_m(old_i, j);
            covariance_m(j, i) = covariance_m(j, old_i);
        }
    }
    
    void remove() { index = -1; }
protected:
    int index;
    f_t process_noise[_size];
    f_t initial_variance[_size];
    static const int size = _size;
};

#ifdef SWIG
%template(state_leaf_vec) state_leaf<v4, 3>;
%template(state_leaf_sca) state_leaf<f_t, 1>;
%template(state_branch_node) state_branch<state_node *>;
#endif

#define PERTURB_FACTOR 1.1

class state_vector: public state_leaf<v4, 3> {
 public:
    state_vector() { reset(); }
    
    f_t *raw_array() { return (f_t *)&(v.data); }

    using state_leaf::set_initial_variance;
    
    void set_initial_variance(f_t x, f_t y, f_t z)
    {
        initial_variance[0] = x;
        initial_variance[1] = y;
        initial_variance[2] = z;
    }
    
    v4 copy_cov_from_row(const matrix &cov, const int i) const
    {
        if(index < 0) return v4(0.);
        return v4(cov(i, index), cov(i, index+1), cov(i, index+2), 0.);
    }
    
    void copy_cov_to_col(matrix &cov, const int j, const v4 &v) const
    {
        cov(index, j) = v[0];
        cov(index+1, j) = v[1];
        cov(index+2, j) = v[2];
    }

    void copy_cov_to_row(matrix &cov, const int j, const v4 &v) const
    {
        cov(j, index) = v[0];
        cov(j, index+1) = v[1];
        cov(j, index+2) = v[2];
    }

    void reset() {
        index = -1;
        v = 0.;
    }
    
    void perturb_variance() {
        cov->cov(index, index) *= PERTURB_FACTOR;
        cov->cov(index + 1, index + 1) *= PERTURB_FACTOR;
        cov->cov(index + 2, index + 2) *= PERTURB_FACTOR;
    }
    
    v4 variance() const {
        if(index < 0) return v4(initial_variance[0], initial_variance[1], initial_variance[2], 0.);
        return v4((*cov)(index, index), (*cov)(index+1, index+1), (*cov)(index+2, index+2), 0.);
    }
};

class state_rotation_vector: public state_leaf<rotation_vector, 3> {
public:
    state_rotation_vector() { reset(); }

    f_t *raw_array() { return v.raw_array(); }

    using state_leaf::set_initial_variance;
    
    void set_initial_variance(f_t x, f_t y, f_t z)
    {
        initial_variance[0] = x;
        initial_variance[1] = y;
        initial_variance[2] = z;
    }
    
    v4 copy_cov_from_row(const matrix &cov, const int i) const
    {
        if(index < 0) return v4(0.);
        return v4(cov(i, index), cov(i, index+1), cov(i, index+2), 0.);
    }
    
    void copy_cov_to_col(matrix &cov, const int j, const v4 &v) const
    {
        cov(index, j) = v[0];
        cov(index+1, j) = v[1];
        cov(index+2, j) = v[2];
    }
    
    void copy_cov_to_row(matrix &cov, const int j, const v4 &v) const
    {
        cov(j, index) = v[0];
        cov(j, index+1) = v[1];
        cov(j, index+2) = v[2];
    }
    
    void reset() {
        index = -1;
        v = rotation_vector(0., 0., 0.);
    }
    
    void perturb_variance() {
        cov->cov(index, index) *= PERTURB_FACTOR;
        cov->cov(index + 1, index + 1) *= PERTURB_FACTOR;
        cov->cov(index + 2, index + 2) *= PERTURB_FACTOR;
    }
    
    v4 variance() const {
        if(index < 0) return v4(initial_variance[0], initial_variance[1], initial_variance[2], 0.);
        return v4((*cov)(index, index), (*cov)(index+1, index+1), (*cov)(index+2, index+2), 0.);
    }
};

class state_quaternion: public state_leaf<quaternion, 4>
{
public:
    state_quaternion() { reset(); }
    
    f_t *raw_array() { return v.raw_array(); }

    using state_leaf::set_initial_variance;
    
    virtual void copy_state_from_array(matrix &state) {
        state_leaf::copy_state_from_array(state);
        normalize();
    }

    void set_initial_variance(f_t w, f_t x, f_t y, f_t z)
    {
        initial_variance[0] = w;
        initial_variance[1] = x;
        initial_variance[2] = y;
        initial_variance[3] = z;
    }
    
    v4 copy_cov_from_row(const matrix &cov, const int i) const
    {
        if(index < 0) return v4(0.);
        return v4(cov(i, index), cov(i, index+1), cov(i, index+2), cov(i, index+3));
    }
    
    void copy_cov_to_col(matrix &cov, const int j, const v4 &v) const
    {
        cov(index, j) = v[0];
        cov(index+1, j) = v[1];
        cov(index+2, j) = v[2];
        cov(index+3, j) = v[3];
    }
    
    void copy_cov_to_row(matrix &cov, const int j, const v4 &v) const
    {
        cov(j, index) = v[0];
        cov(j, index+1) = v[1];
        cov(j, index+2) = v[2];
        cov(j, index+3) = v[3];
    }

    void reset() {
        index = -1;
        v = quaternion(1., 0., 0., 0.);
    }
    
    void perturb_variance() {
        cov->cov(index, index) *= PERTURB_FACTOR;
        cov->cov(index + 1, index + 1) *= PERTURB_FACTOR;
        cov->cov(index + 2, index + 2) *= PERTURB_FACTOR;
        cov->cov(index + 3, index + 3) *= PERTURB_FACTOR;
    }
    
    v4 variance() const {
        if(index < 0) return v4(initial_variance[0], initial_variance[1], initial_variance[2], initial_variance[3]);
        return v4((*cov)(index, index), (*cov)(index+1, index+1), (*cov)(index+2, index+2), (*cov)(index+3, index+3));
    }
    
    void normalize()
    {
        if(index < 0) return;
        f_t ss = v.w() * v.w() + v.x() * v.x() + v.y() * v.y() + v.z() * v.z();
        m4 dWn_dW;
        
#warning Test this
        //n(x) = x / sqrt(w*w + x*x + y*y + z*z)
        //dn(x)/dx = 1 / sqrt(...) + x (derivative (1 / sqrt(...)))
        // = 1/sqrt(...) + x * -1 / (2 * sqrt(...) * (...)) * derivative(...)
        // = 1/sqrt(...) + x * -1 / (2 * sqrt(...) * (...)) * 2 x
        // = 1/sqrt(...) - x^2 / (sqrt(...) * (...))
        // = (1-x^2) / (sqrt(...) * (...))
        //n(x) = x / sqrt(w*w + x*x + y*y + x*x)
        //dn(x)/dw = -x / (2 * sqrt(...) * (...)) * 2w
        // = -xw / (sqrt(...) * (...))
        v4 qvec = v4(v.w(), v.x(), v.y(), v.z());
        dWn_dW = (m4_identity - outer_product(qvec, qvec)) * (1. / (sqrt(ss) * ss));
        matrix &tmp = cov->temp_matrix(size, cov->size());
        for(int i = 0; i < cov->size(); ++i) {
            v4 cov_Q = copy_cov_from_row(cov->cov, i);
            v4 res = dWn_dW * cov_Q;
            for(int j = 0; j < size; ++j) tmp(j, i) = res[j];
        }
        
        m4 self_cov;
        for(int i = 0; i < 4; ++i) self_cov[i] = copy_cov_from_row(tmp, i);
        self_cov = self_cov * dWn_dW;
        for(int i = 0; i < 4; ++i) copy_cov_to_row(tmp, i, self_cov[i]);

        for(int i = 0; i < size; ++i) {
            for(int j = 0; j < cov->size(); ++j) {
                cov->cov(index + i, j) = cov->cov(j, index + i) = tmp(i, j);
            }
        }
        f_t norm = 1. / sqrt(ss);
        v = quaternion(v.w() * norm, v.x() * norm, v.y() * norm, v.z() * norm);
    }
};

class state_scalar: public state_leaf<f_t, 1> {
 public:
    state_scalar() { reset(); }

    f_t *raw_array() { return &v; }

    void reset() {
        index = -1;
        v = 0.;
    }
    
    f_t copy_cov_from_row(const matrix &cov, const int i) const
    {
        if(index < 0) return 0.;
        return cov(i, index);
    }

    void copy_cov_to_col(matrix &cov, const int j, const f_t v) const
    {
        cov(index, j) = v;
    }
    
    void copy_cov_to_row(matrix &cov, const int j, const f_t v) const
    {
        cov(j, index) = v;
    }
    
    void perturb_variance() {
        cov->cov(index, index) *= PERTURB_FACTOR;
    }

    f_t variance() const {
        if(index < 0) return initial_variance[0];
        return (*cov)(index, index);
    }
};

#endif
