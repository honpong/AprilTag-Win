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
#include "spdlog/spdlog.h"
#include "spdlog/sinks/null_sink.h"

#include <vector>
#include <list>
#include <iostream>
#include <cmath>
using namespace std;

#define log_enabled 0 // Only used in state.h now
#define show_tuning 0

//minstatesize = base (38) + 2xref (12) + full group(40) + min group (6) = 96
#define MINSTATESIZE 96
#define MAXGROUPS 8

class state_leaf_base {
public:
    state_leaf_base(const char *name_, int index_, int size_) : name(name_), index(index_), size(size_) {}
    const char *name;
protected:
    int index;
    int size;
};

class state_node {
public:
    state_node(): type(node_type::regular) {}
    virtual ~state_node() {};
    enum class node_type { dynamic, regular, fake } type;
    virtual void copy_state_to_array(matrix &state) = 0;
    virtual void print_matrix_with_state_labels(matrix &state, node_type nt) const = 0;
    virtual void copy_state_from_array(matrix &state) = 0;
    virtual int remap(int i, covariance &cov, node_type nt) = 0;
    virtual void reset() = 0;
    virtual void remove() = 0;
    virtual std::ostream &print_to(std::ostream & s) const = 0;
    friend inline std::ostream & operator <<(std::ostream & s, const state_node &b) {
        return b.print_to(s);
    }
};

template<class T> class state_branch: public state_node {
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

    virtual void print_matrix_with_state_labels(matrix &state, node_type nt) const {
        for(T c : children)
            c->print_matrix_with_state_labels(state, nt);
    }
    
    int remap(int i, covariance &cov, node_type nt) {
        for(T c : children)
            i = c->remap(i, cov, nt);
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
    }
    
    void remove_child(const T n)
    {
        children.remove(n);
        n->remove();
    }
    
    virtual std::ostream &print_to(std::ostream & s) const
    {
        int i = 0;
        s << "{"; for(T c : children) { if (i++) s << ", "; c->print_to(s); } return s << "}";
        return s;
    }

    list<T> children;
};

class state_root: public state_branch<state_node *> {
public:
    state_root(covariance &c): cov(c), current_time(sensor_clock::micros_to_tp(0)) {}

    int statesize, maxstatesize, dynamic_statesize, fake_statesize;
    covariance &cov;
    std::unique_ptr<spdlog::logger> log = std::make_unique<spdlog::logger>("state", make_shared<spdlog::sinks::null_sink_st> ());

    v3 world_up = {0,0,1}, world_initial_forward = {0,1,0}, body_forward = {0,0,1}; // defines our world coordinates

    int remap() {
#ifdef TEST_POSDEF
        if(cov.size() && !test_posdef(cov.cov)) log->error("not pos def at beginning of remap");
#endif
        dynamic_statesize = state_branch<state_node *>::remap(0, cov, node_type::dynamic);
        statesize = state_branch<state_node *>::remap(dynamic_statesize, cov, node_type::regular);
        fake_statesize = state_branch<state_node *>::remap(statesize, cov, node_type::fake) - statesize;
        cov.remap(statesize);
#ifdef TEST_POSDEF
        if(!test_posdef(cov.cov)) {
            log->error("not pos def at end of remap");
            assert(0);
        }
#endif
        return statesize;
    }
    
    void print_matrix_with_state_labels(matrix &state) const {
        if(state.rows() >= dynamic_statesize) state_branch<state_node *>::print_matrix_with_state_labels(state, node_type::dynamic);
        if(state.rows() >= statesize) state_branch<state_node *>::print_matrix_with_state_labels(state, node_type::regular);
        if(state.rows() >= statesize + fake_statesize) state_branch<state_node *>::print_matrix_with_state_labels(state, node_type::fake);
    }

    virtual std::ostream &print_to(std::ostream & s) const
    {
        s << "state: "; return state_branch<state_node*>::print_to(s);
    }

