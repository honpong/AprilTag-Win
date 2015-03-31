// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

%include "numpy.i"
%include "cpointer.i"
%include "stdint.i"

%fragment("NumPy_Fragments");

%pointer_class(int, int_pointer);
%{
static int convert_f_t_array(PyObject *input, f_t *ptr, int size) __attribute__((unused));
static int convert_f_t_array(PyObject *input, f_t *ptr, int size) {
  int i;
  bool isseq = PySequence_Check(input);
  if(isseq) {
    if (PyObject_Length(input) != size) {
        static char temp[1024];
        sprintf(temp, "Sequence size mismatch %d %d", (int)PyObject_Length(input), size);
        PyErr_SetString(PyExc_ValueError, temp);
      return 0;
    }
  }
  for (i =0; i < size; i++) {
      PyObject *o = isseq ? PySequence_GetItem(input,i): input;
      if (!PyFloat_Check(o)) {
         if(isseq) { Py_XDECREF(o); }
         PyErr_SetString(PyExc_ValueError,"Expecting a float");
         return 0;
      }
      ptr[i] = PyFloat_AsDouble(o);
      if(isseq) { Py_DECREF(o); }
  }
  return 1;
}
%}

%{
static int convert_float_array(PyObject *input, float *ptr, int size) __attribute__((unused));
static int convert_float_array(PyObject *input, float *ptr, int size) {
  int i;
  bool isseq = PySequence_Check(input);
  if(isseq) {
    if (PyObject_Length(input) != size) {
        static char temp[1024];
        sprintf(temp, "Sequence size mismatch %d %d", (int)PyObject_Length(input), size);
        PyErr_SetString(PyExc_ValueError, temp);
      return 0;
    }
  }
  for (i =0; i < size; i++) {
      PyObject *o = isseq ? PySequence_GetItem(input,i): input;
      if (!PyFloat_Check(o)) {
         if(isseq) { Py_XDECREF(o); }
         PyErr_SetString(PyExc_ValueError,"Expecting a float");
         return 0;
      }
      ptr[i] = PyFloat_AsDouble(o);
      if(isseq) { Py_DECREF(o); }
  }
  return 1;
}
%}

%typemap(in) f_t [ANY](f_t temp[$1_dim0]) {
   if (!convert_f_t_array($input,temp,$1_dim0)) {
      return NULL;
   }
   $1 = &temp[0];
}

%typemap(in) float [ANY](float temp[$1_dim0]) {
   if (!convert_float_array($input,temp,$1_dim0)) {
      return NULL;
   }
   $1 = &temp[0];
}

%typemap(in) v4 (v4 temp) {
    if (!convert_f_t_array($input,(f_t *)&temp.data,4)) {
      return NULL;
   }
   $1 = temp;
}

%typemap(in) v3 (v3 temp) {
    if (!convert_f_t_array($input,temp.data,3)) {
      return NULL;
   }
   $1 = temp;
}

%typemap(in) m4 (m4 temp, PyArrayObject *array = NULL, int is_new_object = 0) {
    %#ifdef F_T_IS_DOUBLE
         array = obj_to_array_contiguous_allow_conversion($input, NPY_DOUBLE, &is_new_object);
    %#endif
    %#ifdef F_T_IS_SINGLE
          array = obj_to_array_contiguous_allow_conversion($input, NPY_FLOAT, &is_new_object);
    %#endif
    npy_intp size[2] = {4, 4};
    if(!array || !require_dimensions(array, 2) || !require_size(array, size, 2)) SWIG_fail;
    f_t (*ad)[4] = (f_t (*)[4])PyArray_DATA(array);

    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            temp(i,j) = ad[i][j];
        }
    }
    $1 = temp;
}

/*
%typemap(in) feature_t * (PyArrayObject *array = NULL) {
    array = obj_to_array_no_conversion($input, PyArray_FLOAT);
    if(!array || !require_dimensions(array, 2) || !require_contiguous(array)) SWIG_fail;
    $1 = (feature_t *)array_data(array);
    }*/

