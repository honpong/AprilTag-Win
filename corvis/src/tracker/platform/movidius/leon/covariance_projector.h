#ifndef COVARIANCE_PROJECTOR_H_
#define COVARIANCE_PROJECTOR_H_

#include "project_covariance_definitions.h"
#include "Shave.h"

class covariance_projector
{

private:
    int shaves_number;
    int first_shave;
    Shave* shaves[PROJECT_COVARIANE_SHAVES];

public:
    covariance_projector();
    void project_motion_covariance(float *dst, const float *src, project_motion_covariance_data &data);
};

#endif /* COVARIANCE_PROJECTOR_H_ */