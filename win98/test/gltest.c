/* GLTEST.EXE [wgl] [N] [FPS] - OpenGL window 320x240, glClear + triangle, gdi32 SwapBuffers (default) or
   opengl32 wglSwapBuffers (wgl), N frames at FPS. Each swap must count once. */
#include <GL/gl.h>
#include "tpace.h"

int WINAPI WinMain(HINSTANCE hi, HINSTANCE hp, LPSTR cmd, int show)
{
    PIXELFORMATDESCRIPTOR pfd;
    HWND w;
    HDC dc;
    HGLRC rc;
    int pf, i, ok = 0, wgl = strstr(cmd, "wgl") != NULL;
    BOOL(WINAPI * wsb)(HDC) = (BOOL(WINAPI *)(HDC))GetProcAddress(LoadLibraryA("opengl32.dll"), "wglSwapBuffers");
    (void)hi; (void)hp; (void)show;
    tp_args(cmd);
    w = tp_window("GLTEST", 0, 320, 240);
    dc = GetDC(w);
    memset(&pfd, 0, sizeof pfd);
    pfd.nSize = sizeof pfd;
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 16;
    pf = ChoosePixelFormat(dc, &pfd);
    if (!pf || !SetPixelFormat(dc, pf, &pfd) || !(rc = wglCreateContext(dc)) || !wglMakeCurrent(dc, rc)) {
        tp_log("gltest: no GL context (%lu)", GetLastError());
        return 1;
    }
    tp_start();
    for (i = 0; i < tp_n; i++) {
        tp_pump();
        glClearColor((i & 63) / 63.0f, 0.2f, 0.4f, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glBegin(GL_TRIANGLES);
        glColor3f(1, 0, 0); glVertex2f(-0.5f, -0.5f);
        glColor3f(0, 1, 0); glVertex2f(0.5f, -0.5f);
        glColor3f(0, 0, 1); glVertex2f(0, 0.5f);
        glEnd();
        if (wgl ? wsb(dc) : SwapBuffers(dc)) ok++;
        tp_wait();
    }
    tp_log("gltest %s frames %d ok %d fps %d spare %lu renderer %s", wgl ? "wgl" : "gdi", tp_n, ok, tp_fps, tp_spare,
           (const char *)glGetString(GL_RENDERER));
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(rc);
    ReleaseDC(w, dc);
    DestroyWindow(w);
    return 0;
}
