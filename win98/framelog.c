/* FRAMELOG.EXE - Win98 frame-time logger (loader + COM worker). See FRAMELOG.TXT / docs/PROTOCOL.md.
     FRAMELOG [/COM:n] [/BAUD:n] [/LOG] [/NOINJECT] [game.exe [args]]
     FRAMELOG /STOP
   Without a game: global hooks load FLHOOK.DLL into every GUI process; the first process that presents a frame
   is logged, until it exits (E), then the next one. With a game: that process only (FLHOOK.DLL also patched in at
   its entry point), FRAMELOG exits when the game exits. Keys: Q / Esc / Ctrl+C = quit (unhooks everything). */
#include <windows.h>
#include <ddraw.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "flshared.h"

#define VERSION "0.1-w98"

static FLSHM *shm;
static HANDLE hmap, hcom = INVALID_HANDLE_VALUE, hstop;
static HMODULE hdll;
static FILE *logf;
static volatile LONG g_quit;
static int opt_com = 1, opt_inject = 1, opt_quiet;
static DWORD opt_baud = 115200;
static unsigned __int64 tsc_hz;
static char cpu_name[96];
static LPDIRECTDRAW dd;
static HANDLE hgame;

static void say(const char *fmt, ...)
{
    va_list ap;
    char b[256];
    va_start(ap, fmt);
    _vsnprintf(b, sizeof b - 1, fmt, ap);
    b[sizeof b - 1] = 0;
    va_end(ap);
    printf("\r%-78s\n", b);
    if (logf) { fprintf(logf, "%lu %s\n", GetTickCount(), b); fflush(logf); }
}

/* ---------- COM out ---------- */
static unsigned char obuf[4096];
static int olen;

static void oflush(void)
{
    DWORD n;
    if (olen && hcom != INVALID_HANDLE_VALUE) WriteFile(hcom, obuf, olen, &n, NULL);
    olen = 0;
}

static void rec(unsigned char type, const void *pl, int n)
{
    const unsigned char *p = (const unsigned char *)pl;
    unsigned char sum = type;
    int i;
    if (olen + n + 3 > (int)sizeof obuf) oflush();
    obuf[olen++] = 0xA5;
    obuf[olen++] = type;
    for (i = 0; i < n; i++) { obuf[olen++] = p[i]; sum += p[i]; }
    obuf[olen++] = sum;
}

static void put32(unsigned char *p, DWORD v) { p[0] = (unsigned char)v; p[1] = (unsigned char)(v >> 8); p[2] = (unsigned char)(v >> 16); p[3] = (unsigned char)(v >> 24); }

static int open_com(void)
{
    char name[32];
    DCB dcb;
    COMMTIMEOUTS to;
    sprintf(name, opt_com < 10 ? "COM%d" : "\\\\.\\COM%d", opt_com);
    hcom = CreateFileA(name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hcom == INVALID_HANDLE_VALUE) return 0;
    SetupComm(hcom, 4096, 8192);
    memset(&dcb, 0, sizeof dcb);
    dcb.DCBlength = sizeof dcb;
    GetCommState(hcom, &dcb);
    dcb.BaudRate = opt_baud;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = 1;
    dcb.fParity = 0;
    dcb.fOutxCtsFlow = dcb.fOutxDsrFlow = 0;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    dcb.fOutX = dcb.fInX = 0;
    dcb.fNull = 0;
    dcb.fAbortOnError = 0;
    SetCommState(hcom, &dcb);
    memset(&to, 0, sizeof to);
    to.ReadIntervalTimeout = MAXDWORD; /* return at once */
    to.WriteTotalTimeoutConstant = 2000;
    SetCommTimeouts(hcom, &to);
    PurgeComm(hcom, PURGE_RXCLEAR | PURGE_TXCLEAR);
    return 1;
}

/* ---------- machine info ---------- */
static void cpuid(DWORD l, DWORD r[4]) { __asm__ __volatile__("cpuid" : "=a"(r[0]), "=b"(r[1]), "=c"(r[2]), "=d"(r[3]) : "a"(l), "c"(0)); }

