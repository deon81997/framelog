/* FLHOOK.DLL - FRAMELOG Win98 hook DLL. Loaded into every GUI process by FRAMELOG.EXE's global hooks.
   - DirectDraw Flip / Blt-to-primary: FRAMELOG.EXE points the surface vtables (shared on Win9x) at stubs in
     shared memory; a stub jumps to this process's wrapper (procs table) or straight to the original.
   - Glide grBufferSwap (glide2x/glide3x), OpenGL wglSwapBuffers (opengl32 / 3dfx MiniGL / any DLL exporting it):
     5-byte JMP patched in this process's copy of the code (private arena, copy-on-write).
   - gdi32 SwapBuffers: gdi32 is shared + read-only on Win9x, so the importers' IAT entries are patched.
   Per frame: RDTSC + a push to the shared ring; FRAMELOG.EXE's worker sends it out on COM. */
#include <windows.h>
#include <tlhelp32.h>
#include <ddraw.h>
#include <stdio.h>
#include <string.h>
#include "flshared.h"
#include "lde.h"

static FLSHM *shm;
static HANDLE hmap;
static HINSTANCE g_inst;
static DWORD g_pid, g_key;
static int g_slot = -1;
static volatile int g_flipseen;
static volatile DWORD g_claimed;   /* this process owns the log */
static DWORD g_lastscan, g_lastfull;
static volatile LONG g_scanning;
static DWORD g_swaptid;            /* nesting guard: SwapBuffers -> wglSwapBuffers */

static void diag(const char *fmt, ...)
{
    va_list ap;
    LONG i;
    char *d;
    if (!shm) return;
    i = InterlockedIncrement((LONG *)&shm->diag_head) - 1;
    d = shm->diag[i % FL_DIAG];
    va_start(ap, fmt);
    d[0] = 0;
    _vsnprintf(d + 1, sizeof shm->diag[0] - 2, fmt, ap);
    d[sizeof shm->diag[0] - 1] = 0;
    va_end(ap);
    FL_BARRIER();
    d[0] = 1; /* ready */
}

/* ---------- frame events ---------- */

static void meta_add(const char *k, const char *v)
{
    size_t n = strlen(shm->meta);
    char line[160];
    char *p;
    if (!v || !*v) return;
    _snprintf(line, sizeof line - 1, "%s=%s\n", k, v);
    line[sizeof line - 1] = 0;
    for (p = line + strlen(k) + 1; *p && *p != '\n'; p++)
        if ((unsigned char)*p < 32) *p = ' ';
    if (n + strlen(line) < sizeof shm->meta) strcpy(shm->meta + n, line);
}

static void meta_res(int w, int h, int bpp)
{
    char b[32];
    if (w > 0 && h > 0) { sprintf(b, "%dx%d", w, h); meta_add("res", b); }
    if (bpp > 0) { sprintf(b, "%d", bpp); meta_add("depth", b); }
}

/* returns 1 if this event was counted; *claim = 1 when this call made us the owner (fill meta, then meta_ready) */
static int fl_event(unsigned __int64 t, DWORD api, int *claim)
{
    DWORD h, own;
    *claim = 0;
    if (!shm || shm->quitting) return 0;
    if (!g_claimed) {
        own = shm->owner_pid;
        if (own || g_pid == shm->loader_pid) return 0;
        if (shm->only_pid && shm->only_pid != g_pid) return 0;
        if (fl_cas(&shm->owner_pid, 0, g_pid) != 0) return 0;
        g_claimed = 1;
        shm->meta[0] = 0;
        shm->api = 0;
        shm->frames = 0;
        *claim = 1;
        {
            char p[MAX_PATH], *s;
            GetModuleFileNameA(NULL, p, sizeof p);
            s = strrchr(p, '\\');
            meta_add("game", s ? s + 1 : p);
        }
    } else if (shm->owner_pid != g_pid) { /* loader reset us (we were reported gone) */
        g_claimed = 0;
        return 0;
    }
    shm->api |= api;
    for (;;) {
        h = shm->head;
        if (h - shm->tail >= FL_RING) { fl_xadd(&shm->dropped, 1); return 1; }
        if (fl_cas(&shm->head, h, h + 1) == h) break;
    }
    shm->ring[h & (FL_RING - 1)].tsc = t;
    shm->ring[h & (FL_RING - 1)].api = api;
    FL_BARRIER();
    shm->ring[h & (FL_RING - 1)].seq = h + 1;
    shm->frames++;
    return 1;
}

