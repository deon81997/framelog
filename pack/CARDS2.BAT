@ECHO OFF
REM CARDS2.BAT [DUKEDIR] [FRAMELOGDIR] - FRAMELOG video-card test, part 2: FLIP V with FRAMELOG /3DA (vsync)
REM Boot to MS-DOS (no SBEMU needed), then CARDS2. On the host PC: HOST\CARDS.BAT, part 2.
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
%FL%\FRAMELOG /3DA > C:\FLRES\LOAD2.TXT
IF ERRORLEVEL 1 GOTO loadfail
%FL%\FRAMELOG /STATUS > C:\FLRES\LOAD2S.TXT
ECHO.
ECHO Step 1 of 1: FLIP 20 35 10 V (waits for the retrace, 20 seconds)
ECHO   Wait until the host window says "waiting for step", then
IF "%FLNOPAUSE%"=="" PAUSE
%FL%\FLIP 20 35 10 V > C:\FLRES\FLIPV.TXT
%FL%\FRAMELOG /STATUS > C:\FLRES\S5.TXT
ECHO.
IF NOT EXIST C:\FLRES\S4.TXT GOTO done2
FIND "window bank switches" C:\FLRES\S4.TXT > NUL
IF ERRORLEVEL 1 GOTO done2
ECHO Duke 640x480 copied in banks on this card: reboot to MS-DOS and run CARDS3.
GOTO end
:done2
ECHO Done: send the files in C:\FLRES and the host's results folder.
GOTO end
:nofl
ECHO FRAMELOG.EXE is not in %FL%. Run: CARDS2 DUKEDIR FRAMELOGDIR
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
