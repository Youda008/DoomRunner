:: Installs the application from the selected build output into this system.
:: If you want to use it, change the destination first!

@echo off

pushd "%~dp0.."

set TARGET_ENV=%~1
set LINKAGE=%~2
set BUILD_TYPE=%~3

set BUILD_DIR=Build-Windows-%TARGET_ENV%-%LINKAGE%-%BUILD_TYPE%

set "SOURCE=%BUILD_DIR%\%BUILD_TYPE%\DoomRunner.exe"
set "DESTINATION=C:\Users\Youda\Games\Doom\DoomRunner"
echo Installing %SOURCE% to %DESTINATION%
copy "%SOURCE%" "%DESTINATION%"

echo.
echo Done

echo.
popd
