/* PROBE.EXE - what Win9x allows for in-memory hooks: module bases, page protection, DDraw vtables.
   Writes PROBE.TXT next to the exe. Build: win98/makefile (i686-w64-mingw32-gcc). */
#include <windows.h>
#include <ddraw.h>
#include <stdio.h>

static FILE *o;

static void prot(const char *what, void *p)
{
    MEMORY_BASIC_INFORMATION mbi;
    DWORD old = 0, n = 0;
    BOOL vp;
    unsigned char b0, save;
    if (!p) { fprintf(o, "%s: NULL\n", what); return; }
    memset(&mbi, 0, sizeof mbi);
    VirtualQuery(p, &mbi, sizeof mbi);
    b0 = *(unsigned char *)p;
    vp = VirtualProtect(p, 8, PAGE_EXECUTE_READWRITE, &old);
    fprintf(o, "%s: %p first=%02X prot=%lX type=%lX VirtualProtect=%d(err %lu) old=%lX", what, p, b0,
            mbi.Protect, mbi.Type, vp, vp ? 0 : GetLastError(), old);
    if (vp) {
        save = b0;
        *(volatile unsigned char *)p = save; /* write the same byte back */
        fprintf(o, " write=ok");
        VirtualProtect(p, 8, old, &old);
    } else {
        BOOL w = WriteProcessMemory(GetCurrentProcess(), p, &b0, 1, &n);
        fprintf(o, " WPM=%d(err %lu)", w, w ? 0 : GetLastError());
    }
    fprintf(o, "\n");
}

static void mod(const char *name)
{
    HMODULE h = LoadLibraryA(name);
    char path[MAX_PATH] = "";
    if (h) GetModuleFileNameA(h, path, sizeof path);
    fprintf(o, "module %-12s %p %s\n", name, (void *)h, path);
}

int main(void)
{
    char p[MAX_PATH], *s;
    LPDIRECTDRAW dd = NULL;
    LPDIRECTDRAWSURFACE ps = NULL;
    DDSURFACEDESC sd;
    HRESULT hr;
    int i;
    static const GUID *iid[] = { &IID_IDirectDrawSurface2, &IID_IDirectDrawSurface3, &IID_IDirectDrawSurface4,
                                 &IID_IDirectDrawSurface7 };
    OSVERSIONINFOA v;

    GetModuleFileNameA(NULL, p, sizeof p);
    s = strrchr(p, '\\');
    strcpy(s ? s + 1 : p, "PROBE.TXT");
    o = fopen(p, "w");
    if (!o) return 1;
    v.dwOSVersionInfoSize = sizeof v;
    GetVersionExA(&v);
    fprintf(o, "os %lu.%lu.%lu platform %lu %s\n", v.dwMajorVersion, v.dwMinorVersion, v.dwBuildNumber & 0xFFFF,
            v.dwPlatformId, v.szCSDVersion);
    mod("kernel32.dll"); mod("user32.dll"); mod("gdi32.dll"); mod("ddraw.dll"); mod("opengl32.dll");
    mod("glide2x.dll"); mod("glide3x.dll"); mod("d3dim.dll"); mod("d3dim700.dll");
    prot("kernel32!GetTickCount", GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetTickCount"));
    prot("gdi32!SwapBuffers", GetProcAddress(GetModuleHandleA("gdi32.dll"), "SwapBuffers"));
    prot("opengl32!wglSwapBuffers", GetProcAddress(GetModuleHandleA("opengl32.dll"), "wglSwapBuffers"));
    prot("ddraw!DirectDrawCreate", GetProcAddress(GetModuleHandleA("ddraw.dll"), "DirectDrawCreate"));
    if (GetModuleHandleA("glide2x.dll"))
        prot("glide2x!grBufferSwap", GetProcAddress(GetModuleHandleA("glide2x.dll"), "_grBufferSwap@4"));
    hr = DirectDrawCreate(NULL, &dd, NULL);
    fprintf(o, "DirectDrawCreate %08lX\n", hr);
    if (dd) {
        DDDEVICEIDENTIFIER id;
        LPDIRECTDRAW4 dd4 = NULL;
        IDirectDraw_SetCooperativeLevel(dd, NULL, DDSCL_NORMAL);
        if (IDirectDraw_QueryInterface(dd, &IID_IDirectDraw4, (void **)&dd4) == 0) {
            memset(&id, 0, sizeof id);
            hr = IDirectDraw4_GetDeviceIdentifier(dd4, &id, 0);
            fprintf(o, "GetDeviceIdentifier %08lX '%s' '%s' ver %lu.%lu.%lu.%lu\n", hr, id.szDescription, id.szDriver,
                    HIWORD(id.liDriverVersion.HighPart), LOWORD(id.liDriverVersion.HighPart),
                    HIWORD(id.liDriverVersion.LowPart), LOWORD(id.liDriverVersion.LowPart));
            IDirectDraw4_Release(dd4);
        } else fprintf(o, "no IDirectDraw4\n");
        memset(&sd, 0, sizeof sd);
        sd.dwSize = sizeof sd;
        sd.dwFlags = DDSD_CAPS;
        sd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        hr = IDirectDraw_CreateSurface(dd, &sd, &ps, NULL);
        fprintf(o, "CreateSurface(primary) %08lX\n", hr);
        if (ps) {
            void **vt = *(void ***)ps;
            fprintf(o, "Surface1 vtbl %p Blt %p Flip %p\n", (void *)vt, vt[5], vt[11]);
            prot("Surface1 vtbl[11]", &vt[11]);
            prot("Surface1 Flip code", vt[11]);
            for (i = 0; i < 4; i++) {
                void *q = NULL;
                if (IDirectDrawSurface_QueryInterface(ps, iid[i], &q) == 0 && q) {
                    vt = *(void ***)q;
                    fprintf(o, "Surface%d vtbl %p Blt %p Flip %p\n", i == 3 ? 7 : i + 2, (void *)vt, vt[5], vt[11]);
                    ((IUnknown *)q)->lpVtbl->Release((IUnknown *)q);
                }
            }
            IDirectDrawSurface_Release(ps);
        }
        IDirectDraw_Release(dd);
    }
    fclose(o);
    return 0;
}
