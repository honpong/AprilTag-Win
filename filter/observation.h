#ifndef __OBSERVATION_H
#define __OBSERVATION_H

#include "model.h"
#include "../numerics/matrix.h"
#include "../numerics/vec4.h"

class observation {
 public:
    int size;
    uint64_t time;

    virtual void predict(matrix &pred, matrix &meas, int index, matrix *_lp) {}
    virtual void measure(matrix &meas, int index) {}
    virtual void robust_covariance(matrix &inn, matrix &m_cov, int index) {}
 observation(): size(0) {}
};

class observation_vision_base: public observation {
 public:
    m4 R, Rt, Rbc, Rcb, RcbRt;
    m4v4 dR_dW, dRbc_dWc, dRt_dW, dRcb_dWc;

    state_vision *state;
    void preprocess_measurements(bool linearize);
};

class observation_vision_group: public observation {
 public:
    m4 Rr, Rw, Rtot;
    v4 Tw, Ttot;
    m4v4 dRr_dWr, dRtot_dW, dRtot_dWr, dRtot_dWc;
    m4 dTtot_dWc, dTtot_dW, dTtot_dWr, dTtot_dT, dTtot_dTc, dTtot_dTr;

    observation_vision_base *base;

    state_vision *state;
    state_vision_group *group;
    void preprocess_measurements(bool linearize);
};

class observation_vision_feature: public observation {
 public:
    observation_vision_base *base;
    observation_vision_group *group;

    state_vision *state;
    state_vision_group *state_group;
    state_vision_feature *feature;

    f_t measurement[2];

    virtual void predict(matrix &pred, matrix &meas, int index, matrix *_lp);
    virtual void measure(matrix &meas, int index);
    virtual void robust_covariance(matrix &inn, matrix &m_cov, int index);

 observation_vision_feature() { size = 2; }
};

struct saved_measurement {
    int (* predict)(state *, matrix &, matrix *, void *);
    void (* robustify)(struct filter *, matrix &, matrix &, void *);
    matrix meas;
    void *flag;
    int size;
    uint64_t time;
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


#endif;
