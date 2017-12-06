#ifndef PROJECT_COVARIANCE_DEFINITIONS_H_
#define PROJECT_COVARIANCE_DEFINITIONS_H_

#include "state_size.h"

#define PROJECT_COVARIANCE_FIRST_SHAVE 0
#define PROJECT_COVARIANE_SHAVES 4


struct project_covariance_element_data{
    int index;
    float initial_covariance;
    bool use_single_index;
};

enum observation_type { none, vision_feature, accelerometer, gyroscope, velocimeter };

struct project_motion_covariance_data{
    int first_shave = -1;
    int shaves_number = 0;
    const float *src = nullptr;
    int src_rows = 0;
    int src_cols = 0;
    int src_stride = 0;
    float *dst = nullptr;
    int dst_rows = 0;
    int dst_cols = 0;
    int dst_stride = 0;
    project_covariance_element_data w = {0, 0, 0};
    project_covariance_element_data dw = {0, 0, 0};
    project_covariance_element_data ddw = {0, 0, 0};
    project_covariance_element_data V = {0, 0, 0};
    project_covariance_element_data a = {0, 0, 0};
    project_covariance_element_data T = {0, 0, 0};
    project_covariance_element_data da = {0, 0, 0};
    project_covariance_element_data Q = {0, 0, 0};
    const float *dQp_s_dW = NULL;
    int camera_count = 0;
    project_covariance_element_data tr[MAX_GROUPS] = {};
    project_covariance_element_data qr[MAX_GROUPS] = {};
    float* dTrp_dQ_s_matrix[MAX_GROUPS] = {};
    float* dQrp_s_dW_matrix[MAX_GROUPS] = {};
    float* dTrp_ddT_matrix[MAX_GROUPS] = {};
    float dt = 0.f;
};

struct observation_data {
    int size = 0;
    observation_type type = none;

    observation_data(int _size=0, observation_type _type=none) : size(_size), type(_type) {}
};

struct observation_vision_feature_data : observation_data {

    project_covariance_element_data feature = {0, 0, 0};
    project_covariance_element_data Qr = {0, 0, 0};
    project_covariance_element_data Tr = {0, 0, 0};

    float dx_dp[2]  = {0}; //m<2,1>
    float dx_dQr[6] = {0}; //m<2,3>
    float dx_dTr[6] = {0}; //m<2,3>

    struct camera_derivative {
        project_covariance_element_data Q = {0, 0, 0};
        project_covariance_element_data T = {0, 0, 0};
        project_covariance_element_data focal_length = {0, 0, 0};
        project_covariance_element_data center = {0, 0, 0};
        project_covariance_element_data k = {0, 0, 0};

        bool e_estimate = true;
        bool i_estimate = true;

        float dx_dQ[6] = {0}; //m<2,3>
        float dx_dT[6] = {0}; //m<2,3>
        float dx_dF[2] = {0}; //m<2,1>
        float dx_dc[4] = {0}; //m<2,2>
        float dx_dk[8] = {0}; //m<2,4>

    } orig, curr;

    observation_vision_feature_data() : observation_data (0, vision_feature) {}
    observation_vision_feature_data(observation_data& base) : observation_data (base) {}
};

struct observation_accelerometer_data : observation_data {

    project_covariance_element_data a_bias = {0, 0, 0};
    project_covariance_element_data Q = {0, 0, 0};
    project_covariance_element_data a = {0, 0, 0};
    project_covariance_element_data w = {0, 0, 0};
    project_covariance_element_data dw = {0, 0, 0};
    project_covariance_element_data g = {0, 0, 0};
    project_covariance_element_data eQ = {0, 0, 0};
    project_covariance_element_data eT = {0, 0, 0};

    float da_dQ[9] = {0}; //m3
    float da_dw[9] = {0}; //m3
    float da_ddw[9] = {0}; //m3
    float da_dacc[9] = {0}; //m3
    float da_dQa[9] = {0}; //m3
    float da_dTa[9] = {0}; //m3
    const float worldUp[3] = {0}; //v3
    bool e_estimate = true;

    observation_accelerometer_data() : observation_data (0, accelerometer) {}
    observation_accelerometer_data(observation_data& base) : observation_data (base) {}
};

struct observation_gyroscope_data : observation_data {

    project_covariance_element_data w = {0, 0, 0};
    project_covariance_element_data w_bias = {0, 0, 0};
    project_covariance_element_data Q = {0, 0, 0};

    float RwT[9] = {0}; //m3 //Rw.transpose();
    float dw_dQw[9] = {0}; //m3
    bool e_estimate = true;

    observation_gyroscope_data() : observation_data (0, gyroscope) {}
    observation_gyroscope_data(observation_data& base) : observation_data (base) {}
};

struct observation_velocimeter_data : observation_data {

    project_covariance_element_data Q = {0, 0, 0};
    project_covariance_element_data w = {0, 0, 0};
    project_covariance_element_data V = {0, 0, 0};
    project_covariance_element_data eQ = {0, 0, 0};
    project_covariance_element_data eT = {0, 0, 0};

    float* dv_dQ = NULL; //m3
    float* dv_dw = NULL; //m3
    float* dv_dV = NULL; //m3
    float* dv_dQv = NULL; //m3
    float* dv_dTv = NULL; //m3
    bool e_estimate = true;

    observation_velocimeter_data() : observation_data (0, velocimeter) {}
    observation_velocimeter_data(observation_data& base) : observation_data (base) {}
};

struct project_observation_covariance_data {
    int first_shave = -1;
    int shaves_number = 0;
    const float *src = nullptr;
    int src_rows = 0;
    int src_cols = 0;
    int src_stride = 0;
    float *dst = nullptr;
    int dst_rows = 0;
    int dst_cols = 0;
    int dst_stride = 0;
    float *HP = nullptr;
    int HP_rows = 0;
    int HP_src_cols = 0;
    int HP_dst_cols = 0;
    int HP_stride = 0;

    observation_data** observations = NULL;
    int observations_size = 0;
};

#endif /* PROJECT_COVARIANCE_DEFINITIONS_H_ */
