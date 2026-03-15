:: Installs the application from the selected build output into this system.
:: If you want to use it, change the destination first!

@echo off

pushd "%~dp0.."
set "SOURCE_DIR=%cd%"
set "SCRIPT_DIR=%SOURCE_DIR%\Scripts"
set "SHORTEN_PATHS=python3 "%SCRIPT_DIR%\replace.py" "%SOURCE_DIR%" "{SOURCE_DIR}""

set TARGET_ENV=%~1
set LINKAGE=%~2
set BUILD_TYPE=%~3

set "BUILD_DIR=%SOURCE_DIR%\Build-Windows-%TARGET_ENV%-%LINKAGE%-%BUILD_TYPE%"

set "DESTINATION=C:\Users\Youda\Games\Doom\DoomRunner"
echo Installing the application from "%BUILD_DIR%" to "%DESTINATION%" | %SHORTEN_PATHS%

set "FILE=%BUILD_DIR%\%BUILD_TYPE%\DoomRunner.exe"
echo copy "%FILE%" "%DESTINATION%" | %SHORTEN_PATHS%
copy "%FILE%" "%DESTINATION%"

echo.
echo Done

echo.
popd
