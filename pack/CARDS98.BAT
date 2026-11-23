@echo off
rem CARDS98.BAT [QUAKE2DIR] - FRAMELOG video-card test in Windows 98: double-click it in the WIN98 folder.
rem Host PC first: HOST\CARDS.BAT, part 4. Runs DDTEST, D3DTEST, GLTEST and the Quake II software timedemo
rem (default C:\QUAKE2, q2demo1.dm2 of the demo) with FRAMELOG, then copies the logs to C:\FLRES. About 3 minutes.
set Q2=%1
if "%Q2%"=="" set Q2=C:\QUAKE2
if not exist FRAMELOG.EXE goto nodir
if not exist C:\FLRES\nul md C:\FLRES
start FRAMELOG.EXE /COM:1 /LOG /Q
choice /t:y,8 /n > nul
start /w TEST\DDTEST.EXE 600 30
choice /t:y,3 /n > nul
start /w TEST\D3DTEST.EXE hal 300 30
choice /t:y,3 /n > nul
start /w TEST\GLTEST.EXE 300 30
choice /t:y,3 /n > nul
if not exist %Q2%\QUAKE2.EXE goto noq2
cd %Q2%
start /w quake2.exe +set vid_ref soft +set logfile 2 +set vid_fullscreen 1 +set sw_mode 3 +timedemo 1 +alias fin "killserver; quit" +alias go "demomap q2demo1.dm2; set nextserver fin" +go
copy baseq2\qconsole.log C:\FLRES\Q2SOFT.LOG > nul
goto stop
:noq2
echo No %Q2%\QUAKE2.EXE: Quake II skipped (CARDS98 D:\GAMES\QUAKE2 to point at it).
:stop
choice /t:y,3 /n > nul
FRAMELOG.EXE /STOP
choice /t:y,3 /n > nul
copy FRAMELOG.LOG C:\FLRES > nul
for %%f in (DDTEST D3DTEST GLTEST) do if exist TEST\%%f.TXT copy TEST\%%f.TXT C:\FLRES > nul
for %%f in (DDTEST D3DTEST GLTEST) do if exist %%f.TXT copy %%f.TXT C:\FLRES > nul
echo Done: send the files in C:\FLRES and the host's results folder.
pause
goto end
:nodir
echo Run CARDS98.BAT from the FRAMELOG WIN98 folder (next to FRAMELOG.EXE).
pause
:end
