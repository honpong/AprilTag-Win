#include "shave_tracker.h"
#include <DrvLeonL2C.h>
#include <algorithm>
#include "commonDefs.hpp"
#include "mv_trace.h"
#include "tracker.h"
#include "descriptor.h"

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
volatile __attribute__((section(".cmx_direct.data"))) ShaveFastDetectSettings cvrt0_detectParams;
volatile __attribute__((section(".cmx_direct.data"))) ShaveFastDetectSettings cvrt1_detectParams;
volatile __attribute__((section(".cmx_direct.data"))) ShaveFastDetectSettings cvrt2_detectParams;
volatile __attribute__((section(".cmx_direct.data"))) ShaveFastDetectSettings cvrt3_detectParams;
//stereo
static u8 __attribute__((section(".ddr.bss"))) p_kp1[sizeof(float3_t)*MAX_KP1+sizeof(int)]; //l_float3
static u8 __attribute__((section(".ddr.bss"))) p_kp2[sizeof(float3_t)*MAX_KP2+sizeof(int)]; //l_float3

volatile __attribute__((section(".cmx_direct.data")))  ShavekpMatchingSettings cvrt_kpMatchingParams;

__attribute__((section(".cmx_direct.data"))) fast_tracker::xy tracked_features[512];

__attribute__((section(".cmx_direct.data"))) uint8_t * patches1[MAX_KP1];
__attribute__((section(".cmx_direct.data"))) uint8_t * patches2[MAX_KP2];
__attribute__((section(".cmx_direct.data"))) float depths1[MAX_KP1];
__attribute__((section(".cmx_direct.data"))) float errors1[MAX_KP1];
// ----------------------------------------------------------------------------
// 4: Static Local Data
//tracker
extern u32 cvrt0_fast9Detect;
extern u32 cvrt1_fast9Detect;
extern u32 cvrt2_fast9Detect;
extern u32 cvrt3_fast9Detect;

extern u32 cvrt0_fast9Track;
extern u32 cvrt1_fast9Track;
extern u32 cvrt2_fast9Track;
extern u32 cvrt3_fast9Track;
//stereo
extern u32 cvrt0_stereo_kp_matching_and_compare;
extern u32 cvrt1_stereo_kp_matching_and_compare;
extern u32 cvrt2_stereo_kp_matching_and_compare;
extern u32 cvrt3_stereo_kp_matching_and_compare;

//tracker
u32 entryPoints[TRACKER_SHAVES_USED] = {
        (u32)&cvrt0_fast9Detect,
        (u32)&cvrt1_fast9Detect,
        (u32)&cvrt2_fast9Detect,
        (u32)&cvrt3_fast9Detect
};

u32 entryPointsTracking[TRACKER_SHAVES_USED] = {
        (u32)&cvrt0_fast9Track,
        (u32)&cvrt1_fast9Track,
        (u32)&cvrt2_fast9Track,
        (u32)&cvrt3_fast9Track
};
//stereo
u32 entryPoints_intersect_and_compare[4] = {
        (u32)&cvrt0_stereo_kp_matching_and_compare,
        (u32)&cvrt1_stereo_kp_matching_and_compare,
        (u32)&cvrt2_stereo_kp_matching_and_compare,
        (u32)&cvrt3_stereo_kp_matching_and_compare
};

typedef struct _short_score {
    _short_score(short _x=0, short _y=0, short _score=0) : x(_x), y(_y), score(_score) {}
    short x,y,score;
} short_score;

typedef volatile ShaveFastDetectSettings *pvShaveFastDetectSettings;
std::vector<short_score> detected_points;

pvShaveFastDetectSettings cvrt_detectParams[TRACKER_SHAVES_USED] = {
        &cvrt0_detectParams,
        &cvrt1_detectParams,
        &cvrt2_detectParams,
        &cvrt3_detectParams
};

volatile ShavekpMatchingSettings *kpMatchingParams =  &cvrt_kpMatchingParams;

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
    shavesToUse = TRACKER_SHAVES_USED;
    for (unsigned int i = 0; i < shavesToUse; ++i){
       shaves[i] = Shave::get_handle(i);
    }
}

std::vector<tracker::feature_track> & shave_tracker::detect(const tracker::image &image, const std::vector<tracker::feature_track *> &features, size_t number_desired)
{
    //init
    if (!mask)
        mask = std::unique_ptr < scaled_mask
                > (new scaled_mask(image.width_px, image.height_px));
    mask->initialize();
    for (auto &f : features)
        mask->clear((int) f->x, (int) f->y);

    feature_points.clear();
    feature_points.reserve(number_desired);

    //run
    detectMultipleShave(image);
    sortFeatures(image, number_desired);

    return feature_points;
}

