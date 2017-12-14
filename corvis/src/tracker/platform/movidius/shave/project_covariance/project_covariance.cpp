#include <mv_types.h>
#include <svuCommonShave.h>
#include <moviVectorUtils.h>
#include "project_covariance_definitions.h"

inline float4 from_row(const float *src, int row, int stride, int index, int cols, int rows, float initial_covariance, bool use_single_index, int size=3)
{
    float4 res = { 0.f };
    if (index < 0){
        return res;
    }
    if(index >= cols){
        if(row - index >= 0 && row - index < size && index > 0 && !use_single_index){
            switch(row - index){
            case 0:
                res = {initial_covariance, 0.f, 0.f, 0.f};
                break;
            case 1:
                res = {0.f, initial_covariance, 0.f, 0.f};
                break;
            case 2:
                res = {0.f, 0.f, initial_covariance, 0.f};
                break;
            case 3:
                res = {0.f, 0.f, 0.f, initial_covariance};
            }
        }
        else if(use_single_index && row == index){
            res = {initial_covariance, 0.f, 0.f, 0.f};
        }
        return res;
    }
    if((row < 0) || (row >= rows)){
        return res;
    }
    switch (size) {
    case 1:
        res = {*(src + row * stride + index), 0, 0, 0};
        break;
    case 2:
        res = {*(src + row * stride + index), *(src + row * stride + index + 1), 0, 0};
        break;
    case 3:
        res = {*(src + row * stride + index),
            *(src + row * stride + index + 1),
            *(src + row * stride + index + 2),
            0};
        break;
    case 4:
        res = {*(src + row * stride + index),
            *(src + row * stride + index + 1),
            *(src + row * stride + index + 2),
            *(src + row * stride + index + 3)};
        break;
    }
    return res;
}

inline float2 from_row2(const float *src, int row, int stride, int index, int cols, int rows, float initial_covariance, bool use_single_index, int size=2) {

    float2 res = { 0.f };
    if (index < 0){
        return res;
    }
    if(index >= cols){
        if(row - index >= 0 && row - index < size && index > 0 && !use_single_index){
            switch(row - index){
            case 0:
                res = {initial_covariance, 0.f};
                break;
            case 1:
                res = {0.f, initial_covariance};
                break;
           }
        }
        else if(use_single_index && row == index){
            res = {initial_covariance, 0.f};
        }
        return res;
    }
    if((row < 0) || (row >= rows)){
        return res;
    }
    res = *(float2*) (src + row * stride + index);
    return res;
}

inline float from_row1(const float *src, int row, int stride, int index, int cols, int rows, float initial_covariance, bool use_single_index) {

    float res = 0.f;
    if (index < 0){
        return res;
    }
    if(index >= cols){
        if(row == index && !use_single_index){
            res = initial_covariance;
        }
        else {
            res = 0.f;
        }
        return res;
    }
    if((row < 0) || (row >= rows)){
        return res;
    }
    res = *(float*) (src + row * stride + index);
    return res;
}

inline void to_col(float *dst, int col, int stride, int index, int rows, float4 val) {
    if(index >=  0 && index < rows){
        *(dst + col + (index) * stride) = val.s0;
        *(dst + col + (index + 1) * stride) = val.s1;
        *(dst + col + (index + 2) * stride) = val.s2;
    }
}

inline void to_col2(float *dst, int col, int stride, float2 val) {
    *(dst + col) = val.s0;
    *(dst + col + stride) = val.s1;
}

inline void to_col3(float *dst, int col, int stride, float4 val) {
    *(dst + col) = val.s0;
    *(dst + col + stride) = val.s1;
    *(dst + col + 2 * stride) = val.s2;
}

inline float4 mull_m3_v3(const float* mat, float4 vec)
{
    float4 new_vec;
    vec.s3 = 0.f;
    float4 r1 = *(float4*)mat;
    r1.s3 = 0;
    float4 r2 = *(float4*)(mat+3);
    r2.s3 = 0;
    float4 r3 = *(float4*)(mat+6);
    r3.s3 = 0;
    new_vec[0]= mvuDot(r1, vec);
    new_vec[1]= mvuDot(r2, vec);
    new_vec[2]= mvuDot(r3, vec);
    return new_vec;
}

