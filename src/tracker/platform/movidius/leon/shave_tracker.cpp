#include "shave_tracker.h"
#include <DrvLeonL2C.h>
#include <algorithm>
#include "image_defines.h"
#include "tracker.h"
#include "descriptor.h"
#include "fast_detect.h"
#include "state_vision.h"

#if 0 // Configure debug prints
#define DPRINTF(...) printf(__VA_ARGS__)
#else
#define DPRINTF(...)
#endif

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
//## Define Memory useage ##
//tracker
static u8 __attribute__((section(".ddr.bss"))) p_kp1[sizeof(float3_t)*MAX_KP1+sizeof(int)]; //l_float3
static u8 __attribute__((section(".ddr.bss"))) p_kp2[sizeof(float3_t)*MAX_KP2+sizeof(int)]; //l_float3

volatile __attribute__((section(".cmx_direct.data")))  ShavekpMatchingSettings stereo_initialize_kpMatchingParams;

__attribute__((section(".cmx_direct.data"))) fast_tracker::xy tracked_features[512];

__attribute__((section(".cmx_direct.data"))) uint8_t* patches1[MAX_KP1];
__attribute__((section(".cmx_direct.data"))) uint8_t* patches2[MAX_KP2];
__attribute__((section(".cmx_direct.data"))) float depths1[MAX_KP1];
__attribute__((section(".cmx_direct.data"))) float depths2[MAX_KP1];
__attribute__((section(".cmx_direct.data"))) float errors1[MAX_KP1];
__attribute__((section(".cmx_direct.data"))) int matched_kp[MAX_KP1];

extern u32 track0_fast_track;
extern u32 track1_fast_track;
extern u32 track2_fast_track;
extern u32 track3_fast_track;
//stereo
extern u32 stereo_initialize0_stereo_match;
extern u32 stereo_initialize1_stereo_match;
extern u32 stereo_initialize2_stereo_match;
extern u32 stereo_initialize3_stereo_match;

struct shave_entry_point { int shave; u32 *entry_point; };

#define TRACK_SHAVES 4
shave_entry_point fast_track[TRACK_SHAVES] = {
    {0, &track0_fast_track},
    {1, &track1_fast_track},
    {2, &track2_fast_track},
    {3, &track3_fast_track},
};

#define STEREO_SHAVES STEREO_SHAVES_USED // FIXME
shave_entry_point stereo_matching[STEREO_SHAVES] = {
    {0, &stereo_initialize0_stereo_match},
    {1, &stereo_initialize1_stereo_match},
    {2, &stereo_initialize2_stereo_match},
    {3, &stereo_initialize3_stereo_match},
};

static volatile ShavekpMatchingSettings *kpMatchingParams =  &stereo_initialize_kpMatchingParams;

using namespace std;

std::vector<tracker::feature_track> & shave_tracker::detect(const tracker::image &image, const std::vector<tracker::feature_position> &current, size_t number_desired)
{
    init_mask(image, current);

    size_t need = std::min(200.f, number_desired * 3.2f);
    size_t num_found = 0;
    fast_tracker::xy *found = platform_fast_detect(id, image, *mask, need, num_found);

    return finalize_detect(found, found + num_found, image, number_desired);
}

void shave_tracker::trackMultipleShave(std::vector<TrackingData>& trackingData,
        const image& image)
{
    struct int2 { int x,y; } image_size;
    for (int i = 0; i < TRACK_SHAVES; ++i)
        Shave::get_handle(fast_track[i].shave)->start(
            (u32)fast_track[i].entry_point, "iiiiivii",
            &trackingData[trackingData.size() * i / TRACK_SHAVES],
            (trackingData.size() * (i+1) / TRACK_SHAVES
            -trackingData.size() *  i    / TRACK_SHAVES),
            &tracked_features[trackingData.size() * i / TRACK_SHAVES],
            image.image,
            image.stride_px,
            &(image_size = (int2) { image.width_px, image.height_px }),
            full_patch_width, half_patch_width
        );

    for (int i = 0; i < TRACK_SHAVES; ++i)
        Shave::get_handle(fast_track[i].shave)->wait();
}

void shave_tracker::prepTrackingData(std::vector<TrackingData>& trackingData, std::vector<tracker::feature_track *> &predictions)
{
     for(auto * pred : predictions) {
            TrackingData data;
            auto f = std::static_pointer_cast<fast_tracker::fast_feature<DESCRIPTOR>>(pred->feature);
            data.patch = f->descriptor.patch.descriptor.data();
            data.x_dx = pred->x == INFINITY ? INFINITY : pred->x + pred->dx;
            data.y_dy = pred->y == INFINITY ? INFINITY : pred->y + pred->dy;
            data.pred_x = pred->pred_x;
            data.pred_y = pred->pred_y;
            trackingData.push_back(data);
     }
}

void shave_tracker::processTrackingResult(std::vector<tracker::feature_track *>&tracks)
{
     int i = 0;
     for(auto &tp : tracks) {
         auto &t = *tp;
         xy &bestkp = tracked_features[i++];
         if(bestkp.x != INFINITY && t.x != INFINITY) {
             t.dx = bestkp.x - t.x;
             t.dy = bestkp.y - t.y;
         } else {
             t.dx = 0;
             t.dy = 0;
         }
         t.x = bestkp.x;
         t.y = bestkp.y;
         t.score = bestkp.score;
     }
}

void shave_tracker::track(const tracker::image &image, std::vector<tracker::feature_track *> &predictions)
{
    std::vector<TrackingData> trackingData;
    prepTrackingData(trackingData, predictions);
    trackMultipleShave(trackingData, image);
    processTrackingResult(predictions);
}

