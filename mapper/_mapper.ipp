// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

%module mapper
%{
#define SWIG_FILE_WITH_INIT
#include "mapper.h"
%}

%feature("autodoc", "1");

%init %{
      import_array();
%}

%include "../cor/cor_typemaps.i"

%include "mapper.h"