; RMFLIP.COM - real-mode test "game" for FRAMELOG /RM (nasm -f bin): 760 CRTC start-address flips at 70 fps (RDTSC
; timed, calibrated on the BIOS tick count - never the PIT), every 10th frame twice as long, alternating 8-bit OUTs
; and OUT 3D4h,AX - the same pattern as FLIP.EXE 12 70 10, but from V86 / real-mode code. GPL-2.0-or-later.
        cpu 586
        org 100h
start:  push 40h
        pop es
        mov eax, [es:6Ch]
.w0:    cmp eax, [es:6Ch]
        je .w0
        mov ebx, [es:6Ch]
        rdtsc
        mov [t0], eax
.w1:    mov ecx, [es:6Ch]
        sub ecx, ebx
        cmp ecx, 10
        jb .w1
        rdtsc
        sub eax, [t0]           ; TSC ticks in 10 BIOS ticks (65536/1193182 s each)
        mov ecx, 26009          ; * 1193182 / (65536 * 70) = 0.26009 -> ticks per 1/70 s
        mul ecx
        mov ecx, 1000000
        div ecx
        mov [per], eax
        rdtsc
        mov [next], eax
        xor si, si
.loop:  rdtsc
        sub eax, [next]
        js .loop
        mov bx, 50h             ; two text rows apart
        test si, 1
        jz .a
        xor bx, bx
.a:     mov dx, 3D4h
        test si, 2
        jz .bytes
        mov al, 0Ch
        mov ah, bh
        out dx, ax
        mov al, 0Dh
        mov ah, bl
        out dx, ax
        jmp .done
.bytes: mov al, 0Ch
        out dx, al
        inc dx
        mov al, bh
        out dx, al
        dec dx
        mov al, 0Dh
        out dx, al
        inc dx
        mov al, bl
        out dx, al
.done:  inc si
        mov eax, [per]
        push eax
        mov ax, si
        xor dx, dx
        mov cx, 10
        div cx
        pop eax
        test dx, dx
        jnz .one
        shl eax, 1
.one:   add [next], eax
        cmp si, 760
        jb .loop
        mov dx, 3D4h
        mov ax, 000Ch
        out dx, ax
        mov ax, 000Dh
        out dx, ax
        mov ax, si              ; print "RMFLIP: n frames"
        mov di, num + 5
        mov cx, 10
.d:     xor dx, dx
        div cx
        add dl, '0'
        dec di
        mov [di], dl
        test ax, ax
        jnz .d
        mov dx, msg
        mov ah, 9
        int 21h
        mov dx, di
        mov ah, 9
        int 21h
        mov ax, 4C00h
        int 21h
msg:    db 'RMFLIP: $'
num:    db '     ', ' frames at 70 fps, every 10. frame long', 13, 10, '$'
t0:     dd 0
per:    dd 0
next:   dd 0
