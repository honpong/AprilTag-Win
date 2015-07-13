// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#ifndef __STATE_H
#define __STATE_H

extern "C" {
#include "../cor/cor_types.h"
#include "../cor/packet.h"
}
#include "../numerics/vec4.h"
#include "../numerics/matrix.h"
#include "../numerics/quaternion.h"
#include "../numerics/rotation_vector.h"
#include "../numerics/covariance.h"
#include "../cor/platform/sensor_clock.h"

#include <vector>
#include <list>
#include <iostream>
using namespace std;

#define log_enabled 0
#define show_tuning 0
#define plot_enabled 1

//minstatesize = base (38) + 2xref (12) + full group(40) + min group (6) = 96
#define MINSTATESIZE 96
#define MAXGROUPS 8

class state_node {
public:
    state_node(): dynamic(false) {}
    virtual ~state_node() {};
    bool dynamic;
    static int statesize, maxstatesize;
    virtual void copy_state_to_array(matrix &state) = 0;
    virtual void copy_state_from_array(matrix &state) = 0;
    virtual int remap_dynamic(int i, covariance &cov) = 0;
    virtual int remap_static(int i, covariance &cov) = 0;
    virtual void reset() = 0;
    virtual void remove() = 0;
    virtual void print() = 0;
};

template<class T> class state_branch: public state_node {
protected:
    int dynamic_statesize;
public:
    //for some reason i need this typename qualifier, including without the typedef
    typedef typename list<T>::iterator iterator;

    void copy_state_to_array(matrix &state) {
        for(T c : children)
            c->copy_state_to_array(state);
    }

    virtual void copy_state_from_array(matrix &state) {
        for(T c : children)
            c->copy_state_from_array(state);
    }
    
    int remap_dynamic(int i, covariance &cov) {
        int start = i;
        for(T c : children)
            i = c->remap_dynamic(i, cov);
        dynamic_statesize = i - start;
        return i;
    }
    
    int remap_static(int i, covariance &cov) {
        for(T c : children)
            i = c->remap_static(i, cov);
        return i;
    }

    virtual void reset() {
        for(T c : children)
            c->reset();
    }
    
    virtual void remove()
    {
        for(T c : children)
            c->remove();
        children.clear();
    }
    
    void remove_child(const T n)
    {
        children.remove(n);
        n->remove();
    }
    
    virtual void print()
    {
        for(T c : children)
            c->print();
    }
    
    list<T> children;
};

class state_root: public state_branch<state_node *> {
public:
    state_root(covariance &c): cov(c), current_time(sensor_clock::micros_to_tp(0)) {}
    
    covariance &cov;

    int remap() {
#ifdef TEST_POSDEF
        if(cov.size() && !test_posdef(cov.cov)) fprintf(stderr, "not pos def at beginning of remap\n");
#endif
        dynamic_statesize = state_branch<state_node *>::remap_dynamic(0, cov);
        statesize = state_branch<state_node *>::remap_static(dynamic_statesize, cov);
        cov.remap(statesize);
#ifdef TEST_POSDEF
        if(!test_posdef(cov.cov)) {
            fprintf(stderr, "not pos def at end of remap\n");
            assert(0);
        }
#endif
        return statesize;
    }
    
    virtual void print()
    {
        fprintf(stderr, "State dump:\n");
        state_branch::print();
    }
    
    virtual void reset() {
        cov.reset();
        state_branch<state_node *>::reset();
        current_time = sensor_clock::micros_to_tp(0);
    }
    
    virtual void evolve(f_t dt)
    {
        evolve_covariance(dt);
        evolve_state(dt);
    }

    void time_update(sensor_clock::time_point time)
    {
        if(time <= current_time) {
            if(log_enabled && time < current_time) fprintf(stderr, "negative time step: last was %llu, this is %llu, delta %llu\n", sensor_clock::tp_to_micros(current_time), sensor_clock::tp_to_micros(time), std::chrono::duration_cast<std::chrono::microseconds>(current_time - time).count());
            return;
        }
        if(current_time != sensor_clock::micros_to_tp(0)) {
#ifdef TEST_POSDEF
            if(!test_posdef(cov.cov)) fprintf(stderr, "not pos def before explicit time update\n");
#endif
            auto dt = std::chrono::duration_cast<std::chrono::duration<f_t>>(time - current_time).count();
            if(log_enabled && dt > .025) fprintf(stderr, "Time step is %f\n", dt);
            evolve(dt);
#ifdef TEST_POSDEF
            if(!test_posdef(cov.cov)) fprintf(stderr, "not pos def after explicit time update\n");
#endif
        }
        current_time = time;
    }

protected:
    virtual void evolve_covariance(f_t dt) = 0;
    virtual void evolve_state(f_t dt) = 0;