    virtual void reset() {
        cov.reset();
        state_branch<state_node *>::reset();
        current_time = sensor_clock::micros_to_tp(0);

        remap();
    }
    
    void evolve(f_t dt)
    {
        evolve_covariance(dt);
        evolve_state(dt);
    }

    void evolve_covariance(f_t dt)
    {
        cache_jacobians(dt);

        matrix tmp(dynamic_statesize, cov.size() + fake_statesize);

        project_motion_covariance(tmp, cov.cov, dt);

        //fill in the UR and LL matrices
        for(int i = 0; i < dynamic_statesize; ++i)
            for(int j = dynamic_statesize; j < cov.size(); ++j)
                cov(i, j) = cov(j, i) = tmp(i, j);

        //compute the UL matrix
        matrix ul(cov.cov, 0, 0, dynamic_statesize, dynamic_statesize);
        project_motion_covariance(ul, tmp, dt);

        //enforce symmetry
        //for(int i = 0; i < dynamic_statesize; ++i)
        //    for(int j = i + 1; j < dynamic_statesize; ++j)
        //        cov(i, j) = cov(j, i);

        //cov += diag(R)*dt^2
        for(int i = 0; i < cov.size(); ++i)
            cov(i, i) += cov.process_noise[i] * dt * dt;
    }

