/** Creates vocabularies from external imagery.
 * Dorian Galvez-Lopez, October 2017
 */
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <opencv2/highgui.hpp>
#include "orb_descriptor.h"
#include "tracker.h"
#include "fast_tracker.h"
#include "DBoW2/TemplatedVocabulary.h"
#include "image_reader.h"

static void show_usage(char *argv0) {
    std::cout << "Usage: " << argv0 <<
                 " [rc_file0 [rc_file1 ...]] [-i image_list.txt] -o output_dir -k k_0 L_0 [-k k_1 L_1 ... -k k_N L_N]\n\n"
                 "rc_file is a capture file\n"
                 "image_list.txt contains a list of paths to images\n"
                 "Vocabularies are stored in output_dir/voc_k{k_i}_L{L_i}.bin\n";
}

struct vocabulary_size {
    int k, L;
    vocabulary_size() {}
    vocabulary_size(int k, int L) : k(k), L(L) {}
};

struct configuration {
    std::string list_filename;
    std::vector<std::string> rc_filenames;
    std::vector<vocabulary_size> sizes;
    std::string output_dir;
    bool read(int argc, char* argv[]);
};

template<typename T>
class trainer {
 public:
    using vocabulary = DBoW2::TemplatedVocabulary<T, DBoW2::L1_NORM>;

    void add_training_descriptors_from_images(const std::vector<std::string>& image_filenames);
    void add_training_descriptors_from_captures(const std::vector<std::string>& rc_filenames);
    std::unique_ptr<vocabulary> create_vocabulary(vocabulary_size sz);
    size_t get_training_images() const;
    size_t get_training_features() const;

 private:
    void extract_features(const tracker::image& image, std::vector<T>& features) const;

    std::vector<std::vector<T>> training_descriptors;
};

bool read_filenames(const configuration& config, std::vector<std::string>& image_filenames);

int main(int argc, char* argv[]) {
    configuration config;
    if (!config.read(argc, argv)) return 1;

    std::vector<std::string> image_filenames;
    if (!config.list_filename.empty()) {
        std::cout << "Reading images..." << std::endl;
        if (!read_filenames(config, image_filenames)) {
            std::cout << "Could not read file " << config.list_filename << std::endl;
            return 1;
        }
    }
    std::cout << "Computing descriptors..." << std::endl;
    trainer<orb_descriptor::raw> trainer;
    trainer.add_training_descriptors_from_images(image_filenames);
    trainer.add_training_descriptors_from_captures(config.rc_filenames);
    std::cout << trainer.get_training_images() << " images, " <<
                 trainer.get_training_features() << " features" << std::endl;
    for (auto& sz : config.sizes) {
        std::cout << "Creating vocabulary k" << sz.k << " L" << sz.L << "..." << std::endl;
        auto voc = trainer.create_vocabulary(sz);
        std::string filename = config.output_dir + "/voc_k" +
                std::to_string(sz.k) + "_L" + std::to_string(sz.L) + ".bin";
        voc->saveToBinaryFile(filename);
    }
}

template<typename T>
void trainer<T>::add_training_descriptors_from_images(const std::vector<std::string>& image_filenames) {
    for (auto& filename : image_filenames) {
        cv::Mat image = cv::imread(filename, cv::IMREAD_GRAYSCALE);
        if (!image.empty()) {
            assert(image.type() == CV_8UC1);
            tracker::image im;
            im.height_px = image.rows;
            im.width_px = image.cols;
            im.stride_px = image.step1();
            im.image = image.data;

            training_descriptors.emplace_back();
            extract_features(im, training_descriptors.back());
        }
    }
}

