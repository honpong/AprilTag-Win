#include "stereo.h"
#include "stereo_mesh.h"
#include "filter.h"

bool debug_triangulate = false;
// if enabled, adds a 3 pixel jitter in all directions to correspondence
bool enable_jitter = false;
bool enable_rectify = false;
bool enable_eight_point_motion = false;
#ifndef ARCHIVE
bool enable_debug_files = true;
#else
bool enable_debug_files = false;
#endif

void write_image(const char * path, uint8_t * image, int width, int height)
{
    FILE * f = fopen(path, "w");
    fprintf(f, "P5 %d %d 255\n", width, height);
    fwrite(image, 1, width*height, f);
    fclose(f);
}

bool line_endpoints(v4 line, int width, int height, float endpoints[4])
{
    float a = line[0], b = line[1], c = line[2];
    float intersect_y, intersect_x;
    float min_x = 0, max_x = width-1, min_y = 0, max_y = height-1;
    int idx = 0;

    f_t eps = 1e-14;
    // if the line is not vertical
    if(fabs(a) > eps) {
        // intersection with the left edge
        intersect_y = -(a * min_x + c) / b;
        if(intersect_y >= min_y && intersect_y <= max_y) {
            endpoints[idx++] = min_x;
            endpoints[idx++] = intersect_y;
        }

        // intersection with the right edge
        intersect_y = -(a * max_x + c) / b;
        if(intersect_y >= min_y && intersect_y <= max_y) {
            endpoints[idx++] = max_x;
            endpoints[idx++] = intersect_y;
        }
    }

    // if the line is not horizontal
    if(fabs(b) > eps) {
        // intersection with the top edge
        if(idx < 3) {
            intersect_x = -(b * min_y + c) / a;
            if(intersect_x >= min_x && intersect_x <= max_x) {
                endpoints[idx++] = intersect_x;
                endpoints[idx++] = min_y;
            }
        }

        // intersection with the bottom edge
        if(idx < 3) {
            intersect_x = -(b * max_y + c) / a;
            if(intersect_x >= min_x && intersect_x <= max_x) {
                endpoints[idx++] = intersect_x;
                endpoints[idx++] = max_y;
            }
        }
    }

    // if we still haven't found an intersection, return false
    if(idx < 3)
        return false;

    return true;
}

#define WINDOW 4
static const float maximum_match_score = 1;
// 5 pixels average deviation from the mean across the patch
static const float constant_patch_thresh = 3*3;
float score_match(const unsigned char *im1, const bool * im1valid, int xsize, int ysize, int stride, const int x1, const int y1, const unsigned char *im2, const bool * im2valid, const int x2, const int y2, float max_error)
{
    int window = WINDOW;
    int area = 0;

    if(x1 < window || y1 < window || x2 < window || y2 < window || x1 >= xsize - window || x2 >= xsize - window || y1 >= ysize - window || y2 >= ysize - window) return max_error; // + 1.;

    const unsigned char *p1 = im1 + stride * (y1 - window) + x1;
    const bool *p1valid = im1valid + stride * (y1 - window) + x1;
    const unsigned char *p2 = im2 + stride * (y2 - window) + x2;
    const bool *p2valid = im2valid + stride * (y2 - window) + x2;

    int sum1 = 0, sum2 = 0;
    for(int dy = -window; dy <= window; ++dy) {
        for(int dx = -window; dx <= window; ++dx) {
            if(p1valid[dx] && p2valid[dx]) {
                sum1 += p1[dx];
                sum2 += p2[dx];
                area++;
            }
        }
        p1 += stride;
        p2 += stride;
        p1valid += stride;
        p2valid += stride;
    };

    // If less than half the patch is valid, give up
    if(area <= 0.5 * (WINDOW*2 + 1)*(WINDOW*2 + 1))
        return max_error; // + 1.;

    float mean1 = sum1 / (float)area;
    float mean2 = sum2 / (float)area;
    
    p1 = im1 + stride * (y1 - window) + x1;
    p1valid = im1valid + stride * (y1 - window) + x1;
    p2 = im2 + stride * (y2 - window) + x2;
    p2valid = im2valid + stride * (y2 - window) + x2;

    float top = 0, bottom1 = 0, bottom2 = 0;
    for(int dy = -window; dy <= window; ++dy) {
        for(int dx = -window; dx <= window; ++dx) {
            if(p1valid[dx] && p2valid[dx]) {
                float t1 = (float)p1[dx] - mean1;
                float t2 = (float)p2[dx] - mean2;
                top += t1 * t2;
                bottom1 += (t1 * t1);
                bottom2 += (t2 * t2);
            }
        }
        p1 += stride;
        p2 += stride;
        p1valid += stride;
        p2valid += stride;
    }
    // constant patches can't be matched
    //if(fabs(bottom1) < constant_patch_thresh*area || fabs(bottom2) < constant_patch_thresh*area)
    if(bottom1 == 0 || bottom2 == 0)
        return max_error; // + 1.;

    return -top/sqrtf(bottom1 * bottom2);
}

bool track_window(uint8_t * im1, const bool * im1valid, uint8_t * im2, const bool * im2valid, int width, int height, int im1_x, int im1_y, int upper_left_x, int upper_left_y, int lower_right_x, int lower_right_y, float & bestx, float & besty, float & bestscore)
{
    bool valid_match = false;
    for(int y = upper_left_y; y < lower_right_y; y++) {
        for(int x = upper_left_x; x < lower_right_x; x++) {
            float score = score_match(im1, im1valid, width, height, width, im1_x, im1_y, im2, im2valid, x, y, maximum_match_score);
            if(score < bestscore) {
                valid_match = true;
                bestscore = score;
                bestx = x;
                besty = y;
            }
        }
    }
    return valid_match;
}

void match_scores(uint8_t * im1, bool * im1valid, uint8_t * im2,  bool * im2valid, int width, int height, int im1_x, int im1_y, float endpoints[4], vector<struct stereo_match> & matches)
{
    int x0 = endpoints[0];
    int y0 = endpoints[1];
    int x1 = endpoints[2];
    int y1 = endpoints[3];

    int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
    int err = (dx>dy ? dx : -dy)/2, e2;

    while(true) {
        struct stereo_match match;
        match.x = x0;
        match.y = y0;
        match.score = score_match(im1, im1valid, width, height, width, im1_x, im1_y, im2, im2valid, match.x, match.y, maximum_match_score);
        // TODO: use a heap or something else here to do the sorting on insert
        matches.push_back(match);

        // move along the line
        if (x0==x1 && y0==y1) break;
        e2 = err;
        if (e2 >-dx) { err -= dy; x0 += sx; }
        if (e2 < dy) { err += dx; y0 += sy; }
    }
}

