@echo off

:: Produces all currently supported release packages for Linux,
:: and installs the application into this system.

pushd "%~dp0"

call 1-build.bat legacy static release
if %ERRORLEVEL% neq 0 goto after_build_package1
call 2-package.bat legacy static release

:after_build_package1

call 1-build.bat recent static release
if %ERRORLEVEL% neq 0 goto after_build_package2
call 2-package.bat recent static release
call 2-deploy.bat recent static release

:after_build_package2

popd
pause
