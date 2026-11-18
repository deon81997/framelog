/* glload.h - minimal LE DLL loader for GLFLIP (see glload.c) */
#define GL_MAXOBJ 8
typedef struct {
    unsigned long nobj, pagesize, eip, h_names, h_entry;
    unsigned char *base[GL_MAXOBJ];
    unsigned long size[GL_MAXOBJ], flags[GL_MAXOBJ], pg0[GL_MAXOBJ], npg[GL_MAXOBJ];
    unsigned char *ldr;
} GLDLL;
int gl_load(const char *path, GLDLL *m, char *err);   /* 1 = ok, else err = reason */
void *gl_sym(GLDLL *m, const char *name);             /* resident names only (Glide exports live there) */
int gl_start(GLDLL *m);                               /* runs the DLL start code once (not under a DPMI host) */
