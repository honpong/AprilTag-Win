// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#ifndef __STATE_H
#define __STATE_H

#include "cor_types.h"
#include "packet.h"
#include "vec4.h"
#include "matrix.h"
#include "quaternion.h"
#include "rotation_vector.h"
#include "covariance.h"
#include "transformation.h"
#include "platform/sensor_clock.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/null_sink.h"
#include "stdev.h"

#include <vector>
#include <list>
#include <iostream>
#include <cmath>
#include <memory>

static constexpr bool log_enabled = false; // Only used in state.h now
static constexpr bool show_tuning = false;

class state_node {
public:
    virtual ~state_node() {}
    state_node &operator=(const state_node &other) = default;
    enum node_type { dynamic, constant, fake };
    virtual void copy_state_to_array(matrix &state) = 0;
    virtual void print_matrix_with_state_labels(matrix &state, node_type nt) const = 0;
    virtual void copy_state_from_array(matrix &state) = 0;
    virtual int remap(int i, covariance &cov, node_type nt) = 0;
    virtual void reset() = 0;
    virtual bool unmap() = 0;
    virtual std::ostream &print_to(std::ostream & s) const = 0;
    friend inline std::ostream & operator <<(std::ostream & s, const state_node &b) {
        return b.print_to(s);
    }
};

class state_leaf_base {
    friend class state_vision;
public:
    state_leaf_base(const char *name_, state_node::node_type type_, int index_, int size_) : name(name_), index(index_), type(type_), size(size_) {}
    inline bool single_index() const { return size == 1; }
    const char *name;
    int index;
protected:
    state_node::node_type type;
    int size;
};

template<typename T, typename List = std::list<T>> class state_branch: public state_node {
public:
    void copy_state_to_array(matrix &state) {
        if (!estimate) return;
        for(const auto &c : children)
            c->copy_state_to_array(state);
    }

    virtual void copy_state_from_array(matrix &state) {
        if (!estimate) return;
        for(auto &c : children)
            c->copy_state_from_array(state);
    }

    virtual void print_matrix_with_state_labels(matrix &state, node_type nt) const {
        if (!estimate) return;
        for(const auto &c : children)
            c->print_matrix_with_state_labels(state, nt);
    }

    int remap(int i, covariance &cov, node_type nt) {
        if (estimate)
            for(auto &c : children)
                i = c->remap(i, cov, nt);
        return i;
    }

    virtual void reset() {
        for(auto &c : children)
            c->reset();
    }
    
    virtual bool unmap()
    {
        bool mapped = false;
        for(auto &c : children)
            mapped |= c->unmap();
        return mapped;
    }

    void remove_child(const T n)
    {
        children.remove(n);
        n->unmap();
    }

    virtual bool disable_estimation() {
        estimate = false;
        return unmap();
    }

    void enable_estimation() {
        estimate = true;
    }
    
    virtual std::ostream &print_to(std::ostream & s) const
    {
        int i = 0;
        s << "{"; if (estimate) for(const auto &c : children) { if (i++) s << ", "; c->print_to(s); } return s << "}";
        return s;
    }

    List children;
    bool estimate = true;
};

