/* D3D8TEST.EXE / D3D9TEST.EXE [win] [sc] [N] [FPS] - Direct3D 8 / 9 (built twice, -DV9 = 9): HAL device, fullscreen
   640x480 (16-bit, else 32) or "win" = a 640x480 window; per frame Clear + BeginScene/EndScene + Present
   ("sc" = D3D9 only: IDirect3DSwapChain9::Present). d3d8/9.dll are loaded with LoadLibrary (no static import:
   the exe still starts without DirectX 8/9 and logs why). N frames at FPS. Result line in D3D8TEST.TXT / D3D9TEST.TXT. */
#define COBJMACROS
#ifdef V9
#include <d3d9.h>
#define NAME "d3d9test"
#define DLL "d3d9.dll"
#define CREATE "Direct3DCreate9"
#define SDKVER 32
typedef IDirect3D9 D3D;
typedef IDirect3DDevice9 DEV;
typedef D3DPRESENT_PARAMETERS PP;
#define D3D_CheckDeviceType(d, f, w) IDirect3D9_CheckDeviceType(d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, f, f, w)
#define D3D_CreateDevice IDirect3D9_CreateDevice
#define D3D_Release IDirect3D9_Release
#define DEV_Clear IDirect3DDevice9_Clear
#define DEV_BeginScene IDirect3DDevice9_BeginScene
#define DEV_EndScene IDirect3DDevice9_EndScene
#define DEV_Present IDirect3DDevice9_Present
#define DEV_TestCoop IDirect3DDevice9_TestCooperativeLevel
#define DEV_Release IDirect3DDevice9_Release
#else
#include <d3d8.h>
#define NAME "d3d8test"
#define DLL "d3d8.dll"
#define CREATE "Direct3DCreate8"
#define SDKVER 220
typedef IDirect3D8 D3D;
typedef IDirect3DDevice8 DEV;
typedef D3DPRESENT_PARAMETERS PP;
#define D3D_CheckDeviceType(d, f, w) IDirect3D8_CheckDeviceType(d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, f, f, w)
#define D3D_CreateDevice IDirect3D8_CreateDevice
#define D3D_Release IDirect3D8_Release
#define DEV_Clear IDirect3DDevice8_Clear
#define DEV_BeginScene IDirect3DDevice8_BeginScene
#define DEV_EndScene IDirect3DDevice8_EndScene
#define DEV_Present IDirect3DDevice8_Present
#define DEV_TestCoop IDirect3DDevice8_TestCooperativeLevel
#define DEV_Release IDirect3DDevice8_Release
#endif
#include "tpace.h"

int WINAPI WinMain(HINSTANCE hi, HINSTANCE hp, LPSTR cmd, int show)
{
    HMODULE m;
    D3D *(WINAPI * create)(UINT);
    D3D *d3d;
    DEV *dev = NULL;
    PP pp;
    HRESULT hr = -1;
    HWND w;
    int i, ok = 0, win = strstr(cmd, "win") != NULL, sc = 0;
    static const D3DFORMAT fmts[] = {D3DFMT_R5G6B5, D3DFMT_X1R5G5B5, D3DFMT_X8R8G8B8};
#ifdef V9
    IDirect3DSwapChain9 *chain = NULL;
    sc = strstr(cmd, "sc") != NULL;
#endif
    (void)hi; (void)hp; (void)show;
    tp_args(cmd);
    w = tp_window(NAME, !win, 640, 480);
    if (!(m = LoadLibraryA(DLL)) || !(create = (void *)GetProcAddress(m, CREATE))) { tp_log(NAME ": no " DLL); return 1; }
    if (!(d3d = create(SDKVER))) { tp_log(NAME ": " CREATE " failed"); return 1; }
    memset(&pp, 0, sizeof pp);
    pp.BackBufferWidth = 640;
    pp.BackBufferHeight = 480;
    pp.BackBufferCount = 1;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow = w;
    pp.Windowed = win;
    if (win) {
        D3DDISPLAYMODE dm;
        pp.BackBufferFormat = D3DFMT_UNKNOWN; /* D3D9 only; D3D8 wants the desktop format */
#ifndef V9
        if (IDirect3D8_GetAdapterDisplayMode(d3d, D3DADAPTER_DEFAULT, &dm) >= 0) pp.BackBufferFormat = dm.Format;
#else
        (void)dm;
#endif
        hr = D3D_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, w, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &dev);
    } else
        for (i = 0; i < 3 && hr < 0; i++) {
            pp.BackBufferFormat = fmts[i];
            if (D3D_CheckDeviceType(d3d, fmts[i], FALSE) < 0) continue;
            hr = D3D_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, w, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &dev);
        }
    if (hr < 0 || !dev) { tp_log(NAME ": CreateDevice %s %08lX", win ? "win" : "full", hr); D3D_Release(d3d); return 1; }
#ifdef V9
    if (sc && IDirect3DDevice9_GetSwapChain(dev, 0, &chain) < 0) { tp_log(NAME ": GetSwapChain failed"); sc = 0; }
#endif
    tp_start();
    for (i = 0; i < tp_n; i++) {
        tp_pump();
        if (DEV_TestCoop(dev) < 0) { tp_wait(); continue; }
        DEV_Clear(dev, 0, NULL, D3DCLEAR_TARGET, 0xFF000000u | (i & 255) << 8 | (i * 3 & 255), 1.0f, 0);
        if (DEV_BeginScene(dev) >= 0) DEV_EndScene(dev);
#ifdef V9
        if (sc) hr = IDirect3DSwapChain9_Present(chain, NULL, NULL, NULL, NULL, 0);
        else
#endif
            hr = DEV_Present(dev, NULL, NULL, NULL, NULL);
        if (hr >= 0) ok++;
        tp_wait();
    }
    tp_log(NAME " %s%s fmt %d frames %d ok %d fps %d spare %lu", win ? "win" : "full", sc ? " sc" : "",
           (int)pp.BackBufferFormat, tp_n, ok, tp_fps, tp_spare);
#ifdef V9
    if (chain) IDirect3DSwapChain9_Release(chain);
#endif
    DEV_Release(dev);
    D3D_Release(d3d);
    DestroyWindow(w);
    return 0;
}
