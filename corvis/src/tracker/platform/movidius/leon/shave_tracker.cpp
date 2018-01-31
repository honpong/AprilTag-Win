#include "shave_tracker.h"
#include <DrvLeonL2C.h>
#include <algorithm>
#include "commonDefs.hpp"
#include "tracker.h"
#include "descriptor.h"
#include "state_vision.h"
extern "C" {
#include "cor_types.h"
}


// LOG threshold values
#define LOG_THRESHOLD 0

// Set to 1 to skip detection threshold update so initial threshold will be used. 
#define SKIP_THRESHOLD_UPDATE 0

#if 0 // Configure debug prints
#define DPRINTF(...) printf(__VA_ARGS__)
#else
#define DPRINTF(...)
#endif

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
//## Define Memory useage ##
//tracker
static u8 __attribute__((section(".ddr_direct.bss"))) scores[MAX_HEIGHT][MAX_WIDTH + 4];
static u16 __attribute__((section(".ddr_direct.bss"))) offsets[MAX_HEIGHT][MAX_WIDTH + 2];
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
// ----------------------------------------------------------------------------
// 4: Static Local Data
//tracker
extern u32 cvrt4_fast_detect;
extern u32 cvrt8_fast_detect;
extern u32 cvrt9_fast_detect;
extern u32 cvrt10_fast_detect;

extern u32 cvrt0_fast_track;
extern u32 cvrt1_fast_track;
extern u32 cvrt2_fast_track;
extern u32 cvrt3_fast_track;
//stereo
extern u32 stereo_initialize0_stereo_match;
extern u32 stereo_initialize1_stereo_match;
extern u32 stereo_initialize2_stereo_match;
extern u32 stereo_initialize3_stereo_match;

struct shave_entry_point { int shave; u32 *entry_point; };

#define DETECT_SHAVES 4
shave_entry_point fast_detect[DETECT_SHAVES] = {
    {4,  &cvrt4_fast_detect},
    {8,  &cvrt8_fast_detect},
    {9,  &cvrt9_fast_detect},
    {10, &cvrt10_fast_detect},
};

#define TRACK_SHAVES 4
shave_entry_point fast_track[TRACK_SHAVES] = {
    {0, &cvrt0_fast_track},
    {1, &cvrt1_fast_track},
    {2, &cvrt2_fast_track},
    {3, &cvrt3_fast_track},
};

#define STEREO_SHAVES STEREO_SHAVES_USED // FIXME
shave_entry_point stereo_matching[STEREO_SHAVES] = {
    {0, &stereo_initialize0_stereo_match},
    {1, &stereo_initialize1_stereo_match},
    {2, &stereo_initialize2_stereo_match},
    {3, &stereo_initialize3_stereo_match},
};

typedef struct _short_score {
    _short_score(short _x=0, short _y=0, short _score=0) : x(_x), y(_y), score(_score) {}
    short x,y,score;
} short_score;

std::vector<short_score> detected_points;

static volatile ShavekpMatchingSettings *kpMatchingParams =  &stereo_initialize_kpMatchingParams;

// ----------------------------------------------------------------------------
// 5: Static Function Prototypes

static bool point_comp_vector(const short_score &first, const short_score &second)
{
    return first.score > second.score;
}
using namespace std;

shave_tracker::shave_tracker() :
    m_thresholdController(fast_detect_controller_min_features, fast_detect_controller_max_features, fast_detect_threshold,
                         fast_detect_controller_min_threshold, fast_detect_controller_max_threshold,
                         fast_detect_controller_threshold_up_step, fast_detect_controller_threshold_down_step,
                         fast_detect_controller_high_hist, fast_detect_controller_low_hist)
{
}

std::vector<tracker::feature_track> & shave_tracker::detect(const tracker::image &image, const std::vector<tracker::feature_track *> &features, size_t number_desired)
{
    feature_points.clear();
    feature_points.reserve(number_desired);
    if (number_desired > 0) {
        //init
        if (!mask)
            mask = std::unique_ptr <scaled_mask>(new scaled_mask(image.width_px, image.height_px));
        mask->initialize();
        for (auto &f : features)
            mask->clear((int)f->x, (int)f->y);
        //run
        detectMultipleShave(image);
        sortFeatures(image, number_desired);
    }
    return feature_points;
}

