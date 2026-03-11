:: Creates a distributable package from the selected build output.
::
:: Usage: 2-package.bat <target_env> <linkage> <build_type>
::   See 1-build.bat for the parameter description

pushd "%~dp0.."
set "PROJECT_DIR=%cd%"
set TARGET_ENV=%~1
set LINKAGE=%~2
set BUILD_TYPE=%~3
set BUILD_DIR=Build-Windows-%TARGET_ENV%-%LINKAGE%-%BUILD_TYPE%

:: verify the archive tool
set "ZIP_TOOL=C:\Program Files\7-Zip\7z.exe"
if not exist "%ZIP_TOOL%" (
	echo Archive tool not found: %ZIP_TOOL%
	echo Packaging aborted.
	set ERROR_CODE=2
	goto exit
)

:: verify the built executable
set "EXECUTABLE_PATH=%BUILD_DIR%\%BUILD_TYPE%\DoomRunner.exe"
if not exist "%EXECUTABLE_PATH%" (
	echo Build output not found: %EXECUTABLE_PATH%
	echo Packaging aborted.
	set ERROR_CODE=3
	goto exit
)

:: read version number
for /f "delims=" %%A in (version.txt) do set APP_VERSION=%%~A
if %ERRORLEVEL% neq 0 (
	echo Cannot read application version from version.txt
	echo Packaging aborted.
	set ERROR_CODE=4
	goto exit
)

:: determine the file name
if %TARGET_ENV%==legacy set "TARGET_ENV_DESC=legacy(32bit)"
if %TARGET_ENV%==recent set "TARGET_ENV_DESC=recent(64bit)"
set "BASE_NAME=DoomRunner-%APP_VERSION%-Windows-%TARGET_ENV_DESC%-%LINKAGE%"

set "RELEASE_DIR_NAME=Releases"
if not exist "%RELEASE_DIR_NAME%" mkdir "%RELEASE_DIR_NAME%"

:: create a zip archive
set "ARCHIVE_PATH=%RELEASE_DIR_NAME%\%BASE_NAME%.zip"
echo Packaging the executable %EXECUTABLE_PATH% into %ARCHIVE_PATH%
echo.
if exist "%ARCHIVE_PATH%" rm "%ARCHIVE_PATH%"
set "COMMAND="%ZIP_TOOL%" a -tzip -mx=7 "%ARCHIVE_PATH%" "%EXECUTABLE_PATH%""
echo %COMMAND%
call %COMMAND%
if %ERRORLEVEL% neq 0 (
	echo.
	echo Archive tool exited with error: %ERRORLEVEL%
	echo Packaging failed.
	set ERROR_CODE=5
	goto exit
)
echo.
echo Packaging finished successfully.
echo Output: %PROJECT_DIR%\%ARCHIVE_PATH%
set ERROR_CODE=0

:exit
echo.
popd
exit /b %ERROR_CODE%
