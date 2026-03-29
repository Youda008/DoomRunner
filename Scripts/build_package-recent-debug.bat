@echo off

set "SCRIPT_DIR=%~dp0"

call "%SCRIPT_DIR%\1-build.bat" recent static debug || goto :exit

call "%SCRIPT_DIR%\2-package.bat" %BUILD_DIR% recent static_exe debug || goto :exit

:exit
pause
