:: Installs the application from the selected build output into this system.
:: If you want to use it, change the installation destination! (variable INSTALL_DIR)

@echo off

pushd "%~dp0.."
set "SOURCE_DIR=%cd%"
set "SCRIPT_DIR=%SOURCE_DIR%\Scripts"
set "SHORTEN_PATHS=python3 "%SCRIPT_DIR%\replace.py" "%SOURCE_DIR%" "{SOURCE_DIR}""
for %%I in ("%SOURCE_DIR%") do set "PROJECT_NAME=%%~nxI"

set TARGET_ENV=%~1
set LINKAGE=%~2
set BUILD_TYPE=%~3

set "BUILD_DIR=%SOURCE_DIR%\Build-Windows-%TARGET_ENV%-%LINKAGE%-%BUILD_TYPE%"

set "INSTALL_DIR=C:\Users\Youda\Games\Doom\%PROJECT_NAME%"
echo Installing the application from "%BUILD_DIR%" to "%INSTALL_DIR%" | %SHORTEN_PATHS%

set "FILE=%BUILD_DIR%\%BUILD_TYPE%\%PROJECT_NAME%.exe"
echo copy "%FILE%" "%INSTALL_DIR%" | %SHORTEN_PATHS%
copy "%FILE%" "%INSTALL_DIR%"

echo.
echo Done

echo.
popd
