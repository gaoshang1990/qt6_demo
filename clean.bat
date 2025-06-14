@echo off
echo 正在清除文件下临时文件，请稍等......
del /f /s /q .\*.d
del /f /s /q .\*.obj
del /f /s /q .\*.o
del /f /s /q .\*.pdb
del /f /s /q .\*.tlog
del /f /s /q .\*.VC.db
del /f /s /q .\*CMakeCache.txt
echo 清除系统完成！
@REM echo. & pause 