void shave_tracker::stereo_matching_full_shave(struct filter *f, rc_Sensor camera1_id, rc_Sensor camera2_id)
{
    state_camera &camera1 = *f->s.cameras.children[camera1_id];
    state_camera &camera2 = *f->s.cameras.children[camera2_id];
    auto &kp1 = f->s.cameras.children[camera1_id]->standby_tracks;
    auto &kp2 = f->s.cameras.children[camera2_id]->standby_tracks;

    tracker::feature_track * f1_group[MAX_KP1];
    tracker::feature_track * f2_group[MAX_KP2];
    int i = 0;
    for(auto & k1 : kp1)
        f1_group[i++] = &k1;

    i = 0;
    for(auto & k2 : kp2)
        f2_group[i++] = &k2;

    int n1 = kp1.size();
    int n2 = kp2.size();

    feature_t f1_n,f2_n;
    v3 p2_calibrated,p2_cal_transformed,p1_calibrated,p1_cal_transformed;
    v3 o1_transformed = camera1.extrinsics.T.v;
    v3 o2_transformed = camera2.extrinsics.T.v;
    float3_t p_o1_transformed = {o1_transformed[0],o1_transformed[1],o1_transformed[2]};
    float3_t p_o2_transformed = {o2_transformed[0],o2_transformed[1],o2_transformed[2]};
    m3 R1w = camera1.extrinsics.Q.v.toRotationMatrix();
    m3 R2w = camera2.extrinsics.Q.v.toRotationMatrix();

    float3_t* p_kp2_transformed = (float3_t*) (p_kp2+sizeof(int));
    float3_t* p_kp1_transformed = (float3_t*) (p_kp1+sizeof(int));

    //prepare p_kp2_transformed
    for(int i = 0; i < n2; i++) {
        auto * k2 = f2_group[i];
        feature_t f2(k2->x,k2->y);
        f2_n=camera2.intrinsics.undistort_feature(camera2.intrinsics.normalize_feature(f2));
        p2_calibrated << f2_n.x(), f2_n.y(), 1;
        p2_cal_transformed = R2w*p2_calibrated + camera2.extrinsics.T.v;
        auto f = std::static_pointer_cast<fast_tracker::fast_feature<DESCRIPTOR>>(f2_group[i]->feature);
        patches2[i] = f->descriptor.patch.descriptor.data();

        p_kp2_transformed[i][0] = p2_cal_transformed(0); // todo : Amir : check if we can skip the v3;
        p_kp2_transformed[i][1] = p2_cal_transformed(1);
        p_kp2_transformed[i][2] = p2_cal_transformed(2);
    }
    //prepare p_kp1_transformed
    for(int i = 0; i < n1; i++) {
        auto f = std::static_pointer_cast<fast_tracker::fast_feature<DESCRIPTOR>>(f1_group[i]->feature);
        patches1[i] = f->descriptor.patch.descriptor.data();
        depths1[i] = 0;
        errors1[i] = 0;
        matched_kp[i] = -1;

        auto * k1 = f1_group[i];
        feature_t f1(k1->x,k1->y);
        f1_n=camera1.intrinsics.undistort_feature(camera1.intrinsics.normalize_feature(f1));
        p1_calibrated  << f1_n.x(),f1_n.y(),1;
        p1_cal_transformed = R1w*p1_calibrated + camera1.extrinsics.T.v;
        p_kp1_transformed[i][0] = p1_cal_transformed(0);
        p_kp1_transformed[i][1] = p1_cal_transformed(1);
        p_kp1_transformed[i][2] = p1_cal_transformed(2);
    }
    *((int*)p_kp1)=n1;
    *((int*)p_kp2)=n2;

    DPRINTF("\t\t AS:nk1 : %d, nk2: %d \n ",n1,n2);
    //copy to  cvrt; //todo : direcrt copy without local step;
    l_float3_copy   (kpMatchingParams->p_o1_transformed,p_o1_transformed);
    l_float3_copy   (kpMatchingParams->p_o2_transformed,p_o2_transformed);
    kpMatchingParams->EPS=1e-14;
    kpMatchingParams->patch_stride=full_patch_width;
    kpMatchingParams->patch_win_half_width=half_patch_width;
    kpMatchingParams->kp1 = p_kp1;
    kpMatchingParams->kp2 = p_kp2;
    kpMatchingParams->patches1 = patches1;
    kpMatchingParams->patches2 = patches2;
    kpMatchingParams->depth1 = depths1;
    kpMatchingParams->depth2 = depths2;
    kpMatchingParams->errors1 = errors1;
    kpMatchingParams->matched_kp = matched_kp;
    for (int i = 0; i < STEREO_SHAVES; ++i){
        Shave::get_handle(stereo_matching[i].shave)->start(
                (u32)stereo_matching[i].entry_point,
                "i",
                kpMatchingParams);
    }
    for (int i = 0; i < STEREO_SHAVES; ++i){
        Shave::get_handle(stereo_matching[i].shave)->wait();
    }
    i = 0;
    for (auto k1 = kp1.begin(); k1 != kp1.end(); ++k1, ++i) {
        if (matched_kp[i] < 0 || f->s.stereo_matches.count(k1->feature->id)) // didn't match or already stereo
            continue;
        auto k2 = kp2.begin(); std::advance(k2, matched_kp[i]);
        if (k2 == kp2.end() || f->s.stereo_matches.count(k2->feature->id)) // internal error or already stereo
            continue;
        k2->merge(*k1);
        f->s.stereo_matches.emplace(k1->feature->id,
                                    stereo_match(camera1, k1, depths1[i],
                                                 camera2, k2, depths2[i], errors1[i]));
    }
}
