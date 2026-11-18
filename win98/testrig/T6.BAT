@echo off
start D:\FRAMELOG.EXE /COM:1 /LOG /Q
choice /t:y,6 /n > nul
start D:\GLTEST.EXE 1500 50
start D:\DDTEST.EXE blt 1500 50
start D:\G2TEST.EXE 1500 50
choice /t:y,10 /n > nul
D:\FRAMELOG.EXE /STOP
choice /t:y,40 /n > nul
C:\WINDOWS\RUNDLL32.EXE USER.EXE,ExitWindows
