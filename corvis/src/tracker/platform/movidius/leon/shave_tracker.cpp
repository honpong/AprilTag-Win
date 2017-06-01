#include "shave_tracker.h"
#include <DrvLeonL2C.h>
#include <algorithm>
#include <DrvTimer.h>
#include <HglCpr.h>
#include "commonDefs.hpp"
#include "mv_trace.h"
#include <chrono>

extern "C" {
#include "cor_types.h"
}


// LOG threshold values
#define LOG_THRESHOLD 0

// Set to 1 to skip detection threshold update so initial threshold will be used. 
#define SKIP_THRESHOLD_UPDATE 0
#define SHAVE_TRACKER_L2_PARTITION 1

#if 0 // Configure debug prints
#define DPRINTF(...) printf(__VA_ARGS__)
#else
#define DPRINTF(...)
#endif

#define SHAVES_USED  4
#define VECTOR_SIZE 6000

extern u32 cvrt0_fast9Detect;
extern u32 cvrt1_fast9Detect;
extern u32 cvrt2_fast9Detect;
extern u32 cvrt3_fast9Detect;
extern u32 cvrt0_fast9Track;
extern u32 cvrt1_fast9Track;
extern u32 cvrt2_fast9Track;
extern u32 cvrt3_fast9Track;

u32 entryPoints[SHAVES_USED] = {
        (u32)&cvrt0_fast9Detect,
        (u32)&cvrt1_fast9Detect,
        (u32)&cvrt2_fast9Detect,
        (u32)&cvrt3_fast9Detect

};

u32 entryPointsTracking[4] = {
        (u32)&cvrt0_fast9Track,
        (u32)&cvrt1_fast9Track,
        (u32)&cvrt2_fast9Track,
		(u32)&cvrt3_fast9Track
};

static u8 __attribute__((section(".ddr_direct.bss"))) scores[MAX_HEIGHT][MAX_WIDTH + 4];
static u16 __attribute__((section(".ddr_direct.bss"))) offsets[MAX_HEIGHT][MAX_WIDTH + 2];
volatile __attribute__((section(".cmx_direct.data"))) ShaveFastDetectSettings cvrt0_detectParams;
volatile __attribute__((section(".cmx_direct.data"))) ShaveFastDetectSettings cvrt1_detectParams;
volatile __attribute__((section(".cmx_direct.data"))) ShaveFastDetectSettings cvrt2_detectParams;
volatile __attribute__((section(".cmx_direct.data"))) ShaveFastDetectSettings cvrt3_detectParams;

typedef volatile ShaveFastDetectSettings *pvShaveFastDetectSettings;
std::vector< std::vector<short> > detected_points(VECTOR_SIZE , std::vector<short>(3));

pvShaveFastDetectSettings cvrt_detectParams[SHAVES_USED] = {
		&cvrt0_detectParams,
		&cvrt1_detectParams,
		&cvrt2_detectParams,
        &cvrt3_detectParams
};

__attribute__((section(".cmx_direct.data"))) xy tracked_features[512];


bool shave_tracker::s_shavesOpened = false;
osDrvSvuHandler_t shave_tracker::s_handler[12];

static bool point_comp_vector(const std::vector<short> &first, const std::vector<short> &second)
{
    return first[2] > second[2];
}
using namespace std;

shave_tracker::shave_tracker() :
    m_thresholdController(fast_detect_controller_min_features, fast_detect_controller_max_features, fast_detect_threshold,
                         fast_detect_controller_min_threshold, fast_detect_controller_max_threshold,
                         fast_detect_controller_threshold_up_step, fast_detect_controller_threshold_down_step,
                         fast_detect_controller_high_hist, fast_detect_controller_low_hist)

    {

	shavesToUse = SHAVES_USED;
    if (s_shavesOpened == false) {
        s_shavesOpened = true;
        s32 sc = OsDrvSvuInit();
        for(int i = 0; i < shavesToUse; ++i){
            sc = OsDrvSvuOpenShave(&s_handler[i], i, OS_MYR_PROTECTION_SEM);
        }
    }
}