static void meta_done(void)
{
    char b[16];
    if (GetModuleHandleA("d3dim700.dll") || GetModuleHandleA("d3dim.dll") || GetModuleHandleA("d3drm.dll"))
        meta_add("d3d", "1");
    sprintf(b, "%lu", g_pid);
    meta_add("pid", b);
    FL_BARRIER();
    shm->meta_ready = 1;
}

/* ---------- DirectDraw (vtable slots, see FRAMELOG.EXE) ---------- */

typedef HRESULT(WINAPI *FlipFn)(void *, void *, DWORD);
typedef HRESULT(WINAPI *BltFn)(void *, void *, void *, void *, DWORD, void *);
typedef HRESULT(WINAPI *CapsFn)(void *, void *);
typedef HRESULT(WINAPI *QIFn)(void *, const GUID *, void **);
typedef ULONG(WINAPI *RelFn)(void *);

static struct { void *s; int prim; } pcache[8];
static int pc_next;

static int is_primary(void *s)
{
    DWORD caps[4] = {0, 0, 0, 0};
    void **vt;
    int i;
    for (i = 0; i < 8; i++)
        if (pcache[i].s == s) return pcache[i].prim;
    vt = *(void ***)s;
    if (((CapsFn)vt[14])(s, caps) != DD_OK) caps[0] = 0;
    i = pc_next++ & 7;
    pcache[i].s = s;
    pcache[i].prim = (caps[0] & DDSCAPS_PRIMARYSURFACE) != 0;
    return pcache[i].prim;
}

static void dd_meta(void *s)
{
    DDSURFACEDESC sd;
    void **vt = *(void ***)s;
    void *s2 = NULL, *dd = NULL, *dd4 = NULL;
    /* GetSurfaceDesc = slot 22 in Surface1..3 (DDSURFACEDESC); Surface4/7 take DDSURFACEDESC2 - use a v1 surface */
    if (((QIFn)vt[0])(s, &IID_IDirectDrawSurface, &s2) == DD_OK && s2) {
        void **v2 = *(void ***)s2;
        memset(&sd, 0, sizeof sd);
        sd.dwSize = sizeof sd;
        if (((HRESULT(WINAPI *)(void *, DDSURFACEDESC *))v2[22])(s2, &sd) == DD_OK)
            meta_res(sd.dwWidth, sd.dwHeight, sd.ddpfPixelFormat.dwRGBBitCount);
        ((RelFn)v2[2])(s2);
        s2 = NULL;
    }
    /* GetDDInterface (Surface2+ slot 36) -> IDirectDraw4 -> GetDeviceIdentifier */
    if (((QIFn)vt[0])(s, &IID_IDirectDrawSurface2, &s2) == DD_OK && s2) {
        void **v2 = *(void ***)s2;
        if (((HRESULT(WINAPI *)(void *, void **))v2[36])(s2, &dd) == DD_OK && dd) {
            void **vd = *(void ***)dd;
            if (((QIFn)vd[0])(dd, &IID_IDirectDraw4, &dd4) == DD_OK && dd4) {
                DDDEVICEIDENTIFIER id;
                char b[96];
                void **v4 = *(void ***)dd4;
                memset(&id, 0, sizeof id);
                /* IDirectDraw4::GetDeviceIdentifier = slot 27 */
                if (((HRESULT(WINAPI *)(void *, DDDEVICEIDENTIFIER *, DWORD))v4[27])(dd4, &id, 0) == DD_OK) {
                    meta_add("gpu", id.szDescription);
                    if (id.szDriver[0] || id.liDriverVersion.QuadPart) sprintf(b, "%.60s %u.%u.%u.%u", id.szDriver, HIWORD(id.liDriverVersion.HighPart),
                            LOWORD(id.liDriverVersion.HighPart), HIWORD(id.liDriverVersion.LowPart),
                            LOWORD(id.liDriverVersion.LowPart));
                    else b[0] = 0;
                    meta_add("driver", b);
                }
                ((RelFn)v4[2])(dd4);
            }
            ((RelFn)vd[2])(dd);
        }
        ((RelFn)v2[2])(s2);
    }
    meta_done();
}

