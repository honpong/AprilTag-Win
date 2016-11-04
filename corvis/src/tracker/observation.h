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

extern "C" {
#include "cor_types.h"
}

using namespace std;

class observation {
public:
    const int size;
    sensor_clock::time_point time_actual;
    sensor_clock::time_point time_apparent;

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
    
    observation(sensor &src, int _size, sensor_clock::time_point _time_actual, sensor_clock::time_point _time_apparent): size(_size), time_actual(_time_actual), time_apparent(_time_apparent) {}
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
    virtual void set_prediction_covariance(const matrix &cov, const int index) { for(int i = 0; i < size; ++i) for(int j = 0; j < size; ++j) pred_cov(i, j) = cov(index + i, index + j); }
    virtual void compute_innovation() { inn = meas - pred; }
    virtual f_t innovation(const int i) const { return inn[i]; }
    virtual f_t measurement_covariance(const int i) const { return m_cov[i]; }
    observation_storage(sensor_storage<_size> &src, sensor_clock::time_point _time_actual, sensor_clock::time_point _time_apparent): observation(src, _size, _time_actual, _time_apparent), source(src) {}
};

class observation_vision_feature: public observation_storage<2> {
 private:
    f_t projection_residual(const v3 & X, const feature_t &found);
    const state_vision &state;
    const state_vision_intrinsics &intrinsics;
    const state_extrinsics &extrinsics;
 public:
    m3 Rrt, Rct;
    v3 X0, X;
    m3 Rtot;
    v3 Ttot;

    f_t dx_dp, dy_dp;
    v3 dx_dQr, dy_dQr, dx_dTr, dy_dTr;
    v3 dx_dQc, dy_dQc, dx_dTc, dy_dTc;

    f_t dx_dF, dy_dF;
    f_t dx_dk1, dy_dk1, dx_dk2, dy_dk2, dx_dk3, dy_dk3, dx_dcx, dy_dcx, dx_dcy, dy_dcy;

    state_vision_group *state_group;
    state_vision_feature *feature;
    
    feature_t norm_initial, norm_predicted, Xd;

    virtual void predict();
    virtual void compute_measurement_covariance();
    virtual bool measure();
    virtual void cache_jacobians();
    virtual void project_covariance(matrix &dst, const matrix &src);
    virtual void innovation_covariance_hook(const matrix &cov, int index);
    void update_initializing();

    observation_vision_feature(sensor_grey &src, const state_vision &_state, const state_extrinsics &_extrinsics, const state_vision_intrinsics &_intrinsics, sensor_clock::time_point _time_actual, sensor_clock::time_point _time_apparent): observation_storage(src, _time_actual, _time_apparent), state(_state), extrinsics(_extrinsics), intrinsics(_intrinsics) {}

public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

class observation_spatial: public observation_storage<3> {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    f_t variance;
    virtual void compute_measurement_covariance() { for(int i = 0; i < 3; ++i) m_cov[i] = variance; }
    virtual bool measure() { return true; }
    observation_spatial(sensor_storage<3> &src, sensor_clock::time_point _time_actual, sensor_clock::time_point _time_apparent): observation_storage(src, _time_actual, _time_apparent), variance(0.) {}
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
    observation_accelerometer(sensor_accelerometer &src, state_motion &_state, const state_extrinsics &_extrinsics, const state_imu_intrinsics &_intrinsics, sensor_clock::time_point _time_actual, sensor_clock::time_point _time_apparent): observation_spatial(src, _time_actual, _time_apparent), state(_state), extrinsics(_extrinsics), intrinsics(_intrinsics) {}

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
    observation_gyroscope(sensor_gyroscope &src, const state_motion_orientation &_state, const state_extrinsics &_extrinsics, const state_imu_intrinsics &_intrinsics, sensor_clock::time_point _time_actual, sensor_clock::time_point _time_apparent): observation_spatial(src, _time_actual, _time_apparent), state(_state), extrinsics(_extrinsics), intrinsics(_intrinsics) {}
};

#define MAXOBSERVATIONSIZE 256
static_assert(MAXOBSERVATIONSIZE > MAXSTATESIZE*2, "MAXOBSERVATIONSIZE isn't big enough for MAXSTATESIZE tracked features\n");

class observation_queue {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    observation_queue() {}
    void preprocess(state_root &s, sensor_clock::time_point time);
    bool process(state_root &s);
    vector<unique_ptr<observation>> observations;

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
    int remove_invalid_measurements(const state_root &s, int orig_size, matrix &inn);
    bool update_state_and_covariance(state_root &s, const matrix &inn);

    matrix state   {(f_t*)  state_storage,                  1,       MAXSTATESIZE,                  1,       MAXSTATESIZE };
    matrix inn     {(f_t*)    inn_storage,                  1, MAXOBSERVATIONSIZE,                  1, MAXOBSERVATIONSIZE };
    matrix m_cov   {(f_t*)  m_cov_storage,                  1, MAXOBSERVATIONSIZE,                  1, MAXOBSERVATIONSIZE };
    matrix LC      {(f_t*)     LC_storage, MAXOBSERVATIONSIZE,       MAXSTATESIZE, MAXOBSERVATIONSIZE,       MAXSTATESIZE };
    matrix K       {(f_t*)      K_storage, MAXSTATESIZE,       MAXOBSERVATIONSIZE,       MAXSTATESIZE, MAXOBSERVATIONSIZE };
    matrix res_cov {(f_t*)res_cov_storage, MAXOBSERVATIONSIZE, MAXOBSERVATIONSIZE, MAXOBSERVATIONSIZE, MAXOBSERVATIONSIZE };
    matrix res_tmp {(f_t*)res_cov_storage, MAXOBSERVATIONSIZE, MAXOBSERVATIONSIZE, MAXOBSERVATIONSIZE, MAXOBSERVATIONSIZE };

    f_t state_storage[MAXSTATESIZE];
    f_t inn_storage[MAXOBSERVATIONSIZE];
    f_t m_cov_storage[MAXOBSERVATIONSIZE];
    f_t LC_storage[MAXOBSERVATIONSIZE * MAXSTATESIZE];
    f_t K_storage[MAXOBSERVATIONSIZE * MAXSTATESIZE];
    f_t res_cov_storage[MAXOBSERVATIONSIZE * MAXOBSERVATIONSIZE];
    f_t res_tmp_storage[MAXOBSERVATIONSIZE * MAXOBSERVATIONSIZE];

    static bool observation_comp_actual(const unique_ptr<observation> &p1, const unique_ptr<observation> &p2) { return p1->time_actual < p2->time_actual; }
    static bool observation_comp_apparent(const unique_ptr <observation> &p1, const unique_ptr<observation> &p2) { return p1->time_apparent < p2->time_apparent; }
};

//some object should have functions to evolve the mean and covariance
//balance here between generality (give full linearization that could be used by my batched vision approach to get the partials wrt v, acceleration) and speed - direct update of covariance. Is there a single function that does both? (Can we use the output of the direct update to give the jacobian for vision?)
//Ultimately I use T,W. Can we keep the partial derivatives of these updated wrt their integration for use in vision meas?
//include earth's rotation in IMU
//Runge-Kutta



#endif
