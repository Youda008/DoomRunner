pushd "%~dp0.."
set "PROJECT_DIR=%cd%"
set TARGET_ENV=%~1
set LINKAGE=%~2
set BUILD_TYPE=%~3
set BUILD_DIR_NAME=Build-Windows-%TARGET_ENV%-%LINKAGE%-%BUILD_TYPE%

set "SOURCE=%PROJECT_DIR%\%BUILD_DIR_NAME%\%BUILD_TYPE%\DoomRunner.exe"
set "DESTINATION=C:\Users\Youda\Games\Doom\DoomRunner"
echo Installing %SOURCE% to %DESTINATION%
copy "%SOURCE%" "%DESTINATION%"
echo.

popd
