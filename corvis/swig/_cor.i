// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

%module cor
%{
#define SWIG_FILE_WITH_INIT
#include "cor.h"
%}

%feature("autodoc", "1");

%init %{
      import_array();
%}

%include "cor_typemaps.i"

%constant void dispatch_python(void *data, packet_t *p);

void dispatch_addpython(dispatch_t *d, PyObject *cookie);
struct plugin plugins_initialize_python(PyObject *start, PyObject *stop);

%{

//specialize a packet_t so that python can access its data
static PyObject * convert_packet_t_PyObject(packet_t *p)
{
   swig_type_info *swig_type;
   switch(p->header.type) {
     case packet_camera:
          swig_type = SWIGTYPE_p_packet_camera_t;
          break;
     case packet_imu:
          swig_type = SWIGTYPE_p_packet_imu_t;
          break;
     case packet_gyroscope:
          swig_type = SWIGTYPE_p_packet_gyroscope_t;
          break;
     case packet_accelerometer:
          swig_type = SWIGTYPE_p_packet_accelerometer_t;
          break;
     case packet_feature_track:
     case packet_feature_select:
          swig_type = SWIGTYPE_p_packet_feature_track_t;
          break;
     case packet_feature_prediction_variance:
          swig_type = SWIGTYPE_p_packet_feature_prediction_variance_t;
          break;
     case packet_feature_drop:
          swig_type = SWIGTYPE_p_packet_feature_drop_t;
          break;
     case packet_feature_status:
          swig_type = SWIGTYPE_p_packet_feature_status_t;
          break;
     case packet_navsol:
          swig_type = SWIGTYPE_p_packet_navsol_t;
          break;
     case packet_ground_truth:
          swig_type = SWIGTYPE_p_packet_ground_truth_t;
          break;
     case packet_filter_position:
          swig_type = SWIGTYPE_p_packet_filter_position_t;
          break;
     case packet_filter_reconstruction:
          swig_type = SWIGTYPE_p_packet_filter_reconstruction_t;
          break;
     case packet_filter_feature_id_visible:
          swig_type = SWIGTYPE_p_packet_filter_feature_id_visible_t;
          break;
     case packet_filter_feature_id_association:
          swig_type = SWIGTYPE_p_packet_filter_feature_id_association_t;
          break;
     case packet_plot:
          swig_type = SWIGTYPE_p_packet_plot_t;
          break;
     case packet_plot_info:
          swig_type = SWIGTYPE_p_packet_plot_info_t;
          break;
     default:
          swig_type = SWIGTYPE_p_packet_t;
          break;
   }
   return SWIG_NewPointerObj(SWIG_as_voidptr(p), swig_type, 0 );
}

//callback for a python object
void dispatch_python(void *data, packet_t *p)
{
   PyObject *func, *arglist;

   //make sure we have the global interpreter lock
   PyGILState_STATE gstate;    
   gstate = PyGILState_Ensure();

   func = (PyObject *) data;                    // Get Python function
   //create a python list and insert the argument
   arglist = Py_BuildValue("(N)", convert_packet_t_PyObject(p));
   PyObject *result = PyEval_CallObject(func, arglist);
   if(!result) {
      fprintf(stderr, "Python dispatch failed:\n");
      PyErr_Print();
   } else {
      Py_DECREF(result);
   }
   Py_DECREF(arglist);

   PyGILState_Release(gstate);
}

void dispatch_addpython(dispatch_t *d, PyObject *cookie)
{
   PyGILState_STATE gstate;    
   gstate = PyGILState_Ensure();
   if (!PyCallable_Check(cookie)) {
      PyErr_SetString(PyExc_TypeError, "Need a callable object!");
      return;
   }
   Py_INCREF(cookie);
   dispatch_addclient(d, cookie, dispatch_python);
   PyGILState_Release(gstate);
}

struct python_callback {
       PyObject *start;
       PyObject *stop;
};

static void *plugins_start_python(void *data)
{
   //make sure we have the global interpreter lock
   PyGILState_STATE gstate;    
   gstate = PyGILState_Ensure();
   struct python_callback *pc = (struct python_callback *)data;

   if (PyCallable_Check(pc->start)) {
       PyObject *arglist = Py_BuildValue("()");
       PyObject *result = PyEval_CallObject(pc->start, NULL);
       if(!result) {
           fprintf(stderr, "Python plugin launch failed:\n");
           PyErr_Print();
       } else {
           Py_DECREF(result);
       }
       Py_DECREF(arglist);
   }

   PyGILState_Release(gstate);
   return NULL;
}

static void plugins_stop_python(void *data)
{
   //make sure we have the global interpreter lock
   PyGILState_STATE gstate;    
   gstate = PyGILState_Ensure();
   
   struct python_callback *pc = (struct python_callback *)data;
   //create a python list and insert the argument
   if (PyCallable_Check(pc->stop)) {
       PyObject *result = PyEval_CallObject(pc->stop, NULL);
       if(!result) {
           fprintf(stderr, "Python plugin stop failed:\n");
           PyErr_Print();
       } else {
           Py_DECREF(result);
       }
   }
   
   PyGILState_Release(gstate);
}

struct plugin plugins_initialize_python(PyObject *start, PyObject *stop)
{
   PyGILState_STATE gstate;    
   gstate = PyGILState_Ensure();
   if (!PyCallable_Check(start) && !PyCallable_Check(stop)) {
      PyErr_SetString(PyExc_TypeError, "Need two callable objects!");
      return(struct plugin) {};
   }
   struct python_callback *pc = malloc(sizeof(struct python_callback)); 
   Py_INCREF(start);
   Py_INCREF(stop);
   pc->start = start;
   pc->stop = stop;
   PyGILState_Release(gstate);
   return (struct plugin) { .data = pc, .start = plugins_start_python, .stop = plugins_stop_python };
}

%}