static void dd_frame(void *s, unsigned __int64 t, DWORD api)
{
    int claim;
    if (fl_event(t, api, &claim) && claim) {
        meta_add("api", api == FL_API_DDFLIP ? "ddraw-flip" : "ddraw-blt");
        dd_meta(s);
    }
}

#define FW(k)                                                                                                         \
    static HRESULT WINAPI fw##k(void *s, void *tg, DWORD f)                                                           \
    {                                                                                                                 \
        unsigned __int64 t = fl_rdtsc();                                                                              \
        HRESULT hr = ((FlipFn)shm->orig[k])(s, tg, f);                                                                \
        if (hr == DD_OK) { g_flipseen = 1; dd_frame(s, t, FL_API_DDFLIP); }                                           \
        return hr;                                                                                                    \
    }                                                                                                                 \
    static HRESULT WINAPI bw##k(void *s, void *r, void *src, void *sr, DWORD f, void *fx)                             \
    {                                                                                                                 \
        unsigned __int64 t = fl_rdtsc();                                                                              \
        HRESULT hr = ((BltFn)shm->orig[k])(s, r, src, sr, f, fx);                                                     \
        if (hr == DD_OK && !g_flipseen && src && is_primary(s)) dd_frame(s, t, FL_API_DDBLT);                         \
        return hr;                                                                                                    \
    }
FW(0) FW(1) FW(2) FW(3) FW(4) FW(5) FW(6) FW(7) FW(8) FW(9) FW(10) FW(11) FW(12) FW(13) FW(14) FW(15)
static void *const fwt[FL_MAXSLOT] = {fw0, fw1, fw2, fw3, fw4, fw5, fw6, fw7, fw8, fw9, fw10, fw11, fw12, fw13, fw14, fw15};
static void *const bwt[FL_MAXSLOT] = {bw0, bw1, bw2, bw3, bw4, bw5, bw6, bw7, bw8, bw9, bw10, bw11, bw12, bw13, bw14, bw15};
static void *g_jt[FL_MAXSLOT];

/* ---------- inline hooks (Glide, OpenGL) ---------- */

typedef struct {
    unsigned char *target;
    unsigned char save[16];
    int len;
    unsigned char tramp[32];
    HMODULE mod;
    const char *what;
} IHOOK;

#define MAXIH 12
static IHOOK ih[MAXIH];
static int nih;

static unsigned char *follow_jmp(unsigned char *p)
{
    /* import thunks / debug stubs: jmp [abs] or jmp rel32 into a private module */
    for (int i = 0; i < 3; i++) {
        if (p[0] == 0xFF && p[1] == 0x25) p = **(unsigned char ***)(p + 2);
        else if (p[0] == 0xE9) p = p + 5 + *(int *)(p + 1);
        else break;
    }
    return p;
}

