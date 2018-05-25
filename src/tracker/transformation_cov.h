#pragma once

#include "transformation.h"

class transformation_cov {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    transformation_cov() : cov(m<6,6>::Zero()) {};
    transformation_cov(const transformation& G_, const m<6,6>& cov_ = m<6,6>::Zero()) : G(G_.Q, G_.T), cov(cov_) {};

    transformation G;
    m<6,6> cov;
};

transformation_cov operator*(const transformation_cov& G1, const transformation_cov& G2);
transformation_cov invert(const transformation_cov& G12);
transformation_cov compose(const transformation_cov &G1, const transformation_cov &G2, const m<6,6> &cov12);

static inline transformation_cov operator*(const transformation_cov& G1, const transformation& G2) {
    return G1*transformation_cov(G2);
}

static inline transformation_cov operator*(const transformation& G1, const transformation_cov& G2) {
    return transformation_cov(G1)*G2;
}

