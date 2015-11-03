$ErrorActionPreference = "Stop"

$dir = [System.IO.Path]::GetDirectoryName($myInvocation.MyCommand.Definition)

if (!(Test-Path -Path "$dir\build32")) { cd "$dir"; mkdir build32; cd build32; cmake -G "Visual Studio 14 2015"       .. } else { echo "build32 already setup" }
if (!(Test-Path -Path "$dir\build64")) { cd "$dir"; mkdir build64; cd build64; cmake -G "Visual Studio 14 2015 Win64" .. } else { echo "build64 already setup" }

cmake --build "$dir\build32"
cmake --build "$dir\build64"
