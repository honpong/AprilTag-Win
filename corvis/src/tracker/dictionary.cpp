#include <string.h>
#include "dictionary.h"
#include <fstream>
#include <iostream>

using namespace std;

dictionary::dictionary(string filename)
{
    kmeans = vl_kmeans_new(VL_TYPE_FLOAT, VlDistanceL2);

    std::ifstream f(filename, std::ofstream::in | std::ofstream::binary);
    if (!f.is_open()) {
        fprintf(stderr, "Could not open %s for reading\n", filename.c_str());
        return;
    }

    f.read((char*)&dimension, sizeof(dimension));
    f.read((char*)&num_centers, sizeof(dimension));
    float *centers = new float[dimension*num_centers];
    f.read((char*)centers, dimension*num_centers*sizeof(float));
    vl_kmeans_set_centers(kmeans, centers, dimension, num_centers);
    delete[] centers;
}

dictionary::dictionary(int dim, int num, const float c[])
{
    std::vector<float> d; d.resize(dim*num);

    kmeans = vl_kmeans_new(VL_TYPE_FLOAT, VlDistanceL2);
    // Make sure that dictionary elements norm to 1
    for(int i = 0; i < num; i++) {
        float norm = 0;
        for(int j = 0; j < dim; j++)
            norm += c[i*dim + j]*c[i*dim + j];
        norm = sqrt(norm);

        for(int j = 0; j < dim; j++)
            d[i*dim + j] = c[i*dim + j] / norm;
    }
    
    dimension = dim;
    num_centers = num;
    
    // set centers
    vl_kmeans_set_centers(kmeans, d.data(), dimension, num_centers);
}


dictionary::~dictionary()
{
    vl_kmeans_delete(kmeans);
}

void dictionary::write(std::string filename)
{
    std::ofstream f(filename, std::ofstream::out | std::ofstream::binary);
    if (!f.is_open()) {
        std::cerr << "Could not open " << filename << " for writing\n";
        return;
    }

    f.write((char*)&dimension, sizeof(dimension));
    f.write((char*)&num_centers, sizeof(dimension));
    f.write((char*)vl_kmeans_get_centers(kmeans), dimension*num_centers*sizeof(float));
}

void dictionary::write_header(std::string basename)
{
    std::ofstream f(basename + "_dictionary.h", std::ofstream::out | std::ofstream::binary);
    if(!f.is_open()) {
        std::cerr << "Could not open " << basename << "_dictionary.h for writing\n";
        return;
    }

    f << "#ifndef " << basename << "_dictionary_h\n";
    f << "#define " << basename << "_dictionary_h\n";

    f << "#define " << basename << "_dimension " << dimension << "\n";
    f << "#define " << basename << "_num_centers " << num_centers << "\n";
    f << "const static float " << basename << "_centers[" << basename << "_dimension * " << basename << "_num_centers] {\n";

    float *centers = (float *)vl_kmeans_get_centers(kmeans);
    for(int c = 0; c < num_centers; ++c) {
        for(int d = 0; d < dimension; ++d)
            f << centers[c * dimension + d] << ", ";
        f << "\n";
    }

    f << "};\n\n";
    f << "#endif//" << basename << "_dictionary_h\n";
}

uint32_t dictionary::quantize(const descriptor & d)
{
    if(!kmeans)
        return 0;

    vl_uint32 label;
    float distance;
    vl_kmeans_quantize(kmeans, &label, &distance, d.d, 1);
    return label;
}

dictionary::dictionary(vector<descriptor> descriptors, int num_clusters)
{
    float *data = new float[descriptors.size()*descriptor_size];
    for(int i = 0; i < descriptors.size(); i++)
        memcpy(&data[descriptor_size*i], descriptors[i].d, descriptor_size*sizeof(float));

    dimension = descriptor_size;
    num_centers = num_clusters;

    kmeans = vl_kmeans_new(VL_TYPE_FLOAT, VlDistanceL2);

    vl_kmeans_init_centers_with_rand_data(kmeans, data, dimension, descriptors.size(), num_centers);

    // Run at most 100 iterations of cluster refinement using Lloyd algorithm
    vl_kmeans_set_max_num_iterations(kmeans, 100);
    vl_kmeans_refine_centers(kmeans, data, descriptors.size());

    delete[] data;

    // Obtain the energy of the solution
    //energy = vl_kmeans_get_energy(kmeans) ;
    // Obtain the cluster centers
    //centers = vl_kmeans_get_centers(kmeans) ;

    return;
}