inline float2 mull_m23_v3(const float* mat, float4 vec)
{
    float2 new_vec;
    vec.s3 = 0.f;
    float4 r1 = *(float4*)mat;
    r1.s3 = 0;
    float4 r2 = *(float4*)(mat+3);
    r2.s3 = 0;
    new_vec[0]= mvuDot(r1, vec);
    new_vec[1]= mvuDot(r2, vec);
    return new_vec;
}

inline float2 mull_m24_v4(const float* mat, float4 vec)
{
    float2 new_vec;
    float4 r1 = *(float4*)mat;
    float4 r2 = *(float4*)(mat+4);
    new_vec[0]= mvuDot(r1, vec);
    new_vec[1]= mvuDot(r2, vec);
    return new_vec;
}

inline float2 mull_m2_v2(const float* mat, float2 vec)
{
    float2 new_vec;
    new_vec[0]= mat[0]*vec.s0 + mat[1]*vec.s1;
    new_vec[1]= mat[2]*vec.s0 + mat[3]*vec.s1;
    return new_vec;
}

inline float2 mull_v2_f(const float* mat, float v)
{
    float2 new_vec;
    new_vec[0]= mat[0]*v;
    new_vec[1]= mat[1]*v;
    return new_vec;
}

inline float4 mull_v3_f(const float* mat, float v)
{
    float4 new_vec = {0};
    new_vec[0]= mat[0]*v;
    new_vec[1]= mat[1]*v;
    new_vec[2]= mat[2]*v;
    return new_vec;
}

 extern "C" void vision_project_motion_covariance(project_motion_covariance_data* data) {

    float dt =      data->dt;
    int src_cols =   data->src_cols;
    int src_rows =   data->src_rows;
    int src_stride = data->src_stride;
    int dst_stride = data->dst_stride;
    int dst_rows =   data->dst_rows;
    int dst_cols =    data->dst_cols;

    int shave_number = scGetShaveNumber();
    int cols_per_shave = (dst_cols + data->shaves_number - 1) / data->shaves_number;
    int start_col = cols_per_shave * (shave_number - data->first_shave);
    int end_col = start_col + cols_per_shave;
    if (end_col > dst_cols){
        end_col = dst_cols;
    }

    for (int i = start_col; i < end_col; ++i) {
        float4 cov_w = from_row(data->src, i, src_stride, data->w.index, src_cols, src_rows, data->w.initial_covariance, data->w.use_single_index);
        float4 cov_dw = from_row(data->src, i, src_stride, data->dw.index, src_cols, src_rows, data->dw.initial_covariance, data->dw.use_single_index);
        float4 cov_ddw = from_row(data->src, i, src_stride, data->ddw.index, src_cols, src_rows, data->ddw.initial_covariance, data->ddw.use_single_index);
        float4 cov_dW = dt * (cov_w + dt / 2 * (cov_dw + dt / 3 * cov_ddw));
        float4 scov_Q = from_row(data->src, i, src_stride, data->Q.index, src_cols, src_rows, data->Q.initial_covariance, data->Q.use_single_index);

        float4 result = cov_w + dt * (cov_dw + dt / 2 * cov_ddw);
        to_col(data->dst, i, dst_stride, data->w.index, dst_rows, result);

        float4 result2 = cov_dw + dt * cov_ddw;
        to_col(data->dst, i, dst_stride, data->dw.index, dst_rows, result2);

        float4 result9 =  mull_m3_v3(data->dQp_s_dW, cov_dW);
        float4 result3 = scov_Q + result9;
        to_col(data->dst, i, dst_stride, data->Q.index, dst_rows, result3);

        // This should match state_motion::project_covariance
        float4 cov_V = from_row(data->src, i, src_stride, data->V.index, src_cols, src_rows, data->V.initial_covariance, data->V.use_single_index);
        float4 cov_a = from_row(data->src, i, src_stride, data->a.index, src_cols, src_rows, data->a.initial_covariance, data->a.use_single_index);
        float4 cov_T = from_row(data->src, i, src_stride, data->T.index, src_cols, src_rows, data->T.initial_covariance, data->T.use_single_index);
        float4 cov_da = from_row(data->src, i, src_stride, data->da.index, src_cols, src_rows, data->da.initial_covariance, data->da.use_single_index);
        float4 cov_dT = dt * (cov_V + dt / 2 * (cov_a + dt / 3 * cov_da));
        float4 result4 = cov_T + cov_dT;
        to_col(data->dst, i, dst_stride, data->T.index, dst_rows, result4);
        float4 result5 = cov_V + dt * (cov_a + dt / 2 * cov_da);
        to_col(data->dst, i, dst_stride, data->V.index, dst_rows, result5);
        float4 result6 = cov_a + dt * cov_da;
        to_col(data->dst, i, dst_stride, data->a.index, dst_rows, result6);

        for (int j = 0; j < data->camera_count; ++j) {
            float4 cov_Tr = from_row(data->src, i, src_stride, data->tr[j].index, src_cols, src_rows, data->tr[j].initial_covariance, data->tr[j].use_single_index);
            float4 scov_Qr = from_row(data->src, i, src_stride, data->qr[j].index, src_cols, src_rows, data->qr[j].initial_covariance, data->qr[j].use_single_index);

            float4 result7 = cov_Tr + mull_m3_v3(data->dTrp_dQ_s_matrix[j], (scov_Q - scov_Qr))
                    + mull_m3_v3(data->dTrp_ddT_matrix[j], cov_dT);
            to_col(data->dst, i, dst_stride, data->tr[j].index, dst_rows, result7);
            float4 result8 = scov_Qr + mull_m3_v3(data->dQrp_s_dW_matrix[j], cov_dW);
            to_col(data->dst, i, dst_stride, data->qr[j].index, dst_rows, result8);
        }
    }

    SHAVE_HALT;
    return;
}

