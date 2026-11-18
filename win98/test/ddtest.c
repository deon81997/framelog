/* DDTEST.EXE [blt] [dyn] [modex] [N] [FPS] (modex = DDSCL_ALLOWMODEX like Quake II's ref_soft) - DirectDraw: fullscreen 640x480x8 page flips (default) or windowed Blt to the
   primary (blt), N frames at FPS. Also does non-primary Blts and (flip mode) a Blt to the primary after the first
   flip: neither may count. dyn = load DDRAW.DLL with LoadLibrary after the window is up (like Quake II's
   ref_soft); DDDYN.EXE = the same build with no DDRAW import at all. Result line in DDTEST.TXT. */
#include <ddraw.h>
#include "tpace.h"

int WINAPI WinMain(HINSTANCE hi, HINSTANCE hp, LPSTR cmd, int show)
{
    LPDIRECTDRAW dd;
    LPDIRECTDRAWSURFACE pri = NULL, back = NULL, off = NULL;
    LPDIRECTDRAWCLIPPER clip = NULL;
    DDSURFACEDESC sd;
    DDSCAPS caps;
    DDBLTFX fx;
    HRESULT hr;
    HWND w;
    int blt = strstr(cmd, "blt") != NULL, fast = strstr(cmd, "fast") != NULL, i, ok = 0; /* fast = Quake II soft: BltFast to the front buffer */
    (void)hi; (void)hp; (void)show;
    tp_args(cmd);
    w = tp_window("DDTEST", !blt, blt ? 320 : 640, blt ? 240 : 480);
#ifndef DYN
    if (strstr(cmd, "dyn")) {
#else
    {
#endif
        HMODULE m = LoadLibraryA("ddraw.dll");
        HRESULT(WINAPI * ddc)(GUID *, LPDIRECTDRAW *, IUnknown *) = m ? (void *)GetProcAddress(m, "DirectDrawCreate") : NULL;
        hr = ddc ? ddc(NULL, &dd, NULL) : E_FAIL;
    }
#ifndef DYN
    else hr = DirectDrawCreate(NULL, &dd, NULL);
#endif
    if (hr != DD_OK) { tp_log("DirectDrawCreate %08lX", hr); return 1; }
    memset(&sd, 0, sizeof sd);
    sd.dwSize = sizeof sd;
    if (!blt) {
        hr = IDirectDraw_SetCooperativeLevel(dd, w, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | (strstr(cmd, "modex") ? DDSCL_ALLOWMODEX : 0));
        if (hr == DD_OK) hr = IDirectDraw_SetDisplayMode(dd, 640, 480, 8);
        if (hr != DD_OK) { tp_log("flip: mode %08lX", hr); return 1; }
        sd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
        sd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
        sd.dwBackBufferCount = 1;
        if ((hr = IDirectDraw_CreateSurface(dd, &sd, &pri, NULL)) != DD_OK) { tp_log("flip: primary %08lX", hr); return 1; }
        caps.dwCaps = DDSCAPS_BACKBUFFER;
        IDirectDrawSurface_GetAttachedSurface(pri, &caps, &back);
    } else {
        IDirectDraw_SetCooperativeLevel(dd, w, DDSCL_NORMAL);
        sd.dwFlags = DDSD_CAPS;
        sd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        if ((hr = IDirectDraw_CreateSurface(dd, &sd, &pri, NULL)) != DD_OK) { tp_log("blt: primary %08lX", hr); return 1; }
        IDirectDraw_CreateClipper(dd, 0, &clip, NULL);
        IDirectDrawClipper_SetHWnd(clip, 0, w);
        IDirectDrawSurface_SetClipper(pri, clip);
    }
    memset(&sd, 0, sizeof sd);
    sd.dwSize = sizeof sd;
    sd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    sd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | (fast ? DDSCAPS_SYSTEMMEMORY : 0);
    sd.dwWidth = 320;
    sd.dwHeight = 240;
    IDirectDraw_CreateSurface(dd, &sd, &off, NULL);
    memset(&fx, 0, sizeof fx);
    fx.dwSize = sizeof fx;
    tp_start();
    for (i = 0; i < tp_n; i++) {
        tp_pump();
        fx.dwFillColor = i & 255;
        if (off) IDirectDrawSurface_Blt(off, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx); /* not counted */
        if (fast) {
            RECT q = {0, 0, 320, 240};
            if (back) IDirectDrawSurface_BltFast(back, 0, 0, off, &q, DDBLTFAST_WAIT); /* not counted (not primary) */
            hr = IDirectDrawSurface_BltFast(pri, 0, 0, off, &q, DDBLTFAST_WAIT);
            if (hr == DDERR_SURFACELOST) { IDirectDrawSurface_Restore(pri); continue; }
        } else if (!blt) {
            RECT q = {0, 0, 320, 240};
            if (back) IDirectDrawSurface_Blt(back, &q, off, NULL, DDBLT_WAIT, NULL);  /* not counted */
            do hr = IDirectDrawSurface_Flip(pri, NULL, 0); while (hr == DDERR_WASSTILLDRAWING); /* busy flips count once */
            if (hr == DDERR_SURFACELOST) { IDirectDrawSurface_Restore(pri); continue; }
            if (i == 5 && off) IDirectDrawSurface_Blt(pri, NULL, off, NULL, DDBLT_WAIT, NULL); /* after a flip: not counted */
        } else {
            RECT r;
            POINT p = {0, 0};
            GetClientRect(w, &r);
            ClientToScreen(w, &p);
            OffsetRect(&r, p.x, p.y);
            hr = IDirectDrawSurface_Blt(pri, &r, off, NULL, DDBLT_WAIT, NULL);
        }
        if (hr == DD_OK) ok++;
        tp_wait();
    }
    tp_log("ddtest %s%s%s frames %d ok %d fps %d spare %lu vt %p flip %p", blt ? "blt" : "flip", fast ? " fast" : "", strstr(cmd, "modex") ? " modex" : "",
           tp_n, ok, tp_fps, tp_spare, *(void **)pri, (*(void ***)pri)[11]);
    if (off) IDirectDrawSurface_Release(off);
    IDirectDrawSurface_Release(pri);
    if (!blt) IDirectDraw_RestoreDisplayMode(dd);
    IDirectDraw_Release(dd);
    DestroyWindow(w);
    return 0;
}
