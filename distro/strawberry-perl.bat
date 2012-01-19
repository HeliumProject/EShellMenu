@echo off

setlocal

pushd

cd /d "%~dp0StrawberryPerl\c\bin"

"%~dp0StrawberryPerl\perl\bin\perl.exe" -I"C:\Program Files\EShell Menu\StrawberryPerl\site\lib" %*

popd

endlocal