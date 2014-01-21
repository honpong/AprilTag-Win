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

template <class T> class state_leaf: public state_node {
 public:
    state_leaf(): index(-1) {}

    T v;
    T variance;
    T process_noise;
    int index;

    //conversions in and out to reduce the level of indirection
    state_leaf<T> &operator=(const T &other) {
        v = other;
        return *this;
    }

    state_leaf<T> &operator=(const state_leaf<T> &other) {
        v = other.v;
        return *this;
    }
      
    operator T &() {
        return v;
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
            covariance_m(i, j) = 0.;
            covariance_m(j, i) = 0.;
        }
    }
};

#ifdef SWIG
%template(state_leaf_vec) state_leaf<v4>;
%template(state_leaf_sca) state_leaf<f_t>;
%template(state_branch_node) state_branch<state_node *>;
#endif

class state_vector: public state_leaf<v4> {
 public:
    state_vector() { reset(); }

    using state_leaf<v4>::operator=;
    
    void copy_state_to_array(matrix &state) {
        state[index + 0] = v[0];
        state[index + 1] = v[1];
        state[index + 2] = v[2];
    }

    void copy_state_from_array(matrix &state) {
        v[0] = state[index + 0];
        v[1] = state[index + 1];
        v[2] = state[index + 2];
    }

    int remap(int i, int map[], matrix &cov, matrix &p_cov) {
        if(index < 0) {
            int oldsize = cov.rows;
            cov.resize(oldsize+3, oldsize+3);
            p_cov.resize(oldsize+3);
            cov(oldsize,oldsize) = variance[0];
            cov(oldsize+1,oldsize+1) = variance[1];
            cov(oldsize+2,oldsize+2) = variance[2];
            p_cov[oldsize] = process_noise[0];
            p_cov[oldsize+1] = process_noise[1];
            p_cov[oldsize+2] = process_noise[2];
            map[i    ] = -oldsize;
            map[i + 1] = -(oldsize+1);
            map[i + 2] = -(oldsize+2);
        } else {
            map[i    ] = index;
            map[i + 1] = index+1;
            map[i + 2] = index+2;
            variance[0] = cov(index, index);
            variance[1] = cov(index+1, index+1);
            variance[2] = cov(index+2, index+2);
        }
        index = i;
        return i + 3;
    }

    void reset() {
        index = -1;
        v = 0.;
        variance = 0.;
        process_noise = 0.;
    }
};

class state_rotation_vector: public state_leaf<rotation_vector> {
public:
    state_rotation_vector() { reset(); }

    using state_leaf<rotation_vector>::operator=;
    
    void copy_state_to_array(matrix &state) {
        state[index + 0] = v.x();
        state[index + 1] = v.y();
        state[index + 2] = v.z();
    }
    
    void copy_state_from_array(matrix &state) {
        v.x() = state[index + 0];
        v.y() = state[index + 1];
        v.z() = state[index + 2];
    }
    
    int remap(int i, int map[], matrix &cov, matrix &p_cov) {
        if(index < 0) {
            int oldsize = cov.rows;
            cov.resize(oldsize+3, oldsize+3);
            p_cov.resize(oldsize+3);
            cov(oldsize,oldsize) = variance.x();
            cov(oldsize+1,oldsize+1) = variance.y();
            cov(oldsize+2,oldsize+2) = variance.z();
            p_cov[oldsize] = process_noise.x();
            p_cov[oldsize+1] = process_noise.y();
            p_cov[oldsize+2] = process_noise.z();
            map[i    ] = -oldsize;
            map[i + 1] = -(oldsize+1);
            map[i + 2] = -(oldsize+2);
        } else {
            map[i    ] = index;
            map[i + 1] = index+1;
            map[i + 2] = index+2;
            variance.x() = cov(index, index);
            variance.y() = cov(index+1, index+1);
            variance.z() = cov(index+2, index+2);
        }
        index = i;
        return i + 3;
    }
    
    void reset() {
        index = -1;
        v = rotation_vector(0., 0., 0.);
        variance = rotation_vector(0., 0., 0.);
        process_noise = rotation_vector(0., 0., 0.);
    }
};

/*
class state_rotation: public state_leaf

class state_quaternion: public state_leaf<quaternion>
{
public:
    using state_leaf<quaternion>::operator=;
    
    void copy_state_to_array(matrix &state) {
        state[index + 0] = v[0];
        state[index + 1] = v[1];
        state[index + 2] = v[2];
        state[index + 3] = v[3];
    }
    
    void copy_state_from_array(matrix &state) {
        v[0] = state[index + 0];
        v[1] = state[index + 1];
        v[2] = state[index + 2];
        v[3] = state[index + 3];
    }
    
    int remap(int i, int map[], matrix &cov, matrix &p_cov) {
        if(index < 0) {
            int oldsize = cov.rows;
            cov.resize(oldsize+4, oldsize+4);
            p_cov.resize(oldsize+4);
            cov(oldsize,oldsize) = variance[0];
            cov(oldsize+1,oldsize+1) = variance[1];
            cov(oldsize+2,oldsize+2) = variance[2];
            cov(oldsize+3,oldsize+3) = variance[3];
            p_cov[oldsize] = process_noise[0];
            p_cov[oldsize+1] = process_noise[1];
            p_cov[oldsize+2] = process_noise[2];
            p_cov[oldsize+3] = process_noise[3];
            map[i    ] = -oldsize;
            map[i + 1] = -(oldsize+1);
            map[i + 2] = -(oldsize+2);
            map[i + 3] = -(oldsize+3);
        } else {
            map[i    ] = index;
            map[i + 1] = index+1;
            map[i + 2] = index+2;
            map[i + 3] = index+3;
            variance[0] = cov(index, index);
            variance[1] = cov(index+1, index+1);
            variance[2] = cov(index+2, index+2);
            variance[3] = cov(index+3, index+3);
        }
        index = i;
        return i + 4;
    }
    
    void reset() {
        index = -1;
        v = quaternion(1., 0., 0., 0.);
        variance = quaternion(0., 0., 0., 0.);
        process_noise = quaternion(0., 0., 0., 0.);
    }
};
*/
class state_scalar: public state_leaf<f_t> {
 public:
    state_scalar() { reset(); }

    using state_leaf<f_t>::operator=;

    void copy_state_to_array(matrix &state) {
        state[index] = v;
    }

    void copy_state_from_array(matrix &state) {
        v = state[index];
    }

    int remap(int i, int map[], matrix &cov, matrix &p_cov) {
        if(index < 0) {
            int oldsize = cov.rows;
            cov.resize(oldsize+1, oldsize+1);
            p_cov.resize(oldsize+1);
            cov(oldsize,oldsize) = variance;
            p_cov[oldsize] = process_noise;
            map[i] = -oldsize;
        } else {
            map[i] = index;
            variance = cov(index, index);
        }
        index = i;
        return i + 1;
    }

    void reset() {
        index = -1;
        v = 0.;
        variance = 0.;
        process_noise = 0.;
    }
};

#endif
