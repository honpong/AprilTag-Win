@ECHO off
:: %1 is the folder path of captured dataset
ECHO %1

:: set default path
if [%1]==[] (SET SRC=".\capture") else (SET SRC=%1) 
if [%2]==[] (SET PY=0) else (SET PY=%2)

if %PY%==0 (ECHO "Use executables.") else (ECHO "Use python scripts in py\.")

:: call first script
if %PY%==0 (translatev1.exe %SRC%) else (python %2\translatev1.py %SRC%)

:: make output directory
set DST=tagged_capture
RMDIR /S /Q %DST% 2> NUL
MKDIR %DST%

:: call second script
if %PY%==0 (csvrwv1.exe %SRC% %DST%) else (python %2\csvrwv1.py %SRC% %DST%)

DEL /F /Q %SRC%\outputllh.csv
DEL /F /Q %SRC%\outputllh_short.csv

:: remind output directory
MSG * "SOURCE JPEGS FROM %SRC%, TAGGED JPEGS TO FOLDER %cd%\%DST%"

EXIT 0
