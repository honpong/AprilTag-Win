@ECHO off
:: %1 is the folder path of captured dataset
ECHO %1

:: set default path
if [%1]==[] (SET SRC=".\capture") else (SET SRC=%1) 
if [%2]==[] (SET PY=0) else (SET PY=%2)

if %PY%==0 (ECHO "Use executables.") else (ECHO "Use python scripts in py\.")

:: call first script
if %PY%==0 (CALL translatev1.exe %SRC%) else (python %2\translatev1.py %SRC%)

:: make output directory
RMDIR /S /Q output 2> NUL
MKDIR output

:: call second script
if %PY%==0 (CALL csvrwv1.exe %SRC%) else (python %2\csvrwv1.py %SRC%)

DEL /F /Q outputllh.csv

:: remind output directory
SET DST=%cd%
MSG * "SOURCE JPEGS FROM %SRC%, TAGGED JPEGS TO FOLDER %DST%\output"

EXIT 0