static void *ihook(HMODULE mod, unsigned char *fn, void *wrapper, const char *what)
{
    IHOOK *h;
    DWORD old;
    int n = 0, len, rel, i;
    if (!fn || nih >= MAXIH) return NULL;
    fn = follow_jmp(fn);
    for (i = 0; i < nih; i++)
        if (ih[i].target == fn) return NULL; /* already */
    if ((DWORD)fn >= 0x80000000u) { diag("%s at %p is shared (not patched)", what, fn); return NULL; }
    h = &ih[nih];
    while (n < 5) {
        len = lde(fn + n, &rel);
        if (!len || n + len > 16) { diag("%s: unknown prologue %02X %02X %02X", what, fn[n], fn[n + 1], fn[n + 2]); return NULL; }
        if (fn[n] == 0xC3 || fn[n] == 0xC2) { diag("%s: too short", what); return NULL; }
        memcpy(h->tramp + n, fn + n, len);
        if (rel >= 0) *(int *)(h->tramp + n + rel) += (int)(fn - h->tramp);
        n += len;
    }
    h->tramp[n] = 0xE9;
    *(int *)(h->tramp + n + 1) = (int)((fn + n) - (h->tramp + n + 5));
    memcpy(h->save, fn, n);
    h->len = n;
    h->target = fn;
    h->mod = mod;
    h->what = what;
    {
        DWORD o2;
        VirtualProtect(h->tramp, sizeof h->tramp, PAGE_EXECUTE_READWRITE, &o2);
    }
    if (!VirtualProtect(fn, n, PAGE_EXECUTE_READWRITE, &old)) { diag("%s: VirtualProtect failed", what); return NULL; }
    fn[0] = 0xE9;
    *(int *)(fn + 1) = (int)((unsigned char *)wrapper - (fn + 5));
    for (i = 5; i < n; i++) fn[i] = 0xCC;
    VirtualProtect(fn, n, old, &old);
    FlushInstructionCache(GetCurrentProcess(), fn, n);
    nih++;
    diag("%08lX: hooked %s at %p", g_pid, what, fn);
    return h->tramp;
}

static void ihook_restore_all(void)
{
    DWORD old;
    int i;
    for (i = 0; i < nih; i++) {
        if (VirtualProtect(ih[i].target, ih[i].len, PAGE_EXECUTE_READWRITE, &old)) {
            memcpy(ih[i].target, ih[i].save, ih[i].len);
            VirtualProtect(ih[i].target, ih[i].len, old, &old);
        }
    }
    nih = 0;
}

/* Glide */
typedef void(WINAPI *SwapFn)(DWORD);
static SwapFn g2swap, g3swap;
static HMODULE g2mod, g3mod;

static void glide_meta(int v3)
{
    char b[96];
    if (!v3) {
        void(WINAPI * ver)(char *) = (void(WINAPI *)(char *))GetProcAddress(g2mod, "_grGlideGetVersion@4");
        DWORD(WINAPI * sw)(void) = (DWORD(WINAPI *)(void))GetProcAddress(g2mod, "_grSstScreenWidth@0");
        DWORD(WINAPI * shh)(void) = (DWORD(WINAPI *)(void))GetProcAddress(g2mod, "_grSstScreenHeight@0");
        int(WINAPI * qh)(void *) = (int(WINAPI *)(void *))GetProcAddress(g2mod, "_grSstQueryHardware@4");
        DWORD hw[64];
        if (ver) { memset(b, 0, sizeof b); ver(b); b[79] = 0; meta_add("driver", b); }
        memset(hw, 0, sizeof hw);
        if (qh && qh(hw) && hw[0] >= 1) {
            static const char *const ty[] = {"3dfx Voodoo Graphics", "3dfx Voodoo Rush", "Alliance AT3D", "3dfx Voodoo2",
                                             "3dfx Voodoo Banshee/3"};
            meta_add("gpu", hw[1] < 5 ? ty[hw[1]] : "3dfx (Glide2)");
        } else meta_add("gpu", "3dfx (Glide2)");
        if (sw && shh) {
            DWORD w = sw(), hh = shh();
            if (w > 0 && w <= 4096 && hh > 0 && hh <= 4096) meta_res(w, hh, 16);
            else meta_res(0, 0, 16);
        }
        meta_add("api", "glide2");
    } else {
        const char *(WINAPI * gs)(DWORD) = (const char *(WINAPI *)(DWORD))GetProcAddress(g3mod, "_grGetString@4");
        if (gs) {
            const char *hwn = gs(0xA1), *ver = gs(0xA4), *ren = gs(0xA2);
            _snprintf(b, sizeof b - 1, "%s %s", ren ? ren : "", hwn ? hwn : "");
            b[sizeof b - 1] = 0;
            meta_add("gpu", b);
            meta_add("driver", ver);
        }
        meta_res(0, 0, 16);
        meta_add("api", "glide3");
    }
    meta_done();
}