shave_tracker::~shave_tracker() {
}

#ifdef DEBUG_TRACK
	static int directx = 1;
	static int directy = 1;
	static int offsetx = 40;
	static int offsety = 40;
#endif
	std::vector<tracker::point> &shave_tracker::detect(const image &image, const std::vector<point> &features, int number_desired)
{

    START_EVENT(EV_SHAVE_DETECT, 0);
	//init
	if (!mask)
		mask = std::unique_ptr < scaled_mask
				> (new scaled_mask(image.width_px, image.height_px));
	mask->initialize();
	for (auto &f : features)
		mask->clear((int) f.x, (int) f.y);

	feature_points.clear();
	feature_points.reserve(number_desired);

	//run
	detectMultipleShave(image, features, number_desired);
    END_EVENT(EV_SHAVE_DETECT, feature_points.size());

	return feature_points;
}

void shave_tracker::sortFeatures(const image &image, int number_desired) {

	auto start = std::chrono::steady_clock::now();
    int feature_counter = 0;

	for (int y = 8; y < image.height_px - 8; ++y) {
		int numPoints = *(unsigned int *) (scores[y]);
		if (feature_counter + numPoints > (detected_points.size() - 1)){
			detected_points.resize(detected_points.size() * 1.5);
                        DPRINTF("INF: vector resize \n");
		}
		for (int j = 0; j < numPoints; ++j) {
			u16 x = offsets[y][2 + j] + PADDING;
			u8 score = scores[y][4 + j];
#ifdef DEBUG_TRACK
			printf("detect: x %d y %d score %d\n", x, y, score);
#endif
			if (!is_trackable((float) x, (float) y, image.width_px,
					image.height_px) || !mask->test(x, y))
				continue;

			detected_points[feature_counter][0] =   x;
			detected_points[feature_counter][1] =   y;
			detected_points[feature_counter][2] =   score;
			feature_counter++;
		}
	}
	auto stop = std::chrono::steady_clock::now();
	std::chrono::duration<double> diff = stop-start;
	DPRINTF("detected_points: %lf\n" , diff.count());


	//sort
		if(feature_counter > 1){
			std::sort(detected_points.begin(), detected_points.begin()+feature_counter, point_comp_vector);
		}

	DPRINTF("log_threshold_, %d %u\n", m_thresholdController.control(), feature_counter);

#if SKIP_THRESHOLD_UPDATE == 0
    m_thresholdController.update(feature_counter);
#endif
    m_lastDetectedFeatures = int(feature_counter);

    int added = 0;
	for (size_t i = 0; i < feature_counter ; ++i) {
		const auto &d = detected_points[i];
		if(mask->test((int)d[0] , (int)d[1])){
			mask->clear((int) d[0], (int) d[1]);
			auto id = next_id++;
			feature_points.emplace_back(id, (float) d[0], (float) d[1],
					(float) d[2]);
			feature_map.emplace_hint(feature_map.end(), id,
					feature(d[0], d[1], image.image, image.stride_px));
            added++;
            if(added == number_desired) break;
		}
	}
}




