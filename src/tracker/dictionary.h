#ifndef __DICTIONARY_H
#define __DICTIONARY_H

#include <vector>
#include <string>
#include "feature_descriptor.h"
extern "C" {
#include "vl/kmeans.h"
}

class dictionary {
    private:
        VlKMeans * kmeans;
        uint32_t dimension;
        uint32_t num_centers;
        dictionary();

    public:
        dictionary(std::string filename);
        dictionary(int dim, int num, const float centers[]);
        dictionary(std::vector<descriptor> descriptors, int num_clusters = 30);
        ~dictionary();

        void write(std::string filename);
        void write_header(std::string basename);
        int get_num_centers() { return num_centers; }

        uint32_t quantize(const descriptor & d);
};

#endif
