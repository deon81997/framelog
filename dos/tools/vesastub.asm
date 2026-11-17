; FRAMELOG real-mode INT 10h stub (nasm -f bin; bytes go to vesastub.h via xxd -i, copied to seg+1:0 of our DOS block).
;  AX=4F07h, BL in {00h,02h,80h,82h} (set / schedule display start): far call cb (DPMI real-mode callback) = a flip.
;  AX=4F0Ah, BL=0 (VBE 2.0 protected-mode interface): run the BIOS, then far call cb2, which copies the table into our
;  block, adds a thunk in front of the SetDisplayStart entry and returns ES:DI/CX of the copy (see framelog.c).
;  AH=00h / AX=4F02h (set mode): push BX, AX, 'FL', run the BIOS, far call cb3 (metadata: the mode changed).
;  Everything else: jump to the old INT 10h vector.
        org 0
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
