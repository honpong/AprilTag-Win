#include "adaptive_controller.h"

AdaptiveController::AdaptiveController(
                   target_t targetMin,
                   target_t targetMax,
                   control_t initialControl,
                   control_t minControl,
                   control_t maxControl,
                   control_t upStep,
                   control_t downStep,
                   int       lowHist,
                   int       highHist
                   ) :
                   m_targetMin(targetMin),
                   m_targetMax(targetMax),
                   m_controlValue(initialControl),
                   m_minControl(minControl),
                   m_maxControl(maxControl),
                   m_upStep(upStep),
                   m_downStep(downStep),
                   m_highHist(highHist),
                   m_lowHist(lowHist)
{
                   m_lowHistCount = m_highHistCount = 0;
}

void AdaptiveController::update(target_t currentOutput)
{
    if (currentOutput < m_targetMin) {
        ++m_lowHistCount;
        if (m_lowHistCount > m_lowHist) m_lowHistCount = m_lowHist;
        m_highHistCount = 0;
    } else if (currentOutput > m_targetMax) {
        ++m_highHistCount;
        if (m_highHistCount > m_highHist) m_highHistCount = m_highHist;
        m_lowHistCount = 0;
    } else {
        --m_lowHistCount;
        if (m_lowHistCount < 0) m_lowHistCount = 0;
        --m_highHistCount;
        if (m_highHistCount < 0) m_highHistCount = 0;
    }

    if (m_lowHistCount >= m_lowHist) {
        m_controlValue -= m_downStep;
        --m_lowHistCount;
    }
    if (m_highHistCount >= m_highHist) {
        m_controlValue += m_upStep;
        --m_highHistCount;
    }
    if (m_controlValue < m_minControl) m_controlValue = m_minControl;
    if (m_controlValue > m_maxControl) m_controlValue = m_maxControl;
}

