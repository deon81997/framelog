@echo off
start D:\FRAMELOG.EXE /COM:1 /LOG /Q
choice /t:y,8 /n > nul
start /w D:\DDTEST.EXE 200 50
start /w D:\DDTEST.EXE blt 200 50
start /w D:\D3DTEST.EXE 200 40
start /w D:\GLTEST.EXE 200 50
start /w D:\GLTEST.EXE wgl 200 50
start /w D:\G2TEST.EXE 200 60
choice /t:y,3 /n > nul
D:\FRAMELOG.EXE /STOP
choice /t:y,5 /n > nul
C:\WINDOWS\RUNDLL32.EXE USER.EXE,ExitWindows
