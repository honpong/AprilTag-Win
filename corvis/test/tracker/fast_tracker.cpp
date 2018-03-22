#include "gtest/gtest.h"
#include "fast_tracker.h"

void draw_box(uint8_t * image, int width, int height, int stride, int ul_x, int ul_y, int box_width, int box_height)
{
    for(int y = ul_y; y < ul_y + box_height; y++) {
        for(int x = ul_x; x < ul_x + box_width; x++) {
            image[y*stride + x] = 255;
        }
    }
}

std::unique_ptr<uint8_t> black_image(int stride, int height)
{
    std::unique_ptr<uint8_t> image_data(new uint8_t[stride*height]);
    memset(image_data.get(), 0, sizeof(uint8_t)*stride*height);
    return image_data;
}

TEST(FastTracker, DetectBlackImage)
{
    fast_tracker tracker;

    tracker::image image;
    image.width_px = 320;
    image.height_px = 240;
    image.stride_px = image.width_px;
    auto image_data = black_image(image.stride_px, image.height_px);
    image.image = image_data.get();

    std::vector<tracker::feature_position> features;
    std::vector<tracker::feature_track> &detections = tracker.detect(image, features, 10000);
    EXPECT_EQ(detections.size(), 0);
}

TEST(FastTracker, DetectBox)
{
    fast_tracker tracker;

    tracker::image image;
    image.width_px = 320;
    image.height_px = 240;
    image.stride_px = image.width_px;
    auto image_data = black_image(image.stride_px, image.height_px);
    draw_box(image_data.get(), image.width_px, image.height_px, image.stride_px, 100, 100, 20, 20);
    image.image = image_data.get();

    std::vector<tracker::feature_position> features;
    std::vector<tracker::feature_track> &detections = tracker.detect(image, features, 10000);
    EXPECT_EQ(detections.size(), 4);

}

TEST(FastTracker, DetectMasked)
{
    fast_tracker tracker;

    tracker::image image;
    image.width_px = 320;
    image.height_px = 240;
    image.stride_px = image.width_px;
    auto image_data = black_image(image.stride_px, image.height_px);
    draw_box(image_data.get(), image.width_px, image.height_px, image.stride_px, 100, 100, 20, 20);
    image.image = image_data.get();

    // detects at 101 and 118
    std::vector<tracker::feature_position> features;
    features.emplace_back(nullptr, 101,101);
    features.emplace_back(nullptr, 118,101);
    std::vector<tracker::feature_track> &detections = tracker.detect(image, features, 10000);
    EXPECT_EQ(detections.size(), 2);
}

TEST(FastTracker, Track)
{
    fast_tracker tracker;
    int velocity_x_px = 10;
    int velocity_y_px = 10;

    tracker::image image1;
    image1.width_px = 320;
    image1.height_px = 240;
    image1.stride_px = image1.width_px;
    auto image_data1 = black_image(image1.stride_px, image1.height_px);
    draw_box(image_data1.get(), image1.width_px, image1.height_px, image1.stride_px, 100, 100, 20, 20);
    image1.image = image_data1.get();

    tracker::image image2;
    image2.width_px = 320;
    image2.height_px = 240;
    image2.stride_px = image2.width_px;
    auto image_data2 = black_image(image2.stride_px, image2.height_px);
    draw_box(image_data2.get(), image2.width_px, image2.height_px, image2.stride_px, 100 + velocity_x_px, 100 + velocity_y_px, 20, 20);
    image2.image = image_data2.get();

    std::vector<tracker::feature_position> features;
    std::vector<tracker::feature_track> &detections = tracker.detect(image2, features, 10000);

    EXPECT_EQ(detections.size(), 4);

    std::vector<tracker::feature_track *> tracks;
    for(size_t i = 0; i < detections.size(); i++) {
        detections[i].pred_x = detections[i].x + velocity_x_px;
        detections[i].pred_y = detections[i].y + velocity_y_px;
        tracks.push_back(&detections[i]);
    }
    tracker.track(image2, tracks);
    EXPECT_EQ(tracks.size(), 4);
}

TEST(FastTracker, TrackBounds)
{
    fast_tracker tracker;
    int velocity_x_px = 40;
    int velocity_y_px = 40;

    tracker::image image1;
    image1.width_px = 320;
    image1.height_px = 240;
    image1.stride_px = image1.width_px;
    auto image_data1 = black_image(image1.stride_px, image1.height_px);
    draw_box(image_data1.get(), image1.width_px, image1.height_px, image1.stride_px, 100, 100, 20, 20);
    image1.image = image_data1.get();

    tracker::image image2;
    image2.width_px = 320;
    image2.height_px = 240;
    image2.stride_px = image2.width_px;
    auto image_data2 = black_image(image2.stride_px, image2.height_px);
    draw_box(image_data2.get(), image2.width_px, image2.height_px, image2.stride_px, 100 + velocity_x_px, 100 + velocity_y_px, 20, 20);
    image2.image = image_data2.get();

    std::vector<tracker::feature_position> features;
    std::vector<tracker::feature_track> &detections = tracker.detect(image2, features, 10000);

    EXPECT_EQ(detections.size(), 4);

    std::vector<tracker::feature_track *> tracks;
    for(size_t i = 0; i < detections.size(); i++) {
        detections[i].pred_x = detections[i].x + 1000;
        detections[i].pred_y = detections[i].y + 1000;
        detections[i].x = detections[i].x + 1000;
        detections[i].y = detections[i].y + 1000;
        tracks.push_back(&detections[i]);
    }
    tracker.track(image2, tracks);
    EXPECT_EQ(tracks.size(), 4);
    for(size_t i = 0; i < tracks.size(); i++) {
        EXPECT_FALSE(tracks[i]->found());
    }
}


