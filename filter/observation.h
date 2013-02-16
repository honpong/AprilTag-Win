#ifndef __OBSERVATION_H
#define __OBSERVATION_H

#include "model.h"
#include "../numerics/matrix.h"
#include "../numerics/vec4.h"
#include <vector>
#include <algorithm>

using namespace std;

class preobservation {
 public:
    virtual void process(state_vision *state, bool linearize) = 0;
};

class preobservation_vision_base: public preobservation {
 public:
    m4 R, Rt, Rbc, Rcb, RcbRt;
    m4v4 dR_dW, dRbc_dWc, dRt_dW, dRcb_dWc;

    virtual void process(state_vision * state, bool linearize);
};

class preobservation_vision_group: public preobservation {
 public:
    m4 Rr, Rw, Rtot;
    v4 Tw, Ttot;
    m4v4 dRr_dWr, dRtot_dW, dRtot_dWr, dRtot_dWc;
    m4 dTtot_dWc, dTtot_dW, dTtot_dWr, dTtot_dT, dTtot_dTc, dTtot_dTr;

    preobservation_vision_base *base;

    state_vision_group *group;
    virtual void process(state_vision *state, bool linearize);
 preobservation_vision_group(state_vision_group *g, preobservation_vision_base *b): base(b), group(g) {}
};

class observation {
 public:
    int size;
    uint64_t time;

    virtual void predict(state_vision *state, matrix &pred, int index, matrix *_lp) = 0;
    virtual void measure(matrix &meas, int index) = 0;
    virtual void robust_covariance(matrix &inn, matrix &m_cov, int index) = 0;
 observation(): size(0) {}
};

class observation_vision_feature: public observation {
 public:
    preobservation_vision_base *base;
    preobservation_vision_group *group;

    state_vision_group *state_group;
    state_vision_feature *feature;

    f_t measurement[2];

    virtual void predict(state_vision *state, matrix &pred, int index, matrix *_lp);
    virtual void measure(matrix &meas, int index);
    virtual void robust_covariance(matrix &inn, matrix &m_cov, int index);

 observation_vision_feature() { size = 2; }
};

class observation_queue {
 public:
    int meas_size;
    void add_observation(observation *obs);
    void add_preobservation(preobservation *pre);
    int preprocess(state_vision *state, bool linearize);
    void clear();
    void predict(state_vision *state, matrix &pred, matrix *_lp);
    void measure(matrix &meas);
    void robust_covariance(matrix &inn, matrix &m_cov);
    observation_queue();

 private:
    static bool observation_comp(observation *p1, observation *p2) { return p1->time < p2->time; }
    vector<observation *> observations;
    list<preobservation *> preobservations;
};

//have static storage for the measurement and measurement data (similar to static cov storage)?
//make measurement object more complex, similar to mapping of state - copy_from_array, copy_to_array
//a measurement should be a class (eg, feature measurement) that includes all needed data - this should replace the void * passed to measurement functions
//actually, measurement functions should be a member of the measurement class.
//the state objects should have functions to evolve the mean and covariance
//balance here between generality (give full linearization that could be used by my batched vision approach to get the partials wrt v, acceleration) and speed - direct update of covariance. Is there a single function that does both? (Can we use the output of the direct update to give the jacobian for vision?)
//Ultimately I use T,W. Can we keep the partial derivatives of these updated wrt their integration for use in vision meas?
//include earth's rotation in IMU
//Runge-Kutta
/*
class measurement_queue {
    list<saved_measurement> saved_measurements;
    array<list<saved_measurement>::iterator> queue;
      void enqueue(

};
*/


#endif