%typemap(out) packet_t * {
   $result = convert_packet_t_PyObject($1);
}

%typemap(in) packet_t * {
    if(
       SWIG_ConvertPtr($input, (void **)&$1, SWIGTYPE_p_packet_filter_position_t, 0 | 0 ) == -1 
       && SWIG_ConvertPtr($input, (void **)&$1, SWIGTYPE_p_packet_camera_t, 0 | 0 ) == -1 
       && SWIG_ConvertPtr($input, (void **)&$1, SWIGTYPE_p_packet_feature_drop_t, 0 | 0 ) == -1
       && SWIG_ConvertPtr($input, (void **)&$1, SWIGTYPE_p_packet_feature_track_t, 0 | 0 ) == -1
       && SWIG_ConvertPtr($input, (void **)&$1, SWIGTYPE_p_packet_feature_status_t, 0 | 0 ) == -1
       && SWIG_ConvertPtr($input, (void **)&$1, SWIGTYPE_p_packet_imu_t, 0 | 0 ) == -1
       && SWIG_ConvertPtr($input, (void **)&$1, SWIGTYPE_p_packet_accelerometer_t, 0 | 0 ) == -1
       && SWIG_ConvertPtr($input, (void **)&$1, SWIGTYPE_p_packet_gyroscope_t, 0 | 0 ) == -1
       && SWIG_ConvertPtr($input, (void **)&$1, SWIGTYPE_p_packet_filter_reconstruction_t, 0 | 0 ) == -1
       && SWIG_ConvertPtr($input, (void **)&$1, SWIGTYPE_p_packet_filter_feature_id_visible_t, 0 | 0 ) == -1
       && SWIG_ConvertPtr($input, (void **)&$1, SWIGTYPE_p_packet_filter_feature_id_association_t, 0 | 0 ) == -1
       && SWIG_ConvertPtr($input, (void **)&$1, SWIGTYPE_p_packet_navsol_t, 0 | 0 ) == -1
       && SWIG_ConvertPtr($input, (void **)&$1, SWIGTYPE_p_packet_ground_truth_t, 0 | 0 ) == -1
       && SWIG_ConvertPtr($input, (void **)&$1, SWIGTYPE_p_packet_t, 0 | 0 ) == -1
       ) {
        PyErr_SetString(PyExc_TypeError, "Need a converter for the packet_t typemap(in)!");
        return NULL;
    }

}

//%include "cor.h"
%include "debug.h"
%include "dispatch.h"
%include "plugins.h"
%include "mapbuffer.h"
%include "timestamp.h"
%include "packet.h"
%include "cor_types.h"
%include "inbuffer.h"
