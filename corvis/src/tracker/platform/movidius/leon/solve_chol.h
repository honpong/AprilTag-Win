#pragma once
#ifdef __cplusplus
extern "C" {
#endif

void solve_chol(float * A, float * Bt, float * X, int rows, int Bt_rows, int A_stride, int B_stride, int X_stride);
void shave_start_stop();

#ifdef __cplusplus
}
#endif
