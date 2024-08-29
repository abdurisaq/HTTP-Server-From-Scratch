@echo off
SETLOCAL ENABLEDELAYEDEXPANSION
set baseIP = "192.168.1"
set outputFile = "ipaddress.txt"



if exist outputFile (
    echo. > %outputFile%
)


FOR /L %%A IN (0,1,255) DO (
  ECHO %%A
  SET "fullIP=%baseIP%.%%A"


  !fullIP!
)









:ping_ip
SETLOCAL
SET ip = %1
ping -n 1 -w 1000 %ip% >nul 2>&1
if %errorlevel% equ 0 (
    echo %ip% >> %outputFile%
)
ENDLOCAL
