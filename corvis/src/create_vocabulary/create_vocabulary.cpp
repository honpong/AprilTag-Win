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

static void show_usage(char *argv0) {
    std::cout << "Usage: " << argv0 <<
                 " -i image_list.txt -o output_dir -k k_0 L_0 [-k k_1 L_1 ... -k k_N L_N]\n\n"
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
    std::vector<vocabulary_size> sizes;
    std::string output_dir;
    bool read(int argc, char* argv[]);
};

template<typename T>
class trainer {
 public:
    using vocabulary = DBoW2::TemplatedVocabulary<T, DBoW2::L1_NORM>;

    void compute_training_descriptors(const std::vector<std::string>& image_filenames);
    std::unique_ptr<vocabulary> create_vocabulary(vocabulary_size sz);

 private:
    void extract_features(const tracker::image& image, std::vector<T>& features) const;

    std::vector<std::vector<T>> training_descriptors;
};

bool read_filenames(const configuration& config, std::vector<std::string>& image_filenames);

int main(int argc, char* argv[]) {
    configuration config;
    if (!config.read(argc, argv)) return 1;

    std::cout << "Reading image filenames..." << std::endl;
    std::vector<std::string> image_filenames;
    if (!read_filenames(config, image_filenames)) {
        std::cout << "Could not read file " << config.list_filename << std::endl;
        return 1;
    }

    std::cout << "Computing descriptors..." << std::endl;
    trainer<orb_descriptor::raw> trainer;
    trainer.compute_training_descriptors(image_filenames);
    for (auto& sz : config.sizes) {
        std::cout << "Creating vocabulary k" << sz.k << " L" << sz.L << "..." << std::endl;
        auto voc = trainer.create_vocabulary(sz);
        std::string filename = config.output_dir + "/voc_k" +
                std::to_string(sz.k) + "_L" + std::to_string(sz.L) + ".bin";
        voc->saveToBinaryFile(filename);
    }
}

template<typename T>
void trainer<T>::compute_training_descriptors(const std::vector<std::string>& image_filenames) {
    unsigned int total_features = 0;
    training_descriptors.clear();
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

            total_features += training_descriptors.back().size();
        }
    }
    std::cout << training_descriptors.size() << " images, " <<
                 total_features << " features" << std::endl;
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
            features.emplace_back(std::move((orb_descriptor(track.x, track.y, image)).descriptor));
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
        }
    }
    if (list_filename.empty() || output_dir.empty() || sizes.empty()) {
        show_usage(argv[0]);
        return false;
    }
    return true;
}

