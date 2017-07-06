#include <mv_types.h>
#include "covariance_projector.h"

extern u32 cvrt0_vision_project_motion_covariance;
extern u32 cvrt1_vision_project_motion_covariance;
extern u32 cvrt2_vision_project_motion_covariance;
extern u32 cvrt3_vision_project_motion_covariance;

static u32 project_covariance_entry_points[PROJECT_COVARIANE_SHAVES] = {
        (u32) &cvrt0_vision_project_motion_covariance,
        (u32) &cvrt1_vision_project_motion_covariance,
        (u32) &cvrt2_vision_project_motion_covariance,
        (u32) &cvrt3_vision_project_motion_covariance };

covariance_projector::covariance_projector()
{
    shaves_number = PROJECT_COVARIANE_SHAVES;
    first_shave = PROJECT_COVARIANCE_FIRST_SHAVE;
    for (unsigned int i = 0; i < shaves_number; ++i){
       shaves[i] = Shave::get_handle(i + first_shave);
    }
}

void covariance_projector::project_motion_covariance(float *dst, const float *src, project_motion_covariance_data &data)
{
    data.first_shave = first_shave;
    data.shaves_number = shaves_number;
    for (int i = 0; i < shaves_number; ++i) {
        shaves[i]->start(project_covariance_entry_points[i], "iii", (u32)src, (u32)dst, (u32)&data);
    }
    for (int i = 0; i < shaves_number; ++i) {
        shaves[i]->wait();
    }
}
