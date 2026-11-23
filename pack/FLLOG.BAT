@echo off
rem FLLOG.BAT - double-click launcher for fllog.py (FRAMELOG host logger). Keep it in the HOST folder.
cd /d "%~dp0"
set PY=python
py -3 -c "" >nul 2>&1 && set PY=py -3
%PY% -c "" >nul 2>&1 || (echo Python 3 is not installed: get it from https://www.python.org/ ^(tick "Add to PATH"^). & pause & exit /b)
%PY% -m pip show pyserial >nul 2>&1 || %PY% -m pip install pyserial matplotlib
%PY% -c "from serial.tools import list_ports; [print(p.device, ' ', p.description) for p in list_ports.comports()]"
echo.
set /p PORT=Which port (e.g. COM3)? 
set /p NAME=Name for this run (e.g. quake2-voodoo2)? 
echo.
echo Logging to %NAME%.csv - now start the game on the retro PC. Press Ctrl+C here when it is done.
%PY% fllog.py %PORT% -o "%NAME%.csv" --label "%NAME%"
set RC=%ERRORLEVEL%
echo.
if "%RC%"=="2" echo The port did not open - see the PORT BUSY / NO SUCH PORT line above.& goto end
if "%RC%"=="3" echo Not one byte came from the game PC: the cable or a COM port is the problem, not the game.& echo Run LINKTEST.BAT and see HOST.TXT, section "No bytes".& goto end
if "%RC%"=="1" echo Bytes came but no frames: check the baud rate - see the line above.& goto end
if exist "%NAME%.csv" (echo Saved: %CD%\%NAME%.csv & %PY% flstats.py "%NAME%.csv")
echo Double-click REPORT.BAT to turn all the CSVs here into web pages.
:end
pause
