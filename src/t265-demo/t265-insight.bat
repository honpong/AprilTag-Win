@ECHO off
:: %1 is the folder path of captured dataset
ECHO %1

:: path to insight uploader client
set INSIGHT_CLIENT_PATH=InsightSDK\InsightService-1.0-SNAPSHOT-20190516.jar
set INSIGHT_PIX4D_CONFIG=InsightSDK\photogrammetry_config.json
set INSIGHT_ENGINE=bentleyProcessing 
::set INSIGHT_ENGINE=pix4DProcessing 

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

::copy pix4DProcessing configuration
if %INSIGHT_ENGINE%==pix4DProcessing (COPY /Y %INSIGHT_PIX4D_CONFIG% %DST%\)

:: call second script
if %PY%==0 (py\csvrwv1.exe %SRC% %DST%) else (python %2\csvrwv1.py %SRC% %DST%)

:: Use Intel Proxy, -i or --intel-proxy or (before -jar) -DsocksProxyHost=proxy-us.intel.com -DsocksProxyPort=1080
:: processing is not started automatically, you need to add -p or --pix4DProcessing or -b --bentleyProcessing
CMD /C InsightSDK\jdk-11.0.2\bin\java.exe -jar %INSIGHT_CLIENT_PATH% --intel-proxy --user-name hon.pong.ho@intel.com --password rea!sight1 --folder-name %DST% --survey-name %SURVEY_NAME% --%INSIGHT_ENGINE%
:: Not Using Proxy
:: processing is not started automatically, you need to add -p or --pix4DProcessing or -b --bentleyProcessing
::CMD /C InsightSDK\jdk-11.0.2\bin\java.exe -jar %INSIGHT_CLIENT_PATH% --user-name hon.pong.ho@intel.com --password rea!sight1 --folder-name %DST% --survey-name %SURVEY_NAME% --%INSIGHT_ENGINE%

DEL /F /Q %SRC%\outputllh.csv
DEL /F /Q %SRC%\outputllh_short.csv

:: remind output directory
(ECHO REALSENSE-INSIGHT SCRIPT COMPLETED &^
 ECHO --------------------------------------------------------------------&^
 ECHO SOURCE JPEGS: %SRC% &^
 ECHO TAGGED JPEGS: %cd%\%DST% &^
 ECHO UPLOADED TO: "https://dev.ixstack.net/app/browse/projects" &^
 ECHO SURVEY NAME: %SURVEY_NAME%) | MSG *