static void measure_tsc(void)
{
    LARGE_INTEGER f, a, b;
    unsigned __int64 t0, t1;
    int pri = GetThreadPriority(GetCurrentThread());
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&a);
    t0 = fl_rdtsc();
    do QueryPerformanceCounter(&b); while ((double)(b.QuadPart - a.QuadPart) < 0.25 * (double)f.QuadPart);
    t1 = fl_rdtsc();
    SetThreadPriority(GetCurrentThread(), pri);
    tsc_hz = (unsigned __int64)((double)(t1 - t0) * (double)f.QuadPart / (double)(b.QuadPart - a.QuadPart));
}

static void cpu_info(void)
{
    DWORD r[4], fam, mod, max, mhz = (DWORD)((tsc_hz + 500000) / 1000000);
    char ven[13], brand[49];
    const char *n = NULL;
    int i;
    cpuid(0, r);
    memcpy(ven, &r[1], 4); memcpy(ven + 4, &r[3], 4); memcpy(ven + 8, &r[2], 4); ven[12] = 0;
    cpuid(1, r);
    fam = (r[0] >> 8) & 15; mod = (r[0] >> 4) & 15;
    cpuid(0x80000000u, r);
    max = r[0];
    if (max >= 0x80000004u && max < 0x8000FFFFu) {
        for (i = 0; i < 3; i++) cpuid(0x80000002u + i, (DWORD *)(brand + 16 * i));
        brand[48] = 0;
        for (n = brand; *n == ' '; n++) ;
        if (*n) { _snprintf(cpu_name, sizeof cpu_name - 1, "%s (%lu MHz)", n, mhz); return; }
    }
    if (!strcmp(ven, "GenuineIntel")) {
        if (fam == 5) n = mod >= 4 ? "Pentium MMX" : "Pentium";
        else if (fam == 6) n = mod == 1 ? "Pentium Pro" : (mod == 3 || mod == 5) ? "Pentium II" : mod == 6 ? "Celeron" :
                               (mod == 9 || mod == 13) ? "Pentium M" : mod <= 11 ? "Pentium III" : NULL;
    } else if (!strcmp(ven, "AuthenticAMD")) {
        if (fam == 5) n = mod == 8 ? "K6-2" : mod == 9 ? "K6-III" : mod == 13 ? "K6-2+/III+" : mod >= 6 ? "K6" : "K5";
        else if (fam == 6) n = "Athlon/Duron";
    } else if (!strcmp(ven, "CyrixInstead")) n = "Cyrix";
    if (n) _snprintf(cpu_name, sizeof cpu_name - 1, "%s %lu MHz", n, mhz);
    else _snprintf(cpu_name, sizeof cpu_name - 1, "%s %lu/%lu %lu MHz", ven, fam, mod, mhz);
}

/* ---------- DirectDraw vtable slots ---------- */
static void make_stub(int k)
{
    unsigned char *s = shm->stub[k];
    DWORD procs = (DWORD)&shm->procs[0], end = (DWORD)&shm->procs[FL_MAXPROC];
    static const unsigned char tpl[41] = {
        0x51,                                     /* 0  push ecx */
        0x64, 0x8B, 0x0D, 0x30, 0, 0, 0,          /* 1  mov ecx, fs:[30h] */
        0xB8, 0, 0, 0, 0,                         /* 8  mov eax, procs */
        0x39, 0x08,                               /* 13 L: cmp [eax], ecx */
        0x74, 0x11,                               /* 15 je F (34) */
        0x83, 0xC0, 0x08,                         /* 17 add eax, 8 */
        0x3D, 0, 0, 0, 0,                         /* 20 cmp eax, end */
        0x72, 0xF2,                               /* 25 jb L */
        0x59,                                     /* 27 pop ecx */
        0xFF, 0x25, 0, 0, 0, 0,                   /* 28 jmp [orig_k] */
        0x8B, 0x40, 0x04,                         /* 34 F: mov eax, [eax+4] */
        0x59,                                     /* 37 pop ecx */
        0xFF, 0x60, 0                             /* 38 jmp [eax + k*4] */
    };
    memcpy(s, tpl, sizeof tpl);
    put32(s + 9, procs);
    put32(s + 21, end);
    put32(s + 30, (DWORD)&shm->orig[k]);
    s[40] = (unsigned char)(k * 4);
}

