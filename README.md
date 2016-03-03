# Intel SLAM

An egomotion estimation library from the Sensor Fusion team, formerly
RealityCap.

It consists of tracker that combines a stream of data from an
accelerometer, gyro, greyscale camera, and optionally a depth camera
into an estimate of the rotation and translation of the device itself.

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

    apt-get install cmake clang libboost-dev libgl1-mesa-dev libxcursor-dev libxinerama-dev libxrender-dev libxrandr-dev

    ```

3. Build with

    ```sh

    make -C corvis

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

    ./windows/build.ps1

    ```

### Android

1. Install NDK (r10e), SDK, MKL
2. Create `android/RCUtility/local.properties` with following adjusted for your setup

    ```ini

    sdk.dir=/opt/Android/sdk
    ndk.dir=/opt/Android/ndk
    mkl.dir=/opt/intel/mkl

    ```

3. Build with

    ```sh

    cd android/RCUtility
    ./gradlew build

    ```
