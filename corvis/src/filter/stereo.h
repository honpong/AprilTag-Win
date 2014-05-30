#ifndef __STEREO_H
#define __STEREO_H

#include "model.h"
#include "stereo_mesh.h"

extern "C" {
#include "cor.h"
}

enum stereo_status_code {
    stereo_status_success = 1,
    stereo_status_error_triangulate,
    stereo_status_error_correspondence,
    stereo_status_error_too_few_points,
};

class stereo_feature {
public:
    v4 current;
    uint64_t id;
    stereo_feature(): id(0), current(0.) {}
    stereo_feature(const uint64_t &_id, const v4 &_current): current(_current), id(_id) {}
};

class stereo_frame {
public:
    int frame_number;
    uint8_t *image;
    v4 T;
    rotation_vector W;
    list<stereo_feature> features;
    stereo_frame(const int _frame_number, const uint8_t *_image, int width, int height, const v4 &_T, const rotation_vector &_W, const list<stereo_feature> &_features);
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
    stereo_frame *previous, *current;
    int frame_number;
    stereo_mesh mesh;

    void process_frame(const struct stereo_global &g, const uint8_t *data, list<stereo_feature> &features, bool final);
    bool triangulate(int x, int y, v4 & intersection, float * correspondence_score = NULL);
    bool triangulate_mesh(int x, int y, v4 & intersection);
    /*
     * Returns the baseline traveled between the saved state and the
     * current state in the frame of the camera
     */
    v4 baseline();

    bool preprocess();
    bool preprocess_mesh(void(*progress_callback)(float));

    void set_debug_basename(const char * basename) { snprintf(debug_basename, 1024, "%s", basename); }
    void reset() { if(previous) delete previous; previous = 0; if(current) delete current; current = 0; frame_number = 0; }
    stereo(): previous(0), current(0), frame_number(0) {}
    ~stereo() { if(previous) delete previous; if(current) delete current; }
protected:
    bool should_save_frame(struct filter * f);
    void save_frame(struct filter *f, const uint8_t *frame);
    void update_state(struct filter *f);
    bool triangulate_internal(const stereo_frame & s1, const stereo_frame & s2, int s1_x, int s1_y, int s2_x, int s2_y, v4 & intersection);
    // Computes a fundamental matrix between s2 and s1 and stores it in F. Can return stereo_status_success or stereo_status_error_too_few_points
    enum stereo_status_code preprocess_internal(const stereo_frame &from, const stereo_frame &to, m4 &F);

private:
    char debug_basename[1024];
};

m4 eight_point_F(v4 p1[], v4 p2[], int npts);

#endif