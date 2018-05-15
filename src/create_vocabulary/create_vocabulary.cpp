/** Creates vocabularies from external imagery.
 * Dorian Galvez-Lopez, October 2017
 */
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#if HAVE_OPENCV
#include <opencv2/highgui.hpp>
#endif
#include "orb_descriptor.h"
#include "tracker.h"
#include "fast_tracker.h"
#include "DBoW2/TemplatedVocabulary.h"
#include "image_reader.h"

static void show_usage(char *argv0) {
    std::cout << "Usage: " << argv0 <<
                 " [rc_file0 [rc_file1 ...]] [-i image_list.txt] -o output_dir -k k_0 L_0 [-k k_1 L_1 ... -k k_N L_N] [-qres] [-mix-qres]\n\n"
                 "rc_file is a capture file\n"
                 "image_list.txt contains a list of paths to images (only available if compiled with OpenCV)\n"
                 "qres downsamples the image as measure does\n"
                 "mix-qres downsamples every other image only\n"
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
    int qres;
    bool mix_qres;
    bool read(int argc, char* argv[]);
};

template<typename T>
class trainer {
 public:
    using vocabulary = DBoW2::TemplatedVocabulary<T, DBoW2::L1_NORM>;

    void set_qres(int qres, bool mixed) { qres_ = qres; mix_qres_ = mixed; }
    void add_training_descriptors_from_images(const std::vector<std::string>& image_filenames);
    void add_training_descriptors_from_captures(const std::vector<std::string>& rc_filenames);
    std::unique_ptr<vocabulary> create_vocabulary(vocabulary_size sz);
    size_t get_training_images() const;
    size_t get_training_features() const;

 private:
    void extract_features(const tracker::image& image, std::vector<T>& features) const;
    void scale(tracker::image& image) const;

    std::vector<std::vector<T>> training_descriptors;
    int qres_ = 0;
    bool mix_qres_ = false;
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
    trainer.set_qres(config.qres, config.mix_qres);
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
#if HAVE_OPENCV
    bool scale_now = false;
    for (auto& filename : image_filenames) {
        cv::Mat image = cv::imread(filename, cv::IMREAD_GRAYSCALE);
        if (!image.empty()) {
            assert(image.type() == CV_8UC1);
            tracker::image im;
            im.height_px = image.rows;
            im.width_px = image.cols;
            im.stride_px = image.step1();
            im.image = image.data;
            if ((scale_now = qres_ != 0 && (!mix_qres_ || !scale_now)))
                scale(im);

            training_descriptors.emplace_back();
            extract_features(im, training_descriptors.back());
        }
    }
#endif
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
            bool scale_now = false;
            for (auto it = timestamps.begin(); it != timestamps.end(); ) {
                sensor_data image;
                reader.query(*it, image);
                if (image.type == rc_SENSOR_TYPE_STEREO)
                    image = std::move(sensor_data::split(std::move(image)).first);
                auto im = image.tracker_image();
                if ((scale_now = qres_ != 0 && (!mix_qres_ || !scale_now)))
                    scale(im);
                training_descriptors.emplace_back();
                extract_features(im, training_descriptors.back());
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

template <int by_x, int by_y>
static void scale_down_inplace_y8_by(uint8_t *image, int final_width, int final_height, int stride) {
    for (int y=0; y<final_height; y++)
        for (int x = 0; x <final_width; x++) {
            int sum = 0; // FIXME: by_x * by_y / 2;
            for (int i=0; i<by_y; i++)
                for (int j=0; j<by_x; j++)
                    sum += image[stride * (by_y*y + i) + by_x*x + j];
            image[stride * y + x] = sum / (by_x * by_y);
        }
}

template<typename T>
void trainer<T>::scale(tracker::image& im) const {
    uint8_t* data = const_cast<uint8_t*>(im.image);
    if (qres_ == 1 && im.width_px % 2 == 0 && im.height_px % 2 == 0)
        scale_down_inplace_y8_by<2, 2>(data, im.width_px /= 2, im.height_px /= 2, im.stride_px);
    else if (qres_ == 2 && im.width_px % 4 == 0 && im.height_px % 4 == 0)
        scale_down_inplace_y8_by<4, 4>(data, im.width_px /= 4, im.height_px /= 4, im.stride_px);
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
    qres = 0;
    mix_qres = false;
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
        } else if (arg == "-qres") {
            ++qres;
        } else if (arg == "-mix-qres") {
            ++qres;
            mix_qres = true;
        } else {
            rc_filenames.emplace_back(arg);
        }
    }
    if ((list_filename.empty() && rc_filenames.empty()) || output_dir.empty() || sizes.empty()) {
        show_usage(argv[0]);
        return false;
    }
#if !defined(HAVE_OPENCV) || !HAVE_OPENCV
    if (!list_filename.empty()) {
        std::cout << "The -i option is only available when compiling with OpenCV" << std::endl;
        show_usage(argv[0]);
        return false;
    }
#endif
    return true;
}

