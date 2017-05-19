#pragma once

#include "tracker.h"
#include "fast_constants.h"
#include <array>
#include <ippcore.h>
#include <ippcv.h>
#include <ipp.h>
#include <ipptypes.h>

#define check(st) if (st)  { fprintf(stderr, "In %s, %s() returned %d: %s\n", __FUNCTION__, #st, st,ippGetStatusString(st)); throw st; }

class ipp_image_pyramid {
    Ipp8u *pyramid_buffer = nullptr;
    Ipp8u *state_buffer = nullptr;
    public:
    IppiPyramid *pyramid = nullptr;
    static constexpr int levels = 3;

    ~ipp_image_pyramid() { _ipp_image_pyramid(); }
    void _ipp_image_pyramid() {
        if (pyramid)
            for (int i = 0; i < pyramid->level; i++) {
                ippiFree(pyramid->pImage[i]); pyramid->pImage[i] = nullptr;
            }
        pyramid = nullptr;
        ippsFree(pyramid_buffer); pyramid_buffer = nullptr;
        ippsFree(state_buffer); state_buffer = nullptr;
    }
    ipp_image_pyramid &operator=(ipp_image_pyramid&&p) {
        pyramid_buffer =    p.pyramid_buffer; p.pyramid_buffer = nullptr;
        state_buffer =      p.state_buffer;   p.state_buffer   = nullptr;
        pyramid =           p.pyramid;        p.pyramid        = nullptr;
        return *this;
    }
    ipp_image_pyramid(ipp_image_pyramid&&p) {
        *this = std::move(p);
    }
    ipp_image_pyramid &operator=(ipp_image_pyramid&) = delete;
    ipp_image_pyramid(const ipp_image_pyramid&) = delete;
    ipp_image_pyramid() {}
    void ipp_image_pyramid_(int width, int height) {
        IppiSize roi_size = {width, height};
        float rate = 2;
        std::array<Ipp16s, 3> kernel = {1,2,1};

        {
            int pyramid_size, buffer_size;
            check(ippiPyramidGetSize(&pyramid_size, &buffer_size, levels, roi_size, rate));
            pyramid_buffer  = ippsMalloc_8u(pyramid_size);

            Ipp8u *buffer = ippsMalloc_8u(buffer_size);
            IppStatus status1 = ippiPyramidInit(&pyramid, levels, roi_size, rate, pyramid_buffer, buffer);
            ippsFree(buffer);
            check(status1);
        }

        {
            int state_size, buffer_size;
            check(ippiPyramidLayerDownGetSize_8u_C1R(roi_size, rate, kernel.size(), &state_size, &buffer_size));
            Ipp8u *buffer = ippsMalloc_8u(buffer_size);
            state_buffer = ippsMalloc_8u(state_size);
            IppStatus status2 = ippiPyramidLayerDownInit_8u_C1R((IppiPyramidDownState_8u_C1R**)&pyramid->pState, roi_size, rate, kernel.data(), kernel.size(), IPPI_INTER_LINEAR/*IPPI_INTER_NN*/, state_buffer, buffer);
            ippsFree(buffer);
            check(status2);
        }

        for (int i=0; i<pyramid->level; i++)
            pyramid->pImage[i] = ippiMalloc_8u_C1(pyramid->pRoi[i].width, pyramid->pRoi[i].height, &pyramid->pStep[i]);
    }
    void set(const tracker::image &image) {
        if (!pyramid || !(image.width_px == pyramid->pRoi[0].width && image.height_px == pyramid->pRoi[0].height)) {
            this->_ipp_image_pyramid();
            this->ipp_image_pyramid_(image.width_px, image.height_px);
        }
        for (int i = 0; i < image.height_px; i++) // FIXME: we should be copying the shared_ptr here instead of copying the image
            memcpy(pyramid->pImage[0] + i*pyramid->pStep[0], image.image + i*image.stride_px, image.width_px);

        for (int i = 1; i < pyramid->level; i++)
            check(ippiPyramidLayerDown_8u_C1R(pyramid->pImage[i-1], pyramid->pStep[i-1], pyramid->pRoi[i-1],
                                              pyramid->pImage[i  ], pyramid->pStep[i  ], pyramid->pRoi[i  ], (IppiPyramidDownState_8u_C1R*)pyramid->pState));
    }
};
class ipp_fast_detector {
    IppiSize src_size = {0,0}, roi_size = {0,0};
    Ipp8u *corners = nullptr, *scores = nullptr;
    int corners_step = 0, scores_step = 0;
    Ipp8u *work_buffer = nullptr, *spec_buffer = nullptr;
    IppiFastNSpec *spec() { return reinterpret_cast<IppiFastNSpec*>(spec_buffer); }
    std::vector<IppiCornerFastN> corner_vec;
public:
    ~ipp_fast_detector() { _ipp_fast_detector(); }
    void _ipp_fast_detector() {
        ippiFree(corners); corners = nullptr; corners_step = 0;
        ippiFree(scores); scores = nullptr; scores_step = 0;
        ippsFree(spec_buffer); spec_buffer = nullptr;
        ippsFree(work_buffer); work_buffer = nullptr;
        src_size = {0,0}; roi_size = {0,0};
    }
    ipp_fast_detector &operator=(ipp_fast_detector  &d) = delete;
    ipp_fast_detector &operator=(ipp_fast_detector &&d) {
        src_size     = d.src_size;     d.src_size     = {0,0};
        roi_size     = d.roi_size;     d.roi_size     = {0,0};
        corners      = d.corners;      d.corners      = nullptr;
        scores       = d.scores;       d.scores       = nullptr;
        corners_step = d.corners_step; d.corners_step = 0;
        scores_step  = d.scores_step;  d.scores_step  = 0;
        work_buffer  = d.work_buffer;  d.work_buffer  = nullptr;
        spec_buffer  = d.spec_buffer;  d.spec_buffer  = nullptr;

        corner_vec = std::move(d.corner_vec);
        return *this;
    }
    ipp_fast_detector(ipp_fast_detector  &d) = delete;
    ipp_fast_detector(ipp_fast_detector &&d) {
        *this = std::move(d);
    }

