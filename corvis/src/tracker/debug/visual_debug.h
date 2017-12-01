#ifndef __DEBUG_MANAGER_H
#define __DEBUG_MANAGER_H

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <opencv2/core/mat.hpp>
#include "rc_tracker.h"

class sensor_fusion;
class sensor_data;

class visual_debug {
 public:
    /** Batch of images sent together for visualization.
     */
    class batch {
     public:
        /** Adds an image to the batch.
         * @param image image.
         * @param name name displayed on the image.
         */
        void add(cv::Mat image, const std::string& name);

        /** Removes all the queued images.
         */
        void clear();

     private:
        friend class visual_debug;
        std::map<std::string, std::vector<cv::Mat>> queue_;
        std::vector<std::string> ordered_names_;
    };

 public:
    visual_debug() = delete;
    visual_debug(const visual_debug&) = delete;
    visual_debug(visual_debug&&) = delete;

    /** Creates the singleton instance that will use a parent's data callback to
     * send data.
     * Subsequent invocations will return the firstly created instance.
     */
    static visual_debug& create_instance(const sensor_fusion* parent = nullptr);

    /** Sends a batch of images to the gui.
     * @param batch batch. The images are modified.
     * @param pause tells the gui to pause. If the batch is empty and pause is
     * true, a single blank image is sent to force the gui to pause.
     * @param erase_previous_images tells the gui to erase previous images.
     */
    static void send(batch& b, bool pause = true, bool erase_previous_images = true);

    /** Sends a single image to the gui.
     * @param image image.
     * @param name name displayed on the image.
     * @param sensor_id sensor id.
     * @param pause tells the gui to pause.
     * @param erase_previous_images tells the gui to erase previous images.
     */
    static void send(cv::Mat image, const std::string& name,
                     rc_Sensor sensor_id = 0, bool pause = false,
                     bool erase_previous_images = true);

    /** Queues a batch but does not send it to the gui yet.
     */
    static void queue(const batch& b);
    static void queue(batch&& b);

    /** Queues an image but does not send it to the gui yet.
     */
    static void queue(cv::Mat image, const std::string& name);

    /** Sends all the queued images and batches to the gui.
     * The queues are emptied.
     */
    static void dispatch(bool pause = true, bool erase_previous_images = true);

 private:
    /** Creates an instance associated to a sensor_fusion.
     */
    visual_debug(const sensor_fusion* parent);

    static void dispatch_one(const sensor_fusion* parent, const cv::Mat& image,
                             rc_Sensor image_id, const std::string& message,
                             bool pause, bool erase_previous);
    static cv::Mat& draw_text(const std::string& name, cv::Mat& image);
    static void cv_to_sensor_data(const cv::Mat& image, rc_Sensor image_id, const std::string& message,
                                  bool pause, bool erase_previous, sensor_data& debug_data);

    const sensor_fusion* parent_;
    std::vector<batch> queue_;
};

#endif  // __DEBUG_MANAGER_H
