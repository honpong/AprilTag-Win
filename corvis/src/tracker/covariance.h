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
#include <remapper.h>

class covariance
{
public:
    remapper rm;
    matrix &cov, &process_noise;
    covariance(int maxstatesize, matrix &cov_, matrix &process_noise_, matrix &initial_covariance_, matrix &process_covariance_)
        : rm(maxstatesize, initial_covariance_, process_covariance_), cov(cov_), process_noise(process_noise_) {}

    inline f_t &operator() (const int i, const int j) { return cov(i, j); }
    inline const f_t &operator() (const int i, const int j) const { return cov(i, j); }

    void remap(int size)
    {
        rm.remap_matrix(size, cov);
        rm.remap_vector(size, process_noise);
    }

    void remap_from(int size, covariance &other)
    {
        rm.remap_matrix(size, cov, other.cov);
        rm.remap_vector(size, process_noise, other.process_noise);
    }

    void remap_init()
    {
        rm.reset();
    }

    template<int _size>
    void add(int newindex, const v<_size> &p_noise, const m<_size, _size> &initial_covariance)
    {
        rm.add<_size>(newindex, p_noise, initial_covariance);
    }

    void reindex(int newindex, int oldindex, int size)
    {
        rm.reindex(newindex, oldindex, size);
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
