#include "stereo.h"
#include "fundamental.h"
#include "stereo_features.h"
#include "stereo_mesh.h"
#include "../filter/filter.h"

bool debug_triangulate = false;
// if enabled, adds a 3 pixel jitter in all directions to correspondence
bool enable_jitter = false;
bool enable_distortion_correction = false;
bool enable_eight_point_motion = false;
// If DEBUG or RELEASE are set, we are not in LIBRARY (building the RC3DK static library)
// or ARCHIVE (packaging an app for the store with RC3DK as a subproject).
#if DEBUG || RELEASE
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

bool line_endpoints(v3 line, int width, int height, float endpoints[4])
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
// 3 pixels average deviation from the mean across the patch
// static const float constant_patch_thresh = 3*3;
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

// From http://paulbourke.net/geometry/pointlineplane/lineline.c
bool line_line_intersect(v3 p1, v3 p2, v3 p3, v3 p4, v3 & pa, v3 & pb)
{
    v3 p13,p43,p21;
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

    d1343 = p13.dot(p43); //p13.x * p43.x + p13.y * p43.y + p13.z * p43.z;
    d4321 = p43.dot(p21); //p43.x * p21.x + p43.y * p21.y + p43.z * p21.z;
    d1321 = p13.dot(p21); //p13.x * p21.x + p13.y * p21.y + p13.z * p21.z;
    d4343 = p43.dot(p43); //p43.x * p43.x + p43.y * p43.y + p43.z * p43.z;
    d2121 = p21.dot(p21); //p21.x * p21.x + p21.y * p21.y + p21.z * p21.z;

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

bool compare_score(const stereo_match & m1, const stereo_match & m2)
{
    return m1.score < m2.score;
}

bool stereo::find_and_triangulate_top_n(int reference_x, int reference_y, int width, int height, int n, vector<struct stereo_match> & matches) const
{
    v3 p1 = v3(reference_x, reference_y, 1);

    // p2 should lie on this line
    v3 l1 = F*p1;

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
        no_match.point = v3(0,0,0);
        no_match.depth = 0;
        matches.push_back(no_match);
    }

    return success;
}

