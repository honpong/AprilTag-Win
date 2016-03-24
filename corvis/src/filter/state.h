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

#define log_enabled 0 // Only used in state.h now
#define show_tuning 0

//minstatesize = base (38) + 2xref (12) + full group(40) + min group (6) = 96
#define MINSTATESIZE 96
#define MAXGROUPS 8

class state_node {
public:
    state_node(): dynamic(false) {}
    virtual ~state_node() {};
    bool dynamic;
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

    int statesize, maxstatesize;
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
    
    void evolve(f_t dt)
    {
        evolve_covariance(dt);
        evolve_state(dt);
    }

    void evolve_covariance(f_t dt)
    {
        cache_jacobians(dt);

        matrix tmp(dynamic_statesize, cov.size());

        project_motion_covariance(tmp, cov.cov, dt);

        //fill in the UR and LL matrices
        for(int i = 0; i < dynamic_statesize; ++i)
            for(int j = dynamic_statesize; j < cov.size(); ++j)
                cov(i, j) = cov(j, i) = tmp(i, j);

        //compute the UL matrix
        project_motion_covariance(cov.cov, tmp, dt);

        //enforce symmetry
        //for(int i = 0; i < dynamic_statesize; ++i)
        //    for(int j = i + 1; j < dynamic_statesize; ++j)
        //        cov(i, j) = cov(j, i);

        //cov += diag(R)*dt
        for(int i = 0; i < cov.size(); ++i)
            cov(i, i) += cov.process_noise[i] * dt;
    }

    void time_update(sensor_clock::time_point time)
    {
        if(time <= current_time) {
            if(log_enabled && time < current_time) std::cerr << "negative time step: last was " << sensor_clock::tp_to_micros(current_time) << ", this is " << sensor_clock::tp_to_micros(time) << ", delta " << std::chrono::duration_cast<std::chrono::microseconds>(current_time - time).count() << "\n";
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
    virtual void project_motion_covariance(matrix &dst, const matrix &src, f_t dt) = 0;
    virtual void evolve_state(f_t dt) = 0;
    virtual void cache_jacobians(f_t dt) = 0;

    sensor_clock::time_point current_time;
};

template <class T, int _size> class state_leaf: public state_node {
 public:
    state_leaf(const char *_name): name(_name), index(-1), size(_size) {}

    T v;
    
    covariance *cov;
    
    const char *name;
    
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

    inline const Eigen::Map< const Eigen::Matrix<f_t, _size, 1>, Eigen::Unaligned, Eigen::OuterStride<> > from_row(const matrix &c, int i) const
    {
        static const f_t zero[_size] = {};
        return { index >=0 ? &c(i,index) : &zero[0], Eigen::OuterStride<>(index >= 0 ? c.get_stride() : 1) };
    }

    inline Eigen::Map< Eigen::Matrix<f_t, _size, 1>, Eigen::Unaligned, Eigen::InnerStride<> > to_col(matrix &c, int j) const
    {
        static f_t scratch[_size];
        return { index>=0 ? &c(index,j) : &scratch[0], Eigen::InnerStride<>(index >= 0 ? c.get_stride() : 1) };
    }

    void remove() { index = -1; }
protected:
    int index;
    f_t process_noise[_size];
    f_t initial_variance[_size];
    int size;
};

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

    using state_leaf::set_initial_variance;
    
    void set_initial_variance(f_t x, f_t y, f_t z)
    {
        initial_variance[0] = x;
        initial_variance[1] = y;
        initial_variance[2] = z;
    }
    
    void reset() {
        index = -1;
        v = rotation_vector(0., 0., 0.);
        size = 3;
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
        state[index] = v.x();
        state[index+1] = v.y();
        state[index+2] = v.z();
    }
    
    virtual void copy_state_from_array(matrix &state) {
        v.x() = state[index+0];
        v.y() = state[index+1];
        v.z() = state[index+2];
    }
    
    virtual void print()
    {
        cerr << name << v.raw_vector() << variance() << " (rot vec)\n";
    }
};

class state_quaternion: public state_leaf<quaternion, 3>
{
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
public:
    state_quaternion(const char *_name): state_leaf(_name) { reset(); }
    
    using state_leaf::set_initial_variance;
    
    void set_initial_variance(f_t x, f_t y, f_t z)
    {
        initial_variance[0] = x;
        initial_variance[1] = y;
        initial_variance[2] = z;
    }

    void reset() {
        index = -1;
        v = quaternion(1., 0., 0., 0.);
        w = rotation_vector(0,0,0);
    }
    
    void perturb_variance() {
        if(index < 0) return;
        cov->cov(index, index) *= PERTURB_FACTOR;
        cov->cov(index + 1, index + 1) *= PERTURB_FACTOR;
        cov->cov(index + 2, index + 2) *= PERTURB_FACTOR;
    }
    
    v4 variance() const {
        if(index < 0) return v4(initial_variance[0], initial_variance[1], initial_variance[2], 0);
        return v4((*cov)(index, index), (*cov)(index+1, index+1), (*cov)(index+2, index+2), 0);
    }
    
    void copy_state_to_array(matrix &state) {
        state[index+0] = 0;
        state[index+1] = 0;
        state[index+2] = 0;
    }
    
    virtual void copy_state_from_array(matrix &state) {
        w = rotation_vector(state[index+0], state[index+1], state[index+2]);
        v = to_quaternion(w) * v;
    }
    
    virtual void print()
    {
        v4 data(v.w(), v.x(), v.y(), v.z());
        std::cerr << name << data << " w(" << w << ") " << variance() << " (quaternion)\n";
    }

protected:
    rotation_vector w;
};

class state_scalar: public state_leaf<f_t, 1> {
 public:
    state_scalar(const char *_name): state_leaf(_name) { reset(); }

    f_t *raw_array() { return &v; }

    void reset() {
        index = -1;
        v = 0.;
    }
    
    inline f_t from_row(const matrix &c, const int i) const
    {
        return index >= 0 ? c(i, index) : 0;
    }

    inline f_t &to_col(matrix &c, const int j) const
    {
        static f_t scratch;
        return index >= 0 ? c(index, j) : scratch;
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
