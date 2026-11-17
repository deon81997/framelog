@ECHO OFF
REM GAME.BAT duke|heretic|rott|flip|flipv [nofl|fl] [FRAMELOG option, e.g. /3DA] - SC55 (SBEMU + SC-55), FRAMELOG on COM1 (unless nofl), then the game in its
REM attract mode (title demos loop; the rig kills the VM). Games in C:\G\<name>.
C:
CD \NUKEDSB
CALL SETUP C > NUL
CALL C:\SC55\SC55.BAT > C:\SBOUT.TXT
IF NOT "%2"=="nofl" C:\FRAMELOG.EXE %3 > C:\FLOUT.TXT
C:
IF EXIST C:\G\%1\NUL CD \G\%1
IF "%1"=="duke" DUKE3D.EXE
IF "%1"=="heretic" HERETIC.EXE
IF "%1"=="rott" ROTT.EXE
IF "%1"=="flip" C:\FLIP.EXE 40 70 10 M
IF "%1"=="flipv" C:\FLIP.EXE 40 35 10 MV
IF "%1"=="flipw" C:\FLIP.EXE 40 35 10 V
IF "%1"=="flipw" C:\FRAMELOG.EXE /STATUS > C:\FLST.TXT
IF "%1"=="flipv" C:\FRAMELOG.EXE /STATUS > C:\FLST.TXT
IF "%1"=="flip" C:\FRAMELOG.EXE /STATUS > C:\FLST.TXT
ECHO DONE FAIL game exited >> C:\RESULT.LOG
