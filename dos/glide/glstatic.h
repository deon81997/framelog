/* glstatic.h - FRAMELOG static Glide (grBufferSwap linked into the game EXE or a driver file, no GLIDE2X.OVL).
 * See glstatic.c and docs/GLIDE.md "Static Glide". Host-testable: dos/glide/test/gstest.c. */
#ifndef GLSTATIC_H
#define GLSTATIC_H
#include <stdint.h>

/* results (St.glide_sterr, /STATUS): 0 = armed */
enum { GS_OK = 0, GS_NONE = 20, GS_MANY, GS_FMT, GS_FIX, GS_IO };

/* file reader: 1 = all n bytes read at off */
typedef int (*gs_rd)(void *c, uint32_t off, void *dst, uint32_t n);

#define GS_PRO 8                      /* prologue bytes replaced: push ebx/esi/edi/ebp; mov ebx,[esp+14h] */

/* find grBufferSwap in a file of `size` bytes: *fn = file offset of its first byte, code = its first GS_PRO bytes */
int gs_probe(gs_rd rd, void *c, uint32_t size, uint32_t *fn, uint8_t *code);
#endif
