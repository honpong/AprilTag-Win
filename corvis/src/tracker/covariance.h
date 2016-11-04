//
//  covariance.h
//  RC3DK
//
//  Created by Eagle Jones on 1/29/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#ifndef RC3DK_covariance_h
#define RC3DK_covariance_h

#include "matrix.h"

#define MAXSTATESIZE 120

//typedef Eigen::Map<matrixtype, Eigen::Aligned, Eigen::OuterStride<MAXSTATESIZE>> covariance_map;

class covariance
{
protected:
    alignas(64) f_t cov_storage[2][MAXSTATESIZE*MAXSTATESIZE];
    alignas(64) f_t p_cov_storage[2][MAXSTATESIZE];
    alignas(64) int map[MAXSTATESIZE];

public:
    covariance(): cov(cov_storage[0], 0, 0, MAXSTATESIZE, MAXSTATESIZE), cov_scratch(cov_storage[1], 0, 0, MAXSTATESIZE, MAXSTATESIZE), process_noise((f_t *)p_cov_storage[0], 1, 0, 1, MAXSTATESIZE), process_scratch((f_t*)p_cov_storage[1], 1, 0, 1, MAXSTATESIZE) {}

    matrix cov;
    matrix cov_scratch;
    matrix process_noise;
    matrix process_scratch;

    inline f_t &operator() (const int i, const int j) { return cov(i, j); }
    inline const f_t &operator() (const int i, const int j) const { return cov(i, j); }

    void remap(int size)
    {
        std::swap(cov.data, cov_scratch.data);
        cov_scratch.resize(cov.rows(), cov.cols());
        cov.resize(size, size);

        std::swap(process_noise.data, process_scratch.data);
        process_scratch.resize(process_noise.cols());
        process_noise.resize(size);

        for(int i = 0; i < size; ++i) {
            process_noise[i] = process_scratch[abs(map[i])];
            for(int j = 0; j < size; ++j) {
                if(map[i] < 0 || map[j] < 0) {
                    if(i == j) cov(i, j) = cov_scratch(-map[i], -map[j]);
                    else cov(i, j) = 0.;
                } else cov(i, j) = cov_scratch(map[i], map[j]);
            }
        }
    }
    
    template<int _size>
    void add(int newindex, f_t * p_noise, const m<_size, _size> &initial_covariance)
    {
        int oldsize = cov.rows();
        process_noise.resize(oldsize + _size);
        cov.resize(oldsize + _size, oldsize + _size);

        for(int i = 0; i < _size; ++i) {
            process_noise[oldsize+i] = p_noise[i];
            map[newindex+i] = -(oldsize+i);
            for(int j = 0; j < _size; ++j) {
                cov(oldsize+i, oldsize+j) = initial_covariance(i, j);
            }
        }
    }
    
    void reindex(int newindex, int oldindex, int size)
    {
        for(int j = 0; j < size; ++j) {
            map[newindex+j] = oldindex+j;
        }
    }
    
    void reset()
    {
        cov.resize(0,0);
        process_noise.resize(0);
    }
    
    int size() const
    {
        return cov.cols();
    }
};

#endif
