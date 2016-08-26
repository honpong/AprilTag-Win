# Intel SLAM

An egomotion estimation library from the Sensor Fusion team, formerly
RealityCap.

It consists of tracker that combines a stream of data from an
accelerometer, gyro, greyscale camera, and optionally a depth camera
into an estimate of the rotation and translation of the device itself.

The main interface to the library can be found in
[rc_tracker.h](corvis/src/filter/rc_tracker.h).

## Building

```sh

git clone https://github.intel.com/sensorfusion/sensorfusion rc
mkdir rc/build
cd rc/build
cmake ..
cmake --build .

```

### BLAS/LAPACKe

Install MKL (as this is what we release with) and point `cmake` to MKL
with `-DMKLROOT=/opt/intel/mkl`.

When running on ARM, where MKL is not available, install BLAS/LAPACKe:

```sh

apt-get install libblas-dev liblapack-dev liblapacke-dev # Ubuntu

```

To disable MKL auto-detection pass `-DMKLROOT=False` to `cmake`.

### Visualization

Pass `-DENABLE_VISGL=True` to `cmake` to enable the visualizer
`measure` and on Linux, install its dependencies with:

```sh

apt-get install libgl1-mesa-dev libxcursor-dev libxinerama-dev libxrender-dev libxrandr-dev # Ubuntu
dnf install boost-devel libXcursor-devel libXinerama-devel libXrender-devel libXrandr-devel # Fedora

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

  Pass the following to `cmake`.

   - `-DCMAKE_TOOLCHAIN_FILE=../android/Android.toolchain.cmake`,
   - `-DANDROID_NDK_ROOT=<path-to-the-ndk>`
   - `-DANDROID_PLATFORM=android-21`
   - `-DANDROID_ARCH=<x86_64|x86>`

## Running

To load a captured sequence and print out poses using a minimal
wrapper around the [official interface](corvis/src/filter/rc_tracker.h).

    ./corvis/rc_replay --output-summary --output-poses path/to/capture/file

To see a rendering of the data while the program runs try

    ./corvis/measure --realtime path/to/capture/file
