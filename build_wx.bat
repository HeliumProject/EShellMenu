@echo off

setlocal
pushd

cd %~dp0\submodule\wxWidgets\build\msw
cmd.exe /c "call "%VCINSTALLDIR%"\vcvarsall.bat x86 && nmake.exe /f makefile.vc BUILD=debug DEBUG_INFO=1 UNICODE=0 %*"
cmd.exe /c "call "%VCINSTALLDIR%"\vcvarsall.bat x86 && nmake.exe /f makefile.vc BUILD=release DEBUG_INFO=1 UNICODE=0 %*"
cmd.exe /c "call "%VCINSTALLDIR%"\vcvarsall.bat x86_amd64 && nmake.exe /f makefile.vc TARGET_CPU=AMD64 BUILD=debug DEBUG_INFO=1 UNICODE=0 %*"
cmd.exe /c "call "%VCINSTALLDIR%"\vcvarsall.bat x86_amd64 && nmake.exe /f makefile.vc TARGET_CPU=AMD64 BUILD=release DEBUG_INFO=1 UNICODE=0 %*"

endlocal
popd