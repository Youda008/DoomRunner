@echo off
goto :start

:exit_with_help
echo Builds all parts of the project using requested build parameters.
echo.
echo Usage: 1-build.bat ^<target_env^> ^<linkage^> ^<build_type^>
echo   target_env - combines hardware architecture, msys2 build environment and Qt version
echo                  recent = UCRT64,  Qt6
echo                  legacy = MINGW32, Qt5
echo   linkage    - how the library dependencies are linked
echo                  static = produces large standalone executable with all libraries integrated into it
echo                  dynamic = produces small executable, but the libraries have to be installed into the system or bundled with the application
echo   build_type - QMake build type
echo                  release = enables most optimizations, generates debug symbols into a separate file
echo                  profile = enables some optimizations, generates debug symbols into a separate file
echo                  debug = disables optimizations, generates debug symbols into the executable
echo.
echo NOTE: Running the script via the 'call' command allows the caller to re-use the variables such as BUILD_DIR.
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
set TARGET_ENV=%~1
if %TARGET_ENV% NEQ recent ( if %TARGET_ENV% NEQ legacy (
	echo Invalid target_env "%TARGET_ENV%", possible values: recent, legacy
	set ERROR_CODE=1
	goto :exit
))
set LINKAGE=%~2
if %LINKAGE% NEQ static ( if %LINKAGE% NEQ dynamic (
	echo Invalid linkage "%LINKAGE%", possible values: static, dynamic
	set ERROR_CODE=1
	goto :exit
))
set BUILD_TYPE=%~3
if %BUILD_TYPE% NEQ release ( if %BUILD_TYPE% NEQ profile ( if %BUILD_TYPE% NEQ debug (
	echo Invalid build_type "%BUILD_TYPE%", possible values: release, profile, debug
	set ERROR_CODE=1
	goto :exit
)))

:: may be useful for composing file or directory names
if %TARGET_ENV%==legacy set "CPU_ARCH=i386"
if %TARGET_ENV%==recent set "CPU_ARCH=x86_64"

:: compose the build directory
set OS_TYPE=Windows
set "BUILD_DIR_NAME=%OS_TYPE%-%TARGET_ENV%-%LINKAGE%-%BUILD_TYPE%"
set "BUILD_DIR=%SOURCE_DIR%\Builds\%BUILD_DIR_NAME%"

:: setup the msys2 build environment
set "MSYS_ROOT=C:\msys64"
if %TARGET_ENV%==legacy set "MSYS_ENV=mingw32"
if %TARGET_ENV%==recent set "MSYS_ENV=ucrt64"
set "PATH=%MSYS_ROOT%\%MSYS_ENV%\bin;%MSYS_ROOT%\usr\local\bin;%MSYS_ROOT%\usr\bin;%MSYS_ROOT%\bin"

:: select and verify the Qt build tools
if %TARGET_ENV%==legacy if %LINKAGE%==dynamic set "QMAKE=%MSYS_ROOT%\%MSYS_ENV%\bin\qmake.exe"
if %TARGET_ENV%==legacy if %LINKAGE%==static  set "QMAKE=%MSYS_ROOT%\%MSYS_ENV%\qt5-static\bin\qmake.exe"
if %TARGET_ENV%==recent if %LINKAGE%==dynamic set "QMAKE=%MSYS_ROOT%\%MSYS_ENV%\bin\qmake6.exe"
if %TARGET_ENV%==recent if %LINKAGE%==static  set "QMAKE=%MSYS_ROOT%\%MSYS_ENV%\qt6-static\bin\qmake6.exe"
if not exist "%QMAKE%" (
	echo.
	echo Qt build tools not found: %QMAKE%
	echo Build aborted.
	set ERROR_CODE=2
	goto :exit
)

:: prepare QMake build config
set "QMAKE_CONFIG=CONFIG-=qml_debug"
if %BUILD_TYPE%==debug    set "QMAKE_CONFIG=%QMAKE_CONFIG% CONFIG+=debug"
if %BUILD_TYPE%==profile  set "QMAKE_CONFIG=%QMAKE_CONFIG% CONFIG+=profile CONFIG+=force_debug_info CONFIG+=separate_debug_info"
if %BUILD_TYPE%==release  set "QMAKE_CONFIG=%QMAKE_CONFIG% CONFIG+=release CONFIG+=force_debug_info CONFIG+=separate_debug_info"
if %TARGET_ENV%==recent if %LINKAGE%==static set "QMAKE_CONFIG=%QMAKE_CONFIG% QMAKE_LFLAGS+=-Wl,--start-group"

echo Building the application
echo  Source dir: %SOURCE_DIR%
echo  Output dir: %BUILD_DIR%
echo  Build type: %TARGET_ENV% %LINKAGE% %BUILD_TYPE%

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

:: generate the Makefile
echo.
set "COMMAND="%QMAKE%" "%SOURCE_DIR%\%PROJECT_NAME%.pro" %QMAKE_CONFIG%"
echo %COMMAND% | %SHORTEN_PATHS%
call %COMMAND%
if %ERRORLEVEL% neq 0 (
	set ERROR_CODE=100+%ERRORLEVEL%
	goto :exit
)

:: run the Makefile
echo.
set "COMMAND=mingw32-make.exe -j 10"
echo %COMMAND%
call %COMMAND%
if %ERRORLEVEL% neq 0 (
	set ERROR_CODE=200+%ERRORLEVEL%
	goto :exit
)

echo.
echo Build finished successfully.
echo Output: %BUILD_DIR% | %SHORTEN_PATHS%
set ERROR_CODE=0

:exit
echo.
popd
exit /b %ERROR_CODE%
