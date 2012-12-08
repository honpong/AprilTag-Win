// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

%module camera
%{
#include "camera.h"
%}

%apply unsigned long long {uint64_t}
%feature("autodoc", "1");

%include "camera.h"