void observation_vision_feature_project_covariance(const float* src, float* dst,
        int src_rows, int src_cols, int src_stride, int dst_cols, int dst_stride,
        observation_vision_feature_data* data) {

    for (int i = 0; i < dst_cols; ++i) {

         float cov_feat = from_row1(src, i, src_stride, data->feature.index, src_cols, src_rows, data->feature.initial_covariance, data->feature.use_single_index);
         float4 scov_Qr = from_row(src, i, src_stride, data->Qr.index, src_cols, src_rows, data->Qr.initial_covariance, data->Qr.use_single_index);
         float4 cov_Tr = from_row(src, i, src_stride, data->Tr.index, src_cols, src_rows, data->Tr.initial_covariance, data->Tr.use_single_index);

         float2 result = mull_v2_f(data->dx_dp, cov_feat) + mull_m23_v3(data->dx_dQr, scov_Qr) +  mull_m23_v3(data->dx_dTr, cov_Tr);

         if (data->curr.e_estimate) {
            float4 scov_Q = from_row(src, i, src_stride, data->curr.Q.index, src_cols, src_rows, data->curr.Q.initial_covariance, data->curr.Q.use_single_index);
            float4  cov_T = from_row(src, i, src_stride, data->curr.T.index, src_cols, src_rows, data->curr.T.initial_covariance, data->curr.T.use_single_index);
            result += mull_m23_v3(data->curr.dx_dQ, scov_Q) + mull_m23_v3(data->curr.dx_dT, cov_T);
         }

         if (data->orig.e_estimate) {
            float4 scov_Q = from_row(src, i, src_stride, data->orig.Q.index, src_cols, src_rows, data->orig.Q.initial_covariance, data->orig.Q.use_single_index);
            float4  cov_T = from_row(src, i, src_stride, data->orig.T.index, src_cols, src_rows, data->orig.T.initial_covariance, data->orig.T.use_single_index);
            result += mull_m23_v3(data->orig.dx_dQ, scov_Q) + mull_m23_v3(data->orig.dx_dT, cov_T);
         }

         if (data->curr.i_estimate) {
            float cov_F = from_row1(src, i, src_stride, data->curr.focal_length.index, src_cols, src_rows, data->curr.focal_length.initial_covariance, data->curr.focal_length.use_single_index);
            float2 cov_c = from_row2(src, i, src_stride, data->curr.center.index, src_cols, src_rows, data->curr.center.initial_covariance, data->curr.center.use_single_index);
            float4 cov_k = from_row(src, i, src_stride, data->curr.k.index, src_cols, src_rows, data->curr.k.initial_covariance, data->curr.k.use_single_index, 4);
            result += mull_v2_f(data->curr.dx_dF, cov_F) + mull_m2_v2(data->curr.dx_dc, cov_c) + mull_m24_v4(data->curr.dx_dk, cov_k);
         }

         if (data->orig.i_estimate) {
            float cov_F = from_row1(src, i, src_stride, data->orig.focal_length.index, src_cols, src_rows, data->orig.focal_length.initial_covariance, data->orig.focal_length.use_single_index);
            float2 cov_c = from_row2(src, i, src_stride, data->orig.center.index, src_cols, src_rows, data->orig.center.initial_covariance, data->orig.center.use_single_index);
            float4 cov_k = from_row(src, i, src_stride, data->orig.k.index, src_cols, src_rows, data->orig.k.initial_covariance, data->orig.k.use_single_index, 4);
            result += mull_v2_f(data->orig.dx_dF, cov_F) + mull_m2_v2(data->orig.dx_dc, cov_c) + mull_m24_v4(data->orig.dx_dk, cov_k);
         }
         to_col2(dst, i, dst_stride, result);
     }
     return;
}

