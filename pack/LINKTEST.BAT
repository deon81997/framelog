@echo off
rem LINKTEST.BAT - is the serial cable alive? Prints every byte that arrives at 9600 baud (DOS MODE speed).
cd /d "%~dp0"
set PY=python
py -3 -c "" >nul 2>&1 && set PY=py -3
%PY% -m pip show pyserial >nul 2>&1 || %PY% -m pip install pyserial
%PY% -c "from serial.tools import list_ports; [print(p.device, ' ', p.description) for p in list_ports.comports()]"
echo.
set /p PORT=Which port (e.g. COM1)? 
echo.
echo On the game PC, in plain DOS (no FRAMELOG, no SBEMU loaded), type:
echo     MODE COM1:9600,N,8,1
echo     ECHO HELLO ^> COM1
echo HELLO showing up here = cable and ports work. Press Ctrl+C here when done.
echo.
%PY% fllog.py %PORT% --raw --baud 9600
pause
