# Intel® RealSense™ Box Measurement SDK Windows

An extension to the [Intel® RealSense™ SDK](https://github.com/IntelRealSensen/librealsense) with box detection capability for Windows applications.

## Pre-requisites

1. [Intel® RealSense™ Depth Camera D410 / D415](https://www.intel.com/realsense)
2. Microsoft Windows 10
3. Microsoft Visual Studio 2015 or 2017
4. [Git for Windows](https://git-scm.com/download/win) 2.15.1 or above
5. [CMake](https://cmake.org/download/) 3.8 or above 

## SDK Header

The top level header is [`boxsdk/inc/rs_box_sdk.hpp`](boxsdk/inc/rs_box_sdk.hpp).

## Windows x64 build instructions

- (1<sup>st</sup> time build) Delete the folder [`src/thirdparty/librealsense`](src/thirdparty/librealsense) if you download source code from github.com webpage, or `git submodule update --init` failed if you use command line `git clone`.
- Use Windows build script [build_boxsdk_vs2015_x64.bat](build_boxsdk_vs2015_x64.bat) or [build_boxsdk_vs2017_x64.bat](build_boxsdk_vs2017_x64.bat)
- When download completed, Visual Studio application will be opened.
- Right-click on the `INSTALL` project and select `build`.
- When finish building, close the Visual Studio application. 
- An example application solution will be automatically built and opened.
- The default SDK build directory is `C:\temp\boxsdk2\`, defined by the `dst_dir` path variable in the build scripts. 
- The default example application is `C:\temp\boxsdk2\my-box-app\`.

Advanced builds can be done through CMake from the top level.

## Select specific Intel® RealSense™ SDK version

Switch to specific Intel® RealSense™ SDK can be found in [CMakeList.txt](CMakeList.txt) `set(LIBRS_VER v2.8.3)`, changed `v2.8.3` to a compatible version of your choice.

# shapefit

A light-weight online shape fitting utility for real-time applications. Currently it supports plane and box fitting.

To use, you may either cmake build the binary in `src/` or just import sources and headers from `src/api/` and `src/thirdparty/eigen/` to your project. 
Primary header is `src/api/inc/rs_shapefit.h`. Recommend to use Intel C++ Compiler 17.0. OpenCV is optional.

The example application is based on librealsense 2.3.3 samples, having the same UI dependencies
