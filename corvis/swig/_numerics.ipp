// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

%module numerics
%{
#define SWIG_FILE_WITH_INIT
#include "vec4.h"
#include "matrix.h"
%}

%feature("autodoc", "1");

%init %{
      import_array();
%}

%include "cor_typemaps.i"


%typemap(out) matrix {
  npy_intp dims[2] = { $1.rows, $1.cols };
  npy_intp strides[2] = { sizeof(f_t) * $1.stride, sizeof(f_t) };
  //this allows us to modify in-place, but is not particularly safe
%#ifdef F_T_IS_DOUBLE
  PyArray_Descr *desc = PyArray_DescrFromType(PyArray_DOUBLE);
%#endif
%#ifdef F_T_IS_SINGLE
  PyArray_Descr *desc = PyArray_DescrFromType(PyArray_FLOAT);
%#endif
  $result = PyArray_NewFromDescr(&PyArray_Type, desc, 2, dims, strides, (char *)$1.data, NPY_CARRAY, NULL);
}

%include "vec4.h"
