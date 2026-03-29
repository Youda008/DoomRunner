:: Installs the application from the selected build output into this system.
:: If you want to use it, change the installation destination! (variable INSTALL_DIR)

@echo off

pushd "%~dp0.."
set "SOURCE_DIR=%cd%"
set "SCRIPT_DIR=%SOURCE_DIR%\Scripts"
set "SHORTEN_PATHS=python3 "%SCRIPT_DIR%\replace.py" "%SOURCE_DIR%" "{SOURCE_DIR}""
for %%I in ("%SOURCE_DIR%") do set "PROJECT_NAME=%%~nxI"

set "BUILD_DIR=%~1"

:: detect the build type automatically
if exist "%BUILD_DIR%\release\%PROJECT_NAME%.exe" set BUILD_TYPE=release
if exist "%BUILD_DIR%\profile\%PROJECT_NAME%.exe" set BUILD_TYPE=profile
if exist "%BUILD_DIR%\debug\%PROJECT_NAME%.exe"   set BUILD_TYPE=debug
if "%BUILD_TYPE%"=="" (
	echo Failed to auto-detect build_type in "%BUILD_DIR%", please update this code
	set ERROR_CODE=3
	goto exit
)

set "INSTALL_DIR=C:\Users\Youda\Games\Doom\%PROJECT_NAME%"
echo Installing the application from "%BUILD_DIR%" to "%INSTALL_DIR%" | %SHORTEN_PATHS%

set "FILE=%BUILD_DIR%\%BUILD_TYPE%\%PROJECT_NAME%.exe"
echo copy "%FILE%" "%INSTALL_DIR%" | %SHORTEN_PATHS%
copy "%FILE%" "%INSTALL_DIR%"

echo.
echo Done

echo.
popd
