/* lde.c - x86-32 instruction length decoder, enough for function prologues (hook trampolines).
   Returns the length of the instruction at p, or 0 if unknown. *rel = offset of a rel32 operand to fix
   (E8/E9/0F 8x), -1 if none; rel8 branches and other position-dependent forms return 0 (refuse). */
#include "lde.h"

/* one-byte opcodes: bit0 = has ModRM, bits 4-7 = immediate kind:
   0 none, 1 imm8, 2 imm16, 3 imm32 (16 with 66h), 4 imm16+imm8 (ENTER), 5 moffs (addr size), 6 imm32/16 + far (9A/EA),
   15 = unsupported */
#define M 1
#define I8 0x10
#define I16 0x20
#define IZ 0x30
#define IE 0x40
#define MO 0x50
#define FA 0x60
#define XX 0xF0
static const unsigned char op1[256] = {
    /*00*/ M, M, M, M, I8, IZ, 0, 0, M, M, M, M, I8, IZ, 0, XX,
    /*10*/ M, M, M, M, I8, IZ, 0, 0, M, M, M, M, I8, IZ, 0, 0,
    /*20*/ M, M, M, M, I8, IZ, XX, 0, M, M, M, M, I8, IZ, XX, 0,
    /*30*/ M, M, M, M, I8, IZ, XX, 0, M, M, M, M, I8, IZ, XX, 0,
    /*40*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /*50*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /*60*/ 0, 0, M, M, XX, XX, XX, XX, IZ, M | IZ, I8, M | I8, 0, 0, 0, 0,
    /*70*/ XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX,
    /*80*/ M | I8, M | IZ, M | I8, M | I8, M, M, M, M, M, M, M, M, M, M, M, M,
    /*90*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FA, 0, 0, 0, 0, 0,
    /*A0*/ MO, MO, MO, MO, 0, 0, 0, 0, I8, IZ, 0, 0, 0, 0, 0, 0,
    /*B0*/ I8, I8, I8, I8, I8, I8, I8, I8, IZ, IZ, IZ, IZ, IZ, IZ, IZ, IZ,
    /*C0*/ M | I8, M | I8, I16, 0, M, M, M | I8, M | IZ, IE, 0, I16, 0, 0, I8, 0, 0,
    /*D0*/ M, M, M, M, I8, I8, 0, 0, M, M, M, M, M, M, M, M,
    /*E0*/ XX, XX, XX, XX, I8, I8, I8, I8, XX, XX, FA, XX, 0, 0, 0, 0,
    /*F0*/ XX, 0, XX, XX, 0, 0, M, M, 0, 0, 0, 0, 0, 0, M, M,
};

int lde(const unsigned char *p, int *rel)
{
    const unsigned char *s = p;
    int osz = 0, asz = 0, f, mod, rm, has_sib = 0;
    unsigned char op;

    *rel = -1;
    for (;;) { /* prefixes */
        op = *p;
        if (op == 0x66) osz = 1;
        else if (op == 0x67) asz = 1;
        else if (op == 0xF0 || op == 0xF2 || op == 0xF3 || op == 0x2E || op == 0x36 || op == 0x3E || op == 0x26 ||
                 op == 0x64 || op == 0x65) ;
        else break;
        p++;
        if (p - s > 4) return 0;
    }
    op = *p++;
    if (op == 0x0F) {
        op = *p++;
        if (op >= 0x80 && op <= 0x8F) { /* jcc rel32 */
            *rel = (int)(p - s);
            return (int)(p - s) + 4;
        }
        if (op == 0x31 || op == 0xA2 || (op >= 0xC8 && op <= 0xCF)) return (int)(p - s); /* rdtsc cpuid bswap */
        /* movzx/movsx, setcc, cmovcc, imul, bt*, shld/shrd, xadd, cmpxchg: ModRM */
        if ((op >= 0xB6 && op <= 0xBF) || (op >= 0x90 && op <= 0x9F) || (op >= 0x40 && op <= 0x4F) || op == 0xAF ||
            op == 0xA3 || op == 0xAB || op == 0xB3 || op == 0xBB || op == 0xA5 || op == 0xAD || op == 0xC0 ||
            op == 0xC1 || op == 0xB0 || op == 0xB1) {
            f = M;
            if (op == 0xBA) f |= I8;
        } else if (op == 0xA4 || op == 0xAC || op == 0xBA) f = M | I8;
        else return 0;
    } else {
        if (op == 0xE8 || op == 0xE9) { /* call/jmp rel32 */
            *rel = (int)(p - s);
            return (int)(p - s) + 4;
        }
        if (op == 0xC2 || op == 0xC3) return (int)(p - s) + (op == 0xC2 ? 2 : 0); /* ret (ends the function) */
        f = op1[op];
        if (f == XX) return 0;
        if (op == 0xF6 || op == 0xF7) { /* test r/m, imm */
            if (((*p >> 3) & 7) < 2) f |= (op == 0xF6) ? I8 : IZ;
        }
    }
    if (f & M) {
        unsigned char m = *p++;
        mod = m >> 6;
        rm = m & 7;
        if (asz) return 0; /* 16-bit addressing: not in 32-bit prologues */
        if (mod != 3 && rm == 4) { has_sib = 1; rm = *p++ & 7; }
        if (mod == 0 && rm == 5) p += 4;
        else if (mod == 1) p += 1;
        else if (mod == 2) p += 4;
        (void)has_sib;
    }
    switch (f & 0xF0) {
    case I8: p += 1; break;
    case I16: p += 2; break;
    case IZ: p += osz ? 2 : 4; break;
    case IE: p += 3; break;
    case MO: p += asz ? 2 : 4; break;
    case FA: p += (osz ? 2 : 4) + 2; break;
    }
    return (int)(p - s);
}
