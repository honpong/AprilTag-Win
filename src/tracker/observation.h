#ifndef __OBSERVATION_H
#define __OBSERVATION_H

#include "state_vision.h"
#include "matrix.h"
#include "vec4.h"
#include <vector>
#include <map>
#include <algorithm>
#include "platform/sensor_clock.h"
#include <memory>
#include "sensor.h"

#include "cor_types.h"

#ifdef MYRIAD2
    #include "platform_defines.h"
    #include "project_covariance_definitions.h"
#endif

class observation {
public:
    const int size;
    virtual void predict() = 0;
    virtual void compute_innovation() = 0;
    virtual void compute_measurement_covariance() = 0;
    virtual bool measure() = 0;
    virtual void cache_jacobians() = 0;
    virtual void project_covariance(matrix &dst, const matrix &src) const = 0;
    virtual void innovation_covariance_hook(const matrix &cov, int index);
    virtual f_t innovation(const int i) const = 0;
    virtual f_t measurement_covariance(const int i) const = 0;
#ifdef ENABLE_SHAVE_PROJECT_OBSERVATION_COVARIANCE
    virtual observation_data* getData(int index = 0) = 0;
#endif
    
    observation(int _size): size(_size) {}
    virtual ~observation() {};
};

template<int _size> class observation_storage: public observation {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
protected:
    v<_size> m_cov, pred, inn;
public:
    v<_size> meas;
    sensor_storage<_size> &source;

    template <int Rows, typename Stride_ = Eigen::Stride<Rows == 1 ? 0 : Eigen::Dynamic, Rows == 1 ? Eigen::Dynamic : 1> >
    struct inner_stride : Stride_ {
        inner_stride(int outer) : Stride_(Rows == 1 ? 0 : outer, Rows == 1 ? outer : 1) {}
    };

    template<int Rows = 1>
    inline Eigen::Map< m<_size, Rows>, Eigen::Unaligned, inner_stride<Rows>> col(matrix &m, int i) const {
        return decltype(col<Rows>(m,i)) { &m(0,i), inner_stride<Rows>(m.get_stride()) };
    }

    virtual void compute_innovation() { inn = meas - pred; }
    virtual f_t innovation(const int i) const { return inn[i]; }
    virtual f_t measurement_covariance(const int i) const { return m_cov[i]; }
    observation_storage(sensor_storage<_size> &src): observation(_size), source(src) {}
};

class observation_vision_feature: public observation_storage<2> {
 private:
    f_t projection_residual(const v3 & X, const feature_t &found);
 public:
    m3 Rt, Rr, Rw, Rct;
    v3 Tw;
    v3 X0, X;
    m3 Rtot;
    v3 Ttot;

    m<2,1> dx_dp;
    m<2,3> dx_dQr, dx_dQ, dx_dTr, dx_dT;
    struct camera_derivative {
        camera_derivative(const state_camera &c) : camera(c) {}
        const state_camera &camera;
        m<2,3> dx_dQ, dx_dT;
        m<2,1> dx_dF;
        m<2,2> dx_dc;
        m<2,4> dx_dk;
    } orig, curr;

    state_motion &state;
    state_vision_feature *const feature;
    state_vision_track &track;

    feature_t Xd;

    virtual void predict();
    virtual void compute_measurement_covariance();
    virtual bool measure();
    virtual void cache_jacobians();
    template<int N>
             int project_covariance(matrix &dst, const matrix &src, int i) const;
    virtual void project_covariance(matrix &dst, const matrix &src) const;
    virtual void innovation_covariance_hook(const matrix &cov, int index);
#ifdef ENABLE_SHAVE_PROJECT_OBSERVATION_COVARIANCE
    virtual observation_data* getData(int index = 0);
#endif
    void update_initializing();

