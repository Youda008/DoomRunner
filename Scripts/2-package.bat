@echo off
goto :start

:exit_with_help
echo Creates a distributable package from the selected build output.
echo.
echo Usage: 2-package.bat ^<build_dir^> ^<target_env^> ^<package_type^>
echo   build_dir - path to the directory where the application has been built
echo   target_env - only needed to compose the package file name, see 1-build.bat for description
echo   package_type - what kind of package should be produced from the build output
echo                    static_exe = zipped statically linked executable that integrates all dependencies into itself
echo                    bundled_dlls = zipped dynamically linked executable carries the required DLLs with it (currently unsupported)
echo.
echo NOTE: Running the script via the 'call' command allows the caller to re-use the variables such as PACKAGE_PATH.
goto :exit

:start
pushd "%~dp0"
set "SCRIPT_DIR=%cd%"
cd ..
set "SOURCE_DIR=%cd%"
set "SHORTEN_PATHS=python3 "%SCRIPT_DIR%\replace.py" "%SOURCE_DIR%" "{SOURCE_DIR}""
for %%I in ("%SOURCE_DIR%") do set "PROJECT_NAME=%%~nxI"

:: validate the arguments
set count=0
for %%x in (%*) do set /a count+=1
if %count% LSS 3 (
	set ERROR_CODE=1
	goto :exit_with_help
) else if "%~1"=="/?" (
	set ERROR_CODE=0
	goto :exit_with_help
) else if "%~1"=="/help" (
	set ERROR_CODE=0
	goto :exit_with_help
)
set "BUILD_DIR=%~1"
set TARGET_ENV=%~2
if %TARGET_ENV% NEQ recent ( if %TARGET_ENV% NEQ legacy (
	echo Invalid target_env "%TARGET_ENV%", possible values: recent, legacy
	set ERROR_CODE=1
	goto exit
))
set PACKAGE_TYPE=%~3
if %PACKAGE_TYPE% NEQ static_exe ( if %PACKAGE_TYPE% NEQ bundled_dlls (
    echo Invalid package_type "%PACKAGE_TYPE%", possible values: static_exe, bundled_dlls
    set ERROR_CODE=1
    goto exit
))

:: detect the build type automatically
if exist "%BUILD_DIR%\release\%PROJECT_NAME%.exe" set BUILD_TYPE=release
if exist "%BUILD_DIR%\profile\%PROJECT_NAME%.exe" set BUILD_TYPE=profile
if exist "%BUILD_DIR%\debug\%PROJECT_NAME%.exe"   set BUILD_TYPE=debug
if "%BUILD_TYPE%"=="" (
	echo Failed to auto-detect build_type in "%BUILD_DIR%", please update this code
	set ERROR_CODE=3
	goto exit
)

:: verify the build output
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
set "BASE_NAME=%PROJECT_NAME%-%APP_VERSION%-%OS_TYPE%-%TARGET_ENV_DESC%-%PACKAGE_TYPE%"
if %BUILD_TYPE% NEQ release set "BASE_NAME=%BASE_NAME%-%BUILD_TYPE%"

if not exist "%RELEASE_DIR%" mkdir "%RELEASE_DIR%"

:: verify the archive tool
set "ZIP_TOOL=C:\Program Files\7-Zip\7z.exe"
if not exist "%ZIP_TOOL%" (
	echo Archive tool not found: %ZIP_TOOL%
	echo Packaging aborted.
	set ERROR_CODE=2
	goto exit
)

set "PACKAGE_PATH=%RELEASE_DIR%\%BASE_NAME%.zip"


if %PACKAGE_TYPE%==static_exe    goto static_exe
if %PACKAGE_TYPE%==bundled_dlls  goto bundled_dlls
goto exit


:static_exe

echo Packaging the build output into an archive
echo  Build dir: %BUILD_DIR% | %SHORTEN_PATHS%
echo  Archive: %PACKAGE_PATH% | %SHORTEN_PATHS%

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
goto exit


:bundled_dlls

echo Package with bundled DLLs is not implemented yet.
set ERROR_CODE=1
goto exit


:exit
echo.
popd
exit /b %ERROR_CODE%
