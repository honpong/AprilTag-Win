#ifndef __OBSERVATION_H
#define __OBSERVATION_H

#include "state_vision.h"
#include "../numerics/matrix.h"
#include "../numerics/vec4.h"
#include <vector>
#include <algorithm>
#include "tracker.h"

extern "C" {
#include "../cor/cor_types.h"
}

using namespace std;

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
    virtual void cache_jacobians() = 0;
    virtual void project_covariance(matrix &dst, const matrix &src) = 0;
    virtual void set_prediction_covariance(const matrix &cov, const int index) = 0;
    virtual void innovation_covariance_hook(const matrix &cov, int index) = 0;
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
    f_t pred_cov[_size][_size];
public:
    f_t meas[_size];
    virtual void set_prediction_covariance(const matrix &cov, const int index) { for(int i = 0; i < size; ++i) for(int j = 0; j < size; ++j) pred_cov[i][j] = cov(index + i, index + j); }
    virtual void compute_innovation() { for(int i = 0; i < size; ++i) inn[i] = meas[i] - pred[i]; }
    virtual f_t innovation(const int i) const { return inn[i]; }
    virtual f_t measurement_covariance(const int i) const { return m_cov[i]; }
    observation_storage(uint64_t _time_actual, uint64_t _time_apparent): observation(_size, _time_actual, _time_apparent) {}
};

class observation_vision_feature: public observation_storage<2> {
 private:
    f_t projection_residual(const v4 & X_inf, const f_t inv_depth, const xy &found);
    const state_vision &state;
 public:
    static stdev_scalar stdev[2], inn_stdev[2];
    m4 Rt, Rbc, Rcb, RcbRt, Rr, Rw;
    v4 Tlocal;
    v4 X0, X;
    uint8_t *im1, *im2;
    struct tracker tracker;
    m4 Rtot;
    v4 Ttot;
    f_t dx_dp, dy_dp, dx_dF, dy_dF;

    f_t dx_dk1, dy_dk1, dx_dk2, dy_dk2, dx_dcx, dy_dcx, dx_dcy, dy_dcy;
    v4 dx_dW, dy_dW, dx_dWc, dy_dWc, dx_dTc, dy_dTc, dx_dWr, dy_dWr, dx_dTr, dy_dTr;

    state_vision_group *state_group;
    state_vision_feature *feature;
    
    feature_t norm_initial, norm_predicted;

    virtual void predict();
    virtual void compute_measurement_covariance();
    virtual bool measure();
    virtual void cache_jacobians();
    virtual void project_covariance(matrix &dst, const matrix &src);
    virtual void innovation_covariance_hook(const matrix &cov, int index);
    void update_initializing();

    observation_vision_feature(state_vision &_state, uint64_t _time_actual, uint64_t _time_apparent): observation_storage(_time_actual, _time_apparent), state(_state) {}
};

#ifndef SWIG
class observation_spatial: public observation_storage<3> {
 public:
    f_t variance;
    virtual void compute_measurement_covariance() { for(int i = 0; i < 3; ++i) m_cov[i] = variance; }
    virtual bool measure() { return true; }
    observation_spatial(uint64_t _time_actual, uint64_t _time_apparent): observation_storage(_time_actual, _time_apparent), variance(0.) {}
    void innovation_covariance_hook(const matrix &cov, int index)
    {
        if(show_tuning) {
            fprintf(stderr, " predicted stdev is %e %e %e\n", sqrtf(cov(index, index)), sqrtf(cov(index+1, index+1)), sqrtf(cov(index+2, index+2)));
        }
    }
};
#endif

class observation_accelerometer: public observation_spatial {
protected:
    state_motion &state;
    m4 Rt;
    m4v4 dR_dW;
    m4 dya_dW;
 public:
    static stdev_vector stdev, inn_stdev;
    virtual void predict();
    virtual bool measure();
    virtual void compute_measurement_covariance() {
        inn_stdev.data(v4(inn[0], inn[1], inn[2], 0.));
        observation_spatial::compute_measurement_covariance();
    }
    virtual void cache_jacobians();
    virtual void project_covariance(matrix &dst, const matrix &src);
    observation_accelerometer(state_motion &_state, uint64_t _time_actual, uint64_t _time_apparent): observation_spatial(_time_actual, _time_apparent), state(_state) {}
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
    virtual void cache_jacobians();
    virtual void project_covariance(matrix &dst, const matrix &src);
    observation_gyroscope(state_motion_orientation &_state, uint64_t _time_actual, uint64_t _time_apparent): observation_spatial(_time_actual, _time_apparent), state(_state) {}
};

#define MAXOBSERVATIONSIZE 256

class observation_queue {
public:
    observation_queue();
    bool process(state &s, uint64_t time);
    vector<observation *> observations;
    void clear();
    
protected:
    int preprocess();
    void predict();
    void measure();
    void compute_innovation(matrix &inn);
    void compute_measurement_covariance(matrix &m_cov);
    void compute_prediction_covariance(const state &s, int meas_size);
    void compute_innovation_covariance(const matrix &m_cov);
    int remove_invalid_measurements(const state &s, int orig_size, matrix &inn);
    bool update_state_and_covariance(state &s, const matrix &inn);
#ifndef SWIG
    matrix LC;
    matrix K;
    matrix res_cov;
#endif
    static bool observation_comp_actual(observation *p1, observation *p2) { return p1->time_actual < p2->time_actual; }
    static bool observation_comp_apparent(observation *p1, observation *p2) { return p1->time_apparent < p2->time_apparent; }
    
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
