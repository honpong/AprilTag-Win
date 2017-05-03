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

class observation {
public:
    const int size;
    virtual void predict() = 0;
    virtual void compute_innovation() = 0;
    virtual void compute_measurement_covariance() = 0;
    virtual bool measure() = 0;
    virtual void cache_jacobians() = 0;
    virtual void project_covariance(matrix &dst, const matrix &src) = 0;
    virtual void set_prediction_covariance(const matrix &cov, const int index) = 0;
    virtual void innovation_covariance_hook(const matrix &cov, int index) = 0;
    virtual f_t innovation(const int i) const = 0;
    virtual f_t measurement_covariance(const int i) const = 0;
    
    observation(sensor &src, int _size): size(_size) {}
    virtual ~observation() {};
};

template<int _size> class observation_storage: public observation {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
protected:
    v<_size> m_cov, pred, inn;
    m<_size> pred_cov;
public:
    v<_size> meas;
    sensor_storage<_size> &source;

    Eigen::Map< Eigen::Matrix<f_t, _size, 1>, Eigen::Unaligned, Eigen::InnerStride<> >
        col(matrix &m, int i) const { return decltype(col(m,i)) { &m(0,i), Eigen::InnerStride<>(m.get_stride()) }; }

    virtual void set_prediction_covariance(const matrix &cov, const int index) { for(int i = 0; i < size; ++i) for(int j = 0; j < size; ++j) pred_cov(i, j) = cov(index + i, index + j); }
    virtual void compute_innovation() { inn = meas - pred; }
    virtual f_t innovation(const int i) const { return inn[i]; }
    virtual f_t measurement_covariance(const int i) const { return m_cov[i]; }
    observation_storage(sensor_storage<_size> &src): observation(src, _size), source(src) {}
};

class observation_vision_feature: public observation_storage<2> {
 private:
    f_t projection_residual(const v3 & X, const feature_t &found);
 public:
    m3 Rrt, Rct;
    v3 X0, X;
    m3 Rtot;
    v3 Ttot;

    f_t dx_dp, dy_dp;
    v3 dx_dQr, dy_dQr, dx_dTr, dy_dTr;
    struct intrinsics_derivative {
        intrinsics_derivative(const state_camera &c) : camera(c) {}
        const state_camera &camera;
        v3 dx_dQ, dy_dQ, dx_dT, dy_dT;
        f_t dx_dF, dy_dF;
        f_t dx_dk1, dy_dk1, dx_dk2, dy_dk2, dx_dk3, dy_dk3, dx_dk4, dy_dk4, dx_dcx, dy_dcx, dx_dcy, dy_dcy;
    } orig, curr;

    state_vision_feature *const feature;

    feature_t norm_initial, norm_predicted, Xd;

    virtual void predict();
    virtual void compute_measurement_covariance();
    virtual bool measure();
    virtual void cache_jacobians();
    virtual void project_covariance(matrix &dst, const matrix &src);
    virtual void innovation_covariance_hook(const matrix &cov, int index);
    void update_initializing();

