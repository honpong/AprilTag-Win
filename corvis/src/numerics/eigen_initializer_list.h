//
//  eigen_initializer_list.h
//  RC3DK
//
//  Created by Eagle Jones on 4/2/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#ifndef RC3DK_eigen_initializer_list_h
#define RC3DK_eigen_initializer_list_h

#include <initializer_list>

EIGEN_STRONG_INLINE Matrix(std::initializer_list<const Scalar> initlist): Base()
{
    Base::_check_template_params();
    int dim = initlist.size();
    eigen_assert(SizeAtCompileTime == Dynamic || SizeAtCompileTime == dim);
    if(Base::size() == 0) Base::resize(dim);
    eigen_assert(Base::size() == dim);
    int i = 0;
    for(auto x: initlist)
    {
        (*this)[i] = x;
        ++i;
    }
}


EIGEN_STRONG_INLINE Matrix(std::initializer_list<std::initializer_list<const Scalar> > initlist): Base()
{
    Base::_check_template_params();
    unsigned int rows = initlist.size();
    unsigned int cols = initlist.begin()->size();
    eigen_assert(SizeAtCompileTime == Dynamic || (RowsAtCompileTime == rows && ColsAtCompileTime == cols));
    if(Base::size() == 0) Base::resize(rows, cols);
    eigen_assert(Base::rows() == rows && Base::cols() == cols);

    int i = 0;
    for(auto x: initlist)
    {
        eigen_assert(x.size() == cols);
        int j = 0;
        for(auto y: x)
        {
            (*this)(i, j) = y;
            ++j;
        }
        ++i;
    }
}

#endif
