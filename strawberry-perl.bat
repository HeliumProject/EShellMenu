@echo off

setlocal

pushd

cd /d "%~dp0submodule\StrawberryPerl\c\bin"

"%~dp0submodule\StrawberryPerl\perl\bin\perl.exe" -I"%~dp0submodule\StrawberryPerl\site\lib" %*

popd

endlocal