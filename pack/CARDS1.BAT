@ECHO OFF
REM CARDS1.BAT [DUKEDIR] [FRAMELOGDIR] - FRAMELOG video-card test, part 1: FLIP, VESA FLIP, Duke 320x200 and 640x480
REM Boot to MS-DOS (no SBEMU needed), then CARDS1. On the host PC: HOST\CARDS.BAT, part 1.
REM Defaults C:\DUKE3D and C:\FRAMELOG. Results: C:\FLRES. Press a key at each step when the host says "waiting".
SET DUKE=%1
IF "%DUKE%"=="" SET DUKE=C:\DUKE3D
SET FL=%2
IF "%FL%"=="" SET FL=C:\FRAMELOG
IF NOT EXIST %FL%\FRAMELOG.EXE GOTO nofl
IF NOT EXIST C:\FLRES\NUL MD C:\FLRES
IF EXIST %FL%\HDPMI32I.EXE %FL%\HDPMI32I.EXE -r -x > NUL
IF NOT EXIST %FL%\HDPMI32I.EXE IF EXIST C:\HDPMI\HDPMI32I.EXE C:\HDPMI\HDPMI32I.EXE -r -x > NUL
IF NOT EXIST %FL%\HDPMI32I.EXE IF EXIST C:\SC55\HDPMI32I.EXE C:\SC55\HDPMI32I.EXE -r -x > NUL
IF NOT EXIST %FL%\HDPMI32I.EXE IF EXIST C:\NUKEDSB\HDPMI32I.EXE C:\NUKEDSB\HDPMI32I.EXE -r -x > NUL
%FL%\FRAMELOG  > C:\FLRES\LOAD.TXT
IF ERRORLEVEL 1 GOTO loadfail
%FL%\FRAMELOG /STATUS > C:\FLRES\LOADS.TXT
ECHO.
ECHO Step 1 of 4: FLIP 20 70 (20 seconds)
ECHO   Wait until the host window says "waiting for step", then
IF "%FLNOPAUSE%"=="" PAUSE
%FL%\FLIP 20 70 > C:\FLRES\FLIP.TXT
%FL%\FRAMELOG /STATUS > C:\FLRES\S1.TXT
ECHO.
ECHO Step 2 of 4: FLIP 20 70 10 B (VESA, 20 seconds)
ECHO   Wait until the host window says "waiting for step", then
IF "%FLNOPAUSE%"=="" PAUSE
%FL%\FLIP 20 70 10 B > C:\FLRES\FLIPB.TXT
%FL%\FRAMELOG /STATUS > C:\FLRES\S2.TXT
IF NOT EXIST %DUKE%\DUKE3D.EXE GOTO noduke
ECHO.
ECHO Step 3 of 4: Duke 3D 320x200 (ScreenMode 0). It plays its demo: after about a minute press Esc, Q, Y.
ECHO   Wait until the host window says "waiting for step", then
IF "%FLNOPAUSE%"=="" PAUSE
%FL%\DUKECFG %DUKE% 0 320 200
C:
CD %DUKE%
IF "%FLNODUKE%"=="" DUKE3D.EXE
CD \
%FL%\DUKECFG %DUKE% /RESTORE > NUL
%FL%\FRAMELOG /STATUS > C:\FLRES\S3.TXT
ECHO.
ECHO Step 4 of 4: Duke 3D 640x480 (ScreenMode 1). It plays its demo: after about a minute press Esc, Q, Y.
ECHO   Wait until the host window says "waiting for step", then
IF "%FLNOPAUSE%"=="" PAUSE
%FL%\DUKECFG %DUKE% 1 640 480
C:
CD %DUKE%
IF "%FLNODUKE%"=="" DUKE3D.EXE
CD \
%FL%\DUKECFG %DUKE% /RESTORE > NUL
%FL%\FRAMELOG /STATUS > C:\FLRES\S4.TXT
ECHO.
ECHO Part 1 done. Reboot to MS-DOS and run CARDS2.
GOTO end
:nofl
ECHO FRAMELOG.EXE is not in %FL%. Run: CARDS1 DUKEDIR FRAMELOGDIR
GOTO end
:loadfail
TYPE C:\FLRES\LOAD.TXT
ECHO FRAMELOG did not load. If it says HDPMI32i: copy HDPMI32I.EXE (from the
ECHO NUKED SB-EMU pack or the HX package) into %FL% and run this again.
GOTO end
:noduke
ECHO No %DUKE%\DUKE3D.EXE: Duke steps skipped. Run with the Duke folder, e.g.
ECHO   CARDS1 D:\GAMES\DUKE3D
:end
