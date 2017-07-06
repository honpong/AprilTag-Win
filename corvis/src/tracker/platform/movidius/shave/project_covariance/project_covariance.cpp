#include <mv_types.h>
#include <svuCommonShave.h>
#include <moviVectorUtils.h>
#include <swcCdma.h>
#include "project_covariance_definitions.h"

static float __attribute__((section(".cmx.bss")))  covariance_matrix[MAX_DATA_SIZE * MAX_DATA_SIZE];

inline float4 from_row(const float *src, int row, int stride, int index, int cols, int rows, float initial_covariance, bool use_single_index)
{
    float4 res = { 0.f };
    if (index < 0){
        return res;
    }
    if(index >= cols){
        if(row - index >= 0 && row - index < 3 && index > 0 && !use_single_index){
            switch(row - index){
            case 0:
                res = {initial_covariance, 0.f, 0.f, 0.f};
                break;
            case 1:
                res = {0.f, initial_covariance, 0.f, 0.f};
                break;
            case 2:
                res = {0.f, 0.f, initial_covariance, 0.f};
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
    res = *(float4*) (src + row * stride + index);
    return res;
}

inline void to_col(float *dst, int col, int stride, int index, int rows, float4 val) {
    if(index >=  0 && index < rows){
        *(dst + col + (index) * stride) = val.s0;
        *(dst + col + (index + 1) * stride) = val.s1;
        *(dst + col + (index + 2) * stride) = val.s2;
    }
}

inline float4 mull_m3_v3(float* mat, float4 vec)
{
    float4 new_vec;
    vec.s3 = 0.f;
    new_vec[0]= mvuDot( *(float4*)(mat    ), vec);
    new_vec[1]= mvuDot( *(float4*)(mat + 3), vec);
    new_vec[2]= mvuDot( *(float4*)(mat + 6), vec);
    return new_vec;
}


 extern "C" void vision_project_motion_covariance(const float* p_src, float* p_dst,
        project_motion_covariance_data* data) {


    //dma transaction of src
    dmaTransactionList_t dma_task;
    dmaTransactionList_t* dma_ref;
    u32 dma_requster_id = dmaInitRequester(1);

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

    //input DMA
    dma_ref = dmaCreateTransaction(
                    dma_requster_id,
                    &dma_task,
                    (u8*)(p_src),
                    (u8*)covariance_matrix,
                    data->src_rows * data->src_stride * sizeof(float));

    dmaStartListTask(dma_ref);
    dmaWaitTask(dma_ref);

    for (int i = start_col; i < end_col; ++i) {
        float4 cov_w = from_row(covariance_matrix, i, src_stride, data->w.index, src_cols, src_rows, data->w.initial_covariance, data->w.use_single_index);
        float4 cov_dw = from_row(covariance_matrix, i, src_stride, data->dw.index, src_cols, src_rows, data->dw.initial_covariance, data->dw.use_single_index);
        float4 cov_ddw = from_row(covariance_matrix, i, src_stride, data->ddw.index, src_cols, src_rows, data->ddw.initial_covariance, data->ddw.use_single_index);
        float4 cov_dW = dt * (cov_w + dt / 2 * (cov_dw + dt / 3 * cov_ddw));
        float4 scov_Q = from_row(covariance_matrix, i, src_stride, data->Q.index, src_cols, src_rows, data->Q.initial_covariance, data->Q.use_single_index);

        float4 result = cov_w + dt * (cov_dw + dt / 2 * cov_ddw);
        to_col(p_dst, i, dst_stride, data->w.index, dst_rows, result);

        float4 result2 = cov_dw + dt * cov_ddw;
        to_col(p_dst, i, dst_stride, data->dw.index, dst_rows, result2);

        float4 result9 =  mull_m3_v3(data->dQp_s_dW, cov_dW);
        float4 result3 = scov_Q + result9;
        to_col(p_dst, i, dst_stride, data->Q.index, dst_rows, result3);

        // This should match state_motion::project_covariance
        float4 cov_V = from_row(covariance_matrix, i, src_stride, data->V.index, src_cols, src_rows, data->V.initial_covariance, data->V.use_single_index);
        float4 cov_a = from_row(covariance_matrix, i, src_stride, data->a.index, src_cols, src_rows, data->a.initial_covariance, data->a.use_single_index);
        float4 cov_T = from_row(covariance_matrix, i, src_stride, data->T.index, src_cols, src_rows, data->T.initial_covariance, data->T.use_single_index);
        float4 cov_da = from_row(covariance_matrix, i, src_stride, data->da.index, src_cols, src_rows, data->da.initial_covariance, data->da.use_single_index);
        float4 cov_dT = dt * (cov_V + dt / 2 * (cov_a + dt / 3 * cov_da));
        float4 result4 = cov_T + cov_dT;
        to_col(p_dst, i, dst_stride, data->T.index, dst_rows, result4);
        float4 result5 = cov_V + dt * (cov_a + dt / 2 * cov_da);
        to_col(p_dst, i, dst_stride, data->V.index, dst_rows, result5);
        float4 result6 = cov_a + dt * cov_da;
        to_col(p_dst, i, dst_stride, data->a.index, dst_rows, result6);

        for (int j = 0; j < data->camera_count; ++j) {
            float4 cov_Tr = from_row(covariance_matrix, i, src_stride, data->tr[j].index, src_cols, src_rows, data->tr[j].initial_covariance, data->tr[j].use_single_index);
            float4 scov_Qr = from_row(covariance_matrix, i, src_stride, data->qr[j].index, src_cols, src_rows, data->qr[j].initial_covariance, data->qr[j].use_single_index);

            float4 result7 = cov_Tr + mull_m3_v3(data->dTrp_dQ_s_matrix[j], (scov_Q - scov_Qr))
                    + mull_m3_v3(data->dTrp_ddT_matrix[j], cov_dT);
            to_col(p_dst, i, dst_stride, data->tr[j].index, dst_rows, result7);
            float4 result8 = scov_Qr + mull_m3_v3(data->dQrp_s_dW_matrix[j], cov_dW);
            to_col(p_dst, i, dst_stride, data->qr[j].index, dst_rows, result8);
        }
    }

    SHAVE_HALT;
    return;
}