    observation_vision_feature(sensor_grey &src, const state_camera &camera, state_vision_feature &f)
        : observation_storage(src), curr(camera), orig(f.group.camera), feature(&f) {}

public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

class observation_spatial: public observation_storage<3> {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    f_t variance;
    virtual void compute_measurement_covariance() { for(int i = 0; i < 3; ++i) m_cov[i] = variance; }
    virtual bool measure() { return true; }
    observation_spatial(sensor_storage<3> &src): observation_storage(src), variance(0.) {}
    void innovation_covariance_hook(const matrix &cov, int index);
};

class observation_accelerometer: public observation_spatial {
protected:
    state_motion &state;
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
    virtual void project_covariance(matrix &dst, const matrix &src);
    observation_accelerometer(sensor_accelerometer &src, state_motion &_state, const state_extrinsics &_extrinsics, const state_imu_intrinsics &_intrinsics): observation_spatial(src), state(_state), extrinsics(_extrinsics), intrinsics(_intrinsics) {}

public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

class observation_gyroscope: public observation_spatial {
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
    virtual void project_covariance(matrix &dst, const matrix &src);
    observation_gyroscope(sensor_gyroscope &src, const state_motion_orientation &_state, const state_extrinsics &_extrinsics, const state_imu_intrinsics &_intrinsics): observation_spatial(src), state(_state), extrinsics(_extrinsics), intrinsics(_intrinsics) {}
};

#define MAXOBSERVATIONSIZE (MAXSTATESIZE * 2)
#define MAXOBSERVATIONSIZE_PADDED ((MAXOBSERVATIONSIZE + 7) & ~7)
static_assert(MAXOBSERVATIONSIZE >= MAXSTATESIZE*2, "MAXOBSERVATIONSIZE isn't big enough for MAXSTATESIZE tracked features\n");

class observation_queue {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    observation_queue() {}
    void preprocess(state_root &s, sensor_clock::time_point time);
    bool process(state_root &s);
    std::vector<std::unique_ptr<observation>> observations;

    // keep the most recent measurement of a given type around for plotting, etc
    std::unique_ptr<observation_gyroscope> recent_g;
    std::unique_ptr<observation_accelerometer> recent_a;
    std::map<uint64_t, std::unique_ptr<observation_vision_feature>> recent_f_map;
    //std::unique_ptr<std::map<uint64_t, unique_ptr<observation_vision_feature>>> recent_f_map;
    void cache_recent(std::unique_ptr<observation> &&o) {
        if (auto *ovf = dynamic_cast<observation_vision_feature*>(o.get()))
            recent_f_map[ovf->feature->id] = std::unique_ptr<observation_vision_feature>(static_cast<observation_vision_feature*>(o.release()));
        else if (dynamic_cast<observation_accelerometer*>(o.get()))
            recent_a = std::unique_ptr<observation_accelerometer>(static_cast<observation_accelerometer*>(o.release()));
        else if (dynamic_cast<observation_gyroscope*>(o.get()))
            recent_g = std::unique_ptr<observation_gyroscope>(static_cast<observation_gyroscope*>(o.release()));
    }

protected:
    int size();
    void predict();
    void measure_and_prune();
    void compute_innovation(matrix &inn);
    void compute_measurement_covariance(matrix &m_cov);
    void compute_prediction_covariance(const state_root &s, int meas_size);
    void compute_innovation_covariance(const matrix &m_cov);
    bool update_state_and_covariance(state_root &s, const matrix &inn);

    matrix state   {   state_storage };
    matrix inn     {     inn_storage };
    matrix m_cov   {   m_cov_storage };
    matrix HP      {      HP_storage };
    matrix K       {       K_storage };
    matrix res_cov { res_cov_storage };

    alignas(64) f_t state_storage[MAXSTATESIZE];
    alignas(64) f_t inn_storage[MAXOBSERVATIONSIZE];
    alignas(64) f_t m_cov_storage[MAXOBSERVATIONSIZE];
    alignas(64) f_t HP_storage[MAXOBSERVATIONSIZE][MAXSTATESIZE_PADDED];
    alignas(64) f_t K_storage[MAXSTATESIZE][MAXOBSERVATIONSIZE_PADDED];
    alignas(64) f_t res_cov_storage[MAXOBSERVATIONSIZE][MAXOBSERVATIONSIZE_PADDED];
};

//some object should have functions to evolve the mean and covariance
//balance here between generality (give full linearization that could be used by my batched vision approach to get the partials wrt v, acceleration) and speed - direct update of covariance. Is there a single function that does both? (Can we use the output of the direct update to give the jacobian for vision?)
//Ultimately I use T,W. Can we keep the partial derivatives of these updated wrt their integration for use in vision meas?
//include earth's rotation in IMU
//Runge-Kutta



#endif
