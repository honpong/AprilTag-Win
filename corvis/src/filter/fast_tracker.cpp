#include "fast_tracker.h"

#include "scaled_mask.h"

extern "C" {
#include "../cor/cor_types.h"
}

using namespace std;

static constexpr int track_threshold = 5;
static constexpr int detect_threshold = 15;
static constexpr float radius = 5.5f;
static constexpr float min_match = 0.2f*0.2f;
static constexpr float good_match = 0.65f*0.65f;

vector<tracker::point> &fast_tracker::detect(const image & image, int number_desired)
{
    scaled_mask mask(image.width_px, image.height_px);
    mask.initialize();
    for(auto const & kv : feature_map)
        mask.clear(kv.second.x, kv.second.y);

    fast.init(image.width_px, image.height_px, image.stride_px, full_patch_width, half_patch_width);

    feature_points.clear();
    vector<xy> & fast_detections = fast.detect(image.image, &mask, number_desired, detect_threshold, 0, 0, image.width_px, image.height_px);
    for(int i = 0; i < fast_detections.size(); i++) {
        auto d = fast_detections[i];
        if(!is_trackable(d.x, d.y, image.width_px, image.height_px)) continue;
        point p;
        p.x = d.x;
        p.y = d.y;
        p.score = d.score;
        p.id = next_id++;
        feature_map.emplace_hint(feature_map.end(), p.id, feature(d.x, d.y, image.image, image.width_px));
        feature_points.push_back(p);
    }
    return feature_points;
}

vector<tracker::point> &fast_tracker::track(const image &current_image,
                                            const vector<point> &current_features,
                                            const vector<vector<point>> &predictions)
{
    feature_points.clear();

    for(int i = 0; i < current_features.size(); i++) {
        feature &f = feature_map.at(current_features[i].id);

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
            point p;
            p.x = bestkp.x;
            p.y = bestkp.y;
            p.id = current_features[i].id;
            p.score = bestkp.score;
            f.x = p.x;
            f.y = p.y;
            feature_points.push_back(p);
        }
    }

    return feature_points;
}

void fast_tracker::drop_features(const vector<uint64_t> &feature_ids)
{
    for(auto fid : feature_ids)
        feature_map.erase(fid);
}
