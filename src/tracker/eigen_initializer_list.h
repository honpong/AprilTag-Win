//
//  eigen_initializer_list.h
//  RC3DK
//
//  Created by Eagle Jones on 4/2/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#ifndef RC3DK_eigen_initializer_list_h
#define RC3DK_eigen_initializer_list_h

EIGEN_STRONG_INLINE Matrix(std::initializer_list<const Scalar> initlist): Base()
{
    Base::_check_template_params();
    size_t dim = initlist.size();
    eigen_assert(SizeAtCompileTime == Dynamic || SizeAtCompileTime == dim);
    if(Base::size() == 0) Base::resize(dim);
    eigen_assert(Base::size() == dim);
    size_t i = 0;
    for(auto x: initlist)
    {
        (*this)[i] = x;
        ++i;
    }
}


EIGEN_STRONG_INLINE Matrix(std::initializer_list<std::initializer_list<const Scalar> > initlist): Base()
{
    Base::_check_template_params();
    auto rows = initlist.size();
    auto cols = initlist.begin()->size();
    eigen_assert(SizeAtCompileTime == Dynamic || (RowsAtCompileTime == rows && ColsAtCompileTime == cols));
    if(Base::size() == 0) Base::resize(rows, cols);
    eigen_assert(Base::rows() == rows && Base::cols() == cols);

    size_t i = 0;
    for(auto x: initlist)
    {
        eigen_assert(x.size() == cols);
        size_t j = 0;
        for(auto y: x)
        {
            (*this)(i, j) = y;
            ++j;
        }
        ++i;
    }
}

#endif
