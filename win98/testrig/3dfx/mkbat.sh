#!/bin/sh
# mkbat.sh CARD PARTS... > GO.BAT - Win98 game batch for the 3dfx VMs (v1/v2/v3): each part runs once without and once
# with FRAMELOG (resident, D:\FRAMELOG.EXE). Parts: mgl icd soft exp inc ut. Results on D: (Q2*.LOG, EXP*.TXT, INC*.TXT, UT*.LOG).
# A part written 1:NAME runs only with FRAMELOG (a game that hangs at exit goes last: 1:exp on Voodoo1).
# exp needs autook.sh (Enter on its timedemo box), ut needs utkeys.sh (console timedemo + exit) running on the host.
# CARD: v1|v2 = add-on (D3D = secondary device, ICD = standalone 3dfxogl/3dfxvgl), v3 = Voodoo3 (registered ICD).
C=$1; shift
case $C in v1) ICD=3dfxogl SEC=-secondary;; v2) ICD=3dfxvgl SEC=-secondary;; *) ICD=opengl32 SEC=-primary;; esac
Q='+set logfile 2 +set vid_fullscreen 1 +set gl_mode 3 +set sw_mode 3 +timedemo 1 +alias fin "killserver; quit" +alias go "demomap q2demo1.dm2; set nextserver fin" +go'
part() { # $1 part $2 suffix
  case $1 in
    mgl|icd|soft) d=3dfxgl; [ $1 = icd ] && d=$ICD; r=gl; [ $1 = soft ] && r=soft
      echo "cd \\QUAKE2"; echo "start /w quake2.exe +set vid_ref $r +set gl_driver $d $Q"; echo "copy baseq2\\qconsole.log D:\\Q2$1$2.LOG > nul";;
    exp) echo "cd \\EXPEND"; echo "if exist timedemo.txt del timedemo.txt"; echo "start /w go.exe -timedemo"; echo "copy timedemo.txt D:\\EXP$2.TXT > nul";;
    ut) echo "cd \\UT\\System"; echo "start /w UnrealTournament.exe UT-Logo-Map.unr -nosound"; echo "copy UnrealTournament.log D:\\UT$2.LOG > nul";;
    inc) echo "cd \\INCOMING"; echo "if exist fps.txt del fps.txt"; echo "start /w gameindex.exe $SEC -nosound"; echo "copy fps.txt D:\\INC$2.TXT > nul";;
  esac; }
{ echo "@echo off"; echo "C:"
  for p in "$@"; do case $p in 1:*) ;; *) part $p 0;; esac; done
  echo "start D:\\FRAMELOG.EXE /COM:1 /LOG /Q"; echo "choice /t:y,8 /n > nul"
  for p in "$@"; do part ${p#1:} 1; done
  echo "choice /t:y,3 /n > nul"; echo "D:\\FRAMELOG.EXE /STOP"; echo "choice /t:y,5 /n > nul"
  echo "C:\\WINDOWS\\RUNDLL32.EXE USER.EXE,ExitWindows"; } | sed 's/$/\r/'
