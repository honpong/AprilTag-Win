#ifndef __STEREO_H
#define __STEREO_H

#include <list>
using namespace std;

extern "C" {
#include "cor.h"
}
#include "vec4.h"
#include "rotation_vector.h"

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

class stereo: public stereo_global {
public:
    m4 F;
    stereo_frame *previous, *current;
    int frame_number;

    void process_frame(const struct stereo_global &g, const uint8_t *data, list<stereo_feature> &features, bool final);
    bool triangulate(int x, int y, v4 & intersection);
    v4 baseline();
    /*
     * stereo_should_save_state returns true when the intersection between the features the filter
     * knows about currently and the saved previous state (s) is less than 15 features
     */
    bool preprocess();
    void reset() { if(previous) delete previous; previous = 0; if(current) delete current; current = 0; frame_number = 0; }
    stereo(): previous(0), current(0), frame_number(0) {}
    ~stereo() { if(previous) delete previous; if(current) delete current; }
protected:
    bool should_save_frame(struct filter * f);
    void save_frame(struct filter *f, const uint8_t *frame);
    void update_state(struct filter *f);
    bool triangulate_internal(const stereo_frame & s1, const stereo_frame & s2, int s1_x, int s1_y, int s2_x, int s2_y, v4 & intersection);
    enum stereo_status_code preprocess_internal(const stereo_frame &from, const stereo_frame &to, m4 &F);
};

/* This module is used by:
 * - saving a state once features are acquired and stereo is enabled
 * - using stereo_should_save_state to check if the saved state should be refreshed, stereo_save_state to generate a new saved state
 * - when the filter stops, stereo_save_state is called again with the last seen frame
 * - stereo_preprocess returns a fundamental matrix between the current frame (s2) and a previous frame (s1)
 * - stereo_triangulate is repeatedly called when a user clicks on a new point to measure
 */

/*
 * Returns the baseline traveled between the saved state and the
 * current state in the frame of the camera
 */
//v4 baseline(struct filter * f, const stereo_state & saved_state);

// Computes a fundamental matrix between s2 and s1 and stores it in F. Can return stereo_status_success or stereo_status_error_too_few_points
//enum stereo_status_code stereo_preprocess(const stereo_state & s1, const stereo_state & s2, m4 & F);
// Triangulates a feature in s2 by finding a correspondence in s1 and using the motion estimate to
// establish the baseline. Can return:
// stereo_status_success
// stereo_status_error_triangulate
// stereo_status_error_correspondence
//enum stereo_status_code stereo_triangulate(const stereo_state & s1, const stereo_state & s2, m4 F, int s2_x1, int s2_y1, v4 & intersection);

m4 eight_point_F(v4 p1[], v4 p2[], int npts);

#endif
