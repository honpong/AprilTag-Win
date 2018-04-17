set src_dir=%~dp0
echo %src_dir%
set dst_dir=C:\temp\boxsdk2
rmdir /S /Q %dst_dir%
mkdir %dst_dir%
pushd %dst_dir%
"C:\Program Files\CMake\bin\cmake.exe" -G "Visual Studio 15 2017" -T "v140" -DBUILD_SHARED_LIBS=ON %src_dir%
call measure.sln
cd boxsdk2_*
"C:\Program Files\CMake\bin\cmake.exe" -G "Visual Studio 15 2017" -T "v140" -B%dst_dir%\my_box_app -Hexample
call %dst_dir%\my_box_app\my-box-app.sln
popd