    ipp_fast_detector() {}
    void ipp_fast_detector_(int width, int height, int roi_width, int roi_height) {
        src_size = {width, height}; roi_size = {roi_width, roi_height};

        corners = ippiMalloc_8u_C1(width, height, &corners_step);
        scores  = ippiMalloc_8u_C1(width, height, &scores_step);

        int spec_size;
        check(ippiFastNGetSize(src_size, 3/*px radius*/, /*FAST*/9, 8/*orientation bins*/,IPP_FASTN_CIRCLE/*| IPP_FASTN_ORIENTATION*/|IPP_FASTN_SCORE_MODE0|IPP_FASTN_NMS,                                ipp8u, 1/*channels*/, &spec_size));
        spec_buffer = ippsMalloc_8u(spec_size);
        check(   ippiFastNInit(src_size, 3/*px radius*/, /*FAST*/9, 8/*orientation bins*/,IPP_FASTN_CIRCLE/*| IPP_FASTN_ORIENTATION*/|IPP_FASTN_SCORE_MODE0|IPP_FASTN_NMS, fast_detect_threshold /*>=0*/, ipp8u, 1/*channels*/, spec()));
        // FIXME: check() after freeing or using unqiue_ptr
        int work_size = 0;
        check(ippiFastNGetBufferSize(spec(), roi_size, &work_size));
        work_buffer = ippsMalloc_8u(work_size);
    }
    std::vector<IppiCornerFastN> &detect(const tracker::image &image, IppiPoint offset, IppiSize roi_size_, const scaled_mask &mask) {
        if (!(image.width_px == src_size.width && image.height_px == src_size.height &&
              roi_size_.width == roi_size.width && roi_size_.height == roi_size.height)) { // need to resize
            this->_ipp_fast_detector();
            this->ipp_fast_detector_(image.width_px, image.height_px, roi_size_.width, roi_size_.height);
        }
        int num_corners = 0;
        check(ippiFastN_8u_C1R(image.image, image.stride_px,
                               corners, corners_step,
                               scores, scores_step,
                               &num_corners,
                               offset, roi_size,
                               spec(), work_buffer));

        // FIXME: use correct size of scores[] and corners[] which is [offset, roi_size)
        for (int y=0; y<mask.scaled_height; y++)
            for (int x=0; x<mask.scaled_width; x++)
                if (mask.mask[y * mask.scaled_width + x] == 0)
                    std::memset(&corners[y*corners_step + x], 0, sizeof(*corners)*(1<<mask.mask_shift));

        corner_vec.resize(num_corners);
        if (num_corners) // When num_corners == 0, corner_vec.data() == NULL and IPP doesn't like that.
            check(ippiFastN2DToVec_8u(corners, corners_step,
                                      scores, scores_step,
                                      corner_vec.data(), roi_size, corner_vec.size(), &num_corners, spec()));
        return corner_vec;
    }
};

