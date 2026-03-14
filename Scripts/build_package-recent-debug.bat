@echo off

set "SCRIPT_DIR=%~dp0"

set TARGET_ENV=recent
set LINKAGE=static
set BUILD_TYPE=debug

call %SCRIPT_DIR%\1-build.bat %TARGET_ENV% %LINKAGE% %BUILD_TYPE% || goto exit

call %SCRIPT_DIR%\2-package.bat %TARGET_ENV% %LINKAGE% %BUILD_TYPE% || goto exit

:exit
pause