void shave_tracker::detectMultipleShave(const image &image, const std::vector<point> &features, int number_desired)
{
    DPRINTF("##shave_tracker## entered detectMultipleShave\n");
	u32 running = 0, sc = 0, sc2;
	u32 mask = 0xf;
	HglCprTurnOnShaveMask(mask);
	for (int j = 0; j < SHAVES_USED; j++)
	{
		cvrt_detectParams[j]->imgWidth = image.width_px;
		cvrt_detectParams[j]->imgHeight = image.height_px;
		cvrt_detectParams[j]->winWidth = image.width_px;
		cvrt_detectParams[j]->winHeight = image.height_px/SHAVES_USED;
		cvrt_detectParams[j]->imgStride = image.stride_px;
		cvrt_detectParams[j]->x = 0;
		cvrt_detectParams[j]->y = j*(image.height_px/SHAVES_USED);
		cvrt_detectParams[j]->threshold = m_thresholdController.control();
		cvrt_detectParams[j]->halfWindow = half_patch_width;
	}
    for(int i = 0; i < SHAVES_USED; ++i)
    {
    	sc |= OsDrvSvuResetShave(&s_handler[i]);
        sc |= OsDrvSvuSetAbsoluteDefaultStack(&s_handler[i]);
        sc |= OsDrvSvuStartShaveCC(&s_handler[i], entryPoints[i],
        			"iii", image.image, scores, offsets);
    }
    DPRINTF("##shave_tracker## waiting for shaves\n");
	sc |= OsDrvSvuWaitShaves(SHAVES_USED, s_handler, OS_DRV_SVU_WAIT_FOREVER, &running);
    /*if ((sc2 = OsDrvShaveL2CachePartitionFlush(SHAVE_TRACKER_L2_PARTITION,
        PERFORM_INVALIDATION)) != OS_MYR_DRV_SUCCESS)
        printf("ERROR: OsDrvShaveL2CachePartitionFlush %lu\n", sc2);*/
	HglCprTurnOffShaveMask(mask);
	if (0 == sc && 0 == running) {
	    DPRINTF("##shave_tracker## shaves returned, sorting features\n");
		sortFeatures(image, number_desired);
	}
	else {
		printf("error running shaves\n");
	}
	DPRINTF("##shave_tracker## features sorted\n");
}



int track_start, track_end;
void shave_tracker::trackShave(std::vector<TrackingData>& trackingData, const image& image)
{
    DPRINTF("##shave_tracker## entered track shave\n");
	cvrt0_detectParams.imgWidth = image.width_px;
	cvrt0_detectParams.imgHeight = image.height_px;
	cvrt0_detectParams.winWidth = full_patch_width;
	cvrt0_detectParams.winHeight = image.height_px;
	cvrt0_detectParams.imgStride = image.stride_px;
	cvrt0_detectParams.x = 0;
	cvrt0_detectParams.y = 0;
	cvrt0_detectParams.threshold = fast_track_threshold;
	cvrt0_detectParams.halfWindow = half_patch_width;

    u32 running, sc = 0, sc2;

    sc |= OsDrvSvuResetShave(&s_handler[0]);
    sc |= OsDrvSvuSetAbsoluteDefaultStack(&s_handler[0]);
    DPRINTF("##shave_tracker## start shave NEW. number of features = %d\n", trackingData.size());
	sc |= OsDrvSvuStartShaveCC(&s_handler[0], (u32) &cvrt0_fast9Track,
			"iiii", trackingData.data(), trackingData.size(), image.image, tracked_features);
	sc |= OsDrvSvuWaitShaves(1, &s_handler[0], OS_DRV_SVU_WAIT_FOREVER, &running);
	if (sc)
	    printf("ERROR trackShave\n");
	DPRINTF("##shave_tracker## shave returned\n");
	/*if ((sc2 = OsDrvShaveL2CachePartitionFlush(SHAVE_TRACKER_L2_PARTITION,
        PERFORM_INVALIDATION)) != OS_MYR_DRV_SUCCESS)
	    printf("ERROR: OsDrvShaveL2CachePartitionFlush %lu\n", sc2);*/
	//Doron: no need to invalidate lrt cache- tracked_features is in cmx_direct
}


