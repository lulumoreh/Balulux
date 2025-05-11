; Generated assembly code for TASM
; Source file: tests/complete_test.lx

data segment
; Data section with variables needed by the compiler
call_counter db 0 ; Counter for tracking function calls
input_prompt db '? $'
error_msg db 0Dh, 0Ah, 'Invalid input, please try again: $'
data ends

program_stack segment
    dw   128  dup(0)
program_stack ends

code segment
    assume cs:code, ds:data

main_init:
    mov ax, data
    mov ds, ax
    jmp main
; Implementation to print all integer values
lulog:
    push bp
    mov bp, sp
    mov ax, [bp+4]
    push bx
    push cx
    push dx
    push si
    mov si, 0
    test ax, ax
    jns positive_number
    mov si, 1
    neg ax
positive_number:
    test ax, ax
    jnz prepare_conversion
    mov dl, '0'
    mov ah, 2
    int 21h
    jmp print_newline
prepare_conversion:
    test si, si
    jz convert_to_digits
    mov dl, '-'
    mov ah, 2
    int 21h
convert_to_digits:
    mov cx, 0
    mov bx, 10
digit_loop:
    xor dx, dx
    div bx
    push dx
    inc cx
    test ax, ax
    jnz digit_loop
print_digits:
    pop dx
    add dl, '0'
    mov ah, 2
    int 21h
    loop print_digits
print_newline:
    mov dl, 13
    mov ah, 2
    int 21h
    mov dl, 10
    mov ah, 2
    int 21h
end_lulog:
    pop si
    pop dx
    pop cx
    pop bx
    pop bp
    ret
; Fixed luload implementation to correctly read integer values
luload:
    push bp
    mov bp, sp
    push dx
    push cx
    push bx
    mov ah, 9
    mov dx, offset input_prompt
    int 21h
    xor bx, bx
    xor cx, cx
    mov ah, 1
    int 21h
    cmp al, '-'
    jne luload_first_digit
    mov cx, 1
    mov ah, 1
    int 21h
luload_first_digit:
    cmp al, 13
    je luload_done
    cmp al, '0'
    jb luload_ignore
    cmp al, '9'
    ja luload_ignore
    sub al, '0'
    mov bl, al
    mov bh, 0
luload_next_digit:
    mov ah, 1
    int 21h
    cmp al, 13
    je luload_done
    cmp al, '0'
    jb luload_ignore
    cmp al, '9'
    ja luload_ignore
    sub al, '0'
    mov dl, al
    mov ax, 10
    mul bx
    mov bx, ax
    xor dh, dh
    add bx, dx
    jmp luload_next_digit
luload_ignore:
    jmp luload_next_digit
luload_done:
    mov ah, 2
    mov dl, 13
    int 21h
    mov dl, 10
    int 21h
    mov ax, bx
    cmp cx, 1
    jne luload_return
    neg ax
luload_return:
    pop bx
    pop cx
    pop dx
    pop bp
    ret

; Function: main
main:
    push bp
    mov bp, sp
; Parameters:
; Reserve space for local variables (64 bytes)
    sub sp, 64
    ; Declare variable 'a' of type 'int' at offset -2
        mov ax, 1
        mov [bp-2], ax
    ; Declare variable 'b' of type 'int' at offset -4
        mov ax, 1
        mov [bp-4], ax
    ; Declare variable 'c' of type 'int' at offset -6
        mov ax, 5
        mov [bp-6], ax
    ; Declare variable 'temp' of type 'int' at offset -8
    ; Binary operation: +
    ; Load variable 'a' from offset -2
        mov ax, [bp-2]
        push ax
    ; Binary operation: +
    ; Load variable 'b' from offset -4
        mov ax, [bp-4]
        push ax
    ; Load variable 'c' from offset -6
        mov ax, [bp-6]
        pop bx
        add ax, bx
        pop bx
        add ax, bx
        mov [bp-8], ax
    ; lulog statement
    ; Load variable 'temp' from offset -8
        mov ax, [bp-8]
    ; Logging variable 'temp'
    ; Value to log is in AX
        push ax
        call lulog
        add sp, 2
    ; Binary operation: +
        mov ax, 1
        push ax
    ; Binary operation: +
        mov ax, 2
        push ax
        mov ax, 3
        pop bx
        add ax, bx
        pop bx
        add ax, bx
    ; Assign to variable 'temp' at offset -8
        mov [bp-8], ax
    ; lulog statement
    ; Load variable 'temp' from offset -8
        mov ax, [bp-8]
    ; Logging variable 'temp'
    ; Value to log is in AX
        push ax
        call lulog
        add sp, 2
    ; Binary operation: /
        mov ax, 4
        push ax
        mov ax, 2
    ; Division operation: left / right
        mov cx, ax
        pop ax
        cwd
        idiv cx
    ; Assign to variable 'temp' at offset -8
        mov [bp-8], ax
    ; lulog statement
    ; Load variable 'temp' from offset -8
        mov ax, [bp-8]
    ; Logging variable 'temp'
    ; Value to log is in AX
        push ax
        call lulog
        add sp, 2
    ; Binary operation: *
        mov ax, 2
        push ax
    ; Binary operation: *
        mov ax, 2
        push ax
        mov ax, 2
        pop bx
        imul bx
        pop bx
        imul bx
    ; Assign to variable 'temp' at offset -8
        mov [bp-8], ax
        mov ax, 5
    ; Assign to variable 'temp' at offset -8
        mov [bp-8], ax
    ; luloop statement - fixed implementation with condition at top
        jmp luloop_test_0
