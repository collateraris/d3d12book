@ECHO OFF

PUSHD %~dp0
REM 

SET CMAKE_GENERATOR="Visual Studio 16 2019" -A x64
SET CMAKE_BINARY_DIR=build_vs2019

MKDIR %CMAKE_BINARY_DIR% 2>NUL
PUSHD %CMAKE_BINARY_DIR%

cmake -G %CMAKE_GENERATOR% -Wno-dev "%~dp0"

POPD
POPD

PAUSE