#ifndef __FUNDAMENTAL_H
#define __FUNDAMENTAL_H

#include "../cor/cor_types.h"
#include "../numerics/vec4.h"
#include "camera.h"
#include "stereo.h"

#include <vector>
using namespace std;

m4 eight_point_F(v4 p1[], v4 p2[], int npts);
bool decompose_F(const m4 & F, float focal_length, float center_x, float center_y, const v4 & p1, const v4 & p2, m4 & R, v4 & T);
int compute_inliers(const v4 from [], const v4 to [], int nmatches, const m4 & F, float thresh, bool inliers []);
bool ransac_F(const vector<v4> & reference_pts, const vector<v4> target_pts, m4 & F);
m4 estimate_F(const camera &g, const stereo_frame &reference, const stereo_frame &target, m4 & dR, v4 & dT);

#endif