    sensor_clock::time_point current_time;
};

template <class T, int _size> class state_leaf: public state_node {
 public:
    state_leaf(const char *_name): name(_name), index(-1), size(_size) {}

    T v;
    
    covariance *cov;
    
#ifdef SWIG
    %immutable;
#endif
    const char *name;
#ifdef SWIG
    %mutable;
#endif
    
    void set_process_noise(f_t x)
    {
        for(int i = 0; i < size; ++i) process_noise[i] = x;
    }
    
    void set_initial_variance(f_t x)
    {
        for(int i = 0; i < size; ++i) initial_variance[i] = x;
    }

    int remap_leaf(int i, covariance &c) {
        if(index < 0)
            c.add(i, size, process_noise, initial_variance);
        else
            c.reindex(i, index, size);
        index = i;
        this->cov = &c;
        return i + size;
    }

    int remap_dynamic(int i, covariance &c) {
        if(dynamic) return remap_leaf(i, c);
        else return i;
    }

    int remap_static(int i, covariance &c) {
        if(!dynamic) return remap_leaf(i, c);
        else return i;
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
        for(int j = old_i + 1; j < covariance_m.rows(); ++j) {
            covariance_m(i, j) = covariance_m(old_i, j);
            covariance_m(j, i) = covariance_m(j, old_i);
        }
    }
    
    void remove() { index = -1; }
protected:
    int index;
    f_t process_noise[_size];
    f_t initial_variance[_size];
    int size;
};

#ifdef SWIG
%template(state_leaf_rotation_vector) state_leaf<rotation_vector, 3>;
%template(state_leaf_vec) state_leaf<v4, 3>;
%template(state_leaf_sca) state_leaf<f_t, 1>;
%template(state_branch_node) state_branch<state_node *>;
#endif

#define PERTURB_FACTOR 1.1

class state_vector: public state_leaf<v4, 3> {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
public:
    state_vector(const char *_name): state_leaf(_name) { reset(); }

    using state_leaf::set_initial_variance;
    
    void set_initial_variance(f_t x, f_t y, f_t z)
    {
        initial_variance[0] = x;
        initial_variance[1] = y;
        initial_variance[2] = z;
    }
    
    inline v4 copy_cov_from_row(const matrix &c, const int i) const
    {
        if(index < 0) return v4::Zero();
        return v4(c(i, index), c(i, index+1), c(i, index+2), 0.);
    }
    
    inline void copy_cov_to_col(matrix &c, const int j, const v4 &cov_v) const
    {
        if(index < 0) return;
        c(index, j) = cov_v[0];
        c(index+1, j) = cov_v[1];
        c(index+2, j) = cov_v[2];
    }

    inline void copy_cov_to_row(matrix &c, const int j, const v4 &cov_v) const
    {
        if(index < 0) return;
        c(j, index) = cov_v[0];
        c(j, index+1) = cov_v[1];
        c(j, index+2) = cov_v[2];
    }

    void reset() {
        index = -1;
        v = v4::Zero();
    }
    
    void perturb_variance() {
        if(index < 0) return;
        cov->cov(index, index) *= PERTURB_FACTOR;
        cov->cov(index + 1, index + 1) *= PERTURB_FACTOR;
        cov->cov(index + 2, index + 2) *= PERTURB_FACTOR;
    }
    
    v4 variance() const {
        if(index < 0) return v4(initial_variance[0], initial_variance[1], initial_variance[2], 0.);
        return v4((*cov)(index, index), (*cov)(index+1, index+1), (*cov)(index+2, index+2), 0.);
    }
    
    void copy_state_to_array(matrix &state) {
        state[index] = v[0];
        state[index+1] = v[1];
        state[index+2] = v[2];
    }
    
