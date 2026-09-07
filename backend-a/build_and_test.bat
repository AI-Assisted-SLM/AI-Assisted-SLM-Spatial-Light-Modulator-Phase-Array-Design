@echo off
setlocal
pushd "%~dp0"
where cl >nul 2>nul
if not errorlevel 1 goto compile
set "SLM_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%SLM_VSWHERE%" goto no_compiler
for /f "usebackq tokens=*" %%i in (`"%SLM_VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "SLM_VS=%%i"
if not defined SLM_VS goto no_compiler
call "%SLM_VS%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 goto fail
:compile
if not exist build mkdir build
pushd build
cl /nologo /EHsc /std:c++14 /utf-8 /W4 /WX /O2 /I"..\core" ..\core\Fft.cpp ..\core\Asm.cpp ..\core\Metrics.cpp ..\core\Axial.cpp ..\tests\CoreSmokeTests.cpp /Fe:CoreSmokeTests.exe
if errorlevel 1 goto build_fail
CoreSmokeTests.exe
if errorlevel 1 goto build_fail
cl /nologo /EHsc /std:c++14 /utf-8 /W4 /WX /O2 /I"..\core" ..\examples\BackendADemo.cpp Fft.obj Asm.obj Metrics.obj Axial.obj /Fe:BackendADemo.exe
if errorlevel 1 goto build_fail
BackendADemo.exe
if errorlevel 1 goto build_fail
popd
echo Build and tests completed successfully.
popd
exit /b 0
:build_fail
popd
:fail
echo Build or tests failed.
popd
exit /b 1
:no_compiler
echo Install Visual Studio with Desktop development with C++.
popd
exit /b 1
