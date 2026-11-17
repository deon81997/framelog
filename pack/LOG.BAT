@echo off
rem LOG.BAT - FRAMELOG host logger helper (Windows). Needs Python 3 + pyserial.
if "%3"=="" goto usage
py fllog.py %1 -o %2 --label %3
goto end
:usage
echo Usage: LOG PORT FILE.CSV LABEL
echo   e.g. LOG COM3 duke-d810.csv D810
echo Start it, run the game, press Ctrl+C when done. Then:
echo   py flstats.py duke-d810.csv duke-p5pe.csv --png duke.png
:end