    virtual void copy_state_from_array(matrix &state) {
        v[0] = state[index+0];
        v[1] = state[index+1];
        v[2] = state[index+2];
    }
    
    virtual void print()
    {
        std::cerr << name << v << variance() << " (vector)\n";
    }
};

class state_rotation_vector: public state_leaf<rotation_vector, 3> {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
public:
    state_rotation_vector(const char *_name): state_leaf(_name) { reset(); }

    void saturate()
    {
        saturated = true;
        //TODO: This is a temporary hack. In practice it works better to saturate by not updating the state, but still leaving an element in the covariance matrix. I think this is because of the observation of mourikis - lienarization causes the filter to get false information on yaw. Having this degree of freedom keeps the filter happier, instead of stuffing the inconsistency into other states, but we just don't update the state ever.
        //size = 2;
    }

    using state_leaf::set_initial_variance;
    
    void set_initial_variance(f_t x, f_t y, f_t z)
    {
        initial_variance[0] = x;
        initial_variance[1] = y;
        initial_variance[2] = z;
    }
    
    inline v4 copy_cov_from_row(const matrix &c, const int i) const
    {
        if(index < 0) return v4::Zero();
        return v4(c(i, index), c(i, index+1), saturated ? 0. : c(i, index+2), 0.);
    }
    
    inline void copy_cov_to_col(matrix &c, const int j, const v4 &cov_v) const
    {
        if(index < 0) return;
        c(index, j) = cov_v[0];
        c(index+1, j) = cov_v[1];
        if (!saturated) c(index+2, j) = cov_v[2];
    }
    
    inline void copy_cov_to_row(matrix &c, const int j, const v4 &cov_v) const
    {
        if(index < 0) return;
        c(j, index) = cov_v[0];
        c(j, index+1) = cov_v[1];
        if (!saturated) c(j, index+2) = cov_v[2];
    }
    
    void reset() {
        index = -1;
        v = rotation_vector(0., 0., 0.);
        saturated = false;
        size = 3;
    }
    
    void perturb_variance() {
        if(index < 0) return;
        cov->cov(index, index) *= PERTURB_FACTOR;
        cov->cov(index + 1, index + 1) *= PERTURB_FACTOR;
        if(!saturated) cov->cov(index + 2, index + 2) *= PERTURB_FACTOR;
    }
    
    v4 variance() const {
        if(index < 0) return v4(initial_variance[0], initial_variance[1], initial_variance[2], 0.);
        return v4((*cov)(index, index), (*cov)(index+1, index+1), saturated ? 0. : (*cov)(index+2, index+2), 0.);
    }
    
    void copy_state_to_array(matrix &state) {
        state[index] = v.x();
        state[index+1] = v.y();
        if(!saturated) state[index+2] = v.z();
    }
    
    virtual void copy_state_from_array(matrix &state) {
        v.x() = state[index+0];
        v.y() = state[index+1];
        if(!saturated) v.z() = state[index+2];
    }
    
    virtual void print()
    {
        cerr << name << v.raw_vector() << variance() << " (rot vec)\n";
    }

protected:
    bool saturated;
};

class state_quaternion: public state_leaf<quaternion, 4>
{
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
public:
    state_quaternion(const char *_name): state_leaf(_name) { reset(); }
    
    void saturate()
    {
        saturated = true;
        //size = 3;
    }
    
    using state_leaf::set_initial_variance;
    
    void set_initial_variance(f_t w, f_t x, f_t y, f_t z)
    {
        initial_variance[0] = w;
        initial_variance[1] = x;
        initial_variance[2] = y;
        initial_variance[3] = z;
    }
    
    inline v4 copy_cov_from_row(const matrix &c, const int i) const
    {
        if(index < 0) return v4::Zero();
        return v4(c(i, index), c(i, index+1), c(i, index+2), saturated ? 0. : c(i, index+3));
    }
    
    inline void copy_cov_to_col(matrix &c, const int j, const v4 &cov_v) const
    {
        if(index < 0) return;
        c(index, j) = cov_v[0];
        c(index+1, j) = cov_v[1];
        c(index+2, j) = cov_v[2];
        if(!saturated) c(index+3, j) = cov_v[3];
    }
    