static int add_vtable(void **vt)
{
    static const int slot[3] = {11, 5, 7}; /* Flip, Blt, BltFast */
    DWORD i;
    int j;
    for (i = 0; i < shm->nslots; i++)
        if (shm->vtslot[i] == &vt[11] || shm->vtslot[i] == &vt[5]) return 0; /* seen */
    for (j = 0; j < 3; j++) {
        DWORD k = shm->nslots;
        if (k >= FL_MAXSLOT) return 0;
        shm->kind[k] = j == 2 ? FL_KIND_BLTFAST : j ? FL_KIND_BLT : FL_KIND_FLIP;
        shm->vtslot[k] = &vt[slot[j]];
        shm->orig[k] = vt[slot[j]];
        make_stub(k);
        shm->nslots = k + 1;
    }
    return 1;
}

static void dd_setup(void)
{
    static const GUID *const iid[] = {&IID_IDirectDrawSurface2, &IID_IDirectDrawSurface3, &IID_IDirectDrawSurface4,
                                      &IID_IDirectDrawSurface7};
    static const char *const nm[] = {"2", "3", "4", "7"};
    LPDIRECTDRAWSURFACE ps = NULL;
    DDSURFACEDESC sd;
    DWORD i, old;
    int k;
    if (!LoadLibraryA("ddraw.dll") || DirectDrawCreate(NULL, &dd, NULL) != DD_OK) { say("DirectDraw: not available"); return; }
    IDirectDraw_SetCooperativeLevel(dd, NULL, DDSCL_NORMAL);
    memset(&sd, 0, sizeof sd);
    sd.dwSize = sizeof sd;
    sd.dwFlags = DDSD_CAPS;
    sd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    if (IDirectDraw_CreateSurface(dd, &sd, &ps, NULL) != DD_OK || !ps) { say("DirectDraw: no primary surface"); return; }
    add_vtable(*(void ***)ps);
    for (k = 0; k < 4; k++) {
        void *q = NULL;
        if (IDirectDrawSurface_QueryInterface(ps, iid[k], &q) == DD_OK && q) {
            if (add_vtable(*(void ***)q) && logf) fprintf(logf, "Surface%s vtable %p\n", nm[k], *(void **)q);
            ((IUnknown *)q)->lpVtbl->Release((IUnknown *)q);
        }
    }
    IDirectDrawSurface_Release(ps);
    for (i = 0; i < shm->nslots; i++) {
        if ((DWORD)shm->vtslot[i] < 0x80000000u) /* private DDRAW.DLL (not Win9x?): patch is per process only */
            VirtualProtect(shm->vtslot[i], 4, PAGE_READWRITE, &old);
        InterlockedExchange((LONG *)shm->vtslot[i], (LONG)shm->stub[i]);
    }
    say("DirectDraw: %lu vtable slots hooked (Flip, Blt/BltFast to primary)", shm->nslots);
}

/* DDRAW.DLL may rewrite its vtables (seen on Win98 after a process exits): put the stubs back */
static void dd_watch(void)
{
    DWORD i;
    void *v;
    for (i = 0; i < shm->nslots; i++)
        if ((v = *shm->vtslot[i]) != (void *)shm->stub[i]) {
            if (v != shm->orig[i]) { say("vtable slot %lu = %p (was %p): new original", i, v, shm->orig[i]); shm->orig[i] = v; }
            else if (logf) fprintf(logf, "%lu vtable slot %lu reset to the original, hooked again\n", GetTickCount(), i);
            InterlockedExchange((LONG *)shm->vtslot[i], (LONG)shm->stub[i]);
        }
}

static void dd_restore(void)
{
    DWORD i;
    for (i = 0; i < shm->nslots; i++)
        fl_cas((volatile DWORD *)shm->vtslot[i], (DWORD)shm->stub[i], (DWORD)shm->orig[i]);
}