class ipp_tracker : public tracker
{
    static constexpr int max_iterations = 100;
    static constexpr float threshold = .1f;
    static constexpr int win_size = static_cast<int>(2*fast_track_radius + .5);
private:
    Ipp8u *state_buffer = nullptr;
    IppiOptFlowPyrLK_8u_C1R *state = nullptr;
    ipp_fast_detector fast;
    struct state {
        ipp_image_pyramid pyramid;
        std::vector<IppiPoint_32f> points;
    } prev, next;
    std::vector<Ipp8s> statuses;
    std::vector<Ipp32f> errors;
public:
    ~ipp_tracker() {
        ippsFree(state_buffer); state_buffer = nullptr;
    };
    ipp_tracker() {
    };

    virtual std::vector<prediction> &track(const image &image, std::vector<prediction> &predictions) override {
        if(!state_buffer) {
            IppiSize roi_size {image.width_px, image.height_px};
            {
                int state_size = 0;
                check(ippiOpticalFlowPyrLKGetSize(win_size, roi_size, ipp8u, ippAlgHintFast, &state_size));
                state_buffer = ippsMalloc_8u(state_size);
            }
            check(ippiOpticalFlowPyrLKInit_8u_C1R(&state, roi_size, win_size, ippAlgHintFast, state_buffer));
        }
        prev.points.resize(predictions.size());
        next.points.resize(predictions.size());
        statuses.resize(predictions.size());
        errors.resize(predictions.size());
        if (prev.pyramid.pyramid) {
            next.pyramid.set(image);
            for (size_t i = 0; i < predictions.size(); i++) {
                prev.points[i].x = predictions[i].prev_x;
                prev.points[i].y = predictions[i].prev_y;
                next.points[i].x = predictions[i].x;
                next.points[i].y = predictions[i].y;
            }
            check(ippiOpticalFlowPyrLK_8u_C1R(prev.pyramid.pyramid, next.pyramid.pyramid,
                                              prev.points.data(), next.points.data(), statuses.data(), errors.data(), predictions.size(),
                                              win_size/*<= win_size above*/, ipp_image_pyramid::levels-1, max_iterations, threshold, state));
        }
        size_t found=0;
        for (size_t i = 0; i < predictions.size(); i++) {
            if (statuses[i] < ipp_image_pyramid::levels) {
                predictions[i].x = next.points[i].x;
                predictions[i].y = next.points[i].y;
                predictions[i].score = errors[i];
                if(errors[i] < 10) {
                    predictions[i].found = true; found++;
                }
            }
        }
        std::swap(prev, next);
        return predictions;
    }

    virtual std::vector<point> &detect(const image &image, const std::vector<point> &current, int number_desired) override {
        if (!mask)
            mask = std::make_unique<scaled_mask>(image.width_px, image.height_px);
        mask->initialize();
        for (auto &f : current)
            mask->clear((int)f.x, (int)f.y);

        prev.pyramid.set(image);

        IppiSize roi_size = {image.width_px, image.height_px};
        std::vector<IppiCornerFastN> &corner_vec = fast.detect(image, IppiPoint{0,0}, roi_size, *mask);

        auto by_score = [](const point &a, const point &b){ return a.score > b.score; };

        feature_points.clear();
        feature_points.reserve(number_desired+1/*see below*/);
        for (auto &c : corner_vec) {
            if (!mask->test((int)c.x, (int)c.y))
                continue;
            mask->clear((int)c.x, (int)c.y);

            feature_points.emplace_back(std::make_shared<tracker::feature>(), c.x, c.y, c.score);
            push_heap(feature_points.begin(), feature_points.end(), by_score);
            if(feature_points.size() > number_desired) {
                pop_heap(feature_points.begin(), feature_points.end(), by_score);
                feature_points.pop_back();
            }
        }
        std::sort_heap(feature_points.begin(), feature_points.end(), by_score);
        return feature_points;
    }
};

#ifndef check
#error check should be defined or this should be removed
#endif
#undef check
