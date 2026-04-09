@echo off
REM ============================================================================
REM GamerHackCS2 Build Script
REM Requires: CMake 3.20+, Visual Studio 2022 (or Build Tools) with C++ workload
REM Usage: build.bat [debug|release]
REM ============================================================================

setlocal

REM Use pushd so we never need to quote a backslash-terminated path
pushd "%~dp0"

set BUILD_DIR=build
set BUILD_TYPE=Release

if /I "%1"=="debug"   set BUILD_TYPE=Debug
if /I "%1"=="release" set BUILD_TYPE=Release

echo ============================================
echo  GamerHackCS2 Build System
echo ============================================
echo.
echo [*] Build type: %BUILD_TYPE%
echo [*] Build dir:  %CD%\%BUILD_DIR%
echo.

REM Check for CMake
where cmake >nul 2>&1
if errorlevel 1 goto :no_cmake

REM Configure only if already fully configured (solution exists)
if exist "%BUILD_DIR%\GamerHackCS2.sln" goto :build

echo [*] Configuring CMake (first run - downloading dependencies)...
echo     This may take a few minutes.
echo.
cmake -S . -B "%BUILD_DIR%" -A x64
if errorlevel 1 goto :cmake_failed
echo.

:build
echo [*] Building %BUILD_TYPE%...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --parallel
if errorlevel 1 goto :build_failed

echo.
echo ============================================
echo  Build successful!
echo  Output: %CD%\%BUILD_DIR%\bin\%BUILD_TYPE%\GamerHackCS2.dll
echo ============================================
popd
endlocal
pause
exit /b 0

:no_cmake
echo [ERROR] CMake not found. Install CMake 3.20+ and add to PATH.
popd
endlocal
pause
exit /b 1

:cmake_failed
echo [ERROR] CMake configuration failed!
popd
endlocal
pause
exit /b 1

:build_failed
echo.
echo [ERROR] Build failed! Scroll up for compiler errors.
popd
endlocal
pause
exit /b 1