    void time_update(sensor_clock::time_point time)
    {
        if(time <= current_time) {
            if(time < current_time) log->info("negative time step: last was {}, this is {}, delta {}", sensor_clock::tp_to_micros(current_time), sensor_clock::tp_to_micros(time), std::chrono::duration_cast<std::chrono::microseconds>(current_time - time).count());
            return;
        }
        if(current_time != sensor_clock::micros_to_tp(0)) {
#ifdef TEST_POSDEF
            if(!test_posdef(cov.cov)) log->error("not pos def before explicit time update");
#endif
            auto dt = std::chrono::duration_cast<std::chrono::duration<f_t>>(time - current_time).count();
            if(log_enabled && dt > .025) log->warn("Large time step (dt): {}", dt);
            evolve(dt);
#ifdef TEST_POSDEF
            if(!test_posdef(cov.cov)) log->error("not pos def after explicit time update");
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

template <class T, int _size> class state_leaf: public state_leaf_base, public state_node {
 public:
    state_leaf(const char *_name) : state_leaf_base(_name, -1, _size) {}

    T v;
    
    covariance *cov;
    
    void set_process_noise(f_t x)
    {
        for(int i = 0; i < size; ++i) process_noise[i] = x;
    }
    
    void set_initial_variance(f_t x)
    {
        for(int i = 0; i < size; ++i)
            for(int j = 0; j < size; ++j)
                initial_covariance(i, j) = (i==j) ? x : 0;
    }

    void save_initial_variance()
    {
        if (index >= 0 && cov)
            for(int i = 0; i < size; ++i)
                for(int j = 0; j < size; ++j)
                    initial_covariance(i, j) = (*cov)(index + i, index + i);
    }
    
    int remap(int i, covariance &c, node_type nt) {
        if(nt != type) return i;
        if(type != node_type::fake)
        {
            if (index < 0)
                c.add<_size>(i, process_noise, initial_covariance);
            else
                c.reindex(i, index, size);
        }
        index = i;
        this->cov = &c;
        return i + size;
    }

    void reset_covariance(covariance &covariance_m) {
        for(int i = 0; i < size; ++i) {
            if(index >= 0) {
                for(int j = 0; j < covariance_m.size(); ++j) {
                    covariance_m(index + i, j) = covariance_m(j, index + i) = 0.;
                }
                for(int j = 0; j < size; ++j) {
                    covariance_m(index + i, index + j) = initial_covariance(i, j);
                }
            }
        }
    }
    
    inline const Eigen::Map< const Eigen::Matrix<f_t, _size, 1>, Eigen::Unaligned, Eigen::OuterStride<> > from_row(const matrix &c, int i) const
    {
        static const f_t zero[_size] = { 0 };
        if(index < 0) return { &zero[0], Eigen::OuterStride<>(1) };
        if(index >= c.cols()) {
            if((type == node_type::fake) && (i - index >= 0) && (i - index < _size)) return { &initial_covariance(i - index, 0), Eigen::OuterStride<>(_size) };
            else return { &zero[0], Eigen::OuterStride<>(1) };
        }
        if((i < 0) || (i >= c.rows())) return { &zero[0] , Eigen::OuterStride<>(1) };
        return { &c(i,index), Eigen::OuterStride<>(c.get_stride()) };
    }

    inline Eigen::Map< Eigen::Matrix<f_t, _size, 1>, Eigen::Unaligned, Eigen::InnerStride<> > to_col(matrix &c, int j) const
    {
        static f_t scratch[_size];
        if((index < 0) || (index >= c.rows())) return { &scratch[0], Eigen::InnerStride<>(1) };
        else return { &c(index,j), Eigen::InnerStride<>(c.get_stride()) };
    }

    void remove() { index = -1; }

    virtual void print_matrix_with_state_labels(matrix &state, node_type nt) const {
        if(type == nt)
            for (int i=0; i<_size; i++) {
                fprintf(stderr, "%s[%d]: ", name, i); state.row(index+i).print();
            }
    }
protected:
    f_t process_noise[_size];
    m<_size, _size> initial_covariance;
};

#define PERTURB_FACTOR f_t(1.1)

class state_vector: public state_leaf<v3, 3> {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
public:
    state_vector(const char *_name): state_leaf(_name) { reset(); }

    using state_leaf::set_initial_variance;
    
    void set_initial_variance(f_t x, f_t y, f_t z)
    {
        initial_covariance.setZero();
        initial_covariance(0, 0) = x;
        initial_covariance(1, 1) = y;
        initial_covariance(2, 2) = z;
    }
    
    void reset() {
        index = -1;
        v = v3::Zero();
    }
    
    void perturb_variance() {
        if(index < 0 || index >= cov->size()) return;
        cov->cov(index, index) *= PERTURB_FACTOR;
        cov->cov(index + 1, index + 1) *= PERTURB_FACTOR;
        cov->cov(index + 2, index + 2) *= PERTURB_FACTOR;
    }
    
    v3 variance() const {
        if(index < 0 || index >= cov->size()) return v3(initial_covariance(0, 0), initial_covariance(1, 1), initial_covariance(2, 2));
        return v3((*cov)(index, index), (*cov)(index+1, index+1), (*cov)(index+2, index+2));
    }
    
    void copy_state_to_array(matrix &state) {
        if(index < 0 || index >= state.cols()) return;
        state[index] = v[0];
        state[index+1] = v[1];
        state[index+2] = v[2];
    }
    
    virtual void copy_state_from_array(matrix &state) {
        if(index < 0 || index >= state.cols()) return;
        v[0] = state[index+0];
        v[1] = state[index+1];
        v[2] = state[index+2];
    }
    
    virtual std::ostream &print_to(std::ostream & s) const
    {
        return s << name << ": " << v << "±" << variance().array().sqrt();
    }
};

class state_rotation_vector: public state_leaf<rotation_vector, 3> {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
public:
    state_rotation_vector(const char *_name): state_leaf(_name) { reset(); }

    using state_leaf::set_initial_variance;
    
    void set_initial_variance(f_t x, f_t y, f_t z)
    {
        initial_covariance.setZero();
        initial_covariance(0, 0) = x;
        initial_covariance(1, 1) = y;
        initial_covariance(2, 2) = z;
    }
    
    void reset() {
        index = -1;
        v = rotation_vector(0., 0., 0.);
    }
    
    void perturb_variance() {
        if(index < 0 || index >= cov->size()) return;
        cov->cov(index, index) *= PERTURB_FACTOR;
        cov->cov(index + 1, index + 1) *= PERTURB_FACTOR;
        cov->cov(index + 2, index + 2) *= PERTURB_FACTOR;
    }
    
    v3 variance() const {
        if(index < 0 || index > cov->size()) return v3(initial_covariance(0, 0), initial_covariance(1, 1), initial_covariance(2, 2));
        return v3((*cov)(index, index), (*cov)(index+1, index+1), (*cov)(index+2, index+2));
    }
    
    void copy_state_to_array(matrix &state) {
        if(index < 0 || index >= state.cols()) return;
        state[index] = v.x();
        state[index+1] = v.y();
        state[index+2] = v.z();
    }
    
    virtual void copy_state_from_array(matrix &state) {
        if(index < 0 || index >= state.cols()) return;
        v.x() = state[index+0];
        v.y() = state[index+1];
        v.z() = state[index+2];
    }
    
    virtual std::ostream &print_to(std::ostream & s) const
    {
        return s << name << ": " << v << "±" << variance().array().sqrt();
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
        initial_covariance.setZero();
        initial_covariance(0, 0) = x;
        initial_covariance(1, 1) = y;
        initial_covariance(2, 2) = z;
    }

    void reset() {
        index = -1;
        v = quaternion::Identity();
        w = rotation_vector(0,0,0);
    }
    
    void perturb_variance() {
        if(index < 0 || index >= cov->size()) return;
        cov->cov(index, index) *= PERTURB_FACTOR;
        cov->cov(index + 1, index + 1) *= PERTURB_FACTOR;
        cov->cov(index + 2, index + 2) *= PERTURB_FACTOR;
    }
    
    v3 variance() const {
        if(index < 0 || index > cov->size()) return v3(initial_covariance(0, 0), initial_covariance(1, 1), initial_covariance(2, 2));
        return v3((*cov)(index, index), (*cov)(index+1, index+1), (*cov)(index+2, index+2));
    }
    
    void copy_state_to_array(matrix &state) {
        if(index < 0 || index >= state.cols()) return;
        state[index+0] = 0;
        state[index+1] = 0;
        state[index+2] = 0;
    }
    
    virtual void copy_state_from_array(matrix &state) {
        if(index < 0 || index >= state.cols()) return;
        w = rotation_vector(state[index+0], state[index+1], state[index+2]);
        v = to_quaternion(w) * v;
    }
    
    virtual std::ostream &print_to(std::ostream & s) const
    {
        return s << name << ": " << v << "±" << variance().array().sqrt();
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
        if(index < 0) return 0;
        if(index >= c.cols())
        {
            if((type == node_type::fake) && (i == index)) return initial_covariance(0, 0);
            else return 0;
        }
        if((i < 0) || (i >= c.rows())) return 0;
        else return c(i, index);
    }

    inline f_t &to_col(matrix &c, const int j) const
    {
        static f_t scratch;
        if((index < 0) || (index >= c.rows())) return scratch;
        else return c(index, j);
    }
    
    void perturb_variance() {
        if(index < 0 || index >= cov->size()) return;
        cov->cov(index, index) *= PERTURB_FACTOR;
    }

    f_t variance() const {
        if(index < 0) return initial_covariance(0, 0);
        return (*cov)(index, index);
    }
    
    void copy_state_to_array(matrix &state) {
        if(index < 0 || index >= state.cols()) return;
        state[index] = v;
    }
    
    virtual void copy_state_from_array(matrix &state) {
        if(index < 0 || index >= state.cols()) return;
        v = state[index];
    }
    
    virtual std::ostream &print_to(std::ostream & s) const
    {
        return s << name << ": " << v << "±" << std::sqrt(variance());
    }
};

class state_extrinsics: public state_branch<state_node *>
{
public:
    state_quaternion Q;
    state_vector T;
    bool estimate;

    state_extrinsics(const char *Qx, const char *Tx, bool _estimate): T(Tx), Q(Qx), estimate(_estimate)
    {
        if(estimate)
        {
            children.push_back(&T);
            children.push_back(&Q);
        }
    }
};

#endif
