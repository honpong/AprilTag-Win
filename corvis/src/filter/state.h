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

#include <vector>
#include <list>
#include <iostream>
using namespace std;

#define MAXSTATESIZE 160
//minstatesize = base (38) + 2xref (12) + full group(40) + min group (6) = 96
#define MINSTATESIZE 96
#define MAXGROUPS 8

class state_node {
 public:
    static int statesize, maxstatesize;
    virtual void copy_state_to_array(matrix &state) = 0;
    virtual void copy_state_from_array(matrix &state) = 0;
    virtual int remap(int i, int map[], matrix &cov, matrix &p_cov) = 0;
    virtual void reset() = 0;
};

template<class T> class state_branch: public state_node {
 public:
    //for some reason i need this typename qualifier, including without the typedef
    typedef typename list<T>::iterator iterator;

    void copy_state_to_array(matrix &state) {
        for(iterator j = children.begin(); j != children.end(); ++j) {
            (*j)->copy_state_to_array(state);
        }
    }

    void copy_state_from_array(matrix &state) {
        for(iterator j = children.begin(); j != children.end(); ++j) {
            (*j)->copy_state_from_array(state);
        }
    }
    
    int remap(int i, int map[], matrix &cov, matrix &p_cov) {
        for(iterator j = children.begin(); j != children.end(); ++j) {
            i = (*j)->remap(i, map, cov, p_cov);
        }
        return i;
    }

    virtual void reset() {
        for(iterator j = children.begin(); j != children.end(); ++j) {
            (*j)->reset();
        }
    }

    list<T> children;
};

class state_root: public state_branch<state_node *> {
 public:
 state_root(): cov((f_t*)cov_storage[0], 0, 0, MAXSTATESIZE, MAXSTATESIZE), cov_old((f_t*)cov_storage[1], 0, 0, MAXSTATESIZE, MAXSTATESIZE), p_cov((f_t *)p_cov_storage[0], 1, 0, 1, MAXSTATESIZE), p_cov_old((f_t*)p_cov_storage[1], 1, 0, 1, MAXSTATESIZE) {}
    v_intrinsic cov_storage[2][MAXSTATESIZE*MAXSTATESIZE / 4];
    v_intrinsic p_cov_storage[2][MAXSTATESIZE / 4];
#ifndef SWIG
    matrix cov;
    matrix cov_old;
    matrix p_cov;
    matrix p_cov_old;
#endif
    int remap() {
        int map[MAXSTATESIZE];
        statesize = state_branch<state_node *>::remap(0, map, cov, p_cov);
        f_t *temp = cov_old.data;
        cov_old.data = cov.data;
        cov.data = temp;

        cov_old.resize(cov.rows, cov.cols);
        cov.resize(statesize, statesize);

        temp = p_cov.data;
        p_cov_old.data = p_cov.data;
        p_cov.data = temp;

        p_cov_old.resize(p_cov.cols);
        p_cov.resize(statesize);

        for(int i = 0; i < statesize; ++i) {
            p_cov[i] = p_cov_old[abs(map[i])];
            for(int j = 0; j < statesize; ++j) {
                if(map[i] < 0 || map[j] < 0) {
                    if(i == j) cov(i, j) = cov_old(-map[i], -map[j]);
                    else cov(i, j) = 0.;
                } else cov(i, j) = cov_old(map[i], map[j]);
            }
        }
        return statesize;
    }
    
    void reset() {
        cov.resize(0, 0);
        cov_old.resize(0, 0);
        p_cov.resize(0);
        p_cov_old.resize(0);
        state_branch<state_node *>::reset();
    }
};