bool track_line(uint8_t * im1, bool * im1valid, uint8_t * im2,  bool * im2valid, int width, int height, int im1_x, int im1_y, int x0, int y0, int x1, int y1, float & bestx, float & besty, float & bestscore)
{
    int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1; 
    int err = (dx>dy ? dx : -dy)/2, e2;

    bool valid_match = false;
    bestscore = maximum_match_score;

    while(true) {
        float score = score_match(im1, im1valid, width, height, width, im1_x, im1_y, im2, im2valid, x0, y0, maximum_match_score);

        if(score < bestscore) {
          valid_match = true;
          bestscore = score;
          bestx = x0;
          besty = y0;
        }

        // move along the line
        if (x0==x1 && y0==y1) break;
        e2 = err;
        if (e2 >-dx) { err -= dy; x0 += sx; }
        if (e2 < dy) { err += dx; y0 += sy; }
    }

    return valid_match;
}

f_t estimate_kr(v4 point, f_t k1, f_t k2, f_t k3)
{
    f_t r2 = point[0]*point[0] + point[1]*point[1];
    f_t r4 = r2 * r2;
    f_t r6 = r4 * r2;
    f_t kr = 1. + r2 * k1 + r4 * k2 + r6 * k3;
    return kr;
}

v4 calibrate_im_point(v4 normalized_point, float k1, float k2, float k3)
{
    f_t kr;
    v4 calibrated_point = normalized_point;
    //forward calculation - guess calibrated from initial
    kr = estimate_kr(normalized_point, k1, k2, k3);
    calibrated_point[0] /= kr;
    calibrated_point[1] /= kr;
    //backward calculation - use calibrated guess to get new parameters and recompute
    kr = estimate_kr(calibrated_point, k1, k2, k3);
    calibrated_point = normalized_point;
    calibrated_point[0] /= kr;
    calibrated_point[1] /= kr;
    return calibrated_point;
}

v4 project_point(f_t x, f_t y, f_t center_x, f_t center_y, float focal_length)
{
    return v4((x - center_x)/focal_length, (y - center_y)/focal_length, 1, 1);
}

// From http://paulbourke.net/geometry/pointlineplane/lineline.c
bool line_line_intersect(v4 p1, v4 p2, v4 p3, v4 p4, v4 & pa, v4 & pb)
{
    v4 p13,p43,p21;
    double d1343,d4321,d1321,d4343,d2121;
    double numer,denom;

    f_t eps = 1e-14;

    p13 = p1 - p3;
    p43 = p4 - p3;
    if (fabs(p43[0]) < eps && fabs(p43[1]) < eps && fabs(p43[2]) < eps)
      return false;

    p21 = p2 - p1;
    if (fabs(p21[0]) < eps && fabs(p21[1]) < eps && fabs(p21[2]) < eps)
      return false;

    d1343 = sum(p13 * p43); //p13.x * p43.x + p13.y * p43.y + p13.z * p43.z;
    d4321 = sum(p43 * p21); //p43.x * p21.x + p43.y * p21.y + p43.z * p21.z;
    d1321 = sum(p13 * p21); //p13.x * p21.x + p13.y * p21.y + p13.z * p21.z;
    d4343 = sum(p43 * p43); //p43.x * p43.x + p43.y * p43.y + p43.z * p43.z;
    d2121 = sum(p21 * p21); //p21.x * p21.x + p21.y * p21.y + p21.z * p21.z;

    denom = d2121 * d4343 - d4321 * d4321;
    if (fabs(denom) < eps)
      return false;
    numer = d1343 * d4321 - d1321 * d4343;

    float mua = numer / denom;
    float mub = (d1343 + d4321 * mua) / d4343;

    pa = p1 + mua*p21;
    pb = p3 + mub*p43;
    // Return homogeneous points
    pa[3] = 1; 
    pb[3] = 1;

    return true;
}

//TODO: estimate_F doesnt agree with eight point F. This is now correct for F corresponding to X2 = R * X1 + T

m4 estimate_F(const struct stereo_global &g, const stereo_frame &reference, const stereo_frame &target, m4 & dR, v4 & dT)
{
    /*
    x1_w = R1 * x1 + T1
    x2 = R2^t * (R1 * x1 + T1 - T2)
    
    R21 = R2^t * R1
    T21 = R2^t * (T1 - T2)
    */
    m4 R1w = to_rotation_matrix(reference.W);
    m4 R2w = to_rotation_matrix(target.W);
    dR = transpose(R2w)*R1w;
    dT = transpose(R2w) * (reference.T - target.T);

    // E21 is 3x3
    m4 E21 = skew3(dT) * dR;

    m4 Kinv;
    Kinv[0][0] = 1./g.focal_length;
    Kinv[1][1] = 1./g.focal_length;
    Kinv[0][2] = -g.center_x/g.focal_length;
    Kinv[1][2] = -g.center_y/g.focal_length;
    Kinv[2][2] = 1;
    Kinv[3][3] = 1;

    m4 F21 = transpose(Kinv)*E21*Kinv;

    // for numerical conditioning
    // F = F / norm(F) / sign(F(3,3))
    float Fnorm = 0;
    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
            Fnorm += F21[i][j]*F21[i][j];
    Fnorm = sqrt(Fnorm);

    if(F21[2][2] < 0)
        Fnorm = -Fnorm;

    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
            F21[i][j] = F21[i][j] / Fnorm;

    return F21;
}

bool compare_score(const stereo_match & m1, const stereo_match & m2)
{
    return m1.score < m2.score;
}

bool stereo::find_and_triangulate_top_n(int reference_x, int reference_y, int width, int height, int n, vector<struct stereo_match> & matches) const
{
    v4 p1 = v4(reference_x, reference_y, 1, 0);

    // p2 should lie on this line
    v4 l1 = F*p1;

    bool success = false;
    float endpoints[4];
    float error;

    matches.clear();
    vector<struct stereo_match> temp_matches;
    if(line_endpoints(l1, width, height, endpoints)) {
        match_scores(reference->image, reference->valid, target->image, target->valid, width, height, reference_x, reference_y, endpoints, temp_matches);
        std::sort(temp_matches.begin(), temp_matches.end(), compare_score);
        for(int i = 0; i < temp_matches.size(); i++) {
            struct stereo_match match = temp_matches[i];
            if(triangulate_internal(*reference, *target, reference_x, reference_y, match.x, match.y, match.point, match.depth, error))
                matches.push_back(match);
            if(matches.size() >= n) {
                success = true;
                break;
            }
        }
    }

    // pad matches with no match where appropriate
    for(int i = (int)matches.size(); i < n; i++) {
        struct stereo_match no_match;
        no_match.score = maximum_match_score;
        no_match.x = reference_x;
        no_match.y = reference_y;
        no_match.point = v4(0,0,0,0);
        no_match.depth = 0;
        matches.push_back(no_match);
    }

    return success;
}