template<typename T>
void trainer<T>::add_training_descriptors_from_captures(const std::vector<std::string>& rc_filenames) {
    constexpr int interval_us = 500000;  // gap between images in microseconds
    for (auto& filename : rc_filenames) {
        image_reader reader;
        if (reader.open(filename)) {
            reader.create_index();
            std::vector<rc_Timestamp> timestamps = [&reader]() {
                std::vector<rc_Timestamp> timestamps;
                timestamps.reserve(reader.get_raw_index().size());
                for (auto it = reader.get_raw_index().begin(); it != reader.get_raw_index().end(); ++it)
                    timestamps.emplace_back(it->first);
                std::sort(timestamps.begin(), timestamps.end());
                return timestamps;
            }();
            for (auto it = timestamps.begin(); it != timestamps.end(); ) {
                sensor_data image;
                reader.query(*it, image);
                if (image.type == rc_SENSOR_TYPE_STEREO)
                    image = std::move(sensor_data::split(std::move(image)).first);
                training_descriptors.emplace_back();
                extract_features(image.tracker_image(), training_descriptors.back());
                auto target = *it + interval_us;
                auto end = it + 1;
                for (int skip = 2; end != timestamps.end() && *end < target; skip *= 2) {
                    it = end;
                    end = (end - timestamps.begin() + skip < static_cast<int>(timestamps.size()) ? end + skip : timestamps.end());
                }
                it = std::lower_bound(it, end, target);
            }
        }
    }
}

template<typename T>
size_t trainer<T>::get_training_images() const {
    return training_descriptors.size();
}

template<typename T>
size_t trainer<T>::get_training_features() const {
    return std::accumulate(training_descriptors.begin(), training_descriptors.end(), (size_t)0,
                           [](size_t n, const std::vector<T>& v) { return n + v.size(); });
}

template<>
void trainer<orb_descriptor::raw>::extract_features(const tracker::image& image,
                                                    std::vector<orb_descriptor::raw>& features) const {
    features.clear();
    fast_tracker tracker;
    const std::vector<tracker::feature_track>& tracks = tracker.detect(image, {}, 400);

    const float min_xy = orb_descriptor::border_size;
    const float max_x = image.width_px - orb_descriptor::border_size - 1;
    const float max_y = image.height_px - orb_descriptor::border_size - 1;
    for (auto& track : tracks) {
        if (track.x > min_xy && track.y > min_xy && track.x < max_x && track.y < max_y) {
            features.emplace_back(orb_descriptor(track.x, track.y, image).descriptor);
        }
    }
}

template<typename T>
std::unique_ptr<typename trainer<T>::vocabulary> trainer<T>::create_vocabulary(vocabulary_size sz) {
    std::unique_ptr<vocabulary> voc(new vocabulary);
    voc->train(training_descriptors, sz.k, sz.L);
    return voc;
}

bool read_filenames(const configuration& config, std::vector<std::string>& image_filenames) {
    image_filenames.clear();
    std::ifstream file(config.list_filename);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) image_filenames.push_back(line);
    }
    return true;
}

bool configuration::read(int argc, char* argv[]) {
    list_filename.clear();
    rc_filenames.clear();
    output_dir.clear();
    sizes.clear();
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "-i") {
            if (i + 1 < argc) {
                list_filename = argv[i+1];
                ++i;
            }
        } else if (arg == "-o") {
            if (i + 1 < argc) {
                output_dir = argv[i+1];
                ++i;
            }
        } else if (arg == "-k") {
            if (i + 2 < argc) {
                vocabulary_size sz(std::atoi(argv[i+1]), std::atoi(argv[i+2]));
                i += 2;
                if (sz.k > 1 && sz.L > 0) {
                    sizes.push_back(sz);
                } else {
                    std::cout << "Invalid vocabulary size: k=" << sz.k <<
                                 " L=" << sz.L <<
                                 ". Requirement: k > 1 && L > 0" << std::endl;
                    return false;
                }
            }
        } else {
            rc_filenames.emplace_back(arg);
        }
    }
    if ((list_filename.empty() && rc_filenames.empty()) || output_dir.empty() || sizes.empty()) {
        show_usage(argv[0]);
        return false;
    }
    return true;
}

