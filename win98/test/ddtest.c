/* DDTEST.EXE [blt] [N] [FPS] - DirectDraw: fullscreen 640x480x8 page flips (default) or windowed Blt to the
   primary (blt), N frames at FPS. Also does non-primary Blts and (flip mode) a Blt to the primary after the first
   flip: neither may count. Result line in DDTEST.TXT. */
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
    int blt = strstr(cmd, "blt") != NULL, i, ok = 0;
    (void)hi; (void)hp; (void)show;
    tp_args(cmd);
    w = tp_window("DDTEST", !blt, blt ? 320 : 640, blt ? 240 : 480);
    if ((hr = DirectDrawCreate(NULL, &dd, NULL)) != DD_OK) { tp_log("DirectDrawCreate %08lX", hr); return 1; }
    memset(&sd, 0, sizeof sd);
    sd.dwSize = sizeof sd;
    if (!blt) {
        hr = IDirectDraw_SetCooperativeLevel(dd, w, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
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
    sd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
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
        if (!blt) {
            if (back) IDirectDrawSurface_Blt(back, NULL, off, NULL, DDBLT_WAIT, NULL);  /* not counted */
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
    tp_log("ddtest %s frames %d ok %d fps %d spare %lu", blt ? "blt" : "flip", tp_n, ok, tp_fps, tp_spare);
    if (off) IDirectDrawSurface_Release(off);
    IDirectDrawSurface_Release(pri);
    if (!blt) IDirectDraw_RestoreDisplayMode(dd);
    IDirectDraw_Release(dd);
    DestroyWindow(w);
    return 0;
}
