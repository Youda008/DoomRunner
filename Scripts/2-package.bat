:: Creates a distributable package from the selected build output.
::
:: Usage: 2-package.bat <build_dir> <target_env> <linkage> <build_type>
::   build_dir - path to the directory where the application has been built
::   The other parameters are there just to compose the package file name. See 1-build.bat for their description.
::
:: NOTE: Running the script via the 'call' command allows the caller to re-use the variables such as PACKAGE_PATH.

@echo off

pushd "%~dp0"
set "SCRIPT_DIR=%cd%"
cd ..
set "SOURCE_DIR=%cd%"
set "SHORTEN_PATHS=python3 "%SCRIPT_DIR%\replace.py" "%SOURCE_DIR%" "{SOURCE_DIR}""
for %%I in ("%SOURCE_DIR%") do set "PROJECT_NAME=%%~nxI"

:: validate the arguments
set TARGET_ENV=%~2
set LINKAGE=%~3
set BUILD_TYPE=%~4
if %TARGET_ENV% NEQ recent ( if %TARGET_ENV% NEQ legacy (
	echo Invalid target_env "%TARGET_ENV%", possible values: recent, legacy
	set ERROR_CODE=1
	goto exit
))

:: verify the build output
set "BUILD_DIR=%~1"
set "EXECUTABLE_PATH=%BUILD_DIR%\%BUILD_TYPE%\%PROJECT_NAME%.exe"
if not exist "%EXECUTABLE_PATH%" (
	echo There is no build output in "%BUILD_DIR%" | %SHORTEN_PATHS%
	echo Packaging aborted.
	set ERROR_CODE=3
	goto exit
)

set "RELEASE_DIR=%SOURCE_DIR%\Releases"

:: read version number
for /f "delims=" %%A in (version.txt) do set APP_VERSION=%%~A
if %ERRORLEVEL% neq 0 (
	echo Cannot read application version from version.txt
	echo Packaging aborted.
	set ERROR_CODE=3
	goto exit
)

:: compose the package file name
set OS_TYPE=Windows
if %TARGET_ENV%==legacy set "TARGET_ENV_DESC=legacy(i386)"
if %TARGET_ENV%==recent set "TARGET_ENV_DESC=recent(x86_64)"
set "BASE_NAME=%PROJECT_NAME%-%APP_VERSION%-%OS_TYPE%-%TARGET_ENV_DESC%-%LINKAGE%"

:: verify the archive tool
set "ZIP_TOOL=C:\Program Files\7-Zip\7z.exe"
if not exist "%ZIP_TOOL%" (
	echo Archive tool not found: %ZIP_TOOL%
	echo Packaging aborted.
	set ERROR_CODE=2
	goto exit
)

set "PACKAGE_PATH=%RELEASE_DIR%\%BASE_NAME%.zip"

echo Packaging the build output into an archive
echo  Build dir: %BUILD_DIR% | %SHORTEN_PATHS%
echo  Archive: %PACKAGE_PATH% | %SHORTEN_PATHS%

if not exist "%RELEASE_DIR%" mkdir "%RELEASE_DIR%"

:: create a zip archive
if exist "%PACKAGE_PATH%" rm "%PACKAGE_PATH%"
echo.
set "COMMAND="%ZIP_TOOL%" a -tzip -mx=7 "%PACKAGE_PATH%" "%EXECUTABLE_PATH%""
echo %COMMAND% | %SHORTEN_PATHS%
call %COMMAND%
if %ERRORLEVEL% neq 0 (
	set ERROR_CODE=100+%ERRORLEVEL%
	goto exit
)

echo.
echo Packaging finished successfully.
echo Output: %PACKAGE_PATH% | %SHORTEN_PATHS%
set ERROR_CODE=0

:exit
echo.
popd
exit /b %ERROR_CODE%