void shave_tracker::trackMultipleShave(std::vector<TrackingData>& trackingData, const image& image)
{
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

	u32 running = 0, sc = 0, sc2;

	int start [4] = { 0,
					  (trackingData.size() / 4),
					  (trackingData.size() / 2),
					  (trackingData.size() * 3 / 4)};
	int size [4] = { (trackingData.size() / 4),
					 ((trackingData.size() / 2) - (trackingData.size() / 4)),
					 ((trackingData.size() * 3 / 4) - (trackingData.size() / 2)),
					 (trackingData.size() - (trackingData.size() * 3 / 4))};

	for(int i = 0; i < 4; ++i)
	{
		sc |= OsDrvSvuResetShave(&s_handler[i]);
		sc |= OsDrvSvuSetAbsoluteDefaultStack(&s_handler[i]);
		sc |= OsDrvSvuStartShaveCC(&s_handler[i], entryPointsTracking[i],
				"iiii", (u32)&trackingData.data()[start[i]], size[i], image.image, (u32)&tracked_features[start[i]]);

	}
	sc |= OsDrvSvuWaitShaves(4, &s_handler[0], OS_DRV_SVU_WAIT_FOREVER, &running);
	if (sc)
	    printf("ERROR trackShave\n");
	DPRINTF("##shave_tracker## shave returned\n");
	/*if ((sc2 = OsDrvShaveL2CachePartitionFlush(SHAVE_TRACKER_L2_PARTITION,
        PERFORM_INVALIDATION)) != OS_MYR_DRV_SUCCESS)
	    printf("ERROR: OsDrvShaveL2CachePartitionFlush %lu\n", sc2);*/
	//Doron: no need to invalidate lrt cache- tracked_features is in cmx_direct
}

void shave_tracker::prepTrackingData(std::vector<TrackingData>& trackingData, std::vector<prediction> &predictions)
{
	 for(auto &pred : predictions) {
	        auto f_iter = feature_map.find(pred.id);
	        if(f_iter == feature_map.end()) continue;
	        feature &f = f_iter->second;
	        TrackingData data;
	        data.patch = f.patch;
	        data.x1 = pred.prev_x + f.dx;
	        data.y1 = pred.prev_y + f.dy;
	        data.x2 = pred.x;
	        data.y2 = pred.y;
	        data.x3 = pred.prev_x;
	        data.y3 = pred.prev_y;
	        trackingData.push_back(data);
	 }
}

void shave_tracker::processTrackingResult(std::vector<prediction>& predictions)
{
	 int i = 0;
	 for(auto &pred : predictions) {
		        auto f_iter = feature_map.find(pred.id);
		        if(f_iter == feature_map.end()) continue;
		        feature &f = f_iter->second;
		        xy * bestkp = &tracked_features[i];
		        if(bestkp->x != -1) {
		            f.dx = bestkp->x - pred.prev_x;
		            f.dy = bestkp->y - pred.prev_y;
		            pred.x = bestkp->x;
		            pred.y = bestkp->y;
		            pred.score = bestkp->score;
		            pred.found = true;
#ifdef DEBUG_TRACK
		            printf("prevx %f prevy %f x %f y %f dx %f dy %f score %f id %llu\n" ,pred.prev_x, pred.prev_y, bestkp->x, bestkp->y, f.dx, f.dy, bestkp->score, pred.id);
#endif
		        }
		        i++;
		 }
}

std::vector<tracker::prediction> &shave_tracker::track(const image &image, std::vector<prediction> &predictions)
{
    START_EVENT(EV_SHAVE_TRACK, 0);
#ifdef DEBUG_TRACK
	memset(image.image, 0, image.width_px * image.height_px);
	offsetx += 5*directx;
	offsety += 3*directy;

	if( offsetx >= 640 || offsetx <=0){
		directx = -directx;
		offsetx += 5*directx;
	}
	if( offsety >= 480 || offsety <= 0){
		directy = -directy;
		offsety += 3*directy;
	}
	printf("track offsetx %d offsety %d\n",offsetx, offsety);

	for (int i = 0; i < offsety; ++i)
	{
		memset(image.image + i * image.width_px, 255, offsetx);
	}
#endif
	std::vector<TrackingData> trackingData;
	prepTrackingData(trackingData, predictions);
	u32 mask =0xf;
	HglCprTurnOnShaveMask(mask);
    trackMultipleShave(trackingData, image);
    HglCprTurnOffShaveMask(mask);
	processTrackingResult(predictions);
    END_EVENT(EV_SHAVE_TRACK, predictions.size());
	return predictions;
}

void shave_tracker::drop_feature(uint64_t feature_id)
{
    feature_map.erase(feature_id);
}
