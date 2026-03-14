:: Creates a distributable package from the selected build output.
::
:: Usage: 2-package.bat <target_env> <linkage> <build_type>
::   See 1-build.bat for the parameter description

@echo off

pushd "%~dp0.."
set "PROJECT_DIR=%cd%"
set "SCRIPT_DIR=%PROJECT_DIR%\Scripts"

:: validate the arguments
set TARGET_ENV=%~1
if %TARGET_ENV% NEQ recent ( if %TARGET_ENV% NEQ legacy (
	echo Invalid target_env "%TARGET_ENV%", possible values: recent, legacy
	set ERROR_CODE=1
	goto exit
))
set LINKAGE=%~2
if %LINKAGE% NEQ static ( if %LINKAGE% NEQ dynamic (
	echo Invalid linkage "%LINKAGE%", possible values: static, dynamic
	set ERROR_CODE=1
	goto exit
))
set BUILD_TYPE=%~3
if %BUILD_TYPE% NEQ release ( if %BUILD_TYPE% NEQ profile ( if %BUILD_TYPE% NEQ debug (
	echo Invalid build_type "%BUILD_TYPE%", possible values: release, profile, debug
	set ERROR_CODE=1
	goto exit
)))

set "BUILD_DIR=Build-Windows-%TARGET_ENV%-%LINKAGE%-%BUILD_TYPE%"
set "EXECUTABLE_PATH=%BUILD_DIR%\%BUILD_TYPE%\DoomRunner.exe"

:: verify the archive tool
set "ZIP_TOOL=C:\Program Files\7-Zip\7z.exe"
if not exist "%ZIP_TOOL%" (
	echo Archive tool not found: %ZIP_TOOL%
	echo Packaging aborted.
	set ERROR_CODE=2
	goto exit
)

:: read version number
for /f "delims=" %%A in (version.txt) do set APP_VERSION=%%~A
if %ERRORLEVEL% neq 0 (
	echo Cannot read application version from version.txt
	echo Packaging aborted.
	set ERROR_CODE=3
	goto exit
)

:: compose the package file name
if %TARGET_ENV%==legacy set "TARGET_ENV_DESC=legacy(32bit)"
if %TARGET_ENV%==recent set "TARGET_ENV_DESC=recent(64bit)"
set "BASE_NAME=DoomRunner-%APP_VERSION%-Windows-%TARGET_ENV_DESC%-%LINKAGE%"

set "RELEASE_DIR=Releases"
if not exist "%RELEASE_DIR%" mkdir "%RELEASE_DIR%"

:: create a zip archive
set "ARCHIVE_PATH=%RELEASE_DIR%\%BASE_NAME%.zip"
echo Packaging the executable %EXECUTABLE_PATH% into %ARCHIVE_PATH%
echo.
if exist "%ARCHIVE_PATH%" rm "%ARCHIVE_PATH%"
set "COMMAND="%ZIP_TOOL%" a -tzip -mx=7 "%ARCHIVE_PATH%" "%EXECUTABLE_PATH%""
echo %COMMAND%
call %COMMAND%
if %ERRORLEVEL% neq 0 (
	set ERROR_CODE=100+%ERRORLEVEL%
	goto exit
)

echo.
echo Packaging finished successfully.
echo Output: %ARCHIVE_PATH%
set ERROR_CODE=0

:exit
echo.
popd
exit /b %ERROR_CODE%