void observation_accelerometer_project_covariance(const float* src, float* dst,
        int src_rows, int src_cols, int src_stride, int dst_cols, int dst_stride,
        observation_accelerometer_data* data) {

    //input matrix is either symmetric (covariance) or is implicitly transposed (L * C)
    if(dst_cols != src_rows)
        return;
    for(int i = 0; i < dst_cols; ++i) {
        float4 cov_a_bias = from_row(src, i, src_stride, data->a_bias.index, src_cols, src_rows, data->a_bias.initial_covariance, data->a_bias.use_single_index);
        float4 scov_Q = from_row(src, i, src_stride, data->Q.index, src_cols, src_rows, data->Q.initial_covariance, data->Q.use_single_index);
        float4 cov_a = from_row(src, i, src_stride, data->a.index, src_cols, src_rows, data->a.initial_covariance, data->a.use_single_index);
        float4 cov_w = from_row(src, i, src_stride, data->w.index, src_cols, src_rows, data->w.initial_covariance, data->w.use_single_index);
        float4 cov_dw = from_row(src, i, src_stride, data->dw.index, src_cols, src_rows, data->dw.initial_covariance, data->dw.use_single_index);
        float cov_g = from_row1(src, i, src_stride, data->g.index, src_cols, src_rows, data->g.initial_covariance, data->g.use_single_index);
        float4 res = cov_a + mull_v3_f(data->worldUp, cov_g);
        float4 result = cov_a_bias + mull_m3_v3(data->da_dQ, scov_Q) + mull_m3_v3(data->da_dw, cov_w) + mull_m3_v3(data->da_ddw, cov_dw) + mull_m3_v3(data->da_dacc, res);
        if(data->e_estimate) {
            float4 scov_Qa = from_row(src, i, src_stride, data->eQ.index, src_cols, src_rows, data->eQ.initial_covariance, data->eQ.use_single_index);
            float4 cov_Ta = from_row(src, i, src_stride, data->eT.index, src_cols, src_rows, data->eT.initial_covariance, data->eT.use_single_index);
            result += mull_m3_v3(data->da_dQa, scov_Qa) + mull_m3_v3(data->da_dTa, cov_Ta);
        }
        to_col3(dst, i, dst_stride, result);
    }
    return;
}

