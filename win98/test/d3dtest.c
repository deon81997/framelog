/* D3DTEST.EXE [hal] [N] [FPS] - Direct3D 6 immediate mode: fullscreen 640x480x16 flip chain, IDirect3DDevice3 on
   the back buffer (RGB software rasterizer, or the HAL with "hal"), Clear + one lit triangle per frame, Flip.
   N frames at FPS. Result line in D3DTEST.TXT. */
#define INITGUID
#include <ddraw.h>
#include <d3d.h>
#include "tpace.h"

int WINAPI WinMain(HINSTANCE hi, HINSTANCE hp, LPSTR cmd, int show)
{
    LPDIRECTDRAW dd1;
    LPDIRECTDRAW4 dd;
    LPDIRECTDRAWSURFACE4 pri = NULL, back = NULL;
    LPDIRECT3D3 d3d = NULL;
    LPDIRECT3DDEVICE3 dev = NULL;
    LPDIRECT3DVIEWPORT3 vp = NULL;
    DDSURFACEDESC2 sd;
    DDSCAPS2 caps;
    D3DVIEWPORT2 v;
    D3DTLVERTEX tri[3];
    D3DRECT r = {0, 0, 640, 480};
    HRESULT hr;
    HWND w;
    int i, ok = 0, hal = strstr(cmd, "hal") != NULL;
    (void)hi; (void)hp; (void)show;
    tp_args(cmd);
    w = tp_window("D3DTEST", 1, 640, 480);
#define CHK(x, what) if ((hr = (x)) != DD_OK) { tp_log("d3dtest: %s %08lX", what, hr); return 1; }
    CHK(DirectDrawCreate(NULL, &dd1, NULL), "DirectDrawCreate");
    CHK(IDirectDraw_QueryInterface(dd1, &IID_IDirectDraw4, (void **)&dd), "IDirectDraw4");
    CHK(IDirectDraw4_SetCooperativeLevel(dd, w, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_FPUSETUP), "coop");
    CHK(IDirectDraw4_SetDisplayMode(dd, 640, 480, 16, 0, 0), "mode 640x480x16");
    memset(&sd, 0, sizeof sd);
    sd.dwSize = sizeof sd;
    sd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    sd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_3DDEVICE |
                        (hal ? DDSCAPS_VIDEOMEMORY : DDSCAPS_SYSTEMMEMORY);
    sd.dwBackBufferCount = 1;
    hr = IDirectDraw4_CreateSurface(dd, &sd, &pri, NULL);
    if (hr != DD_OK && !hal) { /* some drivers refuse a system-memory primary: let DirectDraw choose */
        sd.ddsCaps.dwCaps &= ~DDSCAPS_SYSTEMMEMORY;
        hr = IDirectDraw4_CreateSurface(dd, &sd, &pri, NULL);
    }
    CHK(hr, "primary");
    memset(&caps, 0, sizeof caps);
    caps.dwCaps = DDSCAPS_BACKBUFFER;
    CHK(IDirectDrawSurface4_GetAttachedSurface(pri, &caps, &back), "back buffer");
    CHK(IDirectDraw4_QueryInterface(dd, &IID_IDirect3D3, (void **)&d3d), "IDirect3D3");
    CHK(IDirect3D3_CreateDevice(d3d, hal ? &IID_IDirect3DHALDevice : &IID_IDirect3DRGBDevice, back, &dev, NULL), "device");
    CHK(IDirect3D3_CreateViewport(d3d, &vp, NULL), "viewport");
    CHK(IDirect3DDevice3_AddViewport(dev, vp), "AddViewport");
    memset(&v, 0, sizeof v);
    v.dwSize = sizeof v;
    v.dwWidth = 640; v.dwHeight = 480;
    v.dvClipX = -1; v.dvClipY = 1; v.dvClipWidth = 2; v.dvClipHeight = 2; v.dvMaxZ = 1;
    CHK(IDirect3DViewport3_SetViewport2(vp, &v), "SetViewport2");
    CHK(IDirect3DDevice3_SetCurrentViewport(dev, vp), "SetCurrentViewport");
    memset(tri, 0, sizeof tri);
    for (i = 0; i < 3; i++) { tri[i].rhw = 1; tri[i].sz = 0.5f; }
    tri[0].sx = 320; tri[0].sy = 60;  tri[0].color = 0xFFFF0000;
    tri[1].sx = 560; tri[1].sy = 420; tri[1].color = 0xFF00FF00;
    tri[2].sx = 80;  tri[2].sy = 420; tri[2].color = 0xFF0000FF;
    tp_start();
    for (i = 0; i < tp_n; i++) {
        tp_pump();
        IDirect3DViewport3_Clear2(vp, 1, &r, D3DCLEAR_TARGET, 0xFF000000u | (i & 255) << 8, 1, 0);
        if (IDirect3DDevice3_BeginScene(dev) == D3D_OK) {
            tri[0].sx = 320 + (float)(i % 100) - 50;
            IDirect3DDevice3_DrawPrimitive(dev, D3DPT_TRIANGLELIST, D3DFVF_TLVERTEX, tri, 3, D3DDP_WAIT);
            IDirect3DDevice3_EndScene(dev);
        }
        hr = IDirectDrawSurface4_Flip(pri, NULL, DDFLIP_WAIT);
        if (hr == DDERR_SURFACELOST) IDirectDrawSurface4_Restore(pri);
        if (hr == DD_OK) ok++;
        tp_wait();
    }
    tp_log("d3dtest %s frames %d ok %d fps %d spare %lu", hal ? "hal" : "rgb", tp_n, ok, tp_fps, tp_spare);
    IDirect3DViewport3_Release(vp);
    IDirect3DDevice3_Release(dev);
    IDirect3D3_Release(d3d);
    IDirectDrawSurface4_Release(pri);
    IDirectDraw4_RestoreDisplayMode(dd);
    IDirectDraw4_Release(dd);
    DestroyWindow(w);
    return 0;
}
