@echo off
rem CARDS.BAT - host side of the FRAMELOG video-card test (retro PC: FRAMELOG\CARDS1/2/3.BAT, WIN98\CARDS98.BAT).
rem Keep it in the HOST folder. CSVs go to results\<PC name>\. One window per part; it waits for each step itself.
cd /d "%~dp0"
set PY=python
py -3 -c "" >nul 2>&1 && set PY=py -3
%PY% -c "" >nul 2>&1 || (echo Python 3 is not installed: get it from https://www.python.org/ ^(tick "Add to PATH"^). & pause & exit /b)
%PY% -m pip show pyserial >nul 2>&1 || %PY% -m pip install pyserial matplotlib
%PY% -c "from serial.tools import list_ports; [print(p.device, ' ', p.description) for p in list_ports.comports()]"
echo.
set /p PORT=Which port (e.g. COM3)? 
set /p PC=Which retro PC (e.g. d810 or p5pe)? 
if not exist results\%PC%\nul mkdir results\%PC%
:menu
echo.
echo   1 = DOS part 1 (retro PC: CARDS1)    2 = DOS part 2 (CARDS2)
echo   3 = DOS part 3 (CARDS3, only if CARDS2 asks for it)
echo   4 = Windows 98 (retro PC: double-click WIN98\CARDS98.BAT)    Q = quit
set P=
set /p P=Part? 
if /i "%P%"=="q" goto end
if "%P%"=="1" for %%s in (flip flipb duke-320x200 duke-640x480) do call :one %%s
if "%P%"=="2" for %%s in (flipv-3da) do call :one %%s
if "%P%"=="3" for %%s in (duke-640x480-bank flipk-bank) do call :one %%s
if "%P%"=="4" call :w98
goto menu
:one
echo.
echo ===== waiting for step "%1": now press a key on the retro PC =====
%PY% fllog.py %PORT% -o "results\%PC%\%PC%-%1.csv" --label "%PC%-%1" --idle 15
if errorlevel 2 echo The port did not open or no byte came: run LINKTEST.BAT, see HOST.TXT "No bytes".& pause
if exist "results\%PC%\%PC%-%1.csv" %PY% flstats.py "results\%PC%\%PC%-%1.csv"
exit /b
:w98
(echo [suite]& echo name = %PC%-win98& echo gap = 0& echo min_frames = 30& echo [r1]& echo [r2]& echo [r3]& echo [r4]& echo [r5]& echo [r6]) > results\%PC%\cards98.ini
echo.
echo ===== logging Windows 98: double-click WIN98\CARDS98.BAT on the retro PC; Ctrl+C here when it says Done =====
%PY% fllog.py %PORT% --suite results\%PC%\cards98.ini -o results\%PC%
exit /b
:end
echo CSVs are in %CD%\results\%PC%. Double-click REPORT.BAT for web pages.
pause
