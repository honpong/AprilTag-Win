@echo off
REM %1 is the folder path of captured dataset
echo %1
REM just do something here
cd %1
python translatev1.py
REM call dist\translatev1\translatev1.exe
mkdir output
python csvrwv1.py
REM call dist\csvrwv1\csvrwv1.exe
MSG * "TAGGED JPEGS AT FOLDER %1"
EXIT
