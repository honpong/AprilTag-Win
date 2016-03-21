# Intel SLAM

An egomotion estimation library from the Sensor Fusion team, formerly
RealityCap.

It consists of tracker that combines a stream of data from an
accelerometer, gyro, greyscale camera, and optionally a depth camera
into an estimate of the rotation and translation of the device itself.

The main interface to the library can be found in
[rc_tracker.h](corvis/src/filter/rc_tracker.h).

## Building

### OS X


1. Optionally install MKL and/or ICC
2. Install homebrew dependencies

    ```sh

    brew install cmake boost

    ```

3. Build with

    ```sh

    make -C corvis

    ```

### Ubuntu (14.04)

1. Install MKL (and optionally ICC)
2. Install build dependencies

    ```sh

    apt-get install cmake clang-3.6 libc++-dev libboost-dev libgl1-mesa-dev libxcursor-dev libxinerama-dev libxrender-dev libxrandr-dev

    ```

3. Build with

    ```sh

    mkdir -p corvis/bin
    cd corvis/bin
    cmake .. -DCMAKE_CXX_COMPILER=clang++-3.6 -DCMAKE_C_COMPILER=clang-3.6
    make

    ```

### Fedora (23)

1. Install Intel MKL (and optionally ICC)
2. Install system packages

    ```sh

    dnf install cmake clang boost-devel libXcursor-devel libXinerama-devel libXrender-devel libXrandr-devel

    ```

3. Build with

    ```sh

    make -C corvis

    ```

### Windows

1. Install Intel MKL (and optionally ICC)
2. Install RSSDK
3. Install cmake https://cmake.org/
4. Build with

    ```powershell

    .\windows\build.ps1

    ```

### Android

1. Install NDK (r10e), SDK, MKL
2. Create `android/RCUtility/local.properties` with following adjusted for your setup

    ```ini

    sdk.dir=/opt/Android/sdk
    ndk.dir=/opt/Android/ndk
    mkl.dir=/opt/intel/mkl

    ```

3. Build every permutation

    ```sh

    cd android/RCUtility
    ./gradlew build

    ```

    or just the x86 Release version

    ```sh

    ./gradlew assembleRc86Release

    ```

    or to install just the x86_64 Relwithdebinfo version

    ```sh

    ./gradlew installRc64Relwithdebinfo

    ```

## Running

To load a captured sequence and print out poses using a minimal
wrapper around the [official interface](corvis/src/filter/rc_tracker.h).

    ./corvis/bin/rc_replay --output-summary --output-poses path/to/capture/file

To see a rendering of the data while the program runs try

    ./corvis/bin/measure --realtime path/to/capture/file
