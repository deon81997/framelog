@echo off
start /w D:\FRAMELOG.EXE /COM:1 /LOG /Q D:\G2TEST.EXE nowin 150 50
copy D:\FRAMELOG.LOG D:\FL1.LOG > nul
start /w D:\FRAMELOG.EXE /COM:1 /LOG /Q /NOINJECT D:\DDTEST.EXE 150 50
copy D:\FRAMELOG.LOG D:\FL2.LOG > nul
start D:\FRAMELOG.EXE /COM:1 /LOG /Q
choice /t:y,8 /n > nul
start /w D:\G2TEST.EXE nowin 100 50
start /w D:\GLTEST.EXE 150 50
choice /t:y,3 /n > nul
D:\FRAMELOG.EXE /STOP
choice /t:y,5 /n > nul
C:\WINDOWS\RUNDLL32.EXE USER.EXE,ExitWindows
