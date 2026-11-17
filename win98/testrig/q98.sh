#!/bin/sh
# q98.sh GO.BAT [FILE...] - boot priavte store-w98qi-qemu (acpi.img copy) in QEMU, Win98 runs D:\GO.BAT at login
# (WIN.INI run=), GO.BAT ends with a shutdown (RUNDLL32 USER.EXE,ExitWindows) -> QEMU exits. FILEs go to D:\.
# Afterwards D:\ is copied out to $OUT (default /tmp/q98out). COM1 = TCP server 127.0.0.1:${PORT:-4598}
# (fllog.py socket://127.0.0.1:4598). env: VM (image, default vms/w98qemu/acpi.img), T (timeout s, default 600),
#   VGA (std|cirrus, default std), DISP (none), KEEP=1 (keep the work copies in /tmp/q98).
set -e
V=${VMS:-/home/claude/vms}; W=/tmp/q98; OUT=${OUT:-/tmp/q98out}
mkdir -p $W; rm -rf "$OUT"; mkdir -p "$OUT"
cp --sparse=always ${VM:-$V/w98qemu/acpi.img} $W/c.img
cp --sparse=always $V/shared/test.img $W/d.img
C="-i $W/c.img@@32256"; D="-i $W/d.img@@32256"
mtype $C ::/WINDOWS/WIN.INI | sed 's/^run=.*/run=D:\\GO.BAT/' | sed 's/$/\r/;s/\r\r$/\r/' > $W/win.ini
mcopy -o $C $W/win.ini ::/WINDOWS/WIN.INI
mdel $D '::/*' 2>/dev/null || true
for f in "$@"; do mcopy -o $D "$f" ::/; done
timeout ${T:-600} qemu-system-i386 -M pc -cpu pentium3 -m 256 -rtc base=localtime \
  -drive file=$W/c.img,format=raw,if=ide,index=0,cache=unsafe -drive file=$W/d.img,format=raw,if=ide,index=1,cache=unsafe \
  -boot c -vga ${VGA:-std} -nic none -usb -device usb-tablet -display ${DISP:-none} -no-reboot \
  -chardev socket,id=c1,host=127.0.0.1,port=${PORT:-4598},server=on,wait=off -serial chardev:c1 \
  -monitor unix:/tmp/q98.sock,server,nowait || echo "q98: qemu exit $? (124 = timeout)"
mcopy -s -n $D '::/*' "$OUT/" 2>/dev/null || true
[ -n "$KEEP" ] || rm -f $W/c.img $W/d.img
ls "$OUT"