void observation_gyroscope_project_covariance(const float* src, float* dst,
        int src_rows, int src_cols, int src_stride, int dst_cols, int dst_stride,
        observation_gyroscope_data* data) {

    for(int i = 0; i < dst_cols; ++i) {
        float4 cov_w = from_row(src, i, src_stride, data->w.index, src_cols, src_rows, data->w.initial_covariance, data->w.use_single_index);
        float4 cov_wbias = from_row(src, i, src_stride, data->w_bias.index, src_cols, src_rows, data->w_bias.initial_covariance, data->w_bias.use_single_index);
        float4 result = cov_wbias + mull_m3_v3(data->RwT, cov_w);
        if(data->e_estimate) {
            float4 scov_Qw = from_row(src, i, src_stride, data->Q.index, src_cols, src_rows, data->Q.initial_covariance, data->Q.use_single_index);
            result += mull_m3_v3(data->dw_dQw, scov_Qw);
        }
        to_col3(dst, i, dst_stride, result);
    }
    return;
}

void observation_velocimeter_project_covariance(const float* src, float* dst,
        int src_rows, int src_cols, int src_stride, int dst_cols, int dst_stride,
        observation_velocimeter_data* data) {

    for(int i = 0; i < dst_cols; ++i) {
        float4 scov_Q = from_row(src, i, src_stride, data->Q.index, src_cols, src_rows, data->Q.initial_covariance, data->Q.use_single_index);
        float4 cov_w = from_row(src, i, src_stride, data->w.index, src_cols, src_rows, data->w.initial_covariance, data->w.use_single_index);
        float4 cov_V = from_row(src, i, src_stride, data->V.index, src_cols, src_rows, data->V.initial_covariance, data->V.use_single_index);
        float4 result = mull_m3_v3(data->dv_dQ, scov_Q) + mull_m3_v3(data->dv_dw, cov_w) + mull_m3_v3(data->dv_dV, cov_V);
        if(data->e_estimate) {
            float4 scov_Qv = from_row(src, i, src_stride, data->eQ.index, src_cols, src_rows, data->eQ.initial_covariance, data->eQ.use_single_index);
            float4 cov_Tv = from_row(src, i, src_stride, data->eT.index, src_cols, src_rows, data->eT.initial_covariance, data->eT.use_single_index);
            result += mull_m3_v3(data->dv_dQv, scov_Qv) + mull_m3_v3(data->dv_dTv, cov_Tv);
        }
        to_col3(dst, i, dst_stride, result);
    }
    return;
}

extern "C" void vision_project_observation_covariance(project_observation_covariance_data* data, int start_index) {

    int src_rows    = data->src_rows;
    int src_cols    = data->src_cols;
    int src_stride  = data->src_stride;
    int dst_cols    = data->dst_cols;
    int dst_stride  = data->dst_stride;
    
    int shave_number = scGetShaveNumber();
    int obs_per_shave = (data->observations_size + data->shaves_number -1) / data->shaves_number;
    int start_obs = obs_per_shave * (shave_number - data->first_shave);
    int end_obs = start_obs + obs_per_shave;
    if (end_obs > data->observations_size){
        end_obs = data->observations_size;
    }

    int index = start_index;
    for (int i = start_obs; i < end_obs; ++i) {
        switch (data->observations[i]->type) {
            case vision_feature:
            {
                observation_vision_feature_data* vision_data = (observation_vision_feature_data*)data->observations[i];
                observation_vision_feature_project_covariance(data->src, data->dst + index*dst_stride, src_rows, src_cols, src_stride, dst_cols, dst_stride, vision_data);
            }
                break;
            case accelerometer:
            {
                observation_accelerometer_data* accel_data = (observation_accelerometer_data*)data->observations[i];
                observation_accelerometer_project_covariance(data->src, data->dst + index*dst_stride, src_rows, src_cols, src_stride, dst_cols, dst_stride, accel_data);
            }
                break;
            case gyroscope:
            {
                observation_gyroscope_data* gyro_data = (observation_gyroscope_data*)data->observations[i];
                observation_gyroscope_project_covariance(data->src, data->dst + index*dst_stride, src_rows, src_cols, src_stride, dst_cols, dst_stride, gyro_data);
            }
                break;
            case velocimeter:
            {
                observation_velocimeter_data* velo_data = (observation_velocimeter_data*)data->observations[i];
                observation_velocimeter_project_covariance(data->dst, data->dst + index*dst_stride, src_rows, src_cols, src_stride, dst_cols, dst_stride, velo_data);
            }
                break;
            default:
                break;
        }
        index+=data->observations[i]->size;
    }

    SHAVE_HALT;
    return;
}