@ECHO OFF
REM GAME.BAT duke|heretic|rott [nofl] - SC55 (SBEMU + SC-55), FRAMELOG on COM1 (unless nofl), then the game in its
REM attract mode (title demos loop; the rig kills the VM). Games in C:\G\<name>.
C:
CD \NUKEDSB
CALL SETUP C > NUL
CALL C:\SC55\SC55.BAT > C:\SBOUT.TXT
IF NOT "%2"=="nofl" C:\FRAMELOG.EXE > C:\FLOUT.TXT
C:
IF NOT "%1"=="flip" CD \G\%1
IF "%1"=="duke" DUKE3D.EXE
IF "%1"=="heretic" HERETIC.EXE
IF "%1"=="rott" ROTT.EXE
IF "%1"=="flip" C:\FLIP.EXE 40 70 10 M
IF "%1"=="flip" C:\FRAMELOG.EXE /STATUS > C:\FLST.TXT
ECHO DONE FAIL game exited >> C:\RESULT.LOG
