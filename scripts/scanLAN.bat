@echo off
set baseIP = " 
















:ping_ip
SETLOCAL
SET ip = %1
ping -n 1 -w 1000 %ip% >nul 2>&1
if %errorlevel% equ 0 (
    echo %ip% >> %outputFile%
)
ENDLOCAL
