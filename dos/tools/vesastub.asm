; FRAMELOG real-mode INT 10h stub (nasm -f bin; bytes go to vesastub.h via xxd -i, copied to seg+1:0 of our DOS block).
;  AX=4F07h, BL in {00h,02h,80h,82h} (set / schedule display start): far call cb (DPMI real-mode callback) = a flip.
;  AX=4F0Ah, BL=0 (VBE 2.0 protected-mode interface): run the BIOS, then far call cb2, which copies the table into our
;  block, adds a thunk in front of the SetDisplayStart entry and returns ES:DI/CX of the copy (see framelog.c).
;  Everything else: jump to the old INT 10h vector.
        org 0
        cmp ax, 4F07h
        jne .pm
        test bl, 7Dh
        jnz .chain
        call far [cs:cb]
.pm:    cmp ax, 4F0Ah
        jne .chain
        test bl, bl
        jnz .chain
        pushf
        call far [cs:old]
        call far [cs:cb2]
        iret
.chain: jmp far [cs:old]
        align 4
cb:     dd 0
old:    dd 0
cb2:    dd 0