void shave_tracker::sortFeatures(const tracker::image &image, int number_desired)
{
    unsigned int feature_counter = 0;

    detected_points.clear();
    for (int y = 8; y < image.height_px - 8; ++y) {
        unsigned int numPoints = *(int *) (scores[y]);
        for (unsigned int j = 0; j < numPoints; ++j) {
            u16 x = offsets[y][2 + j] + PADDING;
            u8 score = scores[y][4 + j];

            if (!is_trackable<DESCRIPTOR::border_size>((float) x, (float) y, image.width_px, image.height_px) ||
                    !mask->test(x, y))
                continue;

            detected_points.emplace_back(x, y, score);
            feature_counter++;
        }
    }

    DPRINTF("detected_points: %u\n" , feature_counter);

    //sort
    if (feature_counter > 1) {
        std::sort(detected_points.begin(), detected_points.end(), point_comp_vector);
    }

    DPRINTF("log_threshold_, %d %u\n", m_thresholdController.control(), feature_counter);

#if SKIP_THRESHOLD_UPDATE == 0
    m_thresholdController.update(feature_counter);
#endif
    m_lastDetectedFeatures = int(feature_counter);

    int added = 0;
    for (size_t i = 0; i < feature_counter; ++i) {
        const auto &d = detected_points[i];
        if (mask->test((int) d.x, (int) d.y)) {
            mask->clear((int) d.x, (int) d.y);
            auto id = next_id++;
            feature_points.emplace_back(
                std::make_shared<fast_feature<DESCRIPTOR>>(d.x, d.y, image),
                (float) d.x, (float) d.y, (float) d.score);
            added++;
            if (added == number_desired)
                break;
        }
    }
}

void shave_tracker::detectMultipleShave(const tracker::image &image)
{
    DPRINTF("##shave_tracker## entered detectMultipleShave\n");

    for (int j = 0; j < shavesToUse; j++) {
        cvrt_detectParams[j]->imgWidth = image.width_px;
        cvrt_detectParams[j]->imgHeight = image.height_px;
        cvrt_detectParams[j]->winWidth = image.width_px;
        cvrt_detectParams[j]->winHeight = image.height_px / TRACKER_SHAVES_USED;
        cvrt_detectParams[j]->imgStride = image.stride_px;
        cvrt_detectParams[j]->x = 0;
        cvrt_detectParams[j]->y = j * (image.height_px / TRACKER_SHAVES_USED);
        cvrt_detectParams[j]->threshold = m_thresholdController.control();
        cvrt_detectParams[j]->halfWindow = half_patch_width;
    }

    for (int i = 0; i < shavesToUse; ++i) {
        shaves[i]->start(entryPoints[i], "iii", (u32) image.image, (u32) scores,
                (u32) offsets);
    } DPRINTF("##shave_tracker## waiting for shaves\n");

    for (int i = 0; i < shavesToUse; ++i) {
        shaves[i]->wait();
    }
    DPRINTF("##shave_tracker## features sorted\n");
}

void shave_tracker::trackMultipleShave(std::vector<TrackingData>& trackingData,
        const image& image)
{
    DPRINTF("##shave_tracker## entered trackMultipleShave\n");
    cvrt0_detectParams.imgWidth = image.width_px;
    cvrt0_detectParams.imgHeight = image.height_px;
    cvrt0_detectParams.winWidth = full_patch_width;
    cvrt0_detectParams.winHeight = image.height_px;
    cvrt0_detectParams.imgStride = image.stride_px;
    cvrt0_detectParams.x = 0;
    cvrt0_detectParams.y = 0;
    cvrt0_detectParams.threshold = fast_track_threshold;
    cvrt0_detectParams.halfWindow = half_patch_width;

    cvrt1_detectParams.imgWidth = image.width_px;
    cvrt1_detectParams.imgHeight = image.height_px;
    cvrt1_detectParams.winWidth = full_patch_width;
    cvrt1_detectParams.winHeight = image.height_px;
    cvrt1_detectParams.imgStride = image.stride_px;
    cvrt1_detectParams.x = 0;
    cvrt1_detectParams.y = 0;
    cvrt1_detectParams.threshold = fast_track_threshold;
    cvrt1_detectParams.halfWindow = half_patch_width;

    cvrt2_detectParams.imgWidth = image.width_px;
    cvrt2_detectParams.imgHeight = image.height_px;
    cvrt2_detectParams.winWidth = full_patch_width;
    cvrt2_detectParams.winHeight = image.height_px;
    cvrt2_detectParams.imgStride = image.stride_px;
    cvrt2_detectParams.x = 0;
    cvrt2_detectParams.y = 0;
    cvrt2_detectParams.threshold = fast_track_threshold;
    cvrt2_detectParams.halfWindow = half_patch_width;

    cvrt3_detectParams.imgWidth = image.width_px;
    cvrt3_detectParams.imgHeight = image.height_px;
    cvrt3_detectParams.winWidth = full_patch_width;
    cvrt3_detectParams.winHeight = image.height_px;
    cvrt3_detectParams.imgStride = image.stride_px;
    cvrt3_detectParams.x = 0;
    cvrt3_detectParams.y = 0;
    cvrt3_detectParams.threshold = fast_track_threshold;
    cvrt3_detectParams.halfWindow = half_patch_width;

    int start[4] = { 0, (trackingData.size() / 4), (trackingData.size() / 2),
            (trackingData.size() * 3 / 4) };
    int size[4] = { (trackingData.size() / 4), ((trackingData.size() / 2)
            - (trackingData.size() / 4)), ((trackingData.size() * 3 / 4)
            - (trackingData.size() / 2)), (trackingData.size()
            - (trackingData.size() * 3 / 4)) };

    for (int i = 0; i < shavesToUse; ++i) {
        shaves[i]->start(entryPointsTracking[i], "iiii",
                (u32) &trackingData.data()[start[i]], size[i],
                (u32) image.image, (u32) &tracked_features[start[i]]);
    }

    for (int i = 0; i < shavesToUse; ++i) {
        shaves[i]->wait();
    }

    DPRINTF("##shave_tracker## shave returned\n");
}