luloop_start_0:
        ; If statement - improved implementation
        ; Evaluate if condition
        ; Binary operation: ==
        ; Load variable 'temp' from offset -8
            mov ax, [bp-8]
            push ax
            mov ax, 1
            mov cx, ax
            pop ax
            cmp ax, cx
            mov ax, 0
            jne skip_2
            mov ax, 1
skip_2:
            test ax, ax
            jz else_1
        ; If block begins
            ; lulog statement
                mov ax, 58
            ; Value to log is in AX
                push ax
                call lulog
                add sp, 2
        ; If block ends
            jmp endif_1
else_1:
        ; Else block begins
            ; lulog statement
            ; Load variable 'temp' from offset -8
                mov ax, [bp-8]
            ; Logging variable 'temp'
            ; Value to log is in AX
                push ax
                call lulog
                add sp, 2
        ; Else block ends
endif_1:
        ; End of if statement
        ; Binary operation: -
        ; Load variable 'temp' from offset -8
            mov ax, [bp-8]
            push ax
            mov ax, 1
            mov cx, ax
            pop ax
            sub ax, cx
        ; Assign to variable 'temp' at offset -8
            mov [bp-8], ax
luloop_test_0:
    ; Binary operation: >
    ; Load variable 'temp' from offset -8
        mov ax, [bp-8]
        push ax
        mov ax, 0
        mov cx, ax
        pop ax
        cmp ax, cx
        mov ax, 0
        jle skip_3
        mov ax, 1
skip_3:
        test ax, ax
        jnz luloop_start_0
luloop_end_0:
    ; End of luloop statement
    ; Declare variable 'a' of type 'int' at offset -2
    ; luload expression
        call luload
        mov [bp-2], ax
    ; If statement - improved implementation
    ; Evaluate if condition
    ; Binary operation: >=
    ; Load variable 'a' from offset -2
        mov ax, [bp-2]
        push ax
        mov ax, 5
        mov cx, ax
        pop ax
        cmp ax, cx
        mov ax, 0
        jl skip_5
        mov ax, 1
skip_5:
        test ax, ax
        jz else_4
    ; If block begins
        ; lulog statement
        ; Load variable 'a' from offset -2
            mov ax, [bp-2]
        ; Logging variable 'a'
        ; Value to log is in AX
            push ax
            call lulog
            add sp, 2
    ; If block ends
        jmp endif_4
else_4:
endif_4:
    ; End of if statement
    ; If statement - improved implementation
    ; Evaluate if condition
    ; Binary operation: !=
    ; Load variable 'a' from offset -2
        mov ax, [bp-2]
        push ax
        mov ax, 1
        mov cx, ax
        pop ax
        cmp ax, cx
        mov ax, 0
        je skip_7
        mov ax, 1
skip_7:
        test ax, ax
        jz else_6
    ; If block begins
        ; lulog statement
            mov ax, 6
        ; Value to log is in AX
            push ax
            call lulog
            add sp, 2
    ; If block ends
        jmp endif_6
else_6:
endif_6:
    ; End of if statement
end_main:
    mov sp, bp
    pop bp
    mov ax, 4c00h
    int 21h
code ends

end main_init
