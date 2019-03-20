@echo off
:: %1 is the folder path of captured dataset
echo %1

:: set default path
if [%1]==[] (set SRC=".\capture") else (set SRC=%1) 

:: call first script
:: python translatev1.py
call translatev1.exe %SRC%

:: make output directory
RMDIR /S /Q output
mkdir output

:: call second script
:: python csvrwv1.py
call csvrwv1.exe %SRC%

:: remind output directory
set DST=%cd%
MSG * "SOURCE JPEGS FROM %SRC%, TAGGED JPEGS TO FOLDER %DST%\output"

EXIT 0
