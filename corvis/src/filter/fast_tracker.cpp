#include "fast_tracker.h"
#include "fast_constants.h"

#include "scaled_mask.h"

extern "C" {
#include "../cor/cor_types.h"
}

using namespace std;

vector<tracker::point> &fast_tracker::detect(const image &image, int number_desired)
{
    scaled_mask mask(image.width_px, image.height_px);
    mask.initialize();
    for(auto const & kv : feature_map)
        mask.clear(kv.second.x, kv.second.y);

    fast.init(image.width_px, image.height_px, image.stride_px, full_patch_width, half_patch_width);

    feature_points.clear();
    vector<xy> & fast_detections = fast.detect(image.image, &mask, number_desired, fast_detect_threshold, 0, 0, image.width_px, image.height_px);
    for(int i = 0; i < fast_detections.size(); i++) {
        auto d = fast_detections[i];
        if(!is_trackable(d.x, d.y, image.width_px, image.height_px)) continue;
        auto id = next_id++;
        feature_map.emplace_hint(feature_map.end(), id, feature(d.x, d.y, image.image, image.stride_px));
        feature_points.emplace_back(id, d.x, d.y, d.score);
    }
    return feature_points;
}

vector<tracker::point> &fast_tracker::track(const image &image, const vector<point> &predictions)
{
    feature_points.clear();

    for(auto &pred : predictions) {
        feature &f = feature_map.at(pred.id);

        xy bestkp = fast.track(f.patch, image.image,
                half_patch_width, half_patch_width,
                f.x + f.dx, f.y + f.dy, fast_track_radius,
                fast_track_threshold, fast_min_match);

        // Not a good enough match, try the filter prediction
        if(bestkp.score < fast_good_match) {
            xy bestkp2 = fast.track(f.patch, image.image,
                    half_patch_width, half_patch_width,
                    pred.x, pred.y, fast_track_radius,
                    fast_track_threshold, bestkp.score);
            if(bestkp2.score > bestkp.score)
                bestkp = bestkp2;
        }

        // Still no match? Guess that we haven't moved at all
        if(bestkp.score < fast_min_match) {
            xy bestkp2 = fast.track(f.patch, image.image,
                    half_patch_width, half_patch_width,
                    f.x, f.y, 5.5,
                    fast_track_threshold, bestkp.score);
            if(bestkp2.score > bestkp.score)
                bestkp = bestkp2;
        }
        bool valid = bestkp.x != INFINITY;

        if(valid) {
            feature_points.emplace_back(pred.id, bestkp.x,bestkp.y, bestkp.score);
            f.dx = bestkp.x - f.x;
            f.dy = bestkp.y - f.y;
            f.x = bestkp.x;
            f.y = bestkp.y;
        }
    }

    return feature_points;
}

void fast_tracker::drop_features(const vector<uint64_t> &feature_ids)
{
    for(auto fid : feature_ids)
        feature_map.erase(fid);
}
