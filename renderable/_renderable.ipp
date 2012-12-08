// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

%module renderable
%{
#define SWIG_FILE_WITH_INIT
#include "renderable.h"
%}

%feature("autodoc", "1");

%init %{
      import_array();
%}

%include "../cor/cor_typemaps.i"

%include "renderable.h"
