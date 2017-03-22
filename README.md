# shapefit

A light-weight online shape fitting utility for real-time applications.

To use, you may either cmake build the binary or just import sources and headers from `src/api/` and `src/thirdparty/eigen/` to your project. 
Primary header is `src/api/inc/rs_shapefit.h`. Recommend to use Intel C++ Compiler 17.0. OpenCV is optional.

The example application is based on librealsense 2.3.3 samples, having the same UI dependencies.
