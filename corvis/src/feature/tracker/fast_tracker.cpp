#include "fast_tracker.h"
#include "fast_constants.h"
#include "descriptor.h"

#include "math.h"

using namespace std;

vector<tracker::feature_track> &fast_tracker::detect(const image &image, const std::vector<feature_track *> &current, size_t number_desired)
{
    if (!mask)
        mask = std::make_unique<scaled_mask>(image.width_px, image.height_px);
    mask->initialize();
    for (auto &f : current)
        mask->clear((int)f->x, (int)f->y);

    fast.init(image.width_px, image.height_px, image.stride_px, full_patch_width, half_patch_width);

    feature_points.clear();
    feature_points.reserve(number_desired);
    for(const auto &d : fast.detect(image.image, mask.get(), number_desired, fast_detect_threshold, 0, 0, image.width_px, image.height_px)) {
        if(!is_trackable<DESCRIPTOR::border_size>((int)d.x, (int)d.y, image.width_px, image.height_px) || !mask->test((int)d.x, (int)d.y))
            continue;
        mask->clear((int)d.x, (int)d.y);
        feature_points.emplace_back(make_shared<fast_feature<DESCRIPTOR>>(d.x, d.y, image), d.x, d.y, d.x, d.y, d.score);
        if (feature_points.size() == number_desired)
            break;
    }
    return feature_points;
}

void fast_tracker::track(const image &image, vector<feature_track *> &tracks)
{
    for(auto &tp : tracks) {
        auto &t = *tp;
        fast_feature<DESCRIPTOR> &f = *static_cast<fast_feature<DESCRIPTOR>*>(t.feature.get());

        xy bestkp;
        if(t.found) bestkp = fast.track(f.descriptor, image,
                t.x + t.dx, t.y + t.dy, fast_track_radius,
                fast_track_threshold);

        // Not a good enough match, try the filter prediction
        if(!t.found || DESCRIPTOR::is_better(DESCRIPTOR::good_score, bestkp.score)) {
            xy bestkp2 = fast.track(f.descriptor, image,
                    t.pred_x, t.pred_y, fast_track_radius,
                    fast_track_threshold);
            if(!t.found || DESCRIPTOR::is_better(bestkp2.score, bestkp.score))
                bestkp = bestkp2;
        }

        if(bestkp.x != INFINITY) {
            t.dx = bestkp.x - t.x;
            t.dy = bestkp.y - t.y;
            t.x = bestkp.x;
            t.y = bestkp.y;
            t.score = bestkp.score;
            t.found = true;
        } else {
            t.score = 0;
            t.found = false;
        }
    }
}
