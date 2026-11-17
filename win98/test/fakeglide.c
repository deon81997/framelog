/* fakeglide.c -> test/glide2x.dll: a stand-in Glide 2 for VMs without a Voodoo (hook mechanics only).
   Exports the MSVC-style names (_grBufferSwap@4 ...) like 3dfx's glide2x.dll. */
#include <windows.h>
static volatile DWORD swaps;
__declspec(dllexport) void WINAPI grGlideInit(void) { swaps = 0; }
__declspec(dllexport) void WINAPI grGlideShutdown(void) {}
__declspec(dllexport) void WINAPI grGlideGetVersion(char *v) { lstrcpyA(v, "Glide Version 2.46 (FRAMELOG fake)"); }
__declspec(dllexport) int WINAPI grSstQueryHardware(DWORD *hw) { hw[0] = 1; hw[1] = 3; return 1; }
__declspec(dllexport) void WINAPI grSstSelect(int n) { (void)n; }
__declspec(dllexport) int WINAPI grSstWinOpen(DWORD w, int r, int f, int c, int o, int nb, int na)
{ (void)w; (void)r; (void)f; (void)c; (void)o; (void)nb; (void)na; return 1; }
__declspec(dllexport) DWORD WINAPI grSstScreenWidth(void) { return 640; }
__declspec(dllexport) DWORD WINAPI grSstScreenHeight(void) { return 480; }
__declspec(dllexport) void WINAPI grBufferClear(DWORD c, DWORD a, DWORD d) { (void)c; (void)a; (void)d; }
__declspec(dllexport) void WINAPI grBufferSwap(DWORD n)
{
    DWORD i;
    for (i = 0; i < n * 1000; i++) swaps += i & 1; /* a real-looking body */
    swaps++;
}