// F is from reference to target
bool find_correspondence(const stereo_frame & reference, const stereo_frame & target, const m4 &F, int reference_x, int reference_y, int width, int height, struct stereo_match & match)
{
    v4 p1 = v4(reference_x, reference_y, 1, 0);

    // p2 should lie on this line
    v4 l1 = F*p1;

    // ground truth sanity check
    // Normalize the line equation so that distances can be computed
    // with the dot product
    //l1 = l1 / sqrt(l1[0]*l1[0] + l1[1]*l1[1]);
    //float d = sum(l1*p2);
    //fprintf(stderr, "distance p1 %f\n", d);

    bool success = false;
    float endpoints[4];
    if(line_endpoints(l1, width, height, endpoints)) {
        success = track_line(reference.image, reference.valid, target.image, target.valid, width, height, p1[0], p1[1],
                                 endpoints[0], endpoints[1], endpoints[2], endpoints[3],
                                 match.x, match.y, match.score);
        if(enable_jitter && success) {
            int upper_left_x = match.x - 3;
            int upper_left_y = match.y - 3;
            int lower_right_x = match.x + 3;
            int lower_right_y = match.y + 3;
            // if this function returns true, then we have changed target_x and target_y to a new value.
            // This happens in most cases, likely due to camera distortion
            track_window(reference.image, reference.valid, target.image, target.valid, width, height, p1[0], p1[1], upper_left_x, upper_left_y, lower_right_x, lower_right_y, match.x, match.y, match.score);
        }
    }

    return success;
}

// note, R,T is from p1 to p2, not from p2 to p1 (as in world coordinates)
bool validate_RT(const m4 & R, const v4 & T, const v4 & p1, const v4 & p2)
{
    // TODO: decide if I should use this or intersection
    // positive depth constraint
    /*
    m4 skew_Rp1 = skew3(R*p1);
    m4 skew_p2 = skew3(p2);
    float alpha1 = -sum((skew_p2*T)*(skew_p2*R*p1))/pow(norm(skew_p2*T), 2);
    float alpha2 =  sum((skew_Rp1*p2)*(skew_Rp1*T))/pow(norm(skew_Rp1*p2), 2);
     */

    // actual intersection
    v4 o1 = T;
    v4 o2 = v4(0,0,0,0);
    v4 p12 = R*p1 + T;
    v4 pa, pb;
    bool success = line_line_intersect(o1, p12, o2, p2, pa, pb);
    if(!success) return false;

    // switch to camera relative coordinates
    pa = transpose(R)*(pa - T);

    //fprintf(stderr, "intersection says %d, alpha says %f %f\n", (pa[2] > 0 & pb[2] > 0), alpha1, alpha2);

    if(pa[2] < 0 || pb[2] < 0) return false;

    return true;
}

bool decompose_F(const m4 & F, float focal_length, float center_x, float center_y, const v4 & p1, const v4 & p2, m4 & R, v4 & T)
{
    m4 Kinv;
    Kinv[0][0] = 1./focal_length;
    Kinv[1][1] = 1./focal_length;
    Kinv[0][2] = -center_x/focal_length;
    Kinv[1][2] = -center_y/focal_length;
    Kinv[2][2] = 1;
    Kinv[3][3] = 1;

    m4 K;
    K[0][0] = focal_length;
    K[1][1] = focal_length;
    K[0][2] = center_x;
    K[1][2] = center_y;
    K[2][2] = 1;
    K[3][3] = 1;

    m4 E4 = transpose(K)*F*K;
    v4 p1p = Kinv*p1;
    v4 p2p = Kinv*p2;

    matrix temp1(3,3);
    matrix temp2(3,3);

    matrix E(3, 3);
    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
            E(i,j) = E4[i][j];

    matrix W(3, 3);
    W(0,0) = 0; W(0,1) = -1; W(0,2) = 0;
    W(1,0) = 1; W(1,1) = 0;  W(1,2) = 0;
    W(2,0) = 0; W(2,1) = 0;  W(2,2) = 1;
    matrix Wt(3, 3);
    matrix_transpose(Wt, W);

    matrix Rzp = W;
    matrix Rzn(3,3);
    Rzn(0,0) = 0;  Rzn(0,1) = 1; Rzn(0,2) = 0;
    Rzn(1,0) = -1; Rzn(1,1) = 0; Rzn(1,2) = 0;
    Rzn(2,0) = 0;  Rzn(2,1) = 0; Rzn(2,2) = 1;

    matrix Z(3, 3);
    Z(0,0) = 0; Z(0,1) = -1; Z(0,2) = 0;
    Z(1,0) = 1; Z(1,1) = 0;  Z(1,2) = 0;
    Z(2,0) = 0; Z(2,1) = 0;  Z(2,2) = 0;

    matrix U(3, 3);
    matrix S(1, 3);
    matrix Vt(3, 3);
    matrix V(3,3);
    matrix_svd(E, U, S, Vt);
    // V = Vt'
    matrix_transpose(V, Vt);

    m4 R1, R2;
    v4 T1, T2;

    float det_U = matrix_3x3_determinant(U);
    float det_V = matrix_3x3_determinant(V);
    if(det_U < 0 && det_V < 0) {
        matrix_negate(U);
        matrix_negate(V);
    }
    else if(det_U < 0 && det_V > 0) {
        // U = -U*Rzn; V = V*Rzp;
        matrix_negate(U);
        matrix_product(temp1, U, Rzn);
        U = temp1;
        matrix_product(temp1, V, Rzp);
        V = temp1;
    }
    else if(det_U > 0 && det_V < 0) {
        // U = U*Rzn; V = -V*Rzp;
        matrix_negate(V);
        matrix_product(temp1, U, Rzn);
        U = temp1;
        matrix_product(temp1, V, Rzp);
        V = temp1;
    }

    // Vt = V' after possibly shifting V
    matrix_transpose(Vt, V);

    // T = u_3
    T1[0] = U(0,2);
    T1[1] = U(1,2);
    T1[2] = U(2,2);
    T2 = -T1;

    // R1 = UWV'
    matrix_product(temp1, W, Vt);
    matrix_product(temp2, U, temp1);
    for(int row = 0; row < 3; row++)
        for(int col = 0; col < 3; col++) {
            R1[row][col] = temp2(row,col);
        }
    R1[3][0] = 0; R1[3][1] = 0; R1[3][2] = 0; R1[3][3] = 1;

    // R2 = UW'V'
    matrix_product(temp1, Wt, Vt);
    matrix_product(temp2, U, temp1);
    for(int row = 0; row < 3; row++)
        for(int col = 0; col < 3; col++) {
            R2[row][col] = temp2(row,col);
        }
    R2[3][0] = 0; R2[3][1] = 0; R2[3][2] = 0; R2[3][3] = 1;

    int nvalid = 0;
    if(validate_RT(R1, T1, p1p, p2p)) {
        R = R1;
        T = T1;
        nvalid++;
    }

    if(validate_RT(R2, T1, p1p, p2p)) {
        R = R2;
        T = T1;
        nvalid++;
    }

    if(validate_RT(R1, T2, p1p, p2p)) {
        R = R1;
        T = T2;
        nvalid++;
    }

    if(validate_RT(R2, T2, p1p, p2p)) {
        R = R2;
        T = T2;
        nvalid++;
    }

    if(nvalid != 1) {
        fprintf(stderr, "Error: There should always be only one solution, but found %d", nvalid);
        return false;
    }

    return true;
}