template <int _size> class state_leaf: public state_leaf_base, public state_node {
 public:
    state_leaf(const char *_name, node_type nt) : state_leaf_base(_name, nt, -1, _size) {}

    covariance *cov;
    
    template <int Cols, typename Stride_ = Eigen::Stride<Cols == 1 ? Eigen::Dynamic : 1, Cols == 1 ? 0 : Eigen::Dynamic> >
    struct outer_stride : Stride_ {
        outer_stride(int inner) : Stride_(Cols == 1 ? inner : 1, Cols == 1 ? 0 : inner) {}
    };

    template <int Rows, typename Stride_ = Eigen::Stride<Rows == 1 ? 0 : Eigen::Dynamic, Rows == 1 ? Eigen::Dynamic : 1> >
    struct inner_stride : Stride_ {
        inner_stride(int outer) : Stride_(Rows == 1 ? 0 : outer, Rows == 1 ? outer : 1) {}
    };

    template<int Cols = 1>
    inline const Eigen::Map< const m<_size, Cols>, Eigen::Unaligned, outer_stride<Cols>> from_row(const matrix &c, int i) const
    {
        typedef decltype(from_row<Cols>(c,i)) map;
        static const f_t zero[_size*Cols] = { 0 };
        if(index < 0)                                                                return map { &zero[0],                          outer_stride<Cols>(1) };
        if(index >= c.cols()) {
            if((type == node_type::fake) && (i - index >= 0) && (i - index < _size)) return map { &initial_covariance(i - index, 0), outer_stride<Cols>(_size) };
            else                                                                     return map { &zero[0],                          outer_stride<Cols>(1) };
        }
        if((i < 0) || (i >= c.rows()))                                               return map { &zero[0] ,                         outer_stride<Cols>(1) };
        else                                                                         return map { &c(i,index),                       outer_stride<Cols>(c.get_stride()) };
    }

    template<int Rows = 1>
    inline Eigen::Map< m<_size, Rows>, Eigen::Unaligned, inner_stride<Rows>> to_col(matrix &c, int j) const
    {
        typedef decltype(to_col<Rows>(c,j)) map;
        static f_t scratch[_size*Rows];
        if((index < 0) || (index >= c.rows())) return map { &scratch[0], inner_stride<Rows>(1) };
        else                                   return map { &c(index,j), inner_stride<Rows>(c.get_stride()) };
    }

    template<int Cols = 1>
    inline Eigen::Map< m<_size, Cols>, Eigen::Unaligned, outer_stride<Cols>> to_row(matrix &c, int i) const
    {
        typedef decltype(to_row<Cols>(c,i)) map;
        static f_t scratch[_size*Cols];
        if((index < 0) || (index >= c.rows())) return map { &scratch[0], outer_stride<Cols>(1) };
        else                                   return map { &c(i,index), outer_stride<Cols>(c.get_stride()) };
    }

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

    inline f_t get_initial_covariance() const {
            return (type == node_type::fake) ? initial_covariance(0, 0) : 0;
    }

    bool unmap() { if (index < 0) return false; else { index = -1; return true; } }

    virtual void print_matrix_with_state_labels(matrix &state, node_type nt) const {
        if(type == nt)
            for (int i=0; i<_size; i++) {
                fprintf(stderr, "%s[%d]: ", name, i); if (index < 0) fprintf(stderr, "unmapped\n"); else state.row(index+i).print();
            }
    }
protected:
    ::v<_size> process_noise;
public:
    m<_size, _size> initial_covariance;
};

#define PERTURB_FACTOR f_t(1.1)

template <int size_>
class state_vector : public state_leaf<size_> {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
public:
    ::v<size_> v;
    state_vector(const char *_name, state_node::node_type nt): state_leaf<size_>(_name, nt) { reset(); }

    using state_leaf<size_>::set_initial_variance;
    using state_leaf<size_>::cov;
    using state_leaf<size_>::index;
    using state_leaf<size_>::name;
    using state_leaf<size_>::initial_covariance;
    using state_leaf<size_>::from_row;
    using state_leaf<size_>::to_col;

    void set_initial_variance(const ::v<size_> &v) {
        initial_covariance.setZero();
        initial_covariance.diagonal() = v;
    }

    void reset() {
        index = -1;
        v = ::v<size_>::Zero();
    }

    void perturb_variance() {
        if(index < 0 || index >= cov->size()) return;
        for (int i=0; i<size_; i++)
            cov->cov(index+i, index+i) *= PERTURB_FACTOR;
    }

    ::v<size_> variance() const {
        ::v<size_> var;
        for (int i=0; i<size_; i++)
            var[i] = index < 0 || index >= cov->size() ? initial_covariance(i, i) : (*cov)(index+i, index+i);
        return var;
    }

    void copy_state_to_array(matrix &state) {
        if(index < 0 || index >= state.cols()) return;
        for (int i=0; i<size_; i++)
            state[index + i] = v[i];
    }

    virtual void copy_state_from_array(matrix &state) {
        if(index < 0 || index >= state.cols()) return;
        for (int i=0; i<size_; i++)
            v[i] = state[index+i];
    }

    virtual std::ostream &print_to(std::ostream & s) const {
        return s << name << ": " << v << "±" << variance().array().sqrt();
    }
};

template <int size_>
class state_vector_ref : public state_leaf<size_> {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
public:
    ::v<size_> &v;
    state_vector_ref(::v<size_> &_v, const char *_name, state_node::node_type nt): state_leaf<size_>(_name, nt), v(_v) { }

    using state_leaf<size_>::set_initial_variance;
    using state_leaf<size_>::cov;
    using state_leaf<size_>::index;
    using state_leaf<size_>::name;
    using state_leaf<size_>::initial_covariance;
    using state_leaf<size_>::from_row;
    using state_leaf<size_>::to_col;

    void set_initial_variance(const ::v<size_> &v) {
        initial_covariance.setZero();
        initial_covariance.diagonal() = v;
    }

    void reset() {
        index = -1;
        v = ::v<size_>::Zero();
    }

    void perturb_variance() {
        if(index < 0 || index >= cov->size()) return;
        for (int i=0; i<size_; i++)
            cov->cov(index+i, index+i) *= PERTURB_FACTOR;
    }

