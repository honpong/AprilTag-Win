#include <opencv2/imgproc.hpp>
#include "visual_debug.h"
#include "sensor_data.h"
#include "sensor_fusion.h"

visual_debug& visual_debug::create_instance(const sensor_fusion* parent) {
    static visual_debug instance(parent);
    assert(instance.parent_);
    return instance;
}

visual_debug::visual_debug(const sensor_fusion* parent) : parent_(parent) {}

void visual_debug::batch::clear() {
    queue_.clear();
    ordered_names_.clear();
}

void visual_debug::batch::add(cv::Mat image, const std::string& name) {
    auto it = queue_.lower_bound(name);
    if (it == queue_.end() || it->first != name) {
        it = queue_.emplace_hint(it, name, std::vector<cv::Mat>());
        ordered_names_.emplace_back(name);
    }
    it->second.emplace_back(image);
}

void visual_debug::queue(const batch& b) {
    create_instance().queue_.emplace_back(b);
}

void visual_debug::queue(batch&& b) {
    create_instance().queue_.emplace_back(b);
}

void visual_debug::queue(cv::Mat image, const std::string& name) {
    batch b;
    b.add(image, name);
    create_instance().queue_.emplace_back(std::move(b));
}

void visual_debug::dispatch(bool pause, bool erase_previous_images) {
    auto& instance = create_instance();
    for (size_t i = 0; i < instance.queue_.size(); ++i) {
        bool first_batch = (i == 0);
        bool last_batch = (i + 1 == instance.queue_.size());
        send(instance.queue_[i], pause && last_batch, erase_previous_images && first_batch);
    }
    instance.queue_.clear();
}

void visual_debug::send(batch& b, bool pause, bool erase_previous_images) {
    auto& instance = create_instance();
    if (instance.parent_ && instance.parent_->data_callback) {
        if (b.queue_.empty() && pause) {
            dispatch_one(instance.parent_, cv::Mat(), 0, "", true, true);
        } else {
            rc_Sensor id = 0;
            for (size_t i_group = 0; i_group < b.ordered_names_.size(); ++i_group) {
                const std::string& name = b.ordered_names_[i_group];
                auto& group = b.queue_.at(name);
                for (size_t i = 0; i < group.size(); ++i, ++id) {
                    cv::Mat image = group[i];
                    std::string text = name + " (" + std::to_string(i+1) + "/" +
                            std::to_string(group.size()) + ")";
                    bool first_image = (i_group == 0 && i == 0);
                    bool last_image = (i_group + 1 == b.ordered_names_.size()) && (i + 1 == group.size());
                    dispatch_one(instance.parent_, draw_text(text, image), id, text, pause && last_image, first_image);
                }
            }
        }
    }
}

void visual_debug::send(cv::Mat image, const std::string& name, rc_Sensor sensor_id,
                         bool pause, bool erase_previous_images) {
    auto& instance = create_instance();
    if (instance.parent_ && instance.parent_->data_callback) {
        dispatch_one(instance.parent_, draw_text(name, image), sensor_id, name, pause, erase_previous_images);
    }
}

void visual_debug::dispatch_one(const sensor_fusion* parent, const cv::Mat& image, rc_Sensor image_id,
                                 const std::string& message, bool pause, bool erase_previous_images) {
    sensor_data debug_data;
    cv_to_sensor_data(image, image_id, message, pause, erase_previous_images, debug_data);
    parent->data_callback(&debug_data);
}

cv::Mat& visual_debug::draw_text(const std::string& name, cv::Mat& image) {
    const auto font_face = cv::FONT_HERSHEY_PLAIN;
    const double font_scale = 1.6;
    const int thickness = 1;
    const cv::Scalar color = (image.channels() == 1 ? cv::Scalar(0) : cv::Scalar(0, 255, 0, 255));
    int baseline;
    cv::Size sz = cv::getTextSize(name, font_face, font_scale, thickness, &baseline);
    cv::putText(image, name, cv::Point(10, image.rows - sz.height - 5), font_face, font_scale, color, thickness);
    return image;
}

void visual_debug::cv_to_sensor_data(const cv::Mat& image, rc_Sensor image_id, const std::string& message,
                                      bool pause, bool erase_previous, sensor_data& debug_data) {
    debug_data.type = rc_SENSOR_TYPE_DEBUG;
    debug_data.debug.pause = pause;
    debug_data.debug.erase_previous_debug_images = erase_previous;
    debug_data.debug.message = (message.empty() ? nullptr : message.data());
    debug_data.id = image_id; // we hack the interface to show the # of desired images
    debug_data.debug.image.width = image.cols;
    debug_data.debug.image.height = image.rows;
    debug_data.debug.image.stride = image.step;
    debug_data.debug.image.format = (image.channels() == 1 ? rc_ImageFormat::rc_FORMAT_GRAY8
                                                           : rc_ImageFormat::rc_FORMAT_RGBA8);
    debug_data.debug.image.image = image.data;
}