/* ---------- launch mode ---------- */
static DWORD exe_entry(const char *exe)
{
    FILE *f = fopen(exe, "rb");
    unsigned char h[1024];
    DWORD lfanew, ep = 0, base;
    if (!f) return 0;
    if (fread(h, 1, sizeof h, f) >= 0x40) {
        lfanew = *(DWORD *)(h + 0x3C);
        if (lfanew + 0x40 < sizeof h && !memcmp(h + lfanew, "PE\0\0", 4)) {
            ep = *(DWORD *)(h + lfanew + 0x28);
            base = *(DWORD *)(h + lfanew + 0x34);
            if (ep) ep += base;
        }
    }
    fclose(f);
    return ep;
}

static void inject(PROCESS_INFORMATION *pi, DWORD entry)
{
    unsigned char *s = shm->inj, jmp[5];
    DWORD n = 0;
    FARPROC ll = GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    FARPROC vp = GetProcAddress(GetModuleHandleA("kernel32.dll"), "VirtualProtect");
    int i = 0;
    if (!entry || !ReadProcessMemory(pi->hProcess, (void *)entry, shm->injsave, 5, &n) || n != 5) {
        say("inject: can't read the entry point (%lu), using the hooks only", GetLastError());
        return;
    }
    shm->injentry = entry;
    s[i++] = 0x60; s[i++] = 0x9C;                                         /* pushad, pushfd */
    s[i++] = 0x68; put32(s + i, (DWORD)shm->dllpath); i += 4;           /* push dllpath */
    s[i++] = 0xB8; put32(s + i, (DWORD)ll); i += 4;                     /* mov eax, LoadLibraryA */
    s[i++] = 0xFF; s[i++] = 0xD0;                                         /* call eax */
    s[i++] = 0x68; put32(s + i, (DWORD)&shm->injold); i += 4;           /* push &old */
    s[i++] = 0x6A; s[i++] = 0x40;                                         /* push PAGE_EXECUTE_READWRITE */
    s[i++] = 0x6A; s[i++] = 0x05;                                         /* push 5 */
    s[i++] = 0x68; put32(s + i, entry); i += 4;                         /* push entry */
    s[i++] = 0xB8; put32(s + i, (DWORD)vp); i += 4;                     /* mov eax, VirtualProtect */
    s[i++] = 0xFF; s[i++] = 0xD0;                                         /* call eax */
    s[i++] = 0xBE; put32(s + i, (DWORD)shm->injsave); i += 4;           /* mov esi, saved bytes */
    s[i++] = 0xBF; put32(s + i, entry); i += 4;                         /* mov edi, entry */
    s[i++] = 0xB9; put32(s + i, 5); i += 4;                             /* mov ecx, 5 */
    s[i++] = 0xFC; s[i++] = 0xF3; s[i++] = 0xA4;                         /* cld, rep movsb */
    s[i++] = 0x9D; s[i++] = 0x61;                                         /* popfd, popad */
    s[i++] = 0x68; put32(s + i, entry); i += 4;                         /* push entry */
    s[i++] = 0xC3;                                                        /* ret */
    jmp[0] = 0xE9;
    put32(jmp + 1, (DWORD)s - (entry + 5));
    if (!WriteProcessMemory(pi->hProcess, (void *)entry, jmp, 5, &n) || n != 5)
        say("inject: can't patch the entry point (%lu), using the hooks only", GetLastError());
    else if (logf) fprintf(logf, "inject: entry %08lX -> stub %p\n", entry, s);
}

static int launch(int argc, char **argv, int first)
{
    char cmd[1024] = "", exe[MAX_PATH], dir[MAX_PATH], *p;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    int i;
    DWORD entry;
    if (!SearchPathA(NULL, argv[first], ".exe", sizeof exe, exe, &p)) lstrcpynA(exe, argv[first], sizeof exe);
    for (i = first; i < argc; i++) {
        int q = strchr(argv[i], ' ') != NULL;
        if (strlen(cmd) + strlen(argv[i]) + 4 >= sizeof cmd) break;
        if (i > first) strcat(cmd, " ");
        if (q) strcat(cmd, "\"");
        strcat(cmd, i == first ? exe : argv[i]);
        if (q) strcat(cmd, "\"");
    }
    lstrcpynA(dir, exe, sizeof dir);
    p = strrchr(dir, '\\');
    if (p) *p = 0; else GetCurrentDirectoryA(sizeof dir, dir);
    memset(&si, 0, sizeof si);
    si.cb = sizeof si;
    entry = exe_entry(exe);
    if (!CreateProcessA(exe, cmd, NULL, NULL, FALSE, opt_inject ? CREATE_SUSPENDED : 0, NULL, dir, &si, &pi)) {
        say("can't start %s (error %lu)", exe, GetLastError());
        return 0;
    }
    shm->only_pid = pi.dwProcessId;
    if (opt_inject) {
        inject(&pi, entry);
        ResumeThread(pi.hThread);
    }
    CloseHandle(pi.hThread);
    hgame = pi.hProcess;
    say("started %s (pid %08lX)", exe, pi.dwProcessId);
    return 1;
}

