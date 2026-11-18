/* tpace.h - test programs: N frames at a fixed rate (QPC busy-wait), result line to <EXE>.TXT */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
static LARGE_INTEGER tp_f, tp_next;
static double tp_per;
static FILE *tp_out;
static int tp_n = 300, tp_fps = 50, tp_spin;
static unsigned long tp_spare; /* busy-wait passes (spin mode): CPU left over, compare with/without FRAMELOG */

static void tp_args(const char *cmd)
{
    char p[MAX_PATH], *s, *e;
    const char *a = cmd;
    GetModuleFileNameA(NULL, p, sizeof p);
    s = strrchr(p, '.');
    if (s) strcpy(s, ".TXT");
    tp_out = fopen(p, "a");
    tp_spin = strstr(cmd, "spin") != NULL;
    while (*a == ' ') a++;
    /* numbers anywhere on the line: first = frames, second = fps */
    {
        int k = 0;
        while (*a) {
            if (*a >= '0' && *a <= '9') {
                long v = strtol(a, &e, 10);
                if (v > 0) { if (k++ == 0) tp_n = (int)v; else tp_fps = (int)v; }
                a = e;
            } else a++;
        }
    }
    QueryPerformanceFrequency(&tp_f);
    tp_per = (double)tp_f.QuadPart / tp_fps;
}

static void tp_start(void) { QueryPerformanceCounter(&tp_next); }

static void tp_wait(void)
{
    LARGE_INTEGER n;
    tp_next.QuadPart += (LONGLONG)tp_per;
    do {
        QueryPerformanceCounter(&n);
        if (tp_spin) tp_spare++;
        else if (tp_next.QuadPart - n.QuadPart > tp_f.QuadPart / 500) Sleep(1);
    } while (n.QuadPart < tp_next.QuadPart);
}

static void tp_log(const char *fmt, ...)
{
    va_list ap;
    if (!tp_out) return;
    va_start(ap, fmt);
    vfprintf(tp_out, fmt, ap);
    va_end(ap);
    fputc('\n', tp_out);
    fflush(tp_out);
}

static LRESULT CALLBACK tp_wndproc(HWND w, UINT m, WPARAM wp, LPARAM lp)
{
    if (m == WM_SETCURSOR) { SetCursor(NULL); return TRUE; }
    return DefWindowProcA(w, m, wp, lp);
}

static HWND tp_window(const char *name, int popup, int w, int h)
{
    WNDCLASSA c;
    RECT r = {0, 0, w, h};
    DWORD st = popup ? WS_POPUP : WS_OVERLAPPEDWINDOW;
    HWND hw;
    memset(&c, 0, sizeof c);
    c.lpfnWndProc = tp_wndproc;
    c.hInstance = GetModuleHandleA(NULL);
    c.lpszClassName = name;
    c.style = CS_OWNDC;
    c.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassA(&c);
    AdjustWindowRect(&r, st, FALSE);
    hw = CreateWindowExA(popup ? WS_EX_TOPMOST : 0, name, name, st | WS_VISIBLE, 0, 0, popup ? w : r.right - r.left,
                         popup ? h : r.bottom - r.top, NULL, NULL, c.hInstance, NULL);
    SetForegroundWindow(hw);
    return hw;
}

static void tp_pump(void)
{
    MSG m;
    while (PeekMessageA(&m, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&m); DispatchMessageA(&m); }
}