static void WINAPI g2wrap(DWORD n)
{
    unsigned __int64 t = fl_rdtsc();
    int claim;
    g2swap(n);
    if (fl_event(t, FL_API_GLIDE2, &claim) && claim) glide_meta(0);
}

static void WINAPI g3wrap(DWORD n)
{
    unsigned __int64 t = fl_rdtsc();
    int claim;
    g3swap(n);
    if (fl_event(t, FL_API_GLIDE3, &claim) && claim) glide_meta(1);
}

/* OpenGL */
#define MAXGL 3
typedef BOOL(WINAPI *WglSwapFn)(HDC);
static WglSwapFn glswap[MAXGL];
static HMODULE glmod[MAXGL];
static BOOL(WINAPI *gdiswap)(HDC);

static void gl_meta(HMODULE m, HDC dc)
{
    typedef const char *(WINAPI * GS)(DWORD);
    GS gs = m ? (GS)GetProcAddress(m, "glGetString") : NULL;
    char b[MAX_PATH], *s;
    RECT r;
    HWND w = WindowFromDC(dc);
    if (gs) {
        const char *ren = gs(0x1F01), *ven = gs(0x1F00), *ver = gs(0x1F02);
        meta_add("gpu", ren);
        _snprintf(b, sizeof b - 1, "%s %s", ven ? ven : "", ver ? ver : "");
        b[sizeof b - 1] = 0;
        meta_add("driver", b);
    }
    if (m && GetModuleFileNameA(m, b, sizeof b)) {
        s = strrchr(b, '\\');
        meta_add("gl_dll", s ? s + 1 : b);
    }
    if (w && GetClientRect(w, &r)) meta_res(r.right, r.bottom, GetDeviceCaps(dc, BITSPIXEL));
    meta_add("api", "opengl");
    meta_done();
}

static BOOL gl_common(int i, HDC dc, DWORD api)
{
    unsigned __int64 t = fl_rdtsc();
    DWORD tid = GetCurrentThreadId();
    BOOL r;
    int claim, nested = (g_swaptid == tid);
    if (!nested) g_swaptid = tid;
    r = i >= 0 ? glswap[i](dc) : gdiswap(dc);
    if (!nested) {
        g_swaptid = 0;
        if (r && fl_event(t, api, &claim) && claim) gl_meta(i >= 0 ? glmod[i] : glmod[0], dc);
    }
    return r;
}
static BOOL WINAPI glw0(HDC dc) { return gl_common(0, dc, FL_API_WGL); }
static BOOL WINAPI glw1(HDC dc) { return gl_common(1, dc, FL_API_WGL); }
static BOOL WINAPI glw2(HDC dc) { return gl_common(2, dc, FL_API_WGL); }
static void *const glwt[MAXGL] = {glw0, glw1, glw2};
static BOOL WINAPI gdiw(HDC dc) { return gl_common(-1, dc, FL_API_GDI); }

/* IAT patch: every private module importing gdi32!SwapBuffers */
#define MAXIAT 16
static void **iat[MAXIAT];
static int niat;

