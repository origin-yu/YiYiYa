org 7c00h
_start:
%define init_base 0x0500
bits 16
    ; 初始化寄存器
    mov ax, cs
    mov ds, ax
    mov ss, ax

    ; 屏蔽所有外部中断
    cli

    ; 清除屏幕
    call cls

    ; 初始化光标
    call set_cursor

    ; 打印boot字符串
    mov si,boot
    call print_string

    ; 读取磁盘
    mov bx,init_base
    call load_init

    ; 读取成功
    mov si,load_init_success
    call print_string

    ; 跳转到loader
    jmp init_base

; 读取磁盘到内存 es:bx 地址
load_init:
    mov ah,0x02
    mov al,0x0e
    mov cl,0x02
    mov ch,0x00
    mov dh,0x00
    mov dl,0x00
    int 0x13
    jc disk_error
    jmp dend
disk_error:
    mov si,disk_erro
    call print_string
    jmp $
dend:
    ret

%include "boot/x86/util.s"

; 字符串定义(0x0a,0x0d,0表结束符)
boot db "boot start",0x0a,0x0d,0
disk_erro db "read boot disk erro",0x0a,0x0d,0
load_init_success db "load init success",0x0a,0x0d,0

; 补齐至512
times 510-($-$$)  db 0

; boot结束标志
dw 0xaa55