//TODO: get this typemap working?
/*%typemap(in) f_t [ANY][ANY](PyArrayObject *array = NULL, int is_new_object = 0) {
%#ifdef F_T_IS_DOUBLE
    array = obj_to_array_contiguous_allow_conversion($input, PyArray_DOUBLE, &is_new_object);
%#endif
%#ifdef F_T_IS_SINGLE
    array = obj_to_array_contiguous_allow_conversion($input, PyArray_FLOAT, &is_new_object);
%#endif
    npy_intp size[2] = {$1_dim0, $1_dim1};
    if(!array || !require_dimensions(array, 2) || !require_size(array, size, 2)) SWIG_fail;
    $1 = (f_t (*) [$1_dim1])array_data(array);
}

%typemap(in) f_t [ANY][ANY][ANY](PyArrayObject *array = NULL, int is_new_object = 0) {
%#ifdef F_T_IS_DOUBLE
    array = obj_to_array_contiguous_allow_conversion($input, PyArray_DOUBLE, &is_new_object);
%#endif
%#ifdef F_T_IS_SINGLE
    array = obj_to_array_contiguous_allow_conversion($input, PyArray_FLOAT, &is_new_object);
%#endif
     npy_intp size[3] = {$1_dim0, $1_dim1, $1_dim2};
    if(!array || !require_dimensions(array, 3) || !require_size(array, size, 3)) SWIG_fail;
    $1 = (f_t (*) [$1_dim1][$1_dim2])array_data(array);
}

%typemap(in) f_t [ANY][ANY][ANY][ANY](PyArrayObject *array = NULL, int is_new_object = 0) {
%#ifdef F_T_IS_DOUBLE
    array = obj_to_array_contiguous_allow_conversion($input, PyArray_DOUBLE, &is_new_object);
%#endif
%#ifdef F_T_IS_SINGLE
    array = obj_to_array_contiguous_allow_conversion($input, PyArray_FLOAT, &is_new_object);
%#endif
     npy_intp size[4] = {$1_dim0, $1_dim1, $1_dim2, $1_dim3};
    if(!array || !require_dimensions(array, 4) || !require_size(array, size, 4)) SWIG_fail;
    $1 = (f_t (*) [$1_dim1][$1_dim2][$1_dim3])array_data(array);
    }*/

//we can't modify in place
%typemap(out) v4 {
  npy_intp dims[1] = { 4 };
%#ifdef F_T_IS_DOUBLE
  $result = PyArray_SimpleNew(1,dims,NPY_DOUBLE);
%#endif
%#ifdef F_T_IS_SINGLE
  $result = PyArray_SimpleNew(1,dims,NPY_FLOAT);
%#endif
     memcpy(PyArray_DATA((PyArrayObject *)$result), &($1.data), sizeof($1));
}

%typemap(out) m4 {
    npy_intp dims[2] = { 4, 4 };
%#ifdef F_T_IS_DOUBLE
  $result = PyArray_SimpleNew(2,dims,NPY_DOUBLE);
%#endif
%#ifdef F_T_IS_SINGLE
  $result = PyArray_SimpleNew(2,dims,NPY_FLOAT);
%#endif
     f_t (*ad)[4] = (f_t (*)[4])PyArray_DATA((PyArrayObject *)$result);
     for(int i = 0; i < 4; ++i)
         for(int j = 0; j < 4; ++j)
             ad[i][j] = $1(i,j);
}

%typemap(out) m4v4 {
    npy_intp dims[3] = { 4, 4, 4 };
%#ifdef F_T_IS_DOUBLE
  $result = PyArray_SimpleNew(3,dims,NPY_DOUBLE);
%#endif
%#ifdef F_T_IS_SINGLE
  $result = PyArray_SimpleNew(3,dims,NPY_FLOAT);
%#endif
     f_t (*ad)[4][4] = (f_t (*)[4][4])PyArray_DATA((PyArrayObject *)$result);
     for(int i = 0; i < 4; ++i)
         for(int j = 0; j < 4; ++j)
             for(int k = 0; k < 4; ++k)
                 ad[i][j][k] = $1[i](j, k);
}

%typemap(out) v4m4 {
    npy_intp dims[3] = { 4, 4, 4 };
%#ifdef F_T_IS_DOUBLE
  $result = PyArray_SimpleNew(3,dims,NPY_DOUBLE);
%#endif
%#ifdef F_T_IS_SINGLE
  $result = PyArray_SimpleNew(3,dims,NPY_FLOAT);
%#endif
     f_t (*ad)[4][4] = (f_t (*)[4][4])PyArray_DATA((PyArrayObject *)$result);
     for(int i = 0; i < 4; ++i)
         for(int j = 0; j < 4; ++j)
             for(int k = 0; k < 4; ++k)
                 ad[i][j][k] = $1[i](j, k);
}

%typemap(out) rotation_vector {
  npy_intp dims[1] = { 3 }; v4 v = $1.raw_vector();
  $result = PyArray_SimpleNew(1,dims, sizeof(v.data[0]) == 4 ? NPY_FLOAT : NPY_DOUBLE);
  memcpy(PyArray_DATA((PyArrayObject *)$result), &(v.data), 3*sizeof(v.data[0]));
}

%typemap(out) f_t [ANY] {
  npy_intp dims[1] = { $1_dim0 };
  //this allows us to modify in-place, but is not particularly safe
%#ifdef F_T_IS_DOUBLE
  $result = PyArray_SimpleNewFromData(1,dims,NPY_DOUBLE,(char *)$1);
%#endif
%#ifdef F_T_IS_SINGLE
  $result = PyArray_SimpleNewFromData(1,dims,NPY_FLOAT,(char *)$1);
%#endif
}

%typemap(out) float [ANY] {
  npy_intp dims[1] = { $1_dim0 };
  //this allows us to modify in-place, but is not particularly safe
  $result = PyArray_SimpleNewFromData(1,dims,NPY_FLOAT,(char *)$1);
}

