#include "image_reader.h"
#include "packet.h"
#include "sensor_data.h"

bool image_reader::open(const std::string& capture_file) {
    file_.close();
    file_.open(capture_file);
    return (file_.is_open());
}

bool image_reader::create_index(std::function<void(float)> callback) {
    index_.clear();
    if (!file_.is_open()) return false;

    double file_size = [&]() {
        file_.seekg(0, std::ios::end);
        auto file_size = file_.tellg();
        file_.seekg(0, std::ios::beg);
        return static_cast<double>(file_size);
    }();

    constexpr int header_size = 16;
    packet_header_t header;
    file_.read((char *)&header, header_size);
    if (file_.bad() || file_.eof()) return false;

    while (!file_.bad() && !file_.eof()) {
        if (header.type == packet_image_raw) {
            auto header_pos = file_.tellg() - std::ifstream::pos_type(header_size);
            sensor_data image;
            if (read_image(header, image)) {
                index_[sensor_clock::tp_to_micros(image.timestamp)] = header_pos;
            } else {
                break;
            }
        } else {
            file_.seekg(header.bytes - header_size, std::ios::cur);  // skip packet
        }

        if (callback) callback((double)file_.tellg() / file_size);

        file_.read((char *)&header, header_size);
    }
    file_.clear();
    return true;
}

bool image_reader::query(rc_Timestamp timestamp, sensor_data& image) {
    auto it = index_.find(timestamp);
    if (it != index_.end()) {
        file_.seekg(it->second);
        packet_header_t header;
        file_.read((char *)&header, 16);
        if (!file_.bad() && !file_.eof()) {
            return read_image(header, image);
        }
    }
    return false;
}

bool image_reader::read_image(const packet_header_t& header, sensor_data& image) {
    auto phandle = std::unique_ptr<void, void(*)(void *)>(malloc(header.bytes), free);
    auto packet = (packet_t *)phandle.get();
    file_.read((char *)(packet)->data, header.bytes - 16);
    if (!file_.bad() && !file_.eof()) {
        packet->header = header;
        packet_image_raw_t *ip = (packet_image_raw_t *)packet;
        image = sensor_data(ip->header.time, rc_SENSOR_TYPE_IMAGE, ip->header.sensor_id,
                            ip->exposure_time_us, ip->width, ip->height, ip->stride,
                            rc_FORMAT_GRAY8, ip->data, std::move(phandle));
        return true;
    }
    return false;
}

bool image_reader::save_index(const std::string& index_file) const {
    std::ofstream file(index_file);
    if (!file.is_open()) return false;
    for (auto& pair : index_) {
        file << pair.first << " " << pair.second << std::endl;
    }
    file.close();
    return static_cast<bool>(file);
}

bool image_reader::load_index(const std::string& index_file) {
    index_.clear();
    std::ifstream file(index_file);
    if (!file.is_open()) return false;

    rc_Timestamp timestamp;
    unsigned long long header_pos;
    while (file >> timestamp >> header_pos) {
        index_[timestamp] = header_pos;
    }
    return true;
}
