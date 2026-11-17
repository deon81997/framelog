/* G2TEST.EXE [N] [FPS] - Glide 2: grSstWinOpen 640x480, grBufferClear + grBufferSwap(1), N frames at FPS.
   Loads glide2x.dll at run time (real 3dfx driver, or test/fakeglide.c's stand-in). */
#include "tpace.h"

int WINAPI WinMain(HINSTANCE hi, HINSTANCE hp, LPSTR cmd, int show)
{
    HMODULE g = LoadLibraryA("glide2x.dll");
    HWND w;
    int i;
    void(WINAPI * init)(void), (WINAPI * shut)(void), (WINAPI * swap)(DWORD), (WINAPI * clr)(DWORD, DWORD, DWORD);
    int(WINAPI * qh)(void *), (WINAPI * open)(DWORD, int, int, int, int, int, int);
    void(WINAPI * sel)(int);
    DWORD hw[64];
    (void)hi; (void)hp; (void)show;
    tp_args(cmd);
    if (!g) { tp_log("g2test: no glide2x.dll"); return 1; }
    init = (void *)GetProcAddress(g, "_grGlideInit@0");
    shut = (void *)GetProcAddress(g, "_grGlideShutdown@0");
    swap = (void *)GetProcAddress(g, "_grBufferSwap@4");
    clr = (void *)GetProcAddress(g, "_grBufferClear@12");
    qh = (void *)GetProcAddress(g, "_grSstQueryHardware@4");
    sel = (void *)GetProcAddress(g, "_grSstSelect@4");
    open = (void *)GetProcAddress(g, "_grSstWinOpen@28");
    if (!init || !swap || !open || !qh || !sel || !clr || !shut) { tp_log("g2test: missing exports"); return 1; }
    w = tp_window("G2TEST", 1, 640, 480);
    init();
    if (!qh(hw)) { tp_log("g2test: no 3dfx hardware"); return 1; }
    sel(0);
    if (!open((DWORD)w, 7 /* 640x480 */, 0 /* 60 Hz */, 0 /* ARGB */, 0 /* upper left */, 2, 1)) {
        tp_log("g2test: grSstWinOpen failed");
        return 1;
    }
    tp_start();
    for (i = 0; i < tp_n; i++) {
        tp_pump();
        clr(0x00FF0000u + (i & 255), 0, 0xFFFF);
        swap(1);
        tp_wait();
    }
    shut();
    tp_log("g2test frames %d fps %d spare %lu", tp_n, tp_fps, tp_spare);
    DestroyWindow(w);
    return 0;
}
