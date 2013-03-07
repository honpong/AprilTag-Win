#ifndef __OBSERVATION_H
#define __OBSERVATION_H

#include "model.h"
#include "../numerics/matrix.h"
#include "../numerics/vec4.h"
#include <vector>
#include <algorithm>

extern "C" {
#include "../calibration/calibration.h"
}

using namespace std;

class preobservation {
 public:
    state_vision *state;
    virtual void process(bool linearize) = 0;
 preobservation(state_vision *s): state(s) {}
};

class preobservation_vision_base: public preobservation {
 public:
    m4 R, Rt, Rbc, Rcb, RcbRt;
    m4v4 dR_dW, dRbc_dWc, dRt_dW, dRcb_dWc;
    camera_calibration *cal;

    virtual void process(bool linearize);
 preobservation_vision_base(state_vision *s): preobservation(s) {}
};

class preobservation_vision_group: public preobservation {
 public:
    m4 Rr, Rw, Rtot;
    v4 Tw, Ttot;
    m4v4 dRr_dWr, dRtot_dW, dRtot_dWr, dRtot_dWc;
    m4 dTtot_dWc, dTtot_dW, dTtot_dWr, dTtot_dT, dTtot_dTc, dTtot_dTr;

    preobservation_vision_base *base;
    state_vision_group *group;

    virtual void process(bool linearize);
 preobservation_vision_group(state_vision *s): preobservation(s) {}
};

class observation {
 public:
    int size;
    state_vision *state;
    uint64_t time_actual;
    uint64_t time_apparent;
    matrix lp;
    f_t *m_cov;
    f_t *pred;
    f_t *meas;
    f_t *inn;

    virtual void predict(bool linearize) = 0;
    virtual void compute_covariance() = 0;
    virtual bool measure() = 0;

 observation(int _size, state_vision *_state, uint64_t _time_actual, uint64_t _time_apparent, int index, matrix &_lp, matrix &_m_cov, matrix &_pred, matrix &_meas, matrix &_inn): size(_size), state(_state), time_actual(_time_actual), time_apparent(_time_apparent), lp(size?&_lp(index, 0):0, size, _lp.cols, size, _lp.stride), m_cov(size?&_m_cov[index]:0), pred(size?&_pred[index]:0), meas(size?&_meas[index]:0), inn(size?&_inn[index]:0) {}
};

class observation_vision_feature: public observation {
 public:
    preobservation_vision_base *base;
    preobservation_vision_group *group;

    state_vision_group *state_group;
    state_vision_feature *feature;

    virtual void predict(bool linearize);
    virtual void compute_covariance();
    virtual bool measure();

 observation_vision_feature(state_vision *_state, uint64_t _time_actual, uint64_t _time_apparent, int index, matrix &_lp, matrix &_m_cov, matrix &_pred, matrix &_meas, matrix &_inn): observation(2, _state, _time_actual, _time_apparent, index, _lp, _m_cov, _pred, _meas, _inn) {}
};

class observation_vision_feature_initializing: public observation {
 public:
    preobservation_vision_base *base;

    state_vision_feature *feature;

    virtual void predict(bool linearize);
    virtual void compute_covariance() {};
    virtual bool measure();

 observation_vision_feature_initializing(state_vision *_state, uint64_t _time_actual, uint64_t _time_apparent, int index, matrix &_lp, matrix &_m_cov, matrix &_pred, matrix &_meas, matrix &_inn): observation(0, _state, _time_actual, _time_apparent, index, _lp, _m_cov, _pred, _meas, _inn) {}
};

class observation_spatial: public observation {
 public:
    f_t variance;
    bool initializing;
    virtual void compute_covariance() { for(int i = 0; i < 3; ++i) m_cov[i] = variance; }
    virtual bool measure() { return true; }

 observation_spatial(state_vision *_state, uint64_t _time_actual, uint64_t _time_apparent, int index, matrix &_lp, matrix &_m_cov, matrix &_pred, matrix &_meas, matrix &_inn): observation(3, _state, _time_actual, _time_apparent, index, _lp, _m_cov, _pred, _meas, _inn) {}
};

class observation_accelerometer: public observation_spatial {
 public:
    virtual void predict(bool linearize);
 observation_accelerometer(state_vision *_state, uint64_t _time_actual, uint64_t _time_apparent, int index, matrix &_lp, matrix &_m_cov, matrix &_pred, matrix &_meas, matrix &_inn): observation_spatial(_state, _time_actual, _time_apparent, index, _lp, _m_cov, _pred, _meas, _inn) {}
};

class observation_gyroscope: public observation_spatial {
 public:
    virtual void predict(bool linearize);
 observation_gyroscope(state_vision *_state, uint64_t _time_actual, uint64_t _time_apparent, int index, matrix &_lp, matrix &_m_cov, matrix &_pred, matrix &_meas, matrix &_inn): observation_spatial(_state, _time_actual, _time_apparent, index, _lp, _m_cov, _pred, _meas, _inn) {}
};

#define MAXOBSERVATIONSIZE 256

class observation_queue {
 public:
    int meas_size;

    observation_vision_feature *new_observation_vision_feature(state *_state, uint64_t _time_actual, uint64_t _time_apparent);
    observation_vision_feature_initializing *new_observation_vision_feature_initializing(state_vision *_state, uint64_t _time_actual, uint64_t _time_apparent);
    observation_accelerometer *new_observation_accelerometer(state *_state, uint64_t _time_actual, uint64_t _time_apparent);
    observation_gyroscope *new_observation_gyroscope(state *_state, uint64_t _time_actual, uint64_t _time_apparent);

    template <class T> T *new_preobservation(state *state);
    int preprocess(bool linearize, int statesize);
    void clear();
    void predict(bool linearize, int statesize);
    void compute_innovation();
    void compute_covariance();
    void grow_matrices(int inc);
    observation_queue();

#ifndef SWIG
    matrix lp;
    matrix m_cov;
    matrix pred;
    matrix meas;
    matrix inn;
#endif

    // private:
    static bool observation_comp_actual(observation *p1, observation *p2) { return p1->time_actual < p2->time_actual; }
    static bool observation_comp_apparent(observation *p1, observation *p2) { return p1->time_apparent < p2->time_apparent; }
    vector<observation *> observations;
    list<preobservation *> preobservations;

    v_intrinsic lp_storage[MAXOBSERVATIONSIZE*MAXSTATESIZE / 4];
    v_intrinsic m_cov_storage[MAXOBSERVATIONSIZE / 4];
    v_intrinsic pred_storage[MAXOBSERVATIONSIZE / 4];
    v_intrinsic meas_storage[MAXOBSERVATIONSIZE / 4];
    v_intrinsic inn_storage[MAXOBSERVATIONSIZE / 4];
};

//some object should have functions to evolve the mean and covariance
//balance here between generality (give full linearization that could be used by my batched vision approach to get the partials wrt v, acceleration) and speed - direct update of covariance. Is there a single function that does both? (Can we use the output of the direct update to give the jacobian for vision?)
//Ultimately I use T,W. Can we keep the partial derivatives of these updated wrt their integration for use in vision meas?
//include earth's rotation in IMU
//Runge-Kutta



#endif
