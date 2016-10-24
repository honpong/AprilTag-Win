#ifndef __STEREO_H
#define __STEREO_H

#include "camera.h"
#include "stereo_mesh.h"
#include <list>
using namespace std;

#include "vec4.h"
#include "rotation_vector.h"

struct stereo_match {
    float x, y;
    float score;
    float depth;
    v3 point;
};

/*
 * Each name corresponds to iOS UIDeviceOrientation,
 * the value is the amount the mesh needs to be rotated to bring it inline with
 * a jpeg saved with this orientation
 * LANDSCAPE_LEFT = home button on the left (landscape)
 * LANDSCAPE_RIGHT = home button on the right (landscape)
 * PORTRAIT = home button on the bottom (portrait)
 * PORTRAIT_UPSIDE_DOWN = home button on the top (portrait)
 */
enum stereo_orientation {
    STEREO_ORIENTATION_LANDSCAPE_LEFT = 0,
    STEREO_ORIENTATION_PORTRAIT = 90,
    STEREO_ORIENTATION_LANDSCAPE_RIGHT = 180,
    STEREO_ORIENTATION_PORTRAIT_UPSIDE_DOWN = 270,
};

class stereo_feature {
public:
    v3 current;
    uint64_t id;
    stereo_feature(): current(v3::Zero()), id(0) {}
    stereo_feature(const uint64_t &_id, const v3 &_current): current(_current), id(_id) {}
};

class stereo_frame {
public:
    uint8_t *image;
    bool *valid;
    v3 T;
    rotation_vector W;
    list<stereo_feature> features;
    stereo_frame(const uint8_t *_image, int width, int height, const v3 &_T, const rotation_vector &_W, const list<stereo_feature> &_features);
    ~stereo_frame();
};

/* How to use this class
*
 * During video processing
 * - process_frame on each frame while video is running
 * When video processing is finished
 * - process_frame with final=true on the last frame
 * - preprocess to calculate the fundamental matrix F between the two saved states
 * - preprocess_mesh to build a delanauy triangulated mesh
 * For each desired point
 * - use either triangulate or triangulate_mesh to determine the world coordinates of the point in the final frame
 */

class stereo {
public:
    class camera camera;
    m3 F;
    m3 F_motion, F_eight_point;
    m3 dR;
    v3 dT;
    stereo_frame *target, *reference;
    stereo_mesh mesh;
    m3 Rw; // set in transform_to_reference
    v3 Tw;
    enum stereo_orientation orientation;

    void process_frame(const class camera &c, const v3 & T, const rotation_vector & W, const uint8_t *data, list<stereo_feature> &features, bool final);
    bool triangulate(int reference_x, int reference_y, v3 & intersection, struct stereo_match * match = NULL) const;
    bool triangulate_top_n(int reference_x, int reference_y, int n, vector<struct stereo_match> & matches) const;
    bool triangulate_mesh(int x, int y, v3 & intersection) const;

    bool preprocess(bool use_eight_point=false, void(*progress_callback)(float)=NULL);
    bool preprocess_mesh(void(*progress_callback)(float));

    void set_debug_basename(const char * basename) { snprintf(debug_basename, 1024, "%s", basename); }
    void set_debug_texture_filename(const char * texture_filename) { snprintf(debug_texturename, 1024, "%s", texture_filename); }
    void transform_to_reference(const stereo_frame * transform_to);
    void set_orientation(enum stereo_orientation current_orientation) { orientation = current_orientation; }

    void reset() { if(target) delete target; target = 0; if(reference) delete reference; reference = 0; }
    stereo(): target(0), reference(0), orientation(STEREO_ORIENTATION_LANDSCAPE_LEFT) {}
    ~stereo() { if(target) delete target; if(reference) delete reference; }
protected:
    bool find_and_triangulate_top_n(int reference_x, int reference_y, int width, int height, int n, vector<struct stereo_match> & matches) const;
    bool triangulate_internal(const stereo_frame & reference, const stereo_frame & target, int reference_x, int reference_y, int target_x, int target_y, v3 & intersection, float & depth, float & error) const;
    void undistort_frames();
    void undistort_features(list<stereo_feature> & features);

    // Computes a fundamental matrix between reference and target and stores it in F.
    bool preprocess_internal(const stereo_frame &reference, stereo_frame & target, m3 &F, bool use_eight_point, void(*progress_callback)(float), float progress_start, float progress_end);
    bool reestimate_F(const stereo_frame & reference, const stereo_frame & target, m3 & F, m3 & R, v3 & T, void(*progress_callback)(float), float progress_start, float progress_end);

private:
    char debug_basename[1024];
    char debug_texturename[1024];
    bool used_eight_point;

    void write_frames(bool is_rectified);
    void write_debug_info();
};

bool line_endpoints(v3 line, int width, int height, float endpoints[4]);

#endif