/* ---------- worker ---------- */
enum { IDLE, ARMED, LOGGING };
static int state;
static unsigned __int64 last_tsc;
static DWORD frame, lost, nframes, drop_seen, armed_wait;

static void send_meta(void)
{
    char t[640];
    unsigned char ch[16];
    int n, i, seq = 0;
    OSVERSIONINFOA v;
    v.dwOSVersionInfoSize = sizeof v;
    GetVersionExA(&v);
    _snprintf(t, sizeof t - 1, "%scpu=%s\nos=Windows %s %lu.%lu.%lu\nhook=FRAMELOG %s\n", shm->meta, cpu_name,
              v.dwMinorVersion == 0 ? "95" : v.dwMinorVersion == 90 ? "ME" : (v.dwBuildNumber & 0xFFFF) >= 2222 ? "98 SE" : "98",
              v.dwMajorVersion, v.dwMinorVersion, v.dwBuildNumber & 0xFFFF, VERSION);
    t[sizeof t - 1] = 0;
    n = (int)strlen(t);
    for (i = 0; i <= n; i += 15) { /* <= : a text ending on a chunk edge gets one all-NUL chunk */
        memset(ch, 0, sizeof ch);
        ch[0] = (unsigned char)seq++;
        memcpy(ch + 1, t + i, n - i < 15 ? n - i : 15);
        rec('M', ch, 16);
    }
}

static void begin(unsigned __int64 t)
{
    unsigned char h[8], b[8];
    h[0] = 1; h[1] = 1;
    put32(h + 2, (DWORD)tsc_hz);
    h[6] = 0x10; h[7] = 0;
    rec('H', h, 8);
    send_meta();
    put32(b, (DWORD)t); put32(b + 4, (DWORD)(t >> 32));
    rec('B', b, 8);
    oflush();
    last_tsc = t;
    frame = lost = nframes = 0;
    drop_seen = shm->dropped;
    state = LOGGING;
    {
        char g[64] = "?", *q = strstr(shm->meta, "game=");
        if (q) { lstrcpynA(g, q + 5, sizeof g); if ((q = strchr(g, '\n'))) *q = 0; }
        say("logging %s (pid %08lX)", g, shm->owner_pid);
    }
}

static void end(void)
{
    unsigned char e[8];
    put32(e, nframes); put32(e + 4, lost);
    rec('E', e, 8);
    oflush();
    state = IDLE;
}

static void frame_ev(unsigned __int64 t)
{
    unsigned char b[10];
    unsigned __int64 d;
    DWORD dr = shm->dropped - drop_seen;
    if (dr) {
        put32(b, dr);
        rec('D', b, 4);
        drop_seen += dr;
        lost += dr;
        frame += dr;
        nframes += dr;
    }
    d = t - last_tsc;
    last_tsc = t;
    frame++;
    nframes++;
    b[0] = (unsigned char)frame; b[1] = (unsigned char)(frame >> 8);
    if (d < 0x100000000ULL) { put32(b + 2, (DWORD)d); rec('F', b, 6); }
    else { put32(b + 2, (DWORD)d); put32(b + 6, (DWORD)(d >> 32)); rec('G', b, 10); }
}

static void owner_reset(void)
{
    DWORD t = shm->head;
    shm->meta_ready = 0;
    shm->api = 0;
    shm->owner_gone = 0;
    shm->tail = t; /* events of the old owner still in the ring are dropped */
    shm->owner_pid = 0;
    armed_wait = 0;
}

