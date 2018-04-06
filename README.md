# Intel SLAM [![Build Status](https://slam-build.intel.com/buildStatus/icon?job=SlamTracker/master)](https://slam-build.intel.com/job/SlamTracker/job/master/)

An egomotion estimation library, originaly from RealityCap, now
developed by the SLAM team.

It consists of tracker that combines a stream of data from
accelerometers, gyros, greyscale cameras, and optionally a depth
cameras into an estimate of the rotation and translation of the device
itself.

The main interface to the library can be found in
[rc_tracker.h](src/tracker/rc_tracker.h).

Please read [docs/COORDINATES.md](docs/COORDINATES.md) to understand
how poses might be used in your application.

An overview of the algorithm can be found in
[docs/OVERVIEW.md](docs/OVERVIEW.md)

A guide to contributing can be found in
[docs/CONTRIBUTING.md](docs/CONTRIBUTING.md)

## Building

After installing [CMake](https://cmake.org/) and MKL (see below) the
following should prepare the build directory on Ubuntu 16.04, Fedora
24, Windows (VS 2015 Update 3), and OS X.  Ubuntu 14.04 and Android
require additional options to `cmake` (again see below).

```sh

git clone https://github.intel.com/slam/tracker rc
mkdir rc/build
cd rc/build
cmake ..

```

If the above fails, be sure to wipe the build directory before trying
again; if it works, then you can build in Visual Studio or at the
command line with:

```sh

cmake --build . --config Release

```

### MKL (BLAS / LAPACKe)

Install MKL (as this is what we release with) and point `cmake` to MKL
with `-DMKLROOT=/opt/intel/mkl`.

When running on ARM, where MKL is not available, install BLAS/LAPACKe:

```sh

apt-get install libblas-dev liblapack-dev liblapacke-dev # Ubuntu

```

To disable MKL auto-detection pass `-DMKLROOT=False` to `cmake`.

To disable LAPACKe auto-detection pass `-DCMAKE_DISABLE_FIND_PACKAGE_lapacke=True` to `cmake`.

If both MKL and LAPACKe are disabled, the system will fallback to using Eigen.

### Visualization

To enable the visualizer in `measure` on Linux, install its dependencies with:

```sh

apt-get install libgl1-mesa-dev libxcursor-dev libxinerama-dev libxrender-dev libxrandr-dev # Ubuntu
dnf install libXcursor-devel libXinerama-devel libXrender-devel libXrandr-devel # Fedora

```

### Ubuntu (14.04)

0. Clang 3.6 is required on trusty, so install it

   ```sh

   apt-get install clang-3.6 libc++-dev

   ```

   and then pass the following to `cmake` to enable it.

   `-DCMAKE_CXX_COMPILER=clang++-3.6 -DCMAKE_C_COMPILER=clang-3.6`

1. Build `cmake` from source, or enable the following repo for a recent build

    ```sh

    sudo add-apt-repository ppa:george-edison55/cmake-3.x
    sudo apt-get update

    ```

### Android

  Pass the following to `cmake`

   - `-DCMAKE_TOOLCHAIN_FILE=../cmake/Android.toolchain.cmake`,
   - `-DCMAKE_ANDROID_NDK=<path-to-the-ndk>`

  and optionally any of these.

   - `-DCMAKE_SYSTEM_VERSION=21`
   - `-DCMAKE_ANDROID_ARCH_ABI=<x86_64|x86>`

## Running

To load a captured sequence and print out poses using a minimal
wrapper around the [official interface](src/tracker/rc_tracker.h).

    ./rc_replay --output-summary --output-poses path/to/capture/file

To see a rendering of the data while the program runs try

    ./measure --realtime path/to/capture/file
