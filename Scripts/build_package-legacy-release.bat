@echo off

pushd "%~dp0"

set TARGET_ENV=legacy
set LINKAGE=static
set BUILD_TYPE=release

call 1-build.bat %TARGET_ENV% %LINKAGE% %BUILD_TYPE%
if %ERRORLEVEL% neq 0 goto exit

call 2-package.bat %TARGET_ENV% %LINKAGE% %BUILD_TYPE%
if %ERRORLEVEL% neq 0 goto exit

:exit
popd
pause
