#include "dictionary.h"

using namespace std;

dictionary::dictionary(string filename)
{
    kmeans = vl_kmeans_new(VL_TYPE_FLOAT, VlDistanceL2);

    // load centers
    FILE * f = fopen(filename.c_str(), "rb");
    if(!f) {
        fprintf(stderr, "Could not open %s for reading\n", filename.c_str());
        return;
    }

    fread(&dimension, sizeof(dimension), 1, f);
    fread(&num_centers, sizeof(num_centers), 1, f);

    float centers[dimension*num_centers];
    fread(centers, sizeof(float), dimension*num_centers, f);

    fclose(f);

    // set centers
    vl_kmeans_set_centers(kmeans, centers, dimension, num_centers);
}

dictionary::~dictionary()
{
    vl_kmeans_delete(kmeans);
}

void dictionary::write(string filename)
{
    FILE * f = fopen(filename.c_str(), "wb");
    if(!f) {
        fprintf(stderr, "Could not open %s for writing\n", filename.c_str());
    }

    fwrite(&dimension, sizeof(dimension), 1, f);
    fwrite(&num_centers, sizeof(num_centers), 1, f);
    fwrite(vl_kmeans_get_centers(kmeans), sizeof(float), dimension*num_centers, f);

    fclose(f);
}

uint32_t dictionary::quantize(const descriptor & d)
{
    if(!kmeans)
        return 0;

    uint32_t label;
    float distance;
    vl_kmeans_quantize(kmeans, &label, &distance, d.d, 1);
    return label;
}

dictionary::dictionary(vector<descriptor> descriptors, int num_clusters)
{
    float data[descriptors.size()*descriptor_size];
    for(int i = 0; i < descriptors.size(); i++)
        memcpy(&data[descriptor_size*i], descriptors[i].d, descriptor_size*sizeof(float));

    dimension = descriptor_size;
    num_centers = num_clusters;

    kmeans = vl_kmeans_new(VL_TYPE_FLOAT, VlDistanceL2);

    vl_kmeans_init_centers_with_rand_data(kmeans, data, dimension, descriptors.size(), num_centers);

    // Run at most 100 iterations of cluster refinement using Lloyd algorithm
    vl_kmeans_set_max_num_iterations(kmeans, 100);
    vl_kmeans_refine_centers(kmeans, data, descriptors.size());

    // Obtain the energy of the solution
    //energy = vl_kmeans_get_energy(kmeans) ;
    // Obtain the cluster centers
    //centers = vl_kmeans_get_centers(kmeans) ;

    return;
}
