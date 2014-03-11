#ifndef __OBSERVATION_H
#define __OBSERVATION_H

#include "model.h"
#include "../numerics/matrix.h"
#include "../numerics/vec4.h"
#include <vector>
#include <algorithm>
#include "tracker.h"

extern "C" {
#include "cor.h"
}

using namespace std;

class preobservation {
 public:
    const state_vision &state;
    virtual void process() = 0;
    preobservation(state_vision &s): state(s) {}
    virtual ~preobservation() {};
};

class preobservation_vision_base: public preobservation {
 public:
    m4 R, Rt, Rbc, Rcb, RcbRt;
    m4v4 dR_dW, dRbc_dWc, dRt_dW, dRcb_dWc;
    uint8_t *im1, *im2;
    struct tracker tracker;

    virtual void process();

    preobservation_vision_base(state_vision &s, struct tracker t): preobservation(s), tracker(t) {
    }
};

class preobservation_vision_group: public preobservation {
 public:
    v4 Tr;
    rotation_vector Wr;
    m4 Rr, Rw, Rtot;
    v4 Tw, Ttot;
    m4v4 dRr_dWr, dRtot_dW, dRtot_dWr, dRtot_dWc;
    m4 dTtot_dWc, dTtot_dW, dTtot_dWr, dTtot_dT, dTtot_dTc, dTtot_dTr;

    preobservation_vision_base *base;

    virtual void process();
 preobservation_vision_group(state_vision &s): preobservation(s) {}
};

class observation {
 public:
    const int size;
    uint64_t time_actual;
    uint64_t time_apparent;
    bool valid;

    virtual void predict() = 0;
    virtual void compute_innovation() = 0;
    virtual void compute_measurement_covariance() = 0;
    virtual bool measure() = 0;
    virtual void project_covariance(matrix &dst, const matrix &src) = 0;
    virtual f_t innovation(const int i) const = 0;
    virtual f_t measurement_covariance(const int i) const = 0;
    
    observation(int _size, uint64_t _time_actual, uint64_t _time_apparent): size(_size), time_actual(_time_actual), time_apparent(_time_apparent), valid(true) {}
    virtual ~observation() {};
};

template<int _size> class observation_storage: public observation {
protected:
    f_t m_cov[_size];
    f_t pred[_size];
    f_t inn[_size];
public:
    f_t meas[_size];
    virtual void compute_innovation() { for(int i = 0; i < size; ++i) inn[i] = meas[i] - pred[i]; }
    virtual f_t innovation(const int i) const { return inn[i]; }
    virtual f_t measurement_covariance(const int i) const { return m_cov[i]; }
    observation_storage(uint64_t _time_actual, uint64_t _time_apparent): observation(_size, _time_actual, _time_apparent) {}
};

class observation_vision_feature: public observation_storage<2> {
 private:
    f_t projection_residual(const v4 & X_inf, const f_t inv_depth, const feature_t &found);
    const state_vision &state;
 public:
    static stdev_scalar stdev[2], inn_stdev[2];
    m4 dy_dX;
    v4 X0;
    v4 dy_dF, dy_dk1, dy_dk2, dy_dk3, dy_dcx, dy_dcy;

    preobservation_vision_base *base;
    preobservation_vision_group *group;

    state_vision_group *state_group;
    state_vision_feature *feature;

    virtual void predict();
    virtual void compute_measurement_covariance();
    virtual bool measure();
    virtual void project_covariance(matrix &dst, const matrix &src);

    observation_vision_feature(state_vision &_state, uint64_t _time_actual, uint64_t _time_apparent): state(_state), observation_storage(_time_actual, _time_apparent) {}
};

#ifndef SWIG
class observation_spatial: public observation_storage<3> {
 public:
    f_t variance;
    virtual void compute_measurement_covariance() { for(int i = 0; i < 3; ++i) m_cov[i] = variance; }
    virtual bool measure() { return true; }
    observation_spatial(uint64_t _time_actual, uint64_t _time_apparent): observation_storage(_time_actual, _time_apparent), variance(0.) {}
};
#endif

