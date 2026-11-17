@echo off
start D:\FRAMELOG.EXE /COM:1 /LOG /Q
choice /t:y,8 /n > nul
start /w D:\DDTEST.EXE 100 50
start /w D:\DDTEST.EXE blt 100 50
D:\FRAMELOG.EXE /STOP
choice /t:y,4 /n > nul
C:\WINDOWS\RUNDLL32.EXE USER.EXE,ExitWindows