    ::v<size_> variance() const {
        ::v<size_> var;
        for (int i=0; i<size_; i++)
            var[i] = index < 0 || index >= cov->size() ? initial_covariance(i, i) : (*cov)(index+i, index+i);
        return var;
    }

    void copy_state_to_array(matrix &state) {
        if(index < 0 || index >= state.cols()) return;
        for (int i=0; i<size_; i++)
            state[index + i] = v[i];
    }

    virtual void copy_state_from_array(matrix &state) {
        if(index < 0 || index >= state.cols()) return;
        for (int i=0; i<size_; i++)
            v[i] = state[index+i];
    }

    virtual std::ostream &print_to(std::ostream & s) const {
        return s << name << ": " << v << "±" << variance().array().sqrt();
    }
};

class state_rotation_vector: public state_leaf<3> {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
public:
    rotation_vector v;

    state_rotation_vector(const char *_name, node_type nt): state_leaf(_name, nt) { reset(); }

    using state_leaf::set_initial_variance;
    
    void set_initial_variance(const v3 &v)
    {
        initial_covariance.setZero();
        initial_covariance.diagonal() = v;
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

class state_quaternion: public state_leaf<3>
{
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
public:
    quaternion v;

    state_quaternion(const char *_name, node_type nt): state_leaf(_name, nt) { reset(); }

    using state_leaf::set_initial_variance;

    void set_initial_variance(const v3 &v) {
        initial_covariance.setZero();
        initial_covariance.diagonal() = v;
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
        v.normalize();
    }

    virtual std::ostream &print_to(std::ostream & s) const {
        return s << name << ": " << v << "±" << variance().array().sqrt();
    }
    
protected:
    rotation_vector w;
};

class state_quaternion_ref: public state_leaf<3>
{
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
public:
    quaternion &v;

    state_quaternion_ref(quaternion &q, const char *_name, node_type nt): state_leaf(_name, nt), v(q) { }

    using state_leaf::set_initial_variance;

    void set_initial_variance(const v3 &v) {
        initial_covariance.setZero();
        initial_covariance.diagonal() = v;
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
    
    virtual std::ostream &print_to(std::ostream & s) const {
        return s << name << ": " << v << "±" << variance().array().sqrt();
    }
    
protected:
    rotation_vector w;
};

class state_scalar: public state_leaf<1> {
 public:
    f_t v;

    state_scalar(const char *_name, node_type nt): state_leaf(_name, nt) { reset(); }

    f_t *raw_array() { return &v; }

    void reset() {
        index = -1;
        v = 0.;
    }
    
    using state_leaf::from_row;
    using state_leaf::to_col;

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
    state_vector<3> T;

    state_extrinsics(const char *Qx, const char *Tx, bool estimate_, bool estimate_translation = true) : Q(Qx, constant), T(Tx, constant) {
        estimate = estimate_;
        children.push_back(&Q);
        if (estimate_translation)
            children.push_back(&T);
    }
    void copy_from(const state_extrinsics &other) {
        Q = other.Q;
        T = other.T;
    }
    transformation G_body_device() const {
        return { Q.v, T.v };
    }
};

class state_root: public state_branch<state_node *> {
public:
    state_scalar      g { "g",  constant };

    state_root(covariance &c, matrix &FP_): cov(c), FP(FP_), current_time(sensor_clock::micros_to_tp(0)) {
        //children.push_back(&g);
        g.v = gravity_magnitude;
    }

    int statesize, dynamic_statesize, fake_statesize;
    covariance &cov;
    std::unique_ptr<spdlog::logger> log = std::make_unique<spdlog::logger>("state", std::make_shared<spdlog::sinks::null_sink_st> ());

    struct world { v3 up{0,1,0}, initial_forward{0,0,-1}, initial_left{-1,0,0}; } world;
    Eigen::Map<m3> world_up_initial_forward_left{world.up.data()};
    v3 body_forward = {0,0,-1};

    int remap() {
        return remap_from(cov);
    }

    int remap_from(covariance &other) {
#ifdef TEST_POSDEF
        if(cov.size() && !test_posdef(cov.cov)) log->error("not pos def at beginning of remap");
#endif
        cov.remap_init();
        dynamic_statesize = state_branch<state_node *>::remap(0, cov, node_type::dynamic);
        statesize = state_branch<state_node *>::remap(dynamic_statesize, cov, node_type::constant);
        fake_statesize = state_branch<state_node *>::remap(statesize, cov, node_type::fake) - statesize;
        cov.remap_from(statesize, other);
#ifdef TEST_POSDEF
        if(!test_posdef(cov.cov)) {
            log->error("not pos def at end of remap");
            assert(0);
        }
#endif
        return statesize;
    }

    void copy_from(const state_root &other) {
        current_time = other.current_time;
        world_up_initial_forward_left = other.world_up_initial_forward_left;
        body_forward = other.body_forward;

        g = other.g;

        remap_from(other.cov); // copy covariance
    }

    sensor_clock::time_point get_current_time() { return current_time; }

    void print_matrix_with_state_labels(matrix &state) const {
        if(state.rows() >= dynamic_statesize) state_branch<state_node *>::print_matrix_with_state_labels(state, node_type::dynamic);
        if(state.rows() >= statesize) state_branch<state_node *>::print_matrix_with_state_labels(state, node_type::constant);
        if(state.rows() >= statesize + fake_statesize) state_branch<state_node *>::print_matrix_with_state_labels(state, node_type::fake);
    }

    virtual std::ostream &print_to(std::ostream & s) const
    {
        s << "state: "; return state_branch<state_node*>::print_to(s);
    }

    stdev<1> project_stats;
    virtual void reset() {
        cov.reset();
        state_branch<state_node *>::reset();
        current_time = sensor_clock::micros_to_tp(0);

        g.v = gravity_magnitude;

        project_stats = stdev<1>();
    }

    void evolve(f_t dt)
    {
        evolve_covariance(dt);
        evolve_state(dt);
    }

    matrix &FP;
    void evolve_covariance(f_t dt)
    {
        cache_jacobians(dt);

        FP.resize(dynamic_statesize, cov.size() + fake_statesize);

        {
            auto start = std::chrono::steady_clock::now();
            project_motion_covariance(FP, cov.cov, dt);
            auto stop = std::chrono::steady_clock::now();
            project_stats.data(v<1> { static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
        }

        //fill in the UR and LL matrices
        auto cov_LL = cov.cov.map().block(dynamic_statesize,0, cov.size()-dynamic_statesize,dynamic_statesize);
        auto cov_UR = cov.cov.map().block(0,dynamic_statesize, dynamic_statesize,cov.size()-dynamic_statesize);
        auto  FP_UR =      FP.map().block(0,dynamic_statesize, dynamic_statesize,cov.size()-dynamic_statesize);
        cov_LL.transpose() = cov_UR = FP_UR;

        //compute the UL matrix
        matrix ul(cov.cov, 0, 0, dynamic_statesize, dynamic_statesize);

        {
            auto start = std::chrono::steady_clock::now();
            project_motion_covariance(ul, FP, dt);
            auto stop = std::chrono::steady_clock::now();
            project_stats.data(v<1> { static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
        }

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
            if(time < current_time) log->trace("negative time step: last was {}, this is {}, delta {}", sensor_clock::tp_to_micros(current_time), sensor_clock::tp_to_micros(time), std::chrono::duration_cast<std::chrono::microseconds>(current_time - time).count());
            return;
        }
        if(current_time != sensor_clock::micros_to_tp(0)) {
#ifdef TEST_POSDEF
            if(!test_posdef(cov.cov)) log->error("not pos def before explicit time update");
#endif
            auto dt = std::chrono::duration_cast<std::chrono::duration<f_t>>(time - current_time).count();
            if(log_enabled && dt > .025f) log->warn("Large time step (dt): {}", dt);
            evolve(dt);
#ifdef TEST_POSDEF
            if(!test_posdef(cov.cov)) log->error("not pos def after explicit time update");
#endif
        }
        current_time = time;
    }

    void compute_gravity(double latitude, double altitude) {
        //http://en.wikipedia.org/wiki/Gravity_of_Earth#Free_air_correction
        double sin_lat = sin(latitude/180. * M_PI);
        double sin_2lat = sin(2*latitude/180. * M_PI);
        g.v = gravity_magnitude = (f_t)(9.780327 * (1 + 0.0053024 * sin_lat*sin_lat - 0.0000058 * sin_2lat*sin_2lat) - 3.086e-6 * altitude);
    }

    inline const sensor_clock::time_point & get_current_time() const { return current_time; }

protected:
    virtual void project_motion_covariance(matrix &dst, const matrix &src, f_t dt) const = 0;
    virtual void evolve_state(f_t dt) = 0;
    virtual void cache_jacobians(f_t dt) = 0;

    sensor_clock::time_point current_time;
private:
    f_t gravity_magnitude = (f_t)9.80665;

    using state_branch<state_node*>::print_matrix_with_state_labels; // explicitly hide; we export our own above
    using state_branch<state_node *>::remap; // explicitly hide; we export our own above
};

#endif