class observation_accelerometer: public observation_spatial {
protected:
    const state_motion &state;
 public:
    static stdev_vector stdev, inn_stdev;
    virtual void predict();
    virtual bool measure() {
        stdev.data(v4(meas[0], meas[1], meas[2], 0.));
        return observation_spatial::measure();
    }                   
    virtual void compute_measurement_covariance() { 
        inn_stdev.data(v4(inn[0], inn[1], inn[2], 0.));
        observation_spatial::compute_measurement_covariance();
    }
    virtual void project_covariance(matrix &dst, const matrix &src);
    observation_accelerometer(state_motion &_state, uint64_t _time_actual, uint64_t _time_apparent): state(_state), observation_spatial(_time_actual, _time_apparent) {}
};

class observation_accelerometer_orientation: public observation_spatial {
protected:
    const state_motion_orientation &state;
public:
    static stdev_vector stdev, inn_stdev;
    virtual void predict();
    virtual bool measure() {
        stdev.data(v4(meas[0], meas[1], meas[2], 0.));
        return observation_spatial::measure();
    }
    virtual void compute_measurement_covariance() {
        inn_stdev.data(v4(inn[0], inn[1], inn[2], 0.));
        observation_spatial::compute_measurement_covariance();
    }
    virtual void project_covariance(matrix &dst, const matrix &src);
    observation_accelerometer_orientation(state_motion_orientation &_state, uint64_t _time_actual, uint64_t _time_apparent): state(_state), observation_spatial(_time_actual, _time_apparent) {}
};

class observation_gyroscope: public observation_spatial {
protected:
    const state_motion_orientation &state;

 public:
    static stdev_vector stdev, inn_stdev;
    virtual void predict();
    virtual bool measure() {
        stdev.data(v4(meas[0], meas[1], meas[2], 0.));
        return observation_spatial::measure();
    }
    virtual void compute_measurement_covariance() { 
        inn_stdev.data(v4(inn[0], inn[1], inn[2], 0.));
        observation_spatial::compute_measurement_covariance();
    }
    virtual void project_covariance(matrix &dst, const matrix &src);
    observation_gyroscope(state_motion_orientation &_state, uint64_t _time_actual, uint64_t _time_apparent): state(_state), observation_spatial(_time_actual, _time_apparent) {}
};

#define MAXOBSERVATIONSIZE 256

class observation_queue {
 public:
    void preprocess();
    void clear();
    void predict();
    void compute_measurement_covariance();
    observation_queue();

#ifndef SWIG
    matrix LC;
    matrix K;
    matrix res_cov;
#endif

    // private:
    static bool observation_comp_actual(observation *p1, observation *p2) { return p1->time_actual < p2->time_actual; }
    static bool observation_comp_apparent(observation *p1, observation *p2) { return p1->time_apparent < p2->time_apparent; }
    vector<observation *> observations;
    list<preobservation *> preobservations;
    
    v_intrinsic LC_storage[MAXOBSERVATIONSIZE * MAXSTATESIZE / 4];
    v_intrinsic K_storage[MAXOBSERVATIONSIZE * MAXSTATESIZE / 4];
    v_intrinsic res_cov_storage[MAXOBSERVATIONSIZE * MAXOBSERVATIONSIZE / 4];
};

//some object should have functions to evolve the mean and covariance
//balance here between generality (give full linearization that could be used by my batched vision approach to get the partials wrt v, acceleration) and speed - direct update of covariance. Is there a single function that does both? (Can we use the output of the direct update to give the jacobian for vision?)
//Ultimately I use T,W. Can we keep the partial derivatives of these updated wrt their integration for use in vision meas?
//include earth's rotation in IMU
//Runge-Kutta



#endif