static int owner_alive(DWORD pid)
{
    HANDLE p = OpenProcess(SYNCHRONIZE, FALSE, pid);
    DWORD r;
    if (!p) return 0;
    r = WaitForSingleObject(p, 0);
    CloseHandle(p);
    return r == WAIT_TIMEOUT;
}

static void poll_com(void)
{
    unsigned char b[64];
    DWORD n = 0, i;
    if (hcom == INVALID_HANDLE_VALUE || !ReadFile(hcom, b, sizeof b, &n, NULL)) return;
    for (i = 0; i < n; i++) {
        if (b[i] == 'S' && state == IDLE) state = ARMED, armed_wait = 0;
        else if (b[i] == 'X') {
            if (state == LOGGING) end();
            state = IDLE;
        }
    }
}

static void drain(void)
{
    for (;;) {
        DWORD t = shm->tail;
        FLEV *e = &shm->ring[t & (FL_RING - 1)];
        if (t == shm->head || e->seq != t + 1) break;
        if (state == ARMED) {
            if (!shm->meta_ready) {
                if (!armed_wait) armed_wait = GetTickCount();
                /* owner still filling meta: a process's first DirectDraw GetDeviceIdentifier can take > 0.5 s
                   on Win98 (86Box: the first DDraw game of a session lost its gpu/driver keys); the ring holds
                   4096 frames with their own TSC stamps, so waiting costs no frames */
                if (GetTickCount() - armed_wait < 3000) break;
                say("meta not complete after 3 s, sending what there is");
            }
            begin(e->tsc);
        } else if (state == LOGGING) frame_ev(e->tsc);
        shm->tail = t + 1;
    }
    oflush();
}

static void diag_out(void)
{
    int i;
    for (i = 0; i < FL_DIAG; i++)
        if (shm->diag[i][0] == 1) {
            say("%s", shm->diag[i] + 1);
            shm->diag[i][0] = 0;
        }
}

static BOOL WINAPI ctrl(DWORD t)
{
    InterlockedExchange(&g_quit, 1);
    if (t == CTRL_CLOSE_EVENT || t == CTRL_LOGOFF_EVENT || t == CTRL_SHUTDOWN_EVENT) Sleep(1500);
    return TRUE;
}

static void usage(void)
{
    printf("FRAMELOG %s - Win98 frame times (DirectDraw/Direct3D 5-7, Glide 2/3, OpenGL) -> COM port\n"
           "  FRAMELOG [/COM:n] [/BAUD:n] [/LOG] [/NOINJECT] [game.exe [args]]\n"
           "  FRAMELOG /STOP\n"
           "Start the host logger (serial-bench fllog.py) on the other PC. Q/Esc/Ctrl+C quits.\n", VERSION);
}