m4 eight_point_F(v4 p1[], v4 p2[], int npts)
{
    // normalize points
    v4 center1 = v4(0., 0., 0., 0.);
    v4 center2 = v4(0., 0., 0., 0.);
    float norm1 = 0, norm2 = 0;
    for(int i = 0; i < npts; i++) {
        center1 += p1[i];
        center2 += p2[i];
    }
    center1 /= npts;
    center2 /= npts;
    for(int i = 0; i < npts; i++) {
        norm1 += sum((p1[i] - center1)*(p1[i] - center1));
        norm2 += sum((p2[i] - center1)*(p2[i] - center2));
    }
    norm1 = sqrt(2.*npts / norm1);
    norm2 = sqrt(2.*npts / norm2);

    matrix T1(3, 3);
    matrix T2(3, 3);
    T1(0, 0) = norm1; T1(0, 1) = 0;     T1(0, 2) = -norm1*center1[0];
    T1(1, 0) = 0;     T1(1, 1) = norm1; T1(1, 2) = -norm1*center1[1];
    T1(2, 0) = 0;     T1(2, 1) = 0;     T1(2, 2) = 1;


    T2(0, 0) = norm2; T2(0, 1) = 0;     T2(0, 2) = -norm2*center2[0];
    T2(1, 0) = 0;     T2(1, 1) = norm2; T2(1, 2) = -norm2*center2[1];
    T2(2, 0) = 0;     T2(2, 1) = 0;     T2(2, 2) = 1;

    matrix constraints(npts, 9);
    for(int i = 0; i < npts; i++) {
        v4 p1n = (p1[i] - center1) * norm1;
        v4 p2n = (p2[i] - center2) * norm2;
        constraints(i, 0) = p1n[0]*p2n[0];
        constraints(i, 1) = p1n[1]*p2n[0];
        constraints(i, 2) = p2n[0];
        constraints(i, 3) = p1n[0]*p2n[1];
        constraints(i, 4) = p1n[1]*p2n[1];
        constraints(i, 5) = p2n[1];
        constraints(i, 6) = p1n[0];
        constraints(i, 7) = p1n[1];
        constraints(i, 8) = 1;
    }


    // some columns of U are inverted compared to matlab
    matrix U(npts, npts);
    matrix S(1, min(npts, 9));
    matrix Vt(9, 9);

    matrix_svd(constraints, U, S, Vt);

    matrix F(3, 3);
    F(0, 0) = Vt(8, 0); F(0, 1) = Vt(8, 1); F(0, 2) = Vt(8, 2);
    F(1, 0) = Vt(8, 3); F(1, 1) = Vt(8, 4); F(1, 2) = Vt(8, 5);
    F(2, 0) = Vt(8, 6); F(2, 1) = Vt(8, 7); F(2, 2) = Vt(8, 8);

    matrix UF(3, 3);
    matrix SF(1, 3);
    matrix VFt(3, 3);
    matrix_svd(F, UF, SF, VFt);
    matrix SE(3, 3);
    // Convert S back to a diagonal matrix (matrix_svd fills a vector)
    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
        {
            if(i == j)
                SE(i,j) = SF(0,i);
            else
                SE(i,j) = 0;
        }
    SE(2,2) = 0;
    
    // TODO: there is probably a cleaner way to do this:
    // F = transpose(T2) * U * S * Vt * T1;
    matrix temp1(3, 3);
    matrix temp2(3, 3);
    matrix_product(temp1, VFt, T1);
    matrix_product(temp2, SE, temp1); 
    matrix_product(temp1, UF, temp2); 
    matrix_product(temp2, T2, temp1, true); // transpose T2
    F = temp2;

    // F = F / norm(F) / sign(F(3,3))
    float Fnorm = 0;
    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
            Fnorm += F(i,j)*F(i,j);
    Fnorm = sqrt(Fnorm);

    if(F(2,2) < 0)
      Fnorm = -Fnorm;

    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
            F(i,j) = F(i,j) / Fnorm;

    m4 estimatedF;
    for(int i = 0; i < 3; i++)
      for(int j = 0; j < 3; j++)
        estimatedF[i][j] = F(i,j);
    estimatedF[3][3] = 1;

    return estimatedF;
}

bool estimate_F_eight_point(const stereo_frame & reference, const stereo_frame & target, m4 & F)
{
    vector<v4> reference_pts;
    vector<v4> target_pts;

    // This assumes reference->features and target->features are sorted by id
    for(list<stereo_feature>::const_iterator s1iter = reference.features.begin(); s1iter != reference.features.end(); ++s1iter) {
        stereo_feature f1 = *s1iter;
        for(list<stereo_feature>::const_iterator s2iter = target.features.begin(); s2iter != target.features.end(); ++s2iter) {
            stereo_feature f2 = *s2iter;
            if(f1.id == f2.id) {
                reference_pts.push_back(f1.current);
                target_pts.push_back(f2.current);
            }
        }
    }

    if(reference_pts.size() < 8) {
        return false;
    }

    F = eight_point_F(&reference_pts[0], &target_pts[0], (int)reference_pts.size());

    return true;
}

#include "sift.h"
typedef struct _sift_keypoint
{
    xy pt;
    float sigma;
    vl_sift_pix d [128] ;
} sift_keypoint;

typedef struct _sift_match
{
    int i1, i2;
    float score;
} sift_match;

int compute_inliers(const v4 from [], const v4 to [], int nmatches, const m4 & F, float thresh, bool inliers [])
{
    int ninliers = 0;
    for(int i = 0; i < nmatches; i++) {
        // Sampson distance x2*F*x1 / (sum((Fx1).^2) + sum((F'x2).^2))
        v4 pfp = to[i]*F*from[i];
        v4 el1 = F*from[i];
        v4 el2 = transpose(F)*to[i];
        float distance = sum(pfp);
        distance = distance*distance;
        distance = distance / (el1[0]*el1[0] + el1[1]*el1[1] + el2[0]* el2[0] + el2[1]*el2[1]);

// distance in pixels
//        v4 line = F*from[i];
//        line = line / sqrt(line[0]*line[0] + line[1]*line[1]);
//        float distance = fabs(sum(to[i]*line));
        if(distance < thresh) {
            inliers[i] = true;
            ninliers++;
        }
        else
            inliers[i] = false;
    }
    return ninliers;
}

