@echo off
echo Cleaning temporary files, please wait...

rmdir /s /q .\build\android\
rmdir /s /q .\build\mingw\
rmdir /s /q .\build\win32\
mkdir .\build\android\
mkdir .\build\mingw\
mkdir .\build\win32\

echo Cleanup completed!
@REM echo. & pause
