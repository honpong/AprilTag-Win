@ECHO off
:: %1 is the folder path of captured dataset
ECHO %1

:: set survey name
set CURRENT_TIME=%time: =0%
set SURVEY_NAME=InSense_%date:~10,4%_%date:~4,2%_%date:~7,2%_%CURRENT_TIME:~0,2%_%CURRENT_TIME:~3,2%_%CURRENT_TIME:~6,2%_%CURRENT_TIME:~9,2%

:: set default path
if [%1]==[] (SET SRC=".\capture") else (SET SRC=%1) 
if [%2]==[] (SET PY=0) else (SET PY=%2)

if %PY%==0 (ECHO "Use executables.") else (ECHO "Use python scripts in py\.")

:: call first script
if %PY%==0 (py\translatev1.exe %SRC%) else (python %2\translatev1.py %SRC%)

:: make output directory
set DST=tagged_capture
RMDIR /S /Q %DST% 2> NUL
MKDIR %DST%

:: copy annotation
::COPY /Y %SRC%\*.geojson %DST%\

:: call second script
if %PY%==0 (py\csvrwv1.exe %SRC% %DST%) else (python %2\csvrwv1.py %SRC% %DST%)

:: Use Intel Proxy
CMD /C InsightSDK\jdk-11.0.2\bin\java.exe -DsocksProxyHost=proxy-us.intel.com -DsocksProxyPort=1080 -jar InsightSDK\InsightService-1.0-SNAPSHOT-20190412.jar --user-name hon.pong.ho@intel.com --password rea!sight1 --folder-name %DST% --survey-name %SURVEY_NAME%
:: Not Using Proxy
::CMD /C InsightSDK\jdk-11.0.2\bin\java.exe -jar InsightSDK\InsightService-1.0-SNAPSHOT-20190412.jar --user-name hon.pong.ho@intel.com --password rea!sight1 --folder-name %DST% --survey-name %SURVEY_NAME%

DEL /F /Q %SRC%\outputllh.csv
DEL /F /Q %SRC%\outputllh_short.csv

:: remind output directory
(ECHO REALSENSE-INSIGHT SCRIPT COMPLETED &^
 ECHO --------------------------------------------------------------------&^
 ECHO SOURCE JPEGS: %SRC% &^
 ECHO TAGGED JPEGS: %cd%\%DST% &^
 ECHO UPLOADED TO: "https://dev.ixstack.net/app/browse/projects" &^
 ECHO SURVEY NAME: %SURVEY_NAME%) | MSG *