// Chooses k random elements from a list with n elements and puts them in inds
// Uses reservoir sampling
void choose_random(int k, int n, int * inds)
{
    int i;

    // Initialize reservoir
    for(i = 0; i < k; i++)
        inds[i] = i;

    for(; i < n; i++) {
        // Pick a random index from 0 to i.
        int j = rand() % (i+1);

        // If the randomly picked index is smaller than k, then replace
        // the element present at the index with new element
        if (j < k)
            inds[j] = i;
    }
}

bool refine_F(const vector<v4> & reference_pts, const vector<v4> target_pts, m4 & F)
{
    vector<v4> estimate_pts1;
    vector<v4> estimate_pts2;

    const int npoints = (int)reference_pts.size();
    bool inliers[npoints];
    float sampson_thresh = 0.5;
    int ninliers;

    for(int row = 0; row < 4; row++) {
        for(int col = 0; col < 4; col++) {
            fprintf(stderr, "%f ", F[row][col]);
        }
        fprintf(stderr, "\n");
    }

    ninliers = compute_inliers(&reference_pts[0], &target_pts[0], npoints, F, sampson_thresh, inliers);
    fprintf(stderr, "%d inliers\n", ninliers);

    estimate_pts1.clear();
    estimate_pts2.clear();
    for(int m = 0; m < npoints; m++) {
        if(inliers[m]) {
            estimate_pts1.push_back(reference_pts[m]);
            estimate_pts2.push_back(target_pts[m]);
        }
    }
    m4 F2 = eight_point_F(&estimate_pts1[0], &estimate_pts2[0], (int)estimate_pts1.size());

    ninliers = compute_inliers(&reference_pts[0], &target_pts[0], npoints, F2, sampson_thresh, inliers);
    fprintf(stderr, "%d inliers sampson\n", ninliers);

    estimate_pts1.clear();
    estimate_pts2.clear();
    for(int m = 0; m < npoints; m++) {
        if(inliers[m]) {
            estimate_pts1.push_back(reference_pts[m]);
            estimate_pts2.push_back(target_pts[m]);
        }
    }
    F2 = eight_point_F(&estimate_pts1[0], &estimate_pts2[0], (int)estimate_pts1.size());
    ninliers = compute_inliers(&reference_pts[0], &target_pts[0], npoints, F2, sampson_thresh, inliers);
    fprintf(stderr, "%d inliers refined tight\n", ninliers);

    F = F2;

    return true;
}

// F is from reference_pts to target_pts
bool ransac_F(const vector<v4> & reference_pts, const vector<v4> target_pts, m4 & F)
{
    vector<v4> estimate_pts1;
    vector<v4> estimate_pts2;
    const int npoints = (int)reference_pts.size();
    fprintf(stderr, "nmatches %d\n", npoints);
    bool inliers[npoints];
    float sampson_thresh = 0.5;

    int iterations = 2000;
    m4 bestF = F;
    int bestinliers = compute_inliers(&reference_pts[0], &target_pts[0], npoints, bestF, sampson_thresh, inliers);
    fprintf(stderr, "started with %d inliers", bestinliers);

    for(int i = 0; i < iterations; i++)
    {
        int points[8];
        // select 8 random points
        choose_random(8, npoints, points);

        // estimate F
        estimate_pts1.clear();
        estimate_pts2.clear();
        for(int m = 0; m < 8; m++) {
            estimate_pts1.push_back(reference_pts[points[m]]);
            estimate_pts2.push_back(target_pts[points[m]]);
        }
        m4 currentF = eight_point_F(&estimate_pts1[0], &estimate_pts2[0], (int)estimate_pts1.size());

        // measure fitness
        int currentinliers = compute_inliers(&reference_pts[0], &target_pts[0], npoints, currentF, sampson_thresh, inliers);
        if(currentinliers > bestinliers) {
            bestinliers = currentinliers;
            bestF = currentF;
        }
    }
    fprintf(stderr, "best_inliers %d\n", bestinliers);

    // measure fitness
    int best_inliers = compute_inliers(&reference_pts[0], &target_pts[0], npoints, bestF, sampson_thresh, inliers);
    fprintf(stderr, "should be the same best_inliers %d\n", best_inliers);

    // estimate F
    estimate_pts1.clear();
    estimate_pts2.clear();
    for(int m = 0; m < npoints; m++) {
        if(inliers[m]) {
            estimate_pts1.push_back(reference_pts[m]);
            estimate_pts2.push_back(target_pts[m]);
        }
    }
    fprintf(stderr, "calling eight point with %lu points (should be best_inliers)\n", estimate_pts1.size());
    bestF = eight_point_F(&estimate_pts1[0], &estimate_pts2[0], (int)estimate_pts1.size());

    // measure fitness
    int final_inliers = compute_inliers(&reference_pts[0], &target_pts[0], npoints, bestF, sampson_thresh, inliers);
    fprintf(stderr, "after estimating with %lu final inliers %d\n", estimate_pts1.size(), final_inliers);

    F = bestF;

    return true;
}

vector<sift_match> ubc_match(vector<sift_keypoint> k1, vector<sift_keypoint>k2)
{
    double thresh = 1.5;

    vector<sift_match> matches;
    for(unsigned int i1 = 0; i1 < k1.size(); i1++) {
        sift_match match;
        float best_score = 1e10;
        float second_best_score = 1e10;
        int bestk = -1;
        for(unsigned int i2 = 0; i2 < k2.size(); i2++) {
            double acc = 0;
            for(int bin = 0; bin < 128; bin++)
            {
                float delta = k1[i1].d[bin] - k2[i2].d[bin];
                acc += delta*delta;
            }

            if(acc < best_score) {
                second_best_score = best_score;
                best_score = acc;
                bestk = i2;
            } else if(acc < second_best_score) {
                second_best_score = acc;
            }
        }
        /* Lowe's method: accept the match only if unique. */
        if(second_best_score == 1e10) continue;

        if(thresh * (float) best_score < (float) second_best_score && bestk != -1) {
            match.i1 = i1;
            match.i2 = bestk;
            match.score = best_score;
            matches.push_back(match);
        }
    }
    return matches;
}