void shave_tracker::sortFeatures(const tracker::image &image, int number_desired)
{
    detected_points.clear();
    for (int y = 8; y < image.height_px - 8; ++y) {
        unsigned int numPoints = *(int *) (scores[y]);
        for (unsigned int j = 0; j < numPoints; ++j) {
            u16 x = offsets[y][2 + j] + PADDING;
            u8 score = scores[y][4 + j];
            if (is_trackable<DESCRIPTOR::border_size>((float) x, (float) y, image.width_px, image.height_px)
                && mask->test(x, y))
                detected_points.emplace_back(x, y, score);
        }
    }

    std::partial_sort(detected_points.begin(),
                      detected_points.begin() + std::min<size_t>(detected_points.size(), 8 * number_desired),
                      detected_points.end(), point_comp_vector);

#if SKIP_THRESHOLD_UPDATE == 0
    m_thresholdController.update(detected_points.size());
#endif
    m_lastDetectedFeatures = int(detected_points.size());

    for (const auto &d : detected_points) {
        if (mask->test((int) d.x, (int) d.y)) {
            mask->clear((int) d.x, (int) d.y);
            if (feature_points.size() < number_desired)
                feature_points.emplace_back(
                    std::make_shared<fast_feature<DESCRIPTOR>>(d.x, d.y, image),
                    (float) d.x, (float) d.y, (float) d.score);
            else
                break;
        }
    }
}

void shave_tracker::detectMultipleShave(const tracker::image &image)
{
    struct int2 { int x,y; } xy, image_size, win_size;
    for (int i=0; i< DETECT_SHAVES; ++i)
        Shave::get_handle(fast_detect[i].shave)->start(
            (u32)fast_detect[i].entry_point, "iiiviivv",
            scores,
            offsets,
            m_thresholdController.control(),
            &(xy = (int2) { 0, i * image.height_px / DETECT_SHAVES }),
            image.image,
            image.stride_px,
            &(image_size = (int2) { image.width_px, image.height_px }),
            &(win_size = (int2) { image.width_px, (i+1) * image.height_px / DETECT_SHAVES
                                                 - i    * image.height_px / DETECT_SHAVES })
        );

    for (int i = 0; i < DETECT_SHAVES; ++i)
        Shave::get_handle(fast_detect[i].shave)->wait();
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
    std::list<tracker::feature_track> &kp1 = f->s.cameras.children[camera1_id]->standby_tracks;
    std::list<tracker::feature_track> &kp2 = f->s.cameras.children[camera2_id]->standby_tracks;

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
    R1w = camera1.extrinsics.Q.v.toRotationMatrix();
    R2w = camera2.extrinsics.Q.v.toRotationMatrix();

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
    //Eigen types
    m3 E_R1w_t=R1w.transpose();
    m3 E_R2w_t=R2w.transpose();
    v3 E_camera1_extrinsics_T_v = camera1.extrinsics.T.v;
    v3 E_camera2_extrinsics_T_v = camera2.extrinsics.T.v;
    //local array vector type
    float4x4_t R1w_transpose { {E_R1w_t(0,0), E_R1w_t(0,1),E_R1w_t(0,2),0 },
                               {E_R1w_t(1,0), E_R1w_t(1,1),E_R1w_t(1,2),0 },
                               {E_R1w_t(2,0), E_R1w_t(2,1),E_R1w_t(2,2),0 },
                               { 0, 0, 0, 0 } };

    float4x4_t R2w_transpose {{ E_R2w_t(0,0), E_R2w_t(0,1),E_R2w_t(0,2),0 },
                               {E_R2w_t(1,0), E_R2w_t(1,1),E_R2w_t(1,2),0 },
                               {E_R2w_t(2,0), E_R2w_t(2,1),E_R2w_t(2,2),0 },
                               { 0, 0, 0, 0 }};
    float4_t camera1_extrinsics_T_v = { E_camera1_extrinsics_T_v[0],E_camera1_extrinsics_T_v[1],E_camera1_extrinsics_T_v[2] };
    float4_t camera2_extrinsics_T_v = { E_camera2_extrinsics_T_v[0],E_camera2_extrinsics_T_v[1],E_camera2_extrinsics_T_v[2] };

    //copy to  cvrt; //todo : direcrt copy without local step;
	l_float4x4_copy (kpMatchingParams->R1w_transpose,R1w_transpose );
	l_float4x4_copy (kpMatchingParams->R2w_transpose,R2w_transpose );
	l_float4_copy   (kpMatchingParams->camera1_extrinsics_T_v,camera1_extrinsics_T_v );
	l_float4_copy   (kpMatchingParams->camera2_extrinsics_T_v,camera2_extrinsics_T_v );
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
    for(auto k1 = kp1.begin(); k1 != kp1.end() && k1->feature.use_count() <= 1; ++k1, ++i) {
        if(matched_kp[i] >= 0){
            auto k2 = kp2.begin();
            for(int j = 0; k2 != kp2.end(), j < matched_kp[i]; ++k2, ++j);
            if (k2 != kp2.end() && k2->feature.use_count() <= 1) {//not already stereo
                if (f->map)
                    f->map->triangulated_tracks.erase(k2->feature->id); // FIXME: check if triangulated_tracks is more accurate than stereo match
                k2->feature = k1->feature;
                f->s.stereo_matches.emplace_back(camera1, k1, depths1[i], camera2, k2, depths2[i],  errors1[i]);
            }
        }
    }

}