void shave_tracker::prepTrackingData(std::vector<TrackingData>& trackingData, std::vector<tracker::feature_track *> &predictions)
{
     for(auto * pred : predictions) {
            TrackingData data;
            fast_tracker::fast_feature<DESCRIPTOR> &f = *static_cast<fast_tracker::fast_feature<DESCRIPTOR>*>(pred->feature.get());
            data.patch = f.descriptor.descriptor.data();
            data.x1 = pred->x + pred->dx;
            data.y1 = pred->y + pred->dy;
            data.x2 = pred->pred_x;
            data.y2 = pred->pred_y;
            data.x3 = pred->x;
            data.y3 = pred->y;
            trackingData.push_back(data);
     }
}

void shave_tracker::processTrackingResult(std::vector<tracker::feature_track *>& predictions)
{
     int i = 0;
     for(auto * pred : predictions) {
                fast_tracker::fast_feature<DESCRIPTOR> &f = *static_cast<fast_tracker::fast_feature<DESCRIPTOR>*>(pred->feature.get());
                xy * bestkp = &tracked_features[i];
                if(bestkp->x != -1) {
                    pred->dx = bestkp->x - pred->x;
                    pred->dy = bestkp->y - pred->y;
                    pred->x = bestkp->x;
                    pred->y = bestkp->y;
                    pred->score = bestkp->score;
                }
                else {
                    pred->dx = 0;
                    pred->dy = 0;
                    pred->score = DESCRIPTOR::min_score;
                }
                i++;
         }
}

void shave_tracker::track(const tracker::image &image, std::vector<tracker::feature_track *> &predictions)
{
    std::vector<TrackingData> trackingData;
    prepTrackingData(trackingData, predictions);
    trackMultipleShave(trackingData, image);
    processTrackingResult(predictions);
}

void shave_tracker::stereo_matching_full_shave(tracker::feature_track * f1_group[], size_t n1, const tracker::feature_track * f2_group[], size_t n2, state_camera & camera1, state_camera & camera2)
{

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
        fast_tracker::fast_feature<DESCRIPTOR> &f = *static_cast<fast_tracker::fast_feature<DESCRIPTOR>*>(f2_group[i]->feature.get());
        patches2[i] = f.descriptor.descriptor.data();

        p_kp2_transformed[i][0] = p2_cal_transformed(0); // todo : Amir : check if we can skip the v3;
        p_kp2_transformed[i][1] = p2_cal_transformed(1);
        p_kp2_transformed[i][2] = p2_cal_transformed(2);
    }
    //prepare p_kp1_transformed
    for(int i = 0; i < n1; i++) {
        fast_tracker::fast_feature<DESCRIPTOR> &f = *static_cast<fast_tracker::fast_feature<DESCRIPTOR>*>(f1_group[i]->feature.get());
        patches1[i] = f.descriptor.descriptor.data();
        depths1[i] = 0;
        errors1[i] = 0;

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
                               {E_R1w_t(2,0), E_R2w_t(2,1),E_R2w_t(2,2),0 },
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

	for (int i = 0; i < STEREO_SHAVES_USED; ++i) {
        shaves[i]->start(entryPoints_intersect_and_compare[i],
                "iiiiiii",
                kpMatchingParams,
                p_kp1,
                p_kp2,
                patches1,
                patches2,
                depths1,
                errors1);
    }

    for (int i = 0; i < STEREO_SHAVES_USED; ++i) {
        shaves[i]->wait();
    }

    for(int i = 0; i < n1; i++) {
        f1_group[i]->depth = depths1[i];
        f1_group[i]->error = errors1[i];
    }
}
