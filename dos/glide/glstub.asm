; glstub.asm - FRAMELOG DOS Glide hook, real-mode INT 21h stub (nasm -f bin; tools/bin2h.py -> glide/glstub.h).
; DOS Glide games load GLIDE2X.OVL (an LE DLL) with plain DOS reads through their DOS extender's real-mode transfer
; buffer. This stub watches those calls only:
;   - open (AH=3Dh, AH=6Ch, AX=716Ch) succeeded -> cb (gl_open_cb): is it GLIDE2X.OVL? then parse it, arm hnd;
;   - read (AH=3Fh) on the armed handle succeeded -> cb2 (gl_read_cb): patch grBufferSwap's first bytes in the
;     buffer before the extender copies it up (a CALL to FRAMELOG's 32-bit thunk, see glhook.c);
;   - close (AH=3Eh) of the armed handle -> disarm.
; Every other DOS call costs 9 compares. busy (cb3+2) = FRAMELOG's own DOS calls from the callbacks: pass through.
; Callbacks are DPMI real-mode callbacks (RETF); they leave the registers as they are. CF of the DOS call is kept.
; Learn call (FRAMELOG's own, at install, through DPMI 0300h): AX=464Ch BX=474Ch -> store the caller's CS = the DPMI
; host's real-mode code (cb3+6). gl_open_cb arms only opens from that CS: a protected-mode program (DOS/4GW game),
; not COPY or a real-mode tool, which would otherwise copy the patched bytes into a file.
        org 0
        cmp ax, 464Ch
        jne .go
        cmp bx, 474Ch
        jne .go
        push bp
        mov bp, sp
        mov ax, [bp+4]
        mov [cs:hostcs], ax
        pop bp
        iret
.go:    cmp byte [cs:cb3+2], 0
        jne .chain
        cmp ah, 3Fh
        je .read
        cmp ah, 3Eh
        je .close
        cmp ah, 3Dh
        je .open
        cmp ah, 6Ch
        je .open
        cmp ax, 716Ch
        je .open
.chain: jmp far [cs:old]
.close: cmp bx, [cs:cb3]
        jne .chain
        mov word [cs:cb3], 0FFFFh
        jmp .chain
.read:  cmp bx, [cs:cb3]
        jne .chain
        pushf
        call far [cs:old]
        jc .ret
        pushf
        call far [cs:cb2]           ; AX = bytes read, BX = handle, DS:DX = buffer
        popf
.ret:   retf 2
.open:  push ax                     ; the request: gl_open_cb finds it behind 'FL' (name at DS:DX or DS:SI)
        push word 464Ch
        pushf
        call far [cs:old]
        jc .o2
        pushf
        call far [cs:cb]            ; AX = new handle
        popf
.o2:    pop word [cs:tmp]           ; drop 'FL' + request, flags untouched
        pop word [cs:tmp]
        retf 2
        align 4
cb:     dd 0
old:    dd 0
cb2:    dd 0
cb3:    dw 0FFFFh                   ; armed handle (FFFFh = none)
        db 0                        ; busy
        db 0
tmp:    dw 0
hostcs: dw 0                        ; cb3+6
