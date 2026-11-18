# FRAMELOG Win98: make (Ubuntu: apt install g++-mingw-w64-i686-win32 = msvcrt), then make check (Win98 imports)
CC = i686-w64-mingw32-gcc
CFLAGS = -O2 -Wall -Wno-unused-function -march=pentium -mtune=pentium2 -fno-asynchronous-unwind-tables
LDFLAGS = -s -static-libgcc
T = test/

all: FRAMELOG.EXE FLHOOK.DLL

FRAMELOG.EXE: framelog.c flshared.h
	$(CC) $(CFLAGS) -o $@ framelog.c $(LDFLAGS) -lddraw -ldxguid
FLHOOK.DLL: flhook.c lde.c flshared.h lde.h
	$(CC) $(CFLAGS) -shared -o $@ flhook.c lde.c $(LDFLAGS) -Wl,--image-base,0x67800000 -ldxguid -lgdi32

tests: $(T)PROBE.EXE $(T)DDTEST.EXE $(T)GLTEST.EXE $(T)D3DTEST.EXE $(T)G2TEST.EXE $(T)D3D8TEST.EXE $(T)D3D9TEST.EXE $(T)glide2x.dll
$(T)PROBE.EXE: $(T)probe.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) -lddraw -ldxguid
$(T)DDTEST.EXE: $(T)ddtest.c $(T)tpace.h
	$(CC) $(CFLAGS) -mwindows -o $@ $< $(LDFLAGS) -lddraw -ldxguid
$(T)D3DTEST.EXE: $(T)d3dtest.c $(T)tpace.h
	$(CC) $(CFLAGS) -mwindows -o $@ $< $(LDFLAGS) -lddraw -ldxguid
$(T)GLTEST.EXE: $(T)gltest.c $(T)tpace.h
	$(CC) $(CFLAGS) -mwindows -o $@ $< $(LDFLAGS) -lopengl32 -lgdi32
$(T)G2TEST.EXE: $(T)g2test.c $(T)tpace.h
	$(CC) $(CFLAGS) -mwindows -o $@ $< $(LDFLAGS)
$(T)D3D8TEST.EXE: $(T)d3dxtest.c $(T)tpace.h
	$(CC) $(CFLAGS) -mwindows -o $@ $< $(LDFLAGS)
$(T)D3D9TEST.EXE: $(T)d3dxtest.c $(T)tpace.h
	$(CC) $(CFLAGS) -DV9 -mwindows -o $@ $< $(LDFLAGS)
$(T)glide2x.dll: $(T)fakeglide.c $(T)fakeglide.def
	$(CC) $(CFLAGS) -shared -o $@ $< $(T)fakeglide.def $(LDFLAGS)

check: all tests
	python3 tools/chkimp98.py FRAMELOG.EXE FLHOOK.DLL $(T)*.EXE $(T)glide2x.dll

clean:
	rm -f *.EXE *.DLL $(T)*.EXE $(T)*.dll
