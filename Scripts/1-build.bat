:: Builds all parts of the project using requested build parameters.
::
:: Usage: 1-build.bat <target_env> <linkage> <build_type>
::   target_env - combines hardware architecture, msys2 build environment and Qt version
::                  recent = UCRT64,  Qt6
::                  legacy = MINGW32, Qt5
::   linkage    - how the library dependencies are linked
::                  static = produces large standalone executable
::                  dynamic = produces small executable, but the DLLs have to be installed into the system or bundled with the application
::   build_type - QMake build type
::                  release = enables most optimizations, generates debug symbols into a separate file
::                  profile = enables some optimizations, generates debug symbols into a separate file
::                  debug = disables optimizations, generates debug symbols into the executable

pushd "%~dp0.."
set "PROJECT_DIR=%cd%"
set TARGET_ENV=%~1
set LINKAGE=%~2
set BUILD_TYPE=%~3
set BUILD_DIR_NAME=Build-Windows-%TARGET_ENV%-%LINKAGE%-%BUILD_TYPE%

echo Building the application
echo Source dir: %PROJECT_DIR%
echo Output dir: %PROJECT_DIR%\%BUILD_DIR_NAME%
echo Build type: %TARGET_ENV% %LINKAGE% %BUILD_TYPE%

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
	goto exit
)

:: prepare QMake build config
if %BUILD_TYPE%==debug    set "QMAKE_CONFIG=CONFIG+=debug"
if %BUILD_TYPE%==profile  set "QMAKE_CONFIG=CONFIG+=profile CONFIG+=separate_debug_info"
if %BUILD_TYPE%==release  set "QMAKE_CONFIG=CONFIG+=release CONFIG+=separate_debug_info"
if %TARGET_ENV%==recent if %LINKAGE%==static set "QMAKE_CONFIG=%QMAKE_CONFIG% QMAKE_LFLAGS+=-Wl,--start-group"

if not exist "%BUILD_DIR_NAME%" mkdir "%BUILD_DIR_NAME%"
cd "%BUILD_DIR_NAME%"

:: generate the Makefile
echo.
set "COMMAND="%QMAKE%" "%PROJECT_DIR%\DoomRunner.pro" %QMAKE_CONFIG%"
echo %COMMAND%
call %COMMAND%
"%QMAKE%" %PROJECT_DIR%\DoomRunner.pro %QMAKE_CONFIG%
if %ERRORLEVEL% neq 0 (
	echo.
	echo QMake exited with error: %ERRORLEVEL%
	echo Build failed.
	set ERROR_CODE=5
	goto exit
)

:: run the Makefile
mingw32-make.exe -j 10
if %ERRORLEVEL% neq 0 (
	echo.
	echo Make exited with error: %ERRORLEVEL%
	echo Build failed.
	set ERROR_CODE=6
	goto exit
)

echo.
echo Build finished successfully.
echo Output: %PROJECT_DIR%\%BUILD_DIR_NAME%
set ERROR_CODE=0

:exit
echo.
popd
exit /b %ERROR_CODE%
