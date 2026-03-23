:: Produces all currently supported release packages for Windows

@echo off

set "SCRIPT_DIR=%~dp0"

call "%SCRIPT_DIR%\1-build.bat" legacy static release || goto after_build_legacy
call "%SCRIPT_DIR%\2-package.bat" %BUILD_DIR% legacy static_exe release

:after_build_legacy

call "%SCRIPT_DIR%\1-build.bat" recent static release || goto after_build_recent
call "%SCRIPT_DIR%\2-package.bat" %BUILD_DIR% recent static_exe release

:after_build_recent

pause