template <class T, int _size> class state_leaf: public state_node {
 public:
    state_leaf(): index(-1) {}

    T v;
    
    void set_process_noise(f_t x)
    {
        for(int i = 0; i < size; ++i) process_noise[i] = x;
    }
    
    void set_initial_variance(f_t x)
    {
        for(int i = 0; i < size; ++i) variance[i] = initial_variance[i] = x;
    }
    
    virtual f_t *raw_array() = 0;
    
    void copy_state_to_array(matrix &state) {
        for(int i = 0; i < size; ++i) state[index + i] = raw_array()[i];
    }
    
    void copy_state_from_array(matrix &state) {
        for(int i = 0; i < size; ++i) raw_array()[i] = state[index + i];
    }
    
    int remap(int i, int map[], matrix &cov, matrix &p_cov) {
        if(index < 0) {
            int oldsize = cov.rows;
            cov.resize(oldsize+size, oldsize+size);
            p_cov.resize(oldsize+size);
            for(int j = 0; j < size; ++j) {
                cov(oldsize+j, oldsize+j) = variance[j];
                p_cov[oldsize+j] = process_noise[j];
                map[i+j] = -(oldsize+j);
            }
        } else {
            for(int j = 0; j < size; ++j) {
                map[i+j] = index+j;
                variance[j] = cov(index+j, index+j);
            }
        }
        index = i;
        return i + size;
    }
    
    void reset_covariance(matrix &covariance_m) {
        for(int i = 0; i < size; ++i) {
            for(int j = 0; j < covariance_m.rows; ++j) {
                covariance_m(i, j) = covariance_m(j, i) = 0.;
            }
            covariance_m(i, i) = initial_variance[i];
            variance[i] = initial_variance[i];
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

    static void clear_covariance(int i, matrix &covariance_m) {
        for(int j = 0; j < covariance_m.rows; ++j) {
            covariance_m(i, j) = covariance_m(j, i) = 0.;
        }
    }
    
    f_t variance[_size];
    f_t process_noise[_size];
    f_t initial_variance[_size];
    int index;
protected:
    static const int size = _size;
};

#ifdef SWIG
%template(state_leaf_vec) state_leaf<v4, 3>;
%template(state_leaf_sca) state_leaf<f_t, 1>;
%template(state_branch_node) state_branch<state_node *>;
#endif

class state_vector: public state_leaf<v4, 3> {
 public:
    state_vector() { reset(); }
    
    f_t *raw_array() { return (f_t *)&(v.data); }

    using state_leaf::set_initial_variance;
    
    void set_initial_variance(f_t x, f_t y, f_t z)
    {
        variance[0] = initial_variance[0] = x;
        variance[1] = initial_variance[1] = y;
        variance[2] = initial_variance[2] = z;
    }

    void reset() {
        index = -1;
        v = 0.;
        for(int i = 0; i < size; ++i) {
            variance[i] = 0.;
            process_noise[i] = 0.;
        }
    }
};

class state_rotation_vector: public state_leaf<rotation_vector, 3> {
public:
    state_rotation_vector() { reset(); }

    f_t *raw_array() { return v.raw_array(); }

    using state_leaf::set_initial_variance;
    
    void set_initial_variance(f_t x, f_t y, f_t z)
    {
        variance[0] = initial_variance[0] = x;
        variance[1] = initial_variance[1] = y;
        variance[2] = initial_variance[2] = z;
    }
    
    void reset() {
        index = -1;
        v = rotation_vector(0., 0., 0.);
        for(int i = 0; i < size; ++i) {
            variance[i] = 0.;
            process_noise[i] = 0.;
        }
    }
};


class state_quaternion: public state_leaf<quaternion, 4>
{
public:
    state_quaternion() { reset(); }
    
    f_t *raw_array() { return v.raw_array(); }

    using state_leaf::set_initial_variance;
    
    void set_initial_variance(f_t w, f_t x, f_t y, f_t z)
    {
        variance[0] = initial_variance[0] = w;
        variance[1] = initial_variance[1] = x;
        variance[2] = initial_variance[2] = y;
        variance[3] = initial_variance[3] = z;
    }

    void reset() {
        index = -1;
        v = quaternion(1., 0., 0., 0.);
        for(int i = 0; i < size; ++i) {
            variance[i] = 0.;
            process_noise[i] = 0.;
        }
    }
};

class state_scalar: public state_leaf<f_t, 1> {
 public:
    state_scalar() { reset(); }

    f_t *raw_array() { return &v; }

    void reset() {
        index = -1;
        v = 0.;
        for(int i = 0; i < size; ++i) {
            variance[i] = 0.;
            process_noise[i] = 0.;
        }
    }
};

#endif