static void iat_patch(HMODULE m)
{
    unsigned char *b = (unsigned char *)m;
    IMAGE_DOS_HEADER *dh = (IMAGE_DOS_HEADER *)b;
    IMAGE_NT_HEADERS *nh;
    IMAGE_IMPORT_DESCRIPTOR *id;
    DWORD rva, old;
    void **t;
    if ((DWORD)b >= 0x80000000u || dh->e_magic != IMAGE_DOS_SIGNATURE) return;
    nh = (IMAGE_NT_HEADERS *)(b + dh->e_lfanew);
    rva = nh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    if (!rva) return;
    for (id = (IMAGE_IMPORT_DESCRIPTOR *)(b + rva); id->Name; id++) {
        if (lstrcmpiA((char *)(b + id->Name), "gdi32.dll")) continue;
        for (t = (void **)(b + id->FirstThunk); *t; t++) {
            if (*t != (void *)gdiswap || niat >= MAXIAT) continue;
            if (VirtualProtect(t, 4, PAGE_READWRITE, &old)) {
                *t = (void *)gdiw;
                VirtualProtect(t, 4, old, &old);
                iat[niat++] = t;
                diag("%08lX: IAT SwapBuffers %p", g_pid, t);
            }
        }
    }
}

static void iat_restore_all(void)
{
    DWORD old;
    int i;
    for (i = 0; i < niat; i++)
        if (VirtualProtect(iat[i], 4, PAGE_READWRITE, &old)) {
            *iat[i] = (void *)gdiswap;
            VirtualProtect(iat[i], 4, old, &old);
        }
    niat = 0;
}

static void try_module(HMODULE m)
{
    void *p, *tr;
    int i;
    if (!m || m == g_inst) return;
    if ((p = GetProcAddress(m, "_grBufferSwap@4"))) {
        int v3 = GetProcAddress(m, "_grGetString@4") != NULL;
        if (v3 && !g3swap && (tr = ihook(m, p, g3wrap, "glide3 grBufferSwap"))) { g3swap = (SwapFn)tr; g3mod = m; }
        if (!v3 && !g2swap && (tr = ihook(m, p, g2wrap, "glide2 grBufferSwap"))) { g2swap = (SwapFn)tr; g2mod = m; }
    }
    if ((p = GetProcAddress(m, "wglSwapBuffers"))) {
        for (i = 0; i < MAXGL; i++)
            if (glmod[i] == m) break;
        if (i == MAXGL)
            for (i = 0; i < MAXGL; i++)
                if (!glmod[i]) {
                    if ((tr = ihook(m, p, glwt[i], "wglSwapBuffers"))) { glswap[i] = (WglSwapFn)tr; glmod[i] = m; }
                    break;
                }
    }
    if (gdiswap) iat_patch(m);
}

static void scan(int full)
{
    static const char *const names[] = {"glide2x.dll", "glide3x.dll", "opengl32.dll", "3dfxgl.dll", "3dfxvgl.dll", NULL};
    int i;
    if (!shm || shm->quitting || InterlockedExchange(&g_scanning, 1)) return;
    for (i = 0; names[i]; i++) try_module(GetModuleHandleA(names[i]));
    if (full) {
        HANDLE sn = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
        MODULEENTRY32 me;
        if (sn != INVALID_HANDLE_VALUE) {
            me.dwSize = sizeof me;
            if (Module32First(sn, &me))
                do {
                    if ((DWORD)me.modBaseAddr < 0x80000000u) try_module(me.hModule);
                } while (Module32Next(sn, &me));
            CloseHandle(sn);
        }
    }
    InterlockedExchange(&g_scanning, 0);
}

static void maybe_scan(int force_full)
{
    DWORD now = GetTickCount();
    if (force_full || now - g_lastfull >= 2000) {
        g_lastfull = g_lastscan = now;
        scan(1);
    } else if (now - g_lastscan >= 250) {
        g_lastscan = now;
        scan(0);
    }
}

/* ---------- hooks that load us, attach/detach ---------- */

/* DDRAW.DLL rewrites its vtables when a process loads it (Win98): hook them again (FRAMELOG.EXE does it too) */
static void dd_fix(void)
{
    DWORD i;
    void *v;
    if (!shm || shm->quitting) return;
    for (i = 0; i < shm->nslots; i++)
        if ((v = *shm->vtslot[i]) != (void *)shm->stub[i]) {
            shm->orig[i] = v;
            InterlockedExchange((LONG *)shm->vtslot[i], (LONG)shm->stub[i]);
        }
}



