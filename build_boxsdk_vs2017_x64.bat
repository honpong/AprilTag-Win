set src_dir=%~dp0
echo %src_dir%
set dst_dir=C:\temp\boxsdk2
rmdir /S /Q %dst_dir%
mkdir %dst_dir%
pushd %dst_dir%
"C:\Program Files\CMake\bin\cmake.exe" -G "Visual Studio 15 2017 Win64" -T "v140" -DBUILD_SHARED_LIBS=ON %src_dir%
call measure.sln
cd install
cd example
"C:\Program Files\CMake\bin\cmake.exe" -G "Visual Studio 15 2017 Win64" -T "v140" .
call my-box-app.sln
popd