vector<sift_keypoint> sift_detect(uint8_t * image, int width, int height, int noctaves, int nlevels, int o_min)
{
    vector<sift_keypoint> keypoints;
    vl_sift_pix * fdata = (vl_sift_pix *)malloc(width*height*sizeof(vl_sift_pix));

    /* convert data type */
    for (int q = 0; q < width * height; ++q) {
        fdata[q] = image[q];
    }

    VlSiftFilt * filt = vl_sift_new(width, height, noctaves, nlevels, o_min);
    /* ...............................................................
     *                                             Process each octave
     * ............................................................ */
    int i     = 0 ;
    int first = 1 ;
    int err;
    while (1) {
        VlSiftKeypoint const *keys = 0 ;
        int                   nkeys ;

        /* calculate the GSS for the next octave .................... */
        if (first) {
            first = 0 ;
            err = vl_sift_process_first_octave (filt, fdata) ;
        } else {
            err = vl_sift_process_next_octave  (filt) ;
        }

        if (err) {
            err = VL_ERR_OK ;
            break ;
        }

        /* run detector ............................................. */
        vl_sift_detect (filt) ;

        keys  = vl_sift_get_keypoints     (filt) ;
        nkeys = vl_sift_get_nkeypoints (filt) ;
        i     = 0 ;

        printf ("sift: detected %d (unoriented) keypoints\n", nkeys) ;

        /* for each keypoint ........................................ */
        for (; i < nkeys ; ++i) {
            double                angles [4] ;
            int                   nangles ;
            VlSiftKeypoint const *k ;

            /* obtain keypoint orientations ........................... */
            k = keys + i ;
            nangles = vl_sift_calc_keypoint_orientations(filt, angles, k) ;

            /* for each orientation ................................... */
            for (int q = 0 ; q < (unsigned) nangles ; ++q) {
                sift_keypoint descr;

                /* compute descriptor (if necessary) */
                vl_sift_calc_keypoint_descriptor(filt, descr.d, k, angles [q]) ;
                descr.pt.x = k->x;
                descr.pt.y = k->y;
                descr.sigma = k->sigma;
                /* possibly should convert to uint8 for matching later */
                /*
                 int l ;
                 for (l = 0 ; l < 128 ; ++l) {
                double x = 512.0 * descr[l] ;
                x = (x < 255.0) ? x : 255.0 ;
                vl_file_meta_put_uint8 (&dsc, (vl_uint8) (x)) ;
                 }
                 */
                keypoints.push_back(descr);
            }
        }
    }
    free(fdata);
    return keypoints;
}

bool stereo::reestimate_F(const stereo_frame & reference, const stereo_frame & target, m4 & F, m4 & R, v4 & T)
{
    // Detect and describe
    int noctaves = -1;
    int nlevels = 3;
    int o_min = 0;
    vector<sift_keypoint> reference_keypoints = sift_detect(reference.image, width, height, noctaves, nlevels, o_min);
    vector<sift_keypoint> target_keypoints = sift_detect(target.image, width, height, noctaves, nlevels, o_min);
    fprintf(stderr, "ref %lu\n", reference_keypoints.size());
    fprintf(stderr, "target %lu\n", target_keypoints.size());

    // Match
    vector<sift_match> matches = ubc_match(reference_keypoints, target_keypoints);
    fprintf(stderr, "size of matches %lu\n", matches.size());

    vector<v4> reference_correspondences;
    vector<v4> target_correspondences;
    for(int m = 0; m < matches.size(); m++) {
        xy p1 = reference_keypoints[matches[m].i1].pt;
        xy p2 = target_keypoints[matches[m].i2].pt;
        reference_correspondences.push_back(v4(p1.x, p1.y, 1, 0));
        target_correspondences.push_back(v4(p2.x, p2.y, 1, 0));
    }

    // Estimate F
    ransac_F(reference_correspondences, target_correspondences, F);
    const int npoints = (int)reference_correspondences.size();
    bool inliers[npoints];
    float sampson_thresh = 0.5;

    compute_inliers(&reference_correspondences[0], &target_correspondences[0], npoints, F, sampson_thresh, inliers);
    v4 p1, p2;
    bool valid = false;
    for(int i = 0; i < npoints; i++) {
        if(inliers[i]) {
            p1 = reference_correspondences[i];
            p2 = target_correspondences[i];
            valid = true;
            break;
        }
    }

    if(!valid)
        return false;

    // Note: R & T are from reference to target, but stereo frames are stored as
    // target to reference
    // TODO: Use more than one correspondence for more robust validation
    decompose_F(F, focal_length, center_x, center_y, p1, p2, R, T);

    return true;
}

// Triangulates a point in the world reference frame from two views
bool stereo::triangulate_internal(const stereo_frame & reference, const stereo_frame & target, int reference_x, int reference_y, int target_x, int target_y, v4 & intersection, float & depth, float & error) const
{
    v4 o1_transformed, o2_transformed;
    v4 pa, pb;
    bool success;

    v4 p1_projected = project_point(reference_x, reference_y, center_x, center_y, focal_length);
    v4 p1_calibrated = calibrate_im_point(p1_projected, k1, k2, k3);
    v4 p2_projected = project_point(target_x, target_y, center_x, center_y, focal_length);
    v4 p2_calibrated = calibrate_im_point(p2_projected, k1, k2, k3);

    m4 R1w = to_rotation_matrix(reference.W);
    m4 R2w = to_rotation_matrix(target.W);

    v4 p1_cal_transformed = R1w*p1_calibrated + reference.T;
    v4 p2_cal_transformed = R2w*p2_calibrated + target.T;
    o1_transformed = reference.T;
    o2_transformed = target.T;

    // pa is the point on the first line closest to the intersection
    // pb is the point on the second line closest to the intersection
    success = line_line_intersect(o1_transformed, p1_cal_transformed, o2_transformed, p2_cal_transformed, pa, pb);
    if(!success) {
        if(debug_triangulate)
            fprintf(stderr, "Failed intersect\n");
        return false;
    }

    error = norm(pa - pb);
    v4 cam1_intersect = transpose(R1w) * (pa - reference.T);
    v4 cam2_intersect = transpose(R2w) * (pb - target.T);
    if(debug_triangulate)
        fprintf(stderr, "Lines were %.2fcm from intersecting at a depth of %.2fcm\n", error*100, cam1_intersect[2]*100);

    if(cam1_intersect[2] < 0 || cam2_intersect[2] < 0) {
        if(debug_triangulate)
            fprintf(stderr, "Lines intersected at a negative camera depth, failing\n");
        return false;
    }

    if(error/cam1_intersect[2] > .1) {
        if(debug_triangulate)
            fprintf(stderr, "Error too large, failing\n");
        return false;
    }
    intersection = pa + (pb - pa)/2;
    depth = cam1_intersect[2];

    return true;
}

