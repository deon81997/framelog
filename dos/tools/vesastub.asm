; FRAMELOG real-mode INT 10h stub (nasm -f bin; bytes go to vesastub.h via xxd -i, copied to seg+1:0 of our DOS block).
;  AX=4F07h, BL in {00h,02h,80h,82h} (set / schedule display start): far call cb (DPMI real-mode callback) = a flip.
;  AX=4F0Ah, BL=0 (VBE 2.0 protected-mode interface): run the BIOS, then far call cb2, which copies the table into our
;  block, adds a thunk in front of the SetDisplayStart entry and returns ES:DI/CX of the copy (see framelog.c).
;  AH=00h / AX=4F02h (set mode): push BX, AX, 'FL', run the BIOS, far call cb3 (metadata: the mode changed).
;  AX=4F05h, BX=0 (set window A bank, banked VESA games that copy each frame in 64 KB banks): count it; a bank
;  LOWER than the last one = the copy wrapped to the top of the screen = a frame (far call cb) if bankon (/BANK).
;  Everything else: jump to the old INT 10h vector.
        org 0
        cmp ax, 4F05h
        jne .flip
        test bx, bx
        jnz .chain
        add word [cs:nbank], 1
        adc word [cs:nbank+2], 0
        cmp dx, [cs:lastb]
        mov [cs:lastb], dx          ; mov keeps the flags
        jae .chain
        cmp byte [cs:bankon], 0
        je .chain
        call far [cs:cb]
        jmp .chain
.flip:
        cmp ax, 4F07h
        jne .pm
        test bl, 7Dh
        jnz .chain
        call far [cs:cb]
.pm:    cmp ax, 4F0Ah
        jne .mode
        test bl, bl
        jnz .chain
        pushf
        call far [cs:old]
        call far [cs:cb2]
        iret
.mode:  cmp word [cs:cb3+2], 0      ; cb3 = 0: no mode hook (/NOMETA)
        je .chain
        test ah, ah                 ; AH=00h set VGA mode
        jz .set
        cmp ax, 4F02h               ; VESA set mode
        jne .chain
.set:   push bx                     ; the request on the stack (AX/BX after the BIOS may differ; BIOSes nest
        push ax                     ;  INT 10h: Voodoo3 4F02h calls AH=00h 5Fh inside - no shared variables)
        push word 464Ch             ; 'FL': mode_cb finds the pair behind it
        pushf
        call far [cs:old]
        call far [cs:cb3]           ; mode_cb: new mode -> fresh M block before the next frame
        add sp, 6
        iret
.chain: jmp far [cs:old]
        align 4
cb:     dd 0
old:    dd 0
cb2:    dd 0
cb3:    dd 0
nbank:  dd 0                        ; window-A bank switches seen (FRAMELOG /STATUS reads it)
lastb:  dw 0FFFFh                     ; first bank 0 = a wrap: frames = the game's frame count
bankon: db 0
