// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

%module filter
%{
#define SWIG_FILE_WITH_INIT
#include "state.h"
#include "model.h"
#include "observation.h"
#include "filter.h"
#include "../numerics/matrix.h"
#include "filter_setup.h"
#include "tracker.h"
%}

%feature("autodoc", "1");

%init %{
      import_array();
%}

%include "../cor/cor_typemaps.i"

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
%include "state.h"
%include "model.h"
%include "observation.h"
%include "filter.h"
%include "filter_setup.h"
%include "tracker.h"