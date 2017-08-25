#ifndef PROJECT_COVARIANCE_DEFINITIONS_H_
#define PROJECT_COVARIANCE_DEFINITIONS_H_

#define PROJECT_COVARIANCE_FIRST_SHAVE 0
#define PROJECT_COVARIANE_SHAVES 4

#define MAX_DATA_SIZE 96

struct project_covariance_element_data{
    int index;
    float initial_covariance;
    bool use_single_index;
};

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
    project_covariance_element_data tr[MAX_DATA_SIZE] = { {0, 0, 0} };
    project_covariance_element_data qr[MAX_DATA_SIZE] = { {0, 0, 0} };
    float* dTrp_dQ_s_matrix[MAX_DATA_SIZE] = {NULL};
    float* dQrp_s_dW_matrix[MAX_DATA_SIZE] = {NULL};
    float* dTrp_ddT_matrix[MAX_DATA_SIZE] = {NULL};
    float dt = 0.f;
};

#endif /* PROJECT_COVARIANCE_DEFINITIONS_H_ */