    observation_vision_feature(state_motion &s, sensor_grey &src, const state_camera &camera, state_vision_feature &f, state_vision_track &t)
        : observation_storage(src), orig(f.group.camera), curr(camera), state(s), feature(&f), track(t) {}

public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

template <int size_>
class observation_spatial : public observation_storage<size_> {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    f_t variance;
    virtual void compute_measurement_covariance() {
        observation_storage<size_>::m_cov = v<size_>::Constant(variance);
    }
    virtual bool measure() { return true; }
    observation_spatial(sensor_storage<size_> &src) : observation_storage<size_>(src), variance(0.) {}
};

class observation_accelerometer: public observation_spatial<3> {
protected:
    const state_root &root;
    const state_motion &state;
    const state_extrinsics &extrinsics;
    const state_imu_intrinsics &intrinsics;
    v3 xcc;
    m3 Rt, Ra, da_dQ, da_dw, da_ddw, da_dacc;
    m3 da_dQa, da_dTa;
public:
    virtual void predict();
    virtual bool measure();
    virtual void compute_measurement_covariance() {
        source.inn_stdev.data(inn);
        observation_spatial::compute_measurement_covariance();
    }
    virtual void cache_jacobians();
    template<int N>
             int project_covariance(matrix &dst, const matrix &src, int i) const;
    virtual void project_covariance(matrix &dst, const matrix &src) const;
#ifdef ENABLE_SHAVE_PROJECT_OBSERVATION_COVARIANCE
    virtual observation_data* getData(int index = 0);
#endif
    observation_accelerometer(sensor_accelerometer &src, const state_root &root_, const state_motion &state_, const state_imu &imu): observation_spatial(src), root(root_), state(state_), extrinsics(imu.extrinsics), intrinsics(imu.intrinsics) {}

public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

class observation_gyroscope: public observation_spatial<3> {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
protected:
    const state_motion_orientation &state;
    const state_extrinsics &extrinsics;
    const state_imu_intrinsics &intrinsics;
    m3 Rw;
    m3 dw_dQw;
public:
    virtual void predict();
    virtual bool measure() {
        source.meas_stdev.data(meas);
        return observation_spatial::measure();
    }
    virtual void compute_measurement_covariance() { 
        source.inn_stdev.data(inn);
        observation_spatial::compute_measurement_covariance();
    }
    virtual void cache_jacobians();
    template<int N>
             int project_covariance(matrix &dst, const matrix &src, int i) const;
    virtual void project_covariance(matrix &dst, const matrix &src) const;
#ifdef ENABLE_SHAVE_PROJECT_OBSERVATION_COVARIANCE
    virtual observation_data* getData(int index = 0);
#endif
    observation_gyroscope(sensor_gyroscope &src, const state_motion_orientation &_state, const state_imu &imu): observation_spatial(src), state(_state), extrinsics(imu.extrinsics), intrinsics(imu.intrinsics) {}
};

class observation_velocimeter: public observation_spatial<3> {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
protected:
    const state_motion &state;
    const state_extrinsics &extrinsics;
    const state_velocimeter_intrinsics &intrinsics;
    m3 dv_dQ, dv_dV, dv_dw;
    m3 dv_dTv, dv_dQv ; // extrinsics
 public:
    virtual void predict();
    virtual bool measure() {
        source.meas_stdev.data(meas);
        return observation_spatial::measure();
    }
    virtual void compute_measurement_covariance() {
        source.inn_stdev.data(inn);
        observation_spatial::compute_measurement_covariance();
    }
    virtual void cache_jacobians();
    template<int N>
             int project_covariance(matrix &dst, const matrix &src, int i) const;
    virtual void project_covariance(matrix &dst, const matrix &src) const;
#ifdef ENABLE_SHAVE_PROJECT_OBSERVATION_COVARIANCE
    virtual observation_data* getData(int index = 0);
#endif
    observation_velocimeter(sensor_velocimeter &src, const state_motion &_state, const state_extrinsics &_extrinsics, const state_velocimeter_intrinsics &_intrinsics): observation_spatial(src), state(_state), extrinsics(_extrinsics), intrinsics(_intrinsics) {}
};

class observation_queue {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    observation_queue(matrix &state_, matrix &inn_, matrix &m_cov_, matrix &HP_, matrix &res_cov_)
        : state(state_), inn(inn_), m_cov(m_cov_), HP(HP_), res_cov(res_cov_)  {}
    void preprocess(state_root &s, sensor_clock::time_point time);
    bool process(state_root &s, bool run_on_shave = false);
    std::vector<std::unique_ptr<observation>> observations;
    stdev<1> project_stats, solve_stats, cholesky_stats, multiply_stats, state_size_stats, meas_size_stats;
    void reset() {
        observations.clear();
        project_stats = stdev<1>();
        solve_stats = stdev<1>();
        cholesky_stats = stdev<1>();
        multiply_stats = stdev<1>();
        meas_size_stats = stdev<1>();
        state_size_stats = stdev<1>();
    }

    // keep the most recent measurement of a given type around for plotting, etc
    std::unique_ptr<observation_gyroscope> recent_g;
    std::unique_ptr<observation_accelerometer> recent_a;
    std::unique_ptr<observation_velocimeter> recent_v;
    std::map<uint64_t, std::unique_ptr<observation_vision_feature>> recent_f_map;
    //std::unique_ptr<std::map<uint64_t, unique_ptr<observation_vision_feature>>> recent_f_map;
    void cache_recent(std::unique_ptr<observation> &&o) {
#ifndef MYRIAD2 // Doesn't support dynamic_cast
        if (auto *ovf = dynamic_cast<observation_vision_feature*>(o.get()))
            recent_f_map[ovf->feature->feature->id] = std::unique_ptr<observation_vision_feature>(static_cast<observation_vision_feature*>(o.release()));
        else if (dynamic_cast<observation_accelerometer*>(o.get()))
            recent_a = std::unique_ptr<observation_accelerometer>(static_cast<observation_accelerometer*>(o.release()));
        else if (dynamic_cast<observation_gyroscope*>(o.get()))
            recent_g = std::unique_ptr<observation_gyroscope>(static_cast<observation_gyroscope*>(o.release()));
        else if (dynamic_cast<observation_velocimeter*>(o.get()))
            recent_v = std::unique_ptr<observation_velocimeter>(static_cast<observation_velocimeter*>(o.release()));
#endif
    }

protected:
    int size();
    void predict();
    void measure_and_prune();
    void compute_innovation(matrix &inn);
    void compute_measurement_covariance(matrix &m_cov);
    void compute_prediction_covariance(const matrix &cov, int statesize, int meas_size);
#ifdef ENABLE_SHAVE_PROJECT_OBSERVATION_COVARIANCE
    void compute_prediction_covariance_shave(const matrix &cov, int statesize, int meas_size);
#endif
    void compute_innovation_covariance(const matrix &m_cov);
    bool update_state_and_covariance(matrix &state, matrix &cov, const matrix &inn, matrix &HP, matrix &res_cov);

    matrix &state, &inn, &m_cov, &HP, &res_cov;
#ifdef ENABLE_SHAVE_PROJECT_OBSERVATION_COVARIANCE
    std::vector<observation_data*> observation_datas;
    int start_index[PROJECT_COVARIANE_SHAVES];
    int obs_per_shave;
#endif
};

//some object should have functions to evolve the mean and covariance
//balance here between generality (give full linearization that could be used by my batched vision approach to get the partials wrt v, acceleration) and speed - direct update of covariance. Is there a single function that does both? (Can we use the output of the direct update to give the jacobian for vision?)
//Ultimately I use T,W. Can we keep the partial derivatives of these updated wrt their integration for use in vision meas?
//include earth's rotation in IMU
//Runge-Kutta



#endif