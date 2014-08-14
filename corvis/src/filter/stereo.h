#ifndef __STEREO_H
#define __STEREO_H

#include "stereo_mesh.h"
#include <list>
using namespace std;

#include "../numerics/vec4.h"
#include "../numerics/rotation_vector.h"

struct stereo_match {
    float x, y;
    float score;
    float depth;
    v4 point;
};

/*
 * Each name corresponds to iOS UIImageOrientation,
 * the value is the amount the mesh needs to be rotated to bring it inline with
 * a jpeg saved with this orientation
 */
enum stereo_orientation {
    STEREO_ORIENTATION_RIGHT = 0,
    STEREO_ORIENTATION_UP = 90,
    STEREO_ORIENTATION_LEFT = 180,
    STEREO_ORIENTATION_DOWN = 270,
};

class stereo_feature {
public:
    v4 current;
    uint64_t id;
    stereo_feature(): current(0.), id(0) {}
    stereo_feature(const uint64_t &_id, const v4 &_current): current(_current), id(_id) {}
};

class stereo_frame {
public:
    uint8_t *image;
    bool *valid;
    v4 T;
    rotation_vector W;
    list<stereo_feature> features;
    stereo_frame(const uint8_t *_image, int width, int height, const v4 &_T, const rotation_vector &_W, const list<stereo_feature> &_features);
    ~stereo_frame();
};

struct stereo_global {
    int width;
    int height;
    v4 T;
    rotation_vector W;
    f_t focal_length;
    f_t center_x;
    f_t center_y;
    f_t k1, k2, k3;
};

/* How to use this class
*
 * During video processing
 * - process_frame on each frame while video is running
 * - baseline to determine when there is enough distance between the saved state and the current state
 * When video processing is finished
 * - process_frame with final=true on the last frame
 * - preprocess to calculate the fundamental matrix F between the two saved states
 * - preprocess_mesh to build a delanauy triangulated mesh
 * For each desired point
 * - use either triangulate or triangulate_mesh to determine the world coordinates of the point in the final frame
 */

class stereo: public stereo_global {
public:
    m4 F;
    m4 F_motion;
    m4 dR;
    v4 dT;
    stereo_frame *target, *reference;
    stereo_mesh mesh;
    enum stereo_orientation orientation;

    void process_frame(const struct stereo_global &g, const uint8_t *data, list<stereo_feature> &features, bool final);
    bool triangulate(int reference_x, int reference_y, v4 & intersection, struct stereo_match * match = NULL) const;
    bool triangulate_top_n(int reference_x, int reference_y, int n, vector<struct stereo_match> & matches) const;
    bool triangulate_mesh(int x, int y, v4 & intersection) const;
    /*
     * Returns the baseline traveled between the saved state and the
     * reference state in the frame of the camera
     */
    v4 baseline();

    bool preprocess(bool use_eight_point=false);
    bool preprocess_mesh(void(*progress_callback)(float));

    void set_debug_basename(const char * basename) { snprintf(debug_basename, 1024, "%s", basename); }
    void set_debug_texture_filename(const char * texture_filename) { snprintf(debug_texturename, 1024, "%s", texture_filename); }
    void transform_to_reference(const stereo_frame * transform_to);
    void set_orientation(enum stereo_orientation current_orientation) { orientation = current_orientation; }

    void reset() { if(target) delete target; target = 0; if(reference) delete reference; reference = 0; }
    stereo(): target(0), reference(0), correspondences(0), orientation(STEREO_ORIENTATION_RIGHT) {}
    ~stereo() { if(target) delete target; if(reference) delete reference; }
protected:
    bool should_save_frame(struct filter * f);
    void save_frame(struct filter *f, const uint8_t *frame);
    void update_state(struct filter *f);
    bool find_and_triangulate_top_n(int reference_x, int reference_y, int width, int height, int n, vector<struct stereo_match> & matches) const;
    bool triangulate_internal(const stereo_frame & reference, const stereo_frame & target, int reference_x, int reference_y, int target_x, int target_y, v4 & intersection, float & depth, float & error) const;
    void rectify_frames();

    // Computes a fundamental matrix between reference and target and stores it in F.
    bool preprocess_internal(const stereo_frame & reference, const stereo_frame & target, m4 &F, bool use_eight_point);

private:
    char debug_basename[1024];
    char debug_texturename[1024];
    bool used_eight_point;
    FILE * correspondences;

    void write_frames(bool is_rectified);
    void write_debug_info();
};

m4 eight_point_F(v4 p1[], v4 p2[], int npts);
bool line_endpoints(v4 line, int width, int height, float endpoints[4]);


#endif
