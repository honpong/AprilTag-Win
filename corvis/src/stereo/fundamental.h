#ifndef __FUNDAMENTAL_H
#define __FUNDAMENTAL_H

#include "../cor/cor_types.h"
#include "../numerics/vec4.h"
#include "camera.h"
#include "stereo.h"

#include <vector>
using namespace std;

m3 eight_point_F(v3 p1[], v3 p2[], int npts);
bool decompose_F(const m3 & F, float focal_length, float center_x, float center_y, const v3 & p1, const v3 & p2, m3 & R, v3 & T);
int compute_inliers(const v3 from [], const v3 to [], int nmatches, const m3 & F, float thresh, bool inliers []);
bool ransac_F(const vector<v3> & reference_pts, const vector<v3> target_pts, m3 & F);
m3 estimate_F(const camera &g, const stereo_frame &reference, const stereo_frame &target, m3 & dR, v3 & dT);

#endif