// F is from reference to target
bool find_correspondence(const stereo_frame & reference, const stereo_frame & target, const m3 &F, int reference_x, int reference_y, int width, int height, struct stereo_match & match)
{
    v3 p1 = v3(reference_x, reference_y, 1);

    // p2 should lie on this line
    v3 l1 = F*p1;

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

bool estimate_F_eight_point(const stereo_frame & reference, const stereo_frame & target, m3 & F)
{
    vector<v3> reference_pts;
    vector<v3> target_pts;

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

bool stereo::reestimate_F(const stereo_frame & reference, const stereo_frame & target, m3 & F, m3 & R, v3 & T, void(*progress_callback)(float), float progress_start, float progress_end)
{
    // Detect and describe
    int noctaves = -1;
    int nlevels = 3;
    int o_min = 0;
    float step_progress = (progress_end - progress_start)*1./3;
    vector<sift_keypoint> reference_keypoints = sift_detect(reference.image, camera.width, camera.height, noctaves, nlevels, o_min, progress_callback, progress_start, progress_start + step_progress);
    vector<sift_keypoint> target_keypoints = sift_detect(target.image, camera.width, camera.height, noctaves, nlevels, o_min, progress_callback, progress_start + step_progress, progress_start + 2*step_progress);

    // Match
    vector<sift_match> matches = ubc_match(reference_keypoints, target_keypoints, progress_callback, progress_start + 2*step_progress, progress_start + 2.8*step_progress);

    vector<v3> reference_correspondences;
    vector<v3> target_correspondences;
    for(int m = 0; m < matches.size(); m++) {
        feature_t p1 = reference_keypoints[matches[m].i1].pt;
        feature_t p2 = target_keypoints[matches[m].i2].pt;
        reference_correspondences.push_back(v3(p1.x(), p1.y(), 1));
        target_correspondences.push_back(v3(p2.x(), p2.y(), 1));
    }

    // Estimate F
    ransac_F(reference_correspondences, target_correspondences, F);
    const int npoints = (int)reference_correspondences.size();
    bool inliers[npoints];
    float sampson_thresh = 0.5;

    compute_inliers(&reference_correspondences[0], &target_correspondences[0], npoints, F, sampson_thresh, inliers);
    v3 p1, p2;
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
    decompose_F(F, camera.focal_length, camera.center_x, camera.center_y, p1, p2, R, T);
    if(progress_callback)
        progress_callback(progress_start + step_progress*3);

    return true;
}

// Triangulates a point in the world reference frame from two views
bool stereo::triangulate_internal(const stereo_frame & reference, const stereo_frame & target, int reference_x, int reference_y, int target_x, int target_y, v3 & intersection, float & depth, float & error) const
{
    v3 o1_transformed, o2_transformed;
    v3 pa, pb;
    bool success;

    v3 p1_calibrated, p2_calibrated;
    if(enable_distortion_correction) {
        p1_calibrated = camera.project_image_point(reference_x, reference_y);
        p2_calibrated = camera.project_image_point(target_x, target_y);
    }
    else {
        p1_calibrated = camera.calibrate_image_point(reference_x, reference_y);
        p2_calibrated = camera.calibrate_image_point(target_x, target_y);
    }

    m3 R1w = to_rotation_matrix(reference.W);
    m3 R2w = to_rotation_matrix(target.W);

    v3 p1_cal_transformed = R1w*p1_calibrated + reference.T;
    v3 p2_cal_transformed = R2w*p2_calibrated + target.T;
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

    error = (pa - pb).norm();
    v3 cam1_intersect = R1w.transpose() * (pa - reference.T);
    v3 cam2_intersect = R2w.transpose() * (pb - target.T);
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
    intersection = pa;
    depth = cam1_intersect[2];

    return true;
}

bool stereo::preprocess_internal(const stereo_frame &from, stereo_frame &to, m3 &F, bool use_eight_point, void(*progress_callback)(float), float progress_start, float progress_end)
{
    bool success = true;
    used_eight_point = use_eight_point;
    // estimate_F uses R,T, and the camera calibration
    if(!use_eight_point)
        F_motion = estimate_F(camera, from, to, dR, dT);
    else
    // estimate_F_eight_point uses common tracked features between the two frames
        success = estimate_F_eight_point(from, to, F);

    m3 R_eight_point;
    v3 T_eight_point;
    F = F_motion;
    bool valid = reestimate_F(from, to, F_eight_point, R_eight_point, T_eight_point, progress_callback, progress_start, progress_end);
    if(valid) {
        F = F_eight_point;
        if(enable_eight_point_motion) {
            m3 Rto = R_eight_point.transpose();
            v3 Tto = -R_eight_point.transpose()*T_eight_point;
            to.W = to_rotation_vector(Rto);
            to.T = Tto * (to.T).norm(); // keep the magnitude of T
        }
    }
#ifdef DEBUG
    else
        fprintf(stderr, "Invalid F estimation\n");
#endif
    
    if(enable_debug_files) {
        write_debug_info();
    }

    return success;
}

void stereo::transform_to_reference(const stereo_frame * transform_to)
{
    Rw = to_rotation_matrix(transform_to->W);
    Tw = transform_to->T;

    quaternion toQ = conjugate(to_quaternion(transform_to->W));

    target->T = toQ * (target->T - transform_to->T);
    target->W = to_rotation_vector(toQ * to_quaternion(target->W));

    reference->T = toQ * (reference->T - transform_to->T);
    reference->W = to_rotation_vector(toQ * to_quaternion(reference->W));
}

bool stereo::triangulate_mesh(int reference_x, int reference_y, v3 & intersection) const
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
    bool ok = find_and_triangulate_top_n(reference_x, reference_y, camera.width, camera.height, n, matches);
    return ok;
}

bool stereo::triangulate(int reference_x, int reference_y, v3 & intersection, struct stereo_match * match) const
{
    if(!reference || !target)
        return false;

    struct stereo_match match_internal;
    float error;
    
    // sets match
    bool ok = find_correspondence(*reference, *target, F, reference_x, reference_y, camera.width, camera.height, match_internal);
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

void stereo::process_frame(const class camera &g, const v3 & T, const rotation_vector & W, const uint8_t *data, list<stereo_feature> &features, bool final)
{
    camera = g;

    if(final) {
        if(reference) delete reference;
        reference = new stereo_frame(data, g.width, g.height, T, W, features);
        if(reference && target) {
            if(enable_debug_files) {
                write_frames(true);
            }

            if(enable_distortion_correction)
                undistort_frames();

            if(enable_debug_files && enable_distortion_correction) {
                write_frames(false);
            }
        }
    } else if(features.size() >= 15) {
        if(!target) {
            target = new stereo_frame(data, g.width, g.height, T, W, features);
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

bool stereo::preprocess(bool use_eight_point, void(*progress_callback)(float))
{
    if(!target || !reference) return false;

    transform_to_reference(reference);

    return preprocess_internal(*reference, *target, F, use_eight_point, progress_callback, 0, 0.2);
}

bool stereo::preprocess_mesh(void(*progress_callback)(float))
{
    if(!target || !reference) return false;

    // creates a mesh by searching for correspondences from reference to target
    mesh = stereo_mesh_create(*this, progress_callback, 0.2, 1);

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

    snprintf(filename, 1024, "%s%s-remesh.json", debug_basename, suffix);
    stereo_mesh_write_rotated_json(filename, mesh, *this, orientation, debug_texturename);

    if(enable_debug_files) {
        snprintf(filename, 1024, "%s%s-remesh.ply", debug_basename, suffix);
        stereo_mesh_write(filename, mesh, debug_texturename);
        snprintf(filename, 1024, "%s%s-remesh-correspondences.csv", debug_basename, suffix);
        stereo_mesh_write_correspondences(filename, mesh);
    }

    // For some reason, we didn't find any correspondences and didn't make a mesh
    if(mesh.triangles.size() == 0) return false;

    return true;
}

void stereo::write_frames(bool is_distorted)
{
    char buffer[1024];
    if(is_distorted) {
        snprintf(buffer, 1024, "%s-target-distorted.pgm", debug_basename);
        write_image(buffer, target->image, camera.width, camera.height);
        snprintf(buffer, 1024, "%s-reference-distorted.pgm", debug_basename);
        write_image(buffer, reference->image, camera.width, camera.height);
    }
    else {
        snprintf(buffer, 1024, "%s-target.pgm", debug_basename);
        write_image(buffer, target->image, camera.width, camera.height);
        snprintf(buffer, 1024, "%s-reference.pgm", debug_basename);
        write_image(buffer, reference->image, camera.width, camera.height);
    }
}

void stereo::undistort_features(list<stereo_feature> & features)
{
    for(list<stereo_feature>::iterator fiter = features.begin(); fiter != features.end(); ++fiter) {
        stereo_feature f = *fiter;
        feature_t undistorted = camera.undistort_image_point(f.current[0], f.current[1]);
        fiter->current = v3(undistorted.x(), undistorted.y(), 1);
    }
}

void stereo::undistort_frames()
{
    if(!reference || !target) return;

    stereo_frame * reference_undistorted = new stereo_frame(reference->image, camera.width, camera.height, reference->T, reference->W, reference->features);
    camera.undistort_image(reference->image, reference_undistorted->image, reference_undistorted->valid);
    undistort_features(reference_undistorted->features);
    delete reference;
    reference = reference_undistorted;

    stereo_frame * target_undistorted = new stereo_frame(target->image, camera.width, camera.height, target->T, target->W, target->features);
    camera.undistort_image(target->image, target_undistorted->image, target_undistorted->valid);
    undistort_features(target_undistorted->features);
    delete target;
    target = target_undistorted;

}

void m3_file_print(FILE * fp, const char * name, m3 M)
{
    fprintf(fp, "%s = [", name);
    for(int r=0; r<3; r++) {
        for(int c=0; c<3; c++) {
            fprintf(fp, " %g ", M(r, c));
        }
        if(r == 2)
            fprintf(fp, "];\n");
        else
            fprintf(fp, ";\n");
    }

}

void v3_file_print(FILE * fp, const char * name, v3 V)
{
    fprintf(fp, "%s = [%g; %g; %g];\n", name, V[0], V[1], V[2]);
}

void stereo::write_debug_info()
{
    char filename[1024];
    if(used_eight_point)
        snprintf(filename, 1024, "%s-eight-point-debug-info.m", debug_basename);
    else
        snprintf(filename, 1024, "%s-debug-info.m", debug_basename);

    FILE * debug_info = fopen(filename, "w");


    fprintf(debug_info, "width = %d;\n", camera.width);
    fprintf(debug_info, "height = %d;\n", camera.height);

    m3 Rtarget = to_rotation_matrix(target->W);
    m3 Rreference = to_rotation_matrix(reference->W);
    m3_file_print(debug_info, "Rtarget", Rtarget);
    m3_file_print(debug_info, "Rreference", Rreference);
    v3_file_print(debug_info, "Ttarget", target->T);
    v3_file_print(debug_info, "Treference", reference->T);

    m3_file_print(debug_info, "dR", dR);
    v3_file_print(debug_info, "dT", dT);
    fprintf(debug_info, "enable_jitter = %d;\n", enable_jitter);
    fprintf(debug_info, "enable_distortion_correction = %d;\n", enable_distortion_correction);

    fprintf(debug_info, "focal_length = %g;\n", camera.focal_length);
    fprintf(debug_info, "center_x = %g;\n", camera.center_x);
    fprintf(debug_info, "center_y = %g;\n", camera.center_y);
    fprintf(debug_info, "k1 = %g;\n", camera.k1);
    fprintf(debug_info, "k2 = %g;\n", camera.k2);
    fprintf(debug_info, "k3 = %g;\n", camera.k3);
    m3_file_print(debug_info, "F", F);
    m3_file_print(debug_info, "F_motion", F_motion);
    m3_file_print(debug_info, "F_eight_point", F_eight_point);

    fclose(debug_info);
}

stereo_frame::stereo_frame(const uint8_t *_image, int width, int height, const v3 &_T, const rotation_vector &_W, const list<stereo_feature > &_features): T(_T), W(_W), features(_features)
{
    image = new uint8_t[width * height];
    valid = new bool [width * height];
    memcpy(image, _image, width*height);
    memset(valid, 1, sizeof(bool)*width*height);
    // Sort features by id so when we do eight point later, they are already sorted
    features.sort(compare_id);
}

stereo_frame::~stereo_frame() { delete [] image; delete [] valid; }
