@echo off
start D:\FRAMELOG.EXE /COM:1 /LOG /Q
choice /t:y,6 /n > nul
start /w D:\GLTEST.EXE 1500 50
D:\FRAMELOG.EXE /STOP
choice /t:y,4 /n > nul
C:\WINDOWS\RUNDLL32.EXE USER.EXE,ExitWindows
