:: Produces all currently supported release packages for Windows

@echo off

set "SCRIPT_DIR=%~dp0"

set LINKAGE=static
set BUILD_TYPE=release

set TARGET_ENV=legacy
call %SCRIPT_DIR%\1-build.bat %TARGET_ENV% %LINKAGE% %BUILD_TYPE% || goto after_build_legacy
call %SCRIPT_DIR%\2-package.bat %BUILD_DIR% %TARGET_ENV% %LINKAGE% %BUILD_TYPE%

:after_build_legacy

set TARGET_ENV=recent
call %SCRIPT_DIR%\1-build.bat %TARGET_ENV% %LINKAGE% %BUILD_TYPE% || goto after_build_recent
call %SCRIPT_DIR%\2-package.bat %BUILD_DIR% %TARGET_ENV% %LINKAGE% %BUILD_TYPE%

:after_build_recent

pause