%typemap(out) double [ANY] {
  npy_intp dims[1] = { $1_dim0 };
  //this allows us to modify in-place, but is not particularly safe
  $result = PyArray_SimpleNewFromData(1,dims,NPY_DOUBLE,(char *)$1);
}

%typemap(out) f_t [ANY][ANY] {
  npy_intp dims[2] = { $1_dim0, $1_dim1 };
  //this allows us to modify in-place, but is not particularly safe
%#ifdef F_T_IS_DOUBLE
  $result = PyArray_SimpleNewFromData(2,dims,NPY_DOUBLE,(char *)$1);
%#endif
%#ifdef F_T_IS_SINGLE
  $result = PyArray_SimpleNewFromData(2,dims,NPY_FLOAT,(char *)$1);
%#endif
}

%typemap(out) f_t [ANY][ANY][ANY] {
    npy_intp dims[3] = { $1_dim0, $1_dim1, $1_dim2 };
  //this allows us to modify in-place, but is not particularly safe
%#ifdef F_T_IS_DOUBLE
  $result = PyArray_SimpleNewFromData(3,dims,NPY_DOUBLE,(char *)$1);
%#endif
%#ifdef F_T_IS_SINGLE
  $result = PyArray_SimpleNewFromData(3,dims,NPY_FLOAT,(char *)$1);
%#endif
}

%typemap(out) f_t [ANY][ANY][ANY][ANY] {
    npy_intp dims[4] = { $1_dim0, $1_dim1, $1_dim2, $1_dim3 };
  //this allows us to modify in-place, but is not particularly safe
%#ifdef F_T_IS_DOUBLE
  $result = PyArray_SimpleNewFromData(4,dims,NPY_DOUBLE,(char *)$1);
%#endif
%#ifdef F_T_IS_SINGLE
  $result = PyArray_SimpleNewFromData(4,dims,NPY_FLOAT,(char *)$1);
%#endif
}

/*
%typemap(in) matrix_t * (PyArrayObject *array = NULL, int is_new_object = 0, matrix_t my_cor_array) {
%#ifdef F_T_IS_DOUBLE
    array = obj_to_array_contiguous_allow_conversion($input, PyArray_DOUBLE, &is_new_object);
%#else
    array = obj_to_array_contiguous_allow_conversion($input, PyArray_FLOAT, &is_new_object);
%#endif
    if(!array) SWIG_fail;
    if(array_numdims(array) == 1) {
        MAT_WRAP(my_cor_array_temp, 1, array_size(array, 0), (v4 *)array_data(array));
        my_cor_array = my_cor_array_temp;
    } else {
        MAT_WRAP(my_cor_array_temp, array_size(array, 0), array_size(array, 1), (v4 *)array_data(array));
        my_cor_array = my_cor_array_temp;
    }
    warning ugly ugly hack
    $1 = &my_cor_array;
    }*/
/*
%typemap(freearg) matrix_t * {
    if(is_new_object$argnum && array$argnum) { Py_DECREF(array$argnum); }
    }*/

%typemap(in) f_t {
    if (!PyFloat_Check($input)) {
         PyErr_SetString(PyExc_ValueError,"Expecting a float");
         return 0;
    }
    $1 = PyFloat_AsDouble($input);
}

%typemap(out) f_t {
    $result = PyFloat_FromDouble($1);
 }

%typemap(out) image_t {
  npy_intp dims[2] = { $1.size.width, $1.size.height };
  $result = PyArray_SimpleNewFromData(2, dims, NPY_BYTE, $1.data);
}

%typemap(out) feature_vector_t {
  npy_intp dims[2] = { $1.size, 2 };
  $result = PyArray_SimpleNewFromData(2, dims, NPY_FLOAT, $1.data);
}

%typemap(out) feature_covariance_vector_t {
  npy_intp dims[2] = { $1.size, 5 };
  $result = PyArray_SimpleNewFromData(2, dims, NPY_FLOAT, $1.data);
}

%typemap(out) point3d_vector_t {
  npy_intp dims[2] = { $1.size, 3 };
  $result = PyArray_SimpleNewFromData(2, dims, NPY_FLOAT, $1.data);
}

%typemap(out) float_vector_t {
  npy_intp dims[1] = { $1.size };
  $result = PyArray_SimpleNewFromData(1, dims, NPY_FLOAT, $1.data);
}

%typemap(out) float_vector_t * {
  npy_intp dims[1] = { $1->size };
  $result = PyArray_SimpleNewFromData(1, dims, NPY_FLOAT, $1->data);
}

%typemap(out) char_vector_t {
  npy_intp dims[1] = { $1.size };
  $result = PyArray_SimpleNewFromData(1, dims, NPY_BYTE, $1.data);
}

%typemap(out) short_vector_t {
  npy_intp dims[1] = { $1.size };
  $result = PyArray_SimpleNewFromData(1, dims, NPY_USHORT, $1.data);
}

%typemap(out) uint64_vector_t {
  npy_intp dims[1] = { $1.size };
  $result = PyArray_SimpleNewFromData(1, dims, NPY_UINT64, $1.data);
}

//%apply (double [ANY]) {f_t[ANY]}