    inline void copy_cov_to_row(matrix &c, const int j, const v4 &cov_v) const
    {
        if(index < 0) return;
        c(j, index) = cov_v[0];
        c(j, index+1) = cov_v[1];
        c(j, index+2) = cov_v[2];
        if(!saturated) c(j, index+3) = cov_v[3];
    }

    void reset() {
        index = -1;
        v = quaternion(1., 0., 0., 0.);
        saturated = false;
        size = 4;
    }
    
    void perturb_variance() {
        if(index < 0) return;
        cov->cov(index, index) *= PERTURB_FACTOR;
        cov->cov(index + 1, index + 1) *= PERTURB_FACTOR;
        cov->cov(index + 2, index + 2) *= PERTURB_FACTOR;
        if(!saturated) cov->cov(index + 3, index + 3) *= PERTURB_FACTOR;
    }
    
    v4 variance() const {
        if(index < 0) return v4(initial_variance[0], initial_variance[1], initial_variance[2], initial_variance[3]);
        return v4((*cov)(index, index), (*cov)(index+1, index+1), (*cov)(index+2, index+2), saturated ? 0. : (*cov)(index+3, index+3));
    }
    
    void copy_state_to_array(matrix &state) {
        state[index] = v.w();
        state[index+1] = v.x();
        state[index+2] = v.y();
        if(!saturated) state[index+3] = v.z();
    }
    
    virtual void copy_state_from_array(matrix &state) {
        v.w() = state[index+0];
        v.x() = state[index+1];
        v.y() = state[index+2];
        if(!saturated) v.z() = state[index+3];
        normalize();
    }
    
    virtual void print()
    {
        v4 data(v.w(), v.x(), v.y(), v.z());
        std::cerr << name << data << variance() << " (quaternion)\n";
    }
    
    void normalize()
    {
        if(index < 0) return;
        f_t ss = v.w() * v.w() + v.x() * v.x() + v.y() * v.y() + v.z() * v.z();
        m4 dWn_dW;
        
        //TODO:  Test this
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
        dWn_dW = (m4::Identity() - qvec * qvec.transpose()) * (1. / (sqrt(ss) * ss));
        matrix tmp(size, cov->size());
        for(int i = 0; i < cov->size(); ++i) {
            v4 cov_Q = copy_cov_from_row(cov->cov, i);
            v4 res = dWn_dW * cov_Q;
            for(int j = 0; j < size; ++j) tmp(j, i) = res[j];
        }
        
        m4 self_cov;
        for(int i = 0; i < size; ++i) self_cov.row(i) = copy_cov_from_row(tmp, i);
        self_cov = self_cov * dWn_dW;
        for(int i = 0; i < size; ++i) copy_cov_to_row(tmp, i, self_cov.row(i));

        for(int i = 0; i < size; ++i) {
            for(int j = 0; j < cov->size(); ++j) {
                cov->cov(index + i, j) = cov->cov(j, index + i) = tmp(i, j);
            }
        }
        f_t norm = 1. / sqrt(ss);
        v = quaternion(v.w() * norm, v.x() * norm, v.y() * norm, v.z() * norm);
    }
protected:
    bool saturated;
};

class state_scalar: public state_leaf<f_t, 1> {
 public:
    state_scalar(const char *_name): state_leaf(_name) { reset(); }

    f_t *raw_array() { return &v; }

    void reset() {
        index = -1;
        v = 0.;
    }
    
    inline f_t copy_cov_from_row(const matrix &c, const int i) const
    {
        if(index < 0) return 0.;
        return c(i, index);
    }

    inline void copy_cov_to_col(matrix &c, const int j, const f_t val) const
    {
        if(index < 0) return;
        c(index, j) = val;
    }
    
    inline void copy_cov_to_row(matrix &c, const int j, const f_t val) const
    {
        if(index < 0) return;
        c(j, index) = val;
    }
    
    void perturb_variance() {
        if(index < 0) return;
        cov->cov(index, index) *= PERTURB_FACTOR;
    }

    f_t variance() const {
        if(index < 0) return initial_variance[0];
        return (*cov)(index, index);
    }
    
    void copy_state_to_array(matrix &state) {
        state[index] = v;
    }
    
    virtual void copy_state_from_array(matrix &state) {
        v = state[index];
    }
    
    virtual void print()
    {
        fprintf(stderr, "%s: %f %f (scalar)\n", name, v, variance());
    }
};

#endif
