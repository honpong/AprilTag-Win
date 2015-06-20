@echo off
if "%2"=="" goto instr
for %%F in (%1\*.rssdk) do (
	echo %%~nF
	%~dp0\RCTrackerDemo %%~dpnxF %2\%%~nF
)
goto end
:instr
echo Usage - "convert_clip <dir1> <dir2>"
echo Converts all rssdk clips in dir1 to RC format clips in dir2
:end
