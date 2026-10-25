@echo off
setlocal
cd /d "%~dp0"

rem nvcc on Windows needs cl.exe. Load a Visual Studio environment if needed.
where cl >nul 2>&1
if not errorlevel 1 goto have_cl

set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCVARS%" set "VCVARS=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCVARS%" set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCVARS%" (
  echo Visual Studio C++ build tools not found.
  exit /b 1
)
call "%VCVARS%" >nul 2>&1

:have_cl
where nvcc >nul 2>&1
if errorlevel 1 (
  echo nvcc not found. Add the CUDA toolkit bin directory to PATH.
  exit /b 1
)

nvcc -O3 -std=c++17 -arch=sm_89 accum.cu -o accum.exe
if errorlevel 1 exit /b 1
echo BUILD_OK: %~dp0accum.exe
