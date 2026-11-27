@echo off
setlocal

set "LO02_CMAKE=cmake.exe"
where.exe /q cmake.exe
if not errorlevel 1 goto build

set "LO02_CMAKE=%ProgramFiles%\MATLAB\R2023b\bin\win64\cmake\bin\cmake.exe"
if not exist "%LO02_CMAKE%" (
    echo CMake could not be found. Install CMake or add it to PATH.
    exit /b 1
)

:build
set "LO02_BUILD=..\..\lo02-build"
"%LO02_CMAKE%" -S . -B "%LO02_BUILD%" -G "Visual Studio 17 2022" -A x64
if errorlevel 1 exit /b 1

"%LO02_CMAKE%" --build "%LO02_BUILD%" --config Debug
if errorlevel 1 exit /b 1

"%LO02_BUILD%\Debug\lo02_flow_demo.exe"
exit /b %ERRORLEVEL%
