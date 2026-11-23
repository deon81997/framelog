/* DUKECFG.EXE - set Duke Nukem 3D's screen mode for the FRAMELOG card test batches (pack/CARDS*.BAT).
 *   DUKECFG DIR MODE WIDTH HEIGHT   DIR\DUKE3D.CFG: ScreenMode / ScreenWidth / ScreenHeight (first run keeps the
 *                                   original as DIR\DUKE3D.FLB)
 *   DUKECFG DIR /RESTORE            put DUKE3D.FLB back
 * Exit 0 = done, 1 = usage, 2 = no DUKE3D.CFG / write error. Everything else in the file stays as it is. GPL-2.0-or-later. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
static int copyf(const char *a, const char *b)
{
    FILE *i = fopen(a, "rb"), *o; char buf[4096]; size_t n;
    if (!i) return -1;
    if (!(o = fopen(b, "wb"))) { fclose(i); return -1; }
    while ((n = fread(buf, 1, sizeof buf, i)) > 0) fwrite(buf, 1, n, o);
    fclose(i); return fclose(o);
}
int main(int argc, char **argv)
{
    char cfg[300], bak[300], line[512];
    if (argc < 3) { puts("DUKECFG DIR MODE WIDTH HEIGHT  |  DUKECFG DIR /RESTORE"); return 1; }
    snprintf(cfg, sizeof cfg, "%s\\DUKE3D.CFG", argv[1]); snprintf(bak, sizeof bak, "%s\\DUKE3D.FLB", argv[1]);
    if (!strcasecmp(argv[2], "/RESTORE")) {
        if (copyf(bak, cfg)) { printf("DUKECFG: no %s to restore\n", bak); return 2; }
        remove(bak); printf("DUKECFG: %s restored\n", cfg); return 0;
    }
    if (argc < 5) { puts("DUKECFG DIR MODE WIDTH HEIGHT  |  DUKECFG DIR /RESTORE"); return 1; }
    FILE *t = fopen(bak, "rb");
    if (t) fclose(t); else if (copyf(cfg, bak)) { printf("DUKECFG: %s not found\n", cfg); return 2; }
    FILE *i = fopen(bak, "rb"), *o = fopen(cfg, "wb");
    if (!i || !o) { printf("DUKECFG: cannot write %s\n", cfg); return 2; }
    const char *key[3] = { "ScreenMode", "ScreenWidth", "ScreenHeight" };
    while (fgets(line, sizeof line, i)) {
        int k; for (k = 0; k < 3; k++) if (!strncasecmp(line, key[k], strlen(key[k])) && strchr(line, '=')) break;
        if (k < 3) fprintf(o, "%s = %s\r\n", key[k], argv[2 + k]); else fputs(line, o);
    }
    fclose(i); fclose(o);
    printf("DUKECFG: ScreenMode %s, %sx%s (original kept as DUKE3D.FLB)\n", argv[2], argv[3], argv[4]);
    return 0;
}