__declspec(dllexport) LRESULT CALLBACK FlCbtProc(int code, WPARAM wp, LPARAM lp)
{
    if (code == HCBT_CREATEWND || code == HCBT_ACTIVATE) { dd_fix(); maybe_scan(code == HCBT_CREATEWND); }
    return CallNextHookEx(shm ? shm->hk_cbt : NULL, code, wp, lp);
}

__declspec(dllexport) LRESULT CALLBACK FlMsgProc(int code, WPARAM wp, LPARAM lp)
{
    if (code >= 0) maybe_scan(0);
    return CallNextHookEx(shm ? shm->hk_msg : NULL, code, wp, lp);
}

/* /INJECT stub and FRAMELOG.EXE call this to scan right away */
__declspec(dllexport) void WINAPI FlScanNow(void) { maybe_scan(1); }

static void dd_restore_if_last(void)
{
    /* last instance out and the loader gone: put the DirectDraw vtables back */
    DWORD i;
    HANDLE p = OpenProcess(SYNCHRONIZE, FALSE, shm->loader_pid);
    if (p) {
        DWORD r = WaitForSingleObject(p, 0);
        CloseHandle(p);
        if (r == WAIT_TIMEOUT) return;
    }
    for (i = 0; i < shm->nslots; i++)
        if (*shm->vtslot[i] == (void *)shm->stub[i]) *shm->vtslot[i] = shm->orig[i];
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD why, LPVOID res)
{
    int i;
    (void)res;
    if (why == DLL_PROCESS_ATTACH) {
        g_inst = inst;
        DisableThreadLibraryCalls(inst);
        hmap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, FL_SHM_NAME);
        if (!hmap) return TRUE; /* loaded outside FRAMELOG: do nothing */
        shm = (FLSHM *)MapViewOfFile(hmap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if (!shm || shm->magic != FL_MAGIC) { if (shm) UnmapViewOfFile(shm); CloseHandle(hmap); shm = NULL; return TRUE; }
        g_pid = GetCurrentProcessId();
        g_key = fl_proc_key();
        gdiswap = (BOOL(WINAPI *)(HDC))GetProcAddress(GetModuleHandleA("gdi32.dll"), "SwapBuffers");
        for (i = 0; i < FL_MAXSLOT; i++) g_jt[i] = (i < (int)shm->nslots && shm->kind[i] == FL_KIND_BLT) ? bwt[i] : fwt[i];
        InterlockedIncrement((LONG *)&shm->dllcount);
        if (g_pid != shm->loader_pid)
            for (i = 0; i < FL_MAXPROC; i++) {
                if (shm->procs[i].key) continue;
                shm->procs[i].jt = g_jt;
                if (fl_cas(&shm->procs[i].key, 0, g_key) == 0) { g_slot = i; break; }
            }
        {
            char e[MAX_PATH], *b;
            GetModuleFileNameA(NULL, e, sizeof e);
            b = strrchr(e, '\\');
            diag("attach %s key %08lX slot %d vt0 %s", b ? b + 1 : e, g_key, g_slot,
                 shm->nslots ? (*shm->vtslot[0] == (void *)shm->stub[0] ? "hooked" : "NOT hooked") : "-");
        }
        dd_fix();
        g_lastscan = GetTickCount();
        scan(0); /* names only: no Toolhelp inside DllMain */
    } else if (why == DLL_PROCESS_DETACH && shm) {
        if (g_slot >= 0) shm->procs[g_slot].key = 0;
        if (g_claimed && shm->owner_pid == g_pid) shm->owner_gone = 1;
        ihook_restore_all();
        iat_restore_all();
        if (InterlockedDecrement((LONG *)&shm->dllcount) == 0 && g_pid != shm->loader_pid) dd_restore_if_last();
        UnmapViewOfFile(shm);
        CloseHandle(hmap);
        shm = NULL;
    }
    return TRUE;
}
