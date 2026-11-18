; glstub.asm - FRAMELOG DOS Glide hook, real-mode INT 21h stub (nasm -f bin; tools/bin2h.py -> glide/glstub.h).
; DOS Glide games load GLIDE2X.OVL (an LE DLL) with plain DOS reads through their DOS extender's real-mode transfer
; buffer. This stub watches those calls only:
;   - open (AH=3Dh, AH=6Ch, AX=716Ch) of ...\GLIDE2X.OVL by a protected-mode program succeeded -> cb (gl_open_cb):
;     parse it, arm hnd (name + caller checked here, so other opens never leave real mode);
;   - read (AH=3Fh) on the armed handle succeeded -> cb2 (gl_read_cb): patch grBufferSwap's first bytes in the
;     buffer before the extender copies it up (a CALL to FRAMELOG's 32-bit thunk, see glhook.c);
;   - close (AH=3Eh) of the armed handle -> disarm.
; Every other DOS call costs 9 compares. busy (cb3+2) = FRAMELOG's own DOS calls from the callbacks: pass through.
; Callbacks are DPMI real-mode callbacks (RETF); they leave the registers as they are. The caller gets its own flags
; back with DOS's arithmetic flags (CF) merged in (see out:).
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
.ret:   jmp out
.open:  push ax                     ; the request: gl_open_cb finds it behind 'FL' (name at DS:DX or DS:SI)
        push word 464Ch
        pushf
        call far [cs:old]
        jc .fail                    ; DOS said no: keep CF
        call isglide                ; GLIDE2X.OVL opened by a protected-mode program? (CF = no)
        jc .ok
        pushf
        call far [cs:cb]            ; AX = new handle
        popf
.ok:    clc
.fail:  pop word [cs:tmp]           ; drop 'FL' + request, flags untouched
        pop word [cs:tmp]
; Back to the caller with ITS flags (interrupt flag!) and DOS's result flags (CF, ZF...). retf 2 would return the
; flags of our pushf/call = IF 0 (we run with interrupts off): the caller then ran with IRQs off (BIOS ticks stopped,
; SC55LOG timed 0 s, SBEMU hung at start).
out:    push bp
        mov bp, sp
        push ax
        pushf
        pop ax
        and ax, 08D5h               ; OF SF ZF AF PF CF from DOS
        and word [bp+6], 0F72Ah     ; caller's flags without them
        or [bp+6], ax
        pop ax
        pop bp
        iret

; CF clear if the file just opened is ...\GLIDE2X.OVL and the caller's CS is the DPMI host's (hostcs, 0 = any).
; Stack: [bp] bp, +2 near return, +4 'FL', +6 request AX, +8 caller IP, +10 caller CS. Registers kept.
isglide:
        push bp
        mov bp, sp
        push ax
        push bx
        push si
        push di
        mov ax, [cs:hostcs]
        test ax, ax
        jz .any
        cmp ax, [bp+10]
        jne .no
.any:   mov bx, si                  ; 6Ch / 716Ch: DS:SI
        mov ax, [bp+6]
        cmp ah, 3Dh
        jne .s
        mov bx, dx                  ; 3Dh: DS:DX
.s:     mov si, bx
        mov di, bx                  ; DI = start of the base name
.l:     mov al, [si]
        test al, al
        jz .e
        inc si
        cmp al, '\'
        je .b
        cmp al, '/'
        je .b
        cmp al, ':'
        jne .l
.b:     mov di, si
        jmp .l
.e:     mov ax, si
        sub ax, di
        cmp ax, 11
        jne .no
        xor bx, bx
.c:     mov al, [di+bx]
        cmp al, 'a'
        jb .u
        cmp al, 'z'
        ja .u
        sub al, 20h
.u:     cmp al, [cs:gname+bx]
        jne .no
        inc bx
        cmp bx, 11
        jb .c
        clc
        jmp .x
.no:    stc
.x:     pop di
        pop si
        pop bx
        pop ax
        pop bp
        ret
gname:  db 'GLIDE2X.OVL'
        align 4
cb:     dd 0
old:    dd 0
cb2:    dd 0
cb3:    dw 0FFFFh                   ; armed handle (FFFFh = none)
        db 0                        ; busy
        db 0
tmp:    dw 0
hostcs: dw 0                        ; cb3+6
