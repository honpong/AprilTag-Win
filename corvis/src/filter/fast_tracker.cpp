#include "fast_tracker.h"

#include "scaled_mask.h"

extern "C" {
#include "../cor/cor_types.h"
}

using namespace std;

const int track_threshold = 5;
const int detect_threshold = 15;
const float radius = 5.5f;
const float min_match = 0.2f*0.2f;
const float good_match = 0.65f*0.65f;

bool is_trackable(int x, int y, int width, int height)
{
    return (x > half_patch_width &&
            y > half_patch_width &&
            x < width-1-half_patch_width &&
            y < height-1-half_patch_width);
}

vector<tracker_point> FastTracker::detect(const tracker_image & image, int number_desired)
{
    scaled_mask mask(image.width_px, image.height_px);
    mask.initialize();
    for(auto const & kv : features)
        mask.clear(kv.second.x, kv.second.y);

    fast.init(image.width_px, image.height_px, image.width_px, full_patch_width, half_patch_width);

    vector<tracker_point> detections;
    vector<xy> & fast_detections = fast.detect(image.image, &mask, number_desired, detect_threshold, 0, 0, image.width_px, image.height_px);
    for(int i = 0; i < fast_detections.size(); i++) {
        auto d = fast_detections[i];
        if(!is_trackable(d.x, d.y, image.width_px, image.height_px)) continue;
        tracker_point p;
        p.x = d.x;
        p.y = d.y;
        p.score = d.score;
        p.id = next_id++;
        features.insert(pair<uint64_t, FastFeature>(p.id, 
                    FastFeature(d.x, d.y, image.image, image.width_px)));
        detections.push_back(p);
    }
    return detections;
}

vector<tracker_point> FastTracker::track(const tracker_image & previous_image,
        const tracker_image & current_image,
        const vector<gyro_measurement> & gyro_measurements,
        const vector<tracker_point> & current_features,
        const vector<vector<tracker_point> > & predictions)
{
    vector<tracker_point> tracked_points;

    for(int i = 0; i < current_features.size(); i++) {
        FastFeature & f = features.at(current_features[i].id);

        xy bestkp = fast.track(f.patch, current_image.image,
                half_patch_width, half_patch_width,
                predictions[i][0].x, predictions[i][0].y, radius,
                track_threshold, min_match);

        // Not a good enough match, try the filter prediction
        if(bestkp.score < good_match) {
            xy bestkp2 = fast.track(f.patch, current_image.image,
                    half_patch_width, half_patch_width,
                    predictions[i][1].x, predictions[i][1].y, radius,
                    track_threshold, bestkp.score);
            if(bestkp2.score > bestkp.score)
                bestkp = bestkp2;
        }

        // Still no match? Guess that we haven't moved at all
        if(bestkp.score < min_match) {
            xy bestkp2 = fast.track(f.patch, current_image.image,
                    half_patch_width, half_patch_width,
                    predictions[i][2].x, predictions[i][2].y, 5.5,
                    track_threshold, bestkp.score);
            if(bestkp2.score > bestkp.score)
                bestkp = bestkp2;
        }
        bool valid = bestkp.x != INFINITY;
    
        if(valid) {
            tracker_point p;
            p.x = bestkp.x;
            p.y = bestkp.y;
            p.id = current_features[i].id;
            p.score = bestkp.score;
            f.x = p.x;
            f.y = p.y;
            tracked_points.push_back(p);
        }
    }

    return tracked_points;
}

void FastTracker::drop_features(const vector<uint64_t> feature_ids)
{
    for(auto fid : feature_ids)
        features.erase(fid);
}
