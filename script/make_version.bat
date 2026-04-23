REM Description: Generate version.h file for GD32F470 project

@echo off
setlocal enabledelayedexpansion

REM 检查并设置输出路径
if "%~1"=="" (
    set "OUTPUT_PATH=%~dp0\"
) else (
    set "OUTPUT_PATH=%~1\"
)

REM 获取git信息
for /f %%i in ('git rev-list --count HEAD') do set COMMIT_COUNT=%%i
for /f %%i in ('git rev-parse --short HEAD') do set COMMIT_HASH=%%i

REM 计算版本号
set /a MAJOR=COMMIT_COUNT/1000
set /a MINOR=COMMIT_COUNT%%1000/100
set /a PATCH=COMMIT_COUNT%%100

REM 生成version.h
(
echo #ifndef _VERSION_H___
echo #define _VERSION_H___
echo.
echo #define APP_VERSION      "%MAJOR%.%MINOR%.%PATCH%"
echo #define APP_FULL_VERSION "%MAJOR%.%MINOR%.%PATCH%.%COMMIT_HASH%"
echo #define GIT_COMMIT_COUNT "%COMMIT_COUNT%"
echo #define GIT_COMMIT_HASH  "%COMMIT_HASH%"
echo.
echo #endif /* _VERSION_H___ */
) > "%OUTPUT_PATH%\version.h"

echo Version file generated successfully!
