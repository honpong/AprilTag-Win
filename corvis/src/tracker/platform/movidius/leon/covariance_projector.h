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
    void project_motion_covariance(project_motion_covariance_data &data);
    void project_observation_covariance(project_observation_covariance_data &data, int start_index[]);
};

#endif /* COVARIANCE_PROJECTOR_H_ */
