#ifndef _ADAPTIVE_CONTROLLER_H
#define _ADAPTIVE_CONTROLLER_H

/**
 * @brief Adaptive controller is designed to track and provide optimal control-value 
 * such that the 'system' in use will generate 'target-value'
 * The class assume that the controlled system is somewhat monotonic in thes sense that larger control
 * value will increast the target value
 */

class AdaptiveController {
public:
    typedef int target_t;
    typedef int control_t;
    explicit AdaptiveController(
                       target_t targetMin, /// Threshold for increasing the control value, in terms of delta from target value
                       target_t targetMax, /// Threshold for decreasing the control value, in terms of delta from target value
                       control_t initialControl, 
                       control_t minControl, 
                       control_t maxControl,
                       control_t upStep, // step size when increasing the control value
                       control_t downStep, // step size when decreasing the control value
                       int highHist, // number of iterations to stay above targetMax before taking action
                       int lowHist // number of cycles to stay below targetMin before taking action
                       );
    void update(target_t currentOutput); /// update the controller with current control target
    inline control_t control()  { return m_controlValue; }
private:
    target_t m_targetMin;
    target_t m_targetMax;
    control_t m_controlValue;
    control_t m_minControl;
    control_t m_maxControl;
    control_t m_upStep;
    control_t m_downStep;
    int       m_highHist;
    int       m_highHistCount;
    int       m_lowHistCount;
    int       m_lowHist;
};

#endif
