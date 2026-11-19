@echo off
rem REPORT.BAT - double-click: the Retro GPU Shootout web pages from every CSV in this folder (and below).
cd /d "%~dp0"
set PY=python
py -3 -c "" >nul 2>&1 && set PY=py -3
%PY% -c "" >nul 2>&1 || (echo Python 3 is not installed: get it from https://www.python.org/ ^(tick "Add to PATH"^). & pause & exit /b)
set D3=
set /p ANS=Add 90s 3D charts too (y/n)? 
if /i "%ANS%"=="y" set D3=--3d
%PY% flreport.py . -o report %D3%
if exist report\index.html (start "" report\index.html) else (echo No report - are there CSVs from FLLOG.BAT in this folder? & pause)
