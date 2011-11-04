@echo off

setlocal
pushd

cd %~dp0\wxWidgets\build\msw
nmake.exe /f makefile.vc BUILD=debug DEBUG_INFO=1
nmake.exe /f makefile.vc BUILD=release DEBUG_INFO=1

endlocal
popd