bool stereo::preprocess_internal(const stereo_frame &from, stereo_frame &to, m4 &F, bool use_eight_point)
{
    bool success = true;
    used_eight_point = use_eight_point;
    // estimate_F uses R,T, and the camera calibration
    if(!use_eight_point)
        F_motion = estimate_F(*this, from, to, dR, dT);
    else
    // estimate_F_eight_point uses common tracked features between the two frames
        success = estimate_F_eight_point(from, to, F);

    m4 R_eight_point;
    v4 T_eight_point;
    F = F_motion;
    bool valid = reestimate_F(from, to, F_eight_point, R_eight_point, T_eight_point);
    if(valid) {
        F = F_eight_point;
        if(enable_eight_point_motion) {
            m4 Rto = transpose(R_eight_point);
            v4 Tto = -transpose(R_eight_point)*T_eight_point;
            v4 Rvec = invrodrigues(Rto, NULL);
            to.W = rotation_vector(Rvec[0], Rvec[1], Rvec[2]);
            to.T = Tto * norm(to.T); // keep the magnitude of T
        }
    }
    else
        fprintf(stderr, "Invalid F estimation\n");

    if(enable_debug_files) {
        write_debug_info();
    }

    return success;
}

void stereo::transform_to_reference(const stereo_frame * transform_to)
{
    quaternion toQ = conjugate(to_quaternion(transform_to->W));

    target->T = quaternion_rotate(toQ, (target->T - transform_to->T));
    target->W = to_rotation_vector(quaternion_product(toQ, to_quaternion(target->W)));

    reference->T = quaternion_rotate(toQ, (reference->T - transform_to->T));
    reference->W = to_rotation_vector(quaternion_product(toQ, to_quaternion(reference->W)));
}

bool stereo::triangulate_mesh(int reference_x, int reference_y, v4 & intersection) const
{
    if(!reference || !target)
        return false;

    bool result = stereo_mesh_triangulate(mesh, *this, reference_x, reference_y, intersection);
    return result;

}

bool stereo::triangulate_top_n(int reference_x, int reference_y, int n, vector<struct stereo_match> & matches) const
{
    if(!reference || !target)
        return false;

    // sets match
    bool ok = find_and_triangulate_top_n(reference_x, reference_y, width, height, n, matches);
    return ok;
}

// TODO: stereo_mesh uses rectified frames directly to find points to
// triangulate, but in general triangulate should use distorted coordinates and
// rectify them to match the internal representation of the images
bool stereo::triangulate(int reference_x, int reference_y, v4 & intersection, struct stereo_match * match) const
{
    if(!reference || !target)
        return false;

    struct stereo_match match_internal;
    float error;
    
    // sets match
    bool ok = find_correspondence(*reference, *target, F, reference_x, reference_y, width, height, match_internal);
    if(ok)
        ok = triangulate_internal(*reference, *target, reference_x, reference_y, match_internal.x, match_internal.y, intersection, match_internal.depth, error);

    match_internal.point = intersection;
    if(match) *match = match_internal;

    return ok;
}

bool compare_id(const stereo_feature & f1, const stereo_feature & f2)
{
    return f1.id < f2.id;
}

int intersection_length(list<stereo_feature> &l1, list<stereo_feature> &l2)
{
    l1.sort(compare_id);
    l2.sort(compare_id);

    vector<stereo_feature> intersection;
    intersection.resize(max(l1.size(),l2.size()));
    vector<stereo_feature>::iterator it;

    it = std::set_intersection(l1.begin(), l1.end(), l2.begin(), l2.end(), intersection.begin(), compare_id);
    int len = (int)(it - intersection.begin());

    return len;
}

v4 stereo::baseline()
{
    if(!target)
        return v4(0,0,0,0);
    
    m4 R1w = to_rotation_matrix(target->W);
    
    return transpose(R1w) * (T - target->T);
}

void stereo::process_frame(const struct stereo_global &g, const uint8_t *data, list<stereo_feature> &features, bool final)
{
    stereo_global::operator=(g);
    
    if(final) {
        if(reference) delete reference;
        reference = new stereo_frame(data, g.width, g.height, g.T, g.W, features);
        if(enable_debug_files) {
            write_frames(false);
        }

        if(enable_rectify)
            rectify_frames();

        if(enable_debug_files && enable_rectify) {
            write_frames(true);
        }
    } else if(features.size() >= 15) {
        if(!target) {
            target = new stereo_frame(data, g.width, g.height, g.T, g.W, features);
        }
        /* Uncomment to automatically enable updating the saved stereo state
         * when there are less than 15 features overlapping
        else if(intersection_length(target->features, features) < 15) {
            if(target) delete target;
            target = new stereo_frame(data, g.width, g.height, g.T, g.W, features);
        }
         */
    }
}

bool stereo::preprocess(bool use_eight_point)
{
    if(!target || !reference) return false;

    transform_to_reference(reference);

    return preprocess_internal(*reference, *target, F, use_eight_point);
}

bool stereo::preprocess_mesh(void(*progress_callback)(float))
{
    if(!target || !reference) return false;

    // creates a mesh by searching for correspondences from reference to target
    mesh = stereo_mesh_create(*this, progress_callback);

    char filename[1024];
    char suffix[1024] = "";
    if(used_eight_point)
        sprintf(suffix, "-eight-point");

    if(enable_debug_files) {
        snprintf(filename, 1024, "%s%s.ply", debug_basename, suffix);
        stereo_mesh_write(filename, mesh, debug_texturename);
        snprintf(filename, 1024, "%s%s.json", debug_basename, suffix);
        stereo_mesh_write_rotated_json(filename, mesh, *this, orientation, debug_texturename);
        snprintf(filename, 1024, "%s%s-correspondences.csv", debug_basename, suffix);
        stereo_mesh_write_correspondences(filename, mesh);
        snprintf(filename, 1024, "%s%s-topn-correspondences.csv", debug_basename, suffix);
        stereo_mesh_write_topn_correspondences(filename);
        snprintf(filename, 1024, "%s%s-unary.csv", debug_basename, suffix);
        stereo_mesh_write_unary(filename);
        //snprintf(filename, 1024, "%s%s-pairwise.csv", debug_basename, suffix);
        //stereo_mesh_write_pairwise(filename);
    }

    stereo_remesh_delaunay(mesh);

    if(enable_debug_files) {
        snprintf(filename, 1024, "%s%s-remesh.ply", debug_basename, suffix);
        stereo_mesh_write(filename, mesh, debug_texturename);
        snprintf(filename, 1024, "%s%s-remesh.json", debug_basename, suffix);
        stereo_mesh_write_rotated_json(filename, mesh, *this, orientation, debug_texturename);
        snprintf(filename, 1024, "%s%s-remesh-correspondences.csv", debug_basename, suffix);
        stereo_mesh_write_correspondences(filename, mesh);
    }

    // For some reason, we didn't find any correspondences and didn't make a mesh
    if(mesh.triangles.size() == 0) return false;

    return true;
}

void stereo::write_frames(bool is_rectified)
{
    char buffer[1024];
    if(is_rectified) {
        snprintf(buffer, 1024, "%s-target-rectified.pgm", debug_basename);
        write_image(buffer, target->image, width, height);
        snprintf(buffer, 1024, "%s-reference-rectified.pgm", debug_basename);
        write_image(buffer, reference->image, width, height);
    }
    else {
        snprintf(buffer, 1024, "%s-target.pgm", debug_basename);
        write_image(buffer, target->image, width, height);
        snprintf(buffer, 1024, "%s-reference.pgm", debug_basename);
        write_image(buffer, reference->image, width, height);
    }
}

#define interp(c0, c1, t) ((c0)*(1-(t)) + ((c1)*(t)))
float bilinear_interp(uint8_t * image, int width, int height, float x, float y)
{
    float result = 0;
    int xi = (int)x;
    int yi = (int)y;
    float tx = x - xi;
    float ty = y - yi;
    if(xi < 0 || xi >= width-1 || yi < 0 || yi >= height-1)
        return 0;
    float c00 = image[yi*width + xi];
    float c01 = image[(yi+1)*width + xi];
    float c10 = image[yi*width + xi + 1];
    float c11 = image[(yi+1)*width + xi + 1];
    result = interp(interp(c00, c10, tx), interp(c01, c11, tx), ty);
    return result;
}

void undistort_coordinate(float x, float y, f_t focal, float cx, float cy, f_t k1, f_t k2, f_t k3, float & x_undistorted, float & y_undistorted)
{
    v4 pt = v4((x - cx)/focal, (y - cy)/focal, 1, 0);
    f_t kr = estimate_kr(pt, k1, k2, k3);
    x_undistorted = pt[0] * focal * kr + cx;
    y_undistorted = pt[1] * focal * kr + cy;
}

void rectify_image(uint8_t * input, uint8_t * output, bool * valid, int width, int height, float k1, float k2, float k3, float center_x, float center_y, float focal_length)
{
    for(int y = 0; y < height; y++)
        for(int x = 0; x < width; x++) {
            float x_undistorted, y_undistorted;
            undistort_coordinate(x, y, focal_length, center_x, center_y, k1, k2, k3, x_undistorted, y_undistorted);
            if(x_undistorted < 0 || x_undistorted >= width-1 ||
               y_undistorted < 0 || y_undistorted >= height-1) {
                valid[y*width + x] = false;
                output[y*width + x] = 0;
            }
            else {
                valid[y*width + x] = true;
                output[y*width + x] = bilinear_interp(input, width, height, x_undistorted, y_undistorted);
            }
        }
}

void rectify_features(list<stereo_feature> & features, float k1, float k2, float k3, float center_x, float center_y, float focal_length)
{
    for(list<stereo_feature>::iterator fiter = features.begin(); fiter != features.end(); ++fiter) {
        stereo_feature f = *fiter;
        float x_undistorted, y_undistorted;
        undistort_coordinate(f.current[0], f.current[1], focal_length, center_x, center_y, k1, k2, k3, x_undistorted, y_undistorted);
        fiter->current = v4(x_undistorted, y_undistorted, 0, 0);
    }
}

void stereo::rectify_frames()
{
    if(!reference || !target) return;

    stereo_frame * reference_rectified = new stereo_frame(reference->image, width, height, reference->T, reference->W, reference->features);
    rectify_image(reference->image, reference_rectified->image, reference_rectified->valid, width, height, k1, k2, k3, center_x, center_y, focal_length);
    rectify_features(reference_rectified->features, k1, k2, k3, center_x, center_y, focal_length);
    delete reference;
    reference = reference_rectified;

    stereo_frame * target_rectified = new stereo_frame(target->image, width, height, target->T, target->W, target->features);
    rectify_image(target->image, target_rectified->image, target_rectified->valid, width, height, k1, k2, k3, center_x, center_y, focal_length);
    rectify_features(target_rectified->features, k1, k2, k3, center_x, center_y, focal_length);
    delete target;
    target = target_rectified;

}

void m4_file_print(FILE * fp, const char * name, m4 M)
{
    fprintf(fp, "%s = [", name);
    for(int r=0; r<4; r++) {
        for(int c=0; c<4; c++) {
            fprintf(fp, " %g ", M[r][c]);
        }
        if(r == 3)
            fprintf(fp, "];\n");
        else
            fprintf(fp, ";\n");
    }

}

void v4_file_print(FILE * fp, const char * name, v4 V)
{
    fprintf(fp, "%s = [%g; %g; %g; %g];\n", name, V[0], V[1], V[2], V[3]);
}

void stereo::write_debug_info()
{
    char filename[1024];
    if(used_eight_point)
        snprintf(filename, 1024, "%s-eight-point-debug-info.m", debug_basename);
    else
        snprintf(filename, 1024, "%s-debug-info.m", debug_basename);

    FILE * debug_info = fopen(filename, "w");


    fprintf(debug_info, "width = %d;\n", width);
    fprintf(debug_info, "height = %d;\n", height);
    v4_file_print(debug_info, "Tglobal", T);
    m4 R = to_rotation_matrix(W);
    m4_file_print(debug_info, "Rglobal", R);

    m4 Rtarget = to_rotation_matrix(target->W);
    m4 Rreference = to_rotation_matrix(reference->W);
    m4_file_print(debug_info, "Rtarget", Rtarget);
    m4_file_print(debug_info, "Rreference", Rreference);
    v4_file_print(debug_info, "Ttarget", target->T);
    v4_file_print(debug_info, "Treference", reference->T);

    m4_file_print(debug_info, "dR", dR);
    v4_file_print(debug_info, "dT", dT);
    fprintf(debug_info, "enable_jitter = %d;\n", enable_jitter);
    fprintf(debug_info, "enable_rectify = %d;\n", enable_rectify);

    fprintf(debug_info, "focal_length = %g;\n", focal_length);
    fprintf(debug_info, "center_x = %g;\n", center_x);
    fprintf(debug_info, "center_y = %g;\n", center_y);
    fprintf(debug_info, "k1 = %g;\n", k1);
    fprintf(debug_info, "k2 = %g;\n", k2);
    fprintf(debug_info, "k3 = %g;\n", k3);
    m4_file_print(debug_info, "F", F);
    m4_file_print(debug_info, "F_motion", F_motion);
    m4_file_print(debug_info, "F_eight_point", F_eight_point);

    fclose(debug_info);
}

stereo_frame::stereo_frame(const uint8_t *_image, int width, int height, const v4 &_T, const rotation_vector &_W, const list<stereo_feature > &_features): T(_T), W(_W), features(_features)
{
    image = new uint8_t[width * height];
    valid = new bool [width * height];
    memcpy(image, _image, width*height);
    memset(valid, 1, sizeof(bool)*width*height);
    // Sort features by id so when we do eight point later, they are already sorted
    features.sort(compare_id);
}

stereo_frame::~stereo_frame() { delete [] image; delete [] valid; }
