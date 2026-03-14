:: Produces all currently supported release packages for Linux,
:: and installs the application into this system.

@echo off

set "SCRIPT_DIR=%~dp0"

set LINKAGE=static
set BUILD_TYPE=release

set TARGET_ENV=legacy
call %SCRIPT_DIR%\1-build.bat %TARGET_ENV% %LINKAGE% %BUILD_TYPE% || goto after_build_package1
call %SCRIPT_DIR%\2-package.bat %TARGET_ENV% %LINKAGE% %BUILD_TYPE%

:after_build_package1

set TARGET_ENV=recent
call %SCRIPT_DIR%\1-build.bat %TARGET_ENV% %LINKAGE% %BUILD_TYPE% || goto after_build_package2
call %SCRIPT_DIR%\2-package.bat %TARGET_ENV% %LINKAGE% %BUILD_TYPE%
call %SCRIPT_DIR%\3-install.bat %TARGET_ENV% %LINKAGE% %BUILD_TYPE%

:after_build_package2

pause
