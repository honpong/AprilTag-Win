@echo off
REM %1 is the folder path of captured dataset
echo %1
REM just do something here
python translatev1.py
mkdir output
python csvrwv1.py
EXIT