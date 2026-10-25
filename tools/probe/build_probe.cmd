@echo off
setlocal
cd /d "%~dp0"

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
cl /nologo /c /O2 /GS- /Gs9999999 /W3 /std:c11 /D_CRT_SECURE_NO_WARNINGS ^
   /I"%~dp0..\..\deps\edk2\MdePkg\Include" /I"%~dp0..\..\deps\edk2\MdePkg\Include\X64" ^
   probe.c libc.c
if errorlevel 1 exit /b 1
link /nologo /SUBSYSTEM:EFI_APPLICATION /ENTRY:EfiMain /NODEFAULTLIB /MACHINE:X64 ^
     /OPT:REF /OUT:"%~dp0AD102-DUMP.EFI" probe.obj libc.obj
if errorlevel 1 exit /b 1
echo BUILD_OK: %~dp0AD102-DUMP.EFI
