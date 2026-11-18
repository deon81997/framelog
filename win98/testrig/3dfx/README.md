# 3dfx game rig (Win98, 86Box Voodoo Graphics / Voodoo2 / Voodoo3)

VMs: priavte store-w98qi-86box-v1 (Voodoo Graphics 4 MB add-on + Cirrus 5446, 3dfx 3.01.00), -v2 (Voodoo2 12 MB
add-on + Cirrus, 3.02.02), -v3 (Voodoo3 3000, 1.02.18), -vb (Voodoo Banshee, 1.04.00; 3DMark99 only). All: Quake II
3.14 demo, Expendable demo (C:\EXPEND), Incoming demo (C:\INCOMING), UT demo 338 (C:\UT, Glide), 3DMark99 (C:\3DMARK99).

    setsid nohup sh go.sh v2 TAG mgl icd soft exp inc   # one boot: each part without, then with FRAMELOG
    setsid nohup sh go.sh v2 ut ut                      # UT in its own boot (utkeys.sh types the console timedemo)
    python3 table.py /home/claude/res/v2-TAG            # own fps off / on, FRAMELOG fps over the game's window

`1:exp` = only with FRAMELOG (Expendable hangs at exit on the Voodoo Graphics). Results of 11/18: results.tgz
(res/<card>-<tag>: game logs, FRAMELOG.LOG, CSVs). Interactive (3DMark99): v3/run.sh with a GO.BAT that starts it,
then tools like /tmp/k.sh key/shot; the Result Browser does not take keys, read the score box only.
