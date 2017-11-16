#ifndef GENERATE_LOOP_GT_IMAGE_READER_H
#define GENERATE_LOOP_GT_IMAGE_READER_H

#include <fstream>
#include <functional>
#include <string>
#include <unordered_map>
#include "rc_tracker.h"
#include "sensor_data.h"
#include "packet.h"

/** Parses a rc file and indexes images by timestamp.
 */
class image_reader {
 public:
    bool open(const std::string& capture_file);
    bool create_index(std::function<void(float)> callback = {});
    bool save_index(const std::string& index_file) const;
    bool load_index(const std::string& index_file);
    bool query(rc_Timestamp timestamp, sensor_data& image);
    const std::unordered_map<rc_Timestamp, std::ifstream::pos_type>& get_raw_index() const {
        return index_;
    }
 private:
    bool read_image(const packet_header_t& header, sensor_data& image);
    std::ifstream file_;
    std::unordered_map<rc_Timestamp, std::ifstream::pos_type> index_;
};

#endif  // GENERATE_LOOP_GT_IMAGE_READER_H
