@echo off
choice /t:y,10 /n > nul
start /w D:\DDTEST.EXE spin 600 30
start /w D:\D3DTEST.EXE spin 100 8
start /w D:\GLTEST.EXE spin 200 20
start /w D:\G2TEST.EXE spin 600 60
start /w D:\DDTEST.EXE blt spin 300 30
start D:\FRAMELOG.EXE /COM:1 /LOG /Q
choice /t:y,10 /n > nul
start /w D:\DDTEST.EXE spin 600 30
start /w D:\D3DTEST.EXE spin 100 8
start /w D:\GLTEST.EXE spin 200 20
start /w D:\G2TEST.EXE spin 600 60
start /w D:\DDTEST.EXE blt spin 300 30
choice /t:y,3 /n > nul
D:\FRAMELOG.EXE /STOP
choice /t:y,5 /n > nul
C:\WINDOWS\RUNDLL32.EXE USER.EXE,ExitWindows