int main(int argc, char **argv)
{
    int i, first = 0;
    char p[MAX_PATH], *s;
    DWORD last_status = 0, last_alive = 0, ownseen = 0;

    for (i = 1; i < argc && !first; i++) {
        if (argv[i][0] != '/' && argv[i][0] != '-') { first = i; break; }
        if (!_strnicmp(argv[i] + 1, "COM:", 4)) opt_com = atoi(argv[i] + 5);
        else if (!_strnicmp(argv[i] + 1, "BAUD:", 5)) opt_baud = strtoul(argv[i] + 6, NULL, 10);
        else if (!_stricmp(argv[i] + 1, "LOG")) {
            GetModuleFileNameA(NULL, p, sizeof p);
            s = strrchr(p, '\\');
            strcpy(s ? s + 1 : p, "FRAMELOG.LOG");
            logf = fopen(p, "w");
        } else if (!_stricmp(argv[i] + 1, "NOINJECT")) opt_inject = 0;
        else if (!_stricmp(argv[i] + 1, "Q")) opt_quiet = 1;
        else if (!_stricmp(argv[i] + 1, "STOP")) {
            HANDLE e = OpenEventA(EVENT_MODIFY_STATE, FALSE, FL_STOP_EVENT);
            if (!e) { printf("FRAMELOG is not running\n"); return 1; }
            SetEvent(e);
            CloseHandle(e);
            return 0;
        } else { usage(); return 1; }
    }
    hmap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(FLSHM), FL_SHM_NAME);
    if (!hmap || GetLastError() == ERROR_ALREADY_EXISTS) { printf("FRAMELOG is already running (FRAMELOG /STOP)\n"); return 1; }
    shm = (FLSHM *)MapViewOfFile(hmap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!shm) return 1;
    memset(shm, 0, sizeof *shm);
    shm->size = sizeof *shm;
    shm->loader_pid = GetCurrentProcessId();
    hstop = CreateEventA(NULL, TRUE, FALSE, FL_STOP_EVENT);
    SetConsoleCtrlHandler(ctrl, TRUE);

    printf("FRAMELOG %s\n", VERSION);
    if (!open_com()) { say("can't open COM%d", opt_com); return 1; }
    measure_tsc();
    cpu_info();
    say("COM%d %lu baud, TSC %.2f MHz, %s", opt_com, opt_baud, (double)tsc_hz / 1e6, cpu_name);
    if ((DWORD)shm < 0x80000000u) say("warning: not Windows 9x? shared memory at %p", shm);

    GetModuleFileNameA(NULL, shm->dllpath, sizeof shm->dllpath);
    s = strrchr(shm->dllpath, '\\');
    strcpy(s ? s + 1 : shm->dllpath, "FLHOOK.DLL");
    dd_setup();
    shm->magic = FL_MAGIC;
    hdll = LoadLibraryA(shm->dllpath);
    if (!hdll) { say("can't load %s", shm->dllpath); dd_restore(); return 1; }
    shm->hk_cbt = SetWindowsHookExA(WH_CBT, (HOOKPROC)GetProcAddress(hdll, "FlCbtProc@12"), hdll, 0);
    shm->hk_msg = SetWindowsHookExA(WH_GETMESSAGE, (HOOKPROC)GetProcAddress(hdll, "FlMsgProc@12"), hdll, 0);
    if (!shm->hk_cbt || !shm->hk_msg) say("SetWindowsHookEx failed (%lu)", GetLastError());
    if (first && !launch(argc, argv, first)) InterlockedExchange(&g_quit, 1);
    else if (!first) say("waiting for a game (first process that presents a frame is logged)");

    while (!g_quit) {
        poll_com();
        drain();
        diag_out();
        if (!shm->quitting) dd_watch();
        if (shm->owner_pid) {
            if (!ownseen) { ownseen = shm->owner_pid; say("presenting: pid %08lX", ownseen); }
            if (shm->owner_gone || (GetTickCount() - last_alive > 1000 && !owner_alive(shm->owner_pid))) {
                drain();
                if (state == LOGGING) end();
                if (state == ARMED) state = ARMED; /* keep waiting for the next game */
                say("process %08lX gone, %lu frames%s", shm->owner_pid, nframes, hgame ? "" : "; waiting for the next");
                owner_reset();
                ownseen = 0;
            }
            if (GetTickCount() - last_alive > 1000) last_alive = GetTickCount();
        }
        if (hgame && WaitForSingleObject(hgame, 0) == WAIT_OBJECT_0 && !shm->owner_pid) {
            if (state == LOGGING) end();
            break;
        }
        if (WaitForSingleObject(hstop, 0) == WAIT_OBJECT_0) break;
        if (_kbhit()) {
            int c = _getch();
            if (c == 'q' || c == 'Q' || c == 27) break;
        }
        if (!opt_quiet && GetTickCount() - last_status > 500) {
            last_status = GetTickCount();
            printf("\r%s pid %lu frames %lu (%lu lost) api %lX   ", state == LOGGING ? "LOG " : state == ARMED ? "ARM " : "idle",
                   shm->owner_pid, state == LOGGING ? nframes : shm->frames, lost, shm->api);
            fflush(stdout);
        }
        Sleep(5);
    }
    poll_com();
    drain();
    if (state == LOGGING) end();
    shm->quitting = 1;
    Sleep(100);
    dd_restore();
    if (shm->hk_cbt) UnhookWindowsHookEx(shm->hk_cbt);
    if (shm->hk_msg) UnhookWindowsHookEx(shm->hk_msg);
    diag_out();
    say("unhooked, bye");
    Sleep(200);
    if (dd) IDirectDraw_Release(dd);
    if (hcom != INVALID_HANDLE_VALUE) CloseHandle(hcom);
    if (logf) fclose(logf);
    return 0;
}
