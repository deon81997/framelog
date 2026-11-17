# FRAMELOG DOS side (DJGPP gcc 12.2: andrewwutw/build-djgpp v3.4, export PATH=<djgpp>/bin:$PATH)
CC := i586-pc-msdosdjgpp-gcc
CFLAGS := -fcommon -O2 -march=pentium -Wall -Ilib -D__DOS__ -DNDEBUG
LIBSRC := lib/hdpmipt.c lib/untrapio.c lib/pic.c lib/serial.c lib/dpmi/dpmi.c lib/dpmi/dpmi_dj2.c lib/dpmi/dpmi_tsr.c lib/dpmi/xms.c lib/dpmi/dbgutil.c
all: output/framelog.exe output/flip.exe output/fltsr.exe output/fltest.exe
output/framelog.exe: framelog.c flstate.h lib/uart.h $(LIBSRC)
	@mkdir -p output
	$(CC) $(CFLAGS) -I. -o $@ framelog.c $(LIBSRC) -s
output/flip.exe: tools/flip.c
	@mkdir -p output
	$(CC) $(CFLAGS) -o $@ tools/flip.c -s
output/fltsr.exe: feas/fltsr.c feas/flstats.h $(LIBSRC)
	@mkdir -p output
	$(CC) $(CFLAGS) -Ifeas -o $@ feas/fltsr.c $(LIBSRC) -s
output/fltest.exe: feas/fltest.c feas/flstats.h
	@mkdir -p output
	$(CC) $(CFLAGS) -Ifeas -o $@ feas/fltest.c -s
clean:
	rm -rf output
