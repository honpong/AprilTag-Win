#include "fast_tracker.h"
#include "fast_constants.h"

extern "C" {
#include "../cor/cor_types.h"
}

using namespace std;

vector<tracker::point> &fast_tracker::detect(const image &image, const std::vector<point> &features, int number_desired)
{
    if (!mask)
        mask = std::make_unique<scaled_mask>(image.width_px, image.height_px);
    mask->initialize();
    for (auto &f : features)
        mask->clear((int)f.x, (int)f.y);

    fast.init(image.width_px, image.height_px, image.stride_px, full_patch_width, half_patch_width);

    feature_points.clear();
    feature_points.reserve(number_desired);
    for(const auto &d : fast.detect(image.image, mask.get(), number_desired, fast_detect_threshold, 0, 0, image.width_px, image.height_px)) {
        if(!is_trackable(d.x, d.y, image.width_px, image.height_px) || !mask->test((int)d.x, (int)d.y))
            continue;
        mask->clear((int)d.x, (int)d.y);
        auto id = next_id++;
        feature_map.emplace_hint(feature_map.end(), id, feature(d.x, d.y, image.image, image.stride_px));
        feature_points.emplace_back(id, d.x, d.y, d.score);
        if (feature_points.size() == number_desired)
            break;
    }
    return feature_points;
}

vector<tracker::prediction> &fast_tracker::track(const image &image, vector<prediction> &predictions)
{
    for(auto &pred : predictions) {
        auto f_iter = feature_map.find(pred.id);
        if(f_iter == feature_map.end()) continue;
        feature &f = f_iter->second;

        xy bestkp = fast.track(f.patch, image.image,
                half_patch_width, half_patch_width,
                pred.prev_x + f.dx, pred.prev_y + f.dy, fast_track_radius,
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
                    pred.prev_x, pred.prev_y, fast_track_radius,
                    fast_track_threshold, bestkp.score);
            if(bestkp2.score > bestkp.score)
                bestkp = bestkp2;
        }

        if(bestkp.x != INFINITY) {
            f.dx = bestkp.x - pred.prev_x;
            f.dy = bestkp.y - pred.prev_y;
            pred.x = bestkp.x;
            pred.y = bestkp.y;
            pred.score = bestkp.score;
            pred.found = true;
        }
    }

    return predictions;
}

void fast_tracker::drop_feature(uint64_t feature_id)
{
    feature_map.erase(feature_id);
}
