/* flshared.h - memory shared by FRAMELOG.EXE (loader, owns COM) and FLHOOK.DLL (in every GUI process).
   Win9x only: a named file mapping lives in the shared arena (>= 80000000h), same address in every process,
   so the DirectDraw vtable stubs placed here run in any process. */
#ifndef FLSHARED_H
#define FLSHARED_H
#include <windows.h>

#define FL_SHM_NAME "FRAMELOG_SHM_1"
#define FL_STOP_EVENT "FRAMELOG_STOP_1"
#define FL_MAGIC 0x31474C46u /* 'FLG1' */
#define FL_RING 4096         /* power of two */
#define FL_MAXPROC 64
#define FL_MAXSLOT 16        /* DirectDraw vtable slots patched (Flip/Blt x Surface1..7) */
#define FL_STUB 48
#define FL_DIAG 32           /* diag lines from DLL instances */

/* api bits (who presented; also meta 'api') */
#define FL_API_DDFLIP 1
#define FL_API_DDBLT 2
#define FL_API_GLIDE2 4
#define FL_API_GLIDE3 8
#define FL_API_WGL 16
#define FL_API_GDI 32

#define FL_KIND_FLIP 0
#define FL_KIND_BLT 1

typedef struct {
    volatile DWORD seq;          /* index + 1 when tsc is valid */
    volatile DWORD api;
    volatile unsigned __int64 tsc;
} FLEV;

typedef struct {
    volatile DWORD key;          /* fs:[30h] of the process (Win9x process database), 0 = free */
    void **volatile jt;          /* that process's wrapper table, one entry per slot */
} FLPROC;

typedef struct {
    DWORD magic, size;
    DWORD loader_pid;
    volatile DWORD quitting;     /* loader stopping: wrappers only call through */
    HHOOK hk_cbt, hk_msg;        /* for CallNextHookEx in DLL instances */
    char dllpath[MAX_PATH];

    /* DirectDraw vtable slots (vtables are in DDRAW.DLL's shared, writable data on Win9x) */
    DWORD nslots;
    DWORD kind[FL_MAXSLOT];
    void **vtslot[FL_MAXSLOT];   /* address of the vtable entry */
    void *orig[FL_MAXSLOT];      /* original method */
    unsigned char stub[FL_MAXSLOT][FL_STUB];
    unsigned char inj[80];       /* /INJECT entry-point stub (launch mode) */
    unsigned char injsave[16];   /* original entry bytes */
    DWORD injentry, injlen, injold;

    FLPROC procs[FL_MAXPROC];
    volatile LONG dllcount;

    /* owner = first presenting process */
    volatile DWORD owner_pid;    /* 0 = none */
    volatile DWORD owner_gone;   /* set by the owner's DLL at process detach */
    volatile DWORD only_pid;     /* launch mode: only this process may own */
    volatile DWORD api;          /* FL_API_* bits seen from the owner */
    volatile DWORD meta_ready;
    char meta[480];              /* key=value\n lines from the owner (game, gpu, driver, res, depth, api) */

    /* frame ring: producers = owner threads, consumer = loader worker */
    volatile DWORD head, tail;
    volatile DWORD dropped;
    volatile DWORD frames;       /* pushed since the owner claimed */
    FLEV ring[FL_RING];

    /* diagnostics */
    volatile LONG diag_head;
    char diag[FL_DIAG][96];
} FLSHM;

static __inline unsigned __int64 fl_rdtsc(void)
{
    unsigned int lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((unsigned __int64)hi << 32) | lo;
}

static __inline DWORD fl_cas(volatile DWORD *p, DWORD old, DWORD nw) /* returns the previous value */
{
    DWORD prev;
    __asm__ __volatile__("lock cmpxchgl %2,%1" : "=a"(prev), "+m"(*p) : "r"(nw), "0"(old) : "memory");
    return prev;
}

static __inline DWORD fl_xadd(volatile DWORD *p, DWORD v)
{
    __asm__ __volatile__("lock xaddl %0,%1" : "+r"(v), "+m"(*p) : : "memory");
    return v;
}

static __inline DWORD fl_proc_key(void)
{
    DWORD k;
    __asm__ __volatile__("movl %%fs:0x30,%0" : "=r"(k));
    return k;
}

#define FL_BARRIER() __asm__ __volatile__("" ::: "memory")

#endif
