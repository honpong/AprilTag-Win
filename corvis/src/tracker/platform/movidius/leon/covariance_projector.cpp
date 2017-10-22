#include <mv_types.h>
#include <OsCommon.h>
#include "covariance_projector.h"

extern u32 project_covariance0_vision_project_motion_covariance;
extern u32 project_covariance1_vision_project_motion_covariance;
extern u32 project_covariance2_vision_project_motion_covariance;
extern u32 project_covariance3_vision_project_motion_covariance;

static u32 project_covariance_entry_points[PROJECT_COVARIANE_SHAVES] = {
        (u32) &project_covariance0_vision_project_motion_covariance,
        (u32) &project_covariance1_vision_project_motion_covariance,
        (u32) &project_covariance2_vision_project_motion_covariance,
        (u32) &project_covariance3_vision_project_motion_covariance };

extern u32 project_covariance0_vision_project_observation_covariance;
extern u32 project_covariance1_vision_project_observation_covariance;
extern u32 project_covariance2_vision_project_observation_covariance;
extern u32 project_covariance3_vision_project_observation_covariance;

static u32 project_observation_covariance_entry_points[PROJECT_COVARIANE_SHAVES] = {
        (u32) &project_covariance0_vision_project_observation_covariance,
        (u32) &project_covariance1_vision_project_observation_covariance,
        (u32) &project_covariance2_vision_project_observation_covariance,
        (u32) &project_covariance3_vision_project_observation_covariance };

covariance_projector::covariance_projector()
{
    shaves_number = PROJECT_COVARIANE_SHAVES;
    first_shave = PROJECT_COVARIANCE_FIRST_SHAVE;
    for (unsigned int i = 0; i < shaves_number; ++i){
       shaves[i] = Shave::get_handle(i + first_shave);
    }
}

void covariance_projector::project_motion_covariance(project_motion_covariance_data &data)
{
    data.first_shave = first_shave;
    data.shaves_number = shaves_number;
    for (int i = 0; i < shaves_number; ++i) {
        shaves[i]->start(project_covariance_entry_points[i], "i", (u32)&data);
    }
    for (int i = 0; i < shaves_number; ++i) {
        shaves[i]->wait();
    }
    rtems_cache_invalidate_data_range(data.dst, data.dst_rows * data.dst_stride * sizeof(float));
}

void covariance_projector::project_observation_covariance(project_observation_covariance_data &data, int start_index[])
{
    int start_shave = 0;
    data.first_shave = first_shave;
    data.shaves_number = shaves_number;

    if (data.observations_size < 4) {
        data.shaves_number = 1;

    }

    int HP_rows     = data.HP_rows;
    int HP_src_cols = data.HP_src_cols;
    int HP_dst_cols = data.HP_dst_cols;
    int HP_stride   = data.HP_stride;
    float* HP       = data.HP;
    int dst_cols    = data.dst_cols;
    int dst_stride  = data.dst_stride;
    int dst_rows    = data.dst_rows;
    float* dst      = data.dst;

    // HP = H * P'
    data.dst_cols   = HP_dst_cols;
    data.dst_stride = HP_stride;
    data.dst_rows   = HP_rows;
    data.dst        = HP;
    for (int i = 0; i < data.shaves_number; ++i) {
        shaves[i + start_shave]->start(project_observation_covariance_entry_points[i + start_shave], "ii", (u32)&data, start_index[i]);
    }
    for (int i = 0; i < data.shaves_number; ++i) {
        shaves[i + start_shave]->wait();
    }
    rtems_cache_invalidate_data_range(HP, HP_rows * HP_stride * sizeof(float));
    // res_cov = H * (H * P')' = H * P * H'
    data.src_rows   = HP_rows;
    data.src_cols   = HP_src_cols;
    data.src_stride = HP_stride;
    data.src        = HP;
    data.dst_cols   = dst_cols;
    data.dst_stride = dst_stride;
    data.dst_rows   = dst_rows;
    data.dst        = dst;
    for (int i = 0; i < data.shaves_number; ++i) {
        shaves[i + start_shave]->start(project_observation_covariance_entry_points[i + start_shave], "ii", (u32)&data, start_index[i]);
    }
    for (int i = 0; i < data.shaves_number; ++i) {
        shaves[i + start_shave]->wait();
    }
    rtems_cache_invalidate_data_range(HP, HP_rows * HP_stride * sizeof(float));
    rtems_cache_invalidate_data_range(dst, dst_rows * dst_stride * sizeof(float));
}
