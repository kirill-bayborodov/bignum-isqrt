; @file bignum_isqrt.asm
; @brief x86-64 YASM implementation of floor(sqrt(x)) for bignum_t.
; @version 1.0.0
; @date 2026-08-22
;
; @details Uses Newton iteration y'=(y+x/y)/2. Private fixed-size records keep
; caller-owned storage unchanged until final publication. Division and addition
; are delegated to the existing bignum modules. The public dispatcher validates
; NULL, byte-range overlap, capacity and normalized input before arithmetic.
;
; ABI boundary: System V AMD64 receives result in RDI and x in RSI; RAX returns
; the signed bignum_isqrt_status_t value. RBX and R12-R15 are callee-saved and
; preserved; RBP is used as the frame base. The stack is aligned for every call,
; private records use 264-byte little-endian bignum_t layout, and no caller-owned
; memory is written before successful publication. Dependency calls may clobber
; caller-saved registers, so persistent pointers remain in R12/R13.
;
; Revision history:
; - rev. 1 (2026-08-22): Initial x86-64 YASM Newton implementation.
; - rev. 2 (2026-08-22): Register-stable pointers, transactional paths and
;                          GNU-stack metadata added.

BITS 64
default rel

%define CAPACITY 32
%define WORD_BYTES 8
%define RECORD_BYTES 264
%define LEN_OFFSET 256
%define FRAME_BYTES 1592
%define FRAME_QWORDS 198
%define INPUT_OFF 0
%define GUESS_OFF 264
%define QUOT_OFF 528
%define REM_OFF 792
%define SUM_OFF 1056
%define NEXT_OFF 1320

%define SUCCESS 0
%define ERR_NULL -1
%define ERR_BAD_LENGTH -2
%define ERR_OVERLAP -3
%define ERR_ARITHMETIC -4

section .text

global bignum_isqrt
extern bignum_div_bignum
extern bignum_add_bignum

; Public symbol boundary:
; bignum_isqrt(bignum_t *result, const bignum_t *x)
; RDI=result [out], RSI=x [in], RAX=typed status; flags and caller-saved
; registers are volatile. On every non-success path result is unchanged.
bignum_isqrt:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, FRAME_BYTES

    mov r12, rdi
    mov r13, rsi
    test rdi, rdi
    jz .error_null
    test rsi, rsi
    jz .error_null

    ; Reject exact and partial overlap of the two complete records.
    lea rax, [rdi + RECORD_BYTES]
    cmp rsi, rax
    jae .length_check
    lea rax, [rsi + RECORD_BYTES]
    cmp rdi, rax
    jae .length_check
    mov eax, ERR_OVERLAP
    jmp .epilogue

.length_check:
    mov rbx, [rsi + LEN_OFFSET]
    cmp rbx, CAPACITY
    ja .error_bad_length
    test rbx, rbx
    jz .zero_result
    mov rax, [rsi + rbx*8 - 8]
    test rax, rax
    jz .error_bad_length

    ; Clear all private records once, including unused tails.
    mov rdi, rsp
    xor eax, eax
    mov ecx, FRAME_QWORDS
    rep stosq

    ; input private copy; the other records are private scratch.
    lea rdi, [rsp + INPUT_OFF]
    mov rsi, r13
    mov ecx, 33
    rep movsq

    ; initial guess = 1 << ceil(bit_length(input)/2).
    mov rax, [r13 + rbx*8 - 8]
    bsr rcx, rax
    lea rdx, [rbx - 1]
    shl rdx, 6
    add rdx, rcx
    inc rdx
    inc rdx
    shr rdx, 1
    mov rax, rdx
    shr rdx, 6
    and eax, 63
    mov ecx, eax
    mov eax, 1
    shl rax, cl
    lea rdi, [rsp + GUESS_OFF]
    mov [rdi + rdx*8], rax
    inc rdx
    mov [rdi + LEN_OFFSET], rdx

    mov qword [rsp + 1584], 2048
.newton_loop:
    cmp qword [rsp + 1584], 0
    je .error_arithmetic
    dec qword [rsp + 1584]

    lea rdi, [rsp + INPUT_OFF]
    lea rsi, [rsp + GUESS_OFF]
    lea rdx, [rsp + QUOT_OFF]
    lea rcx, [rsp + REM_OFF]
    call bignum_div_bignum
    test eax, eax
    jnz .error_arithmetic
    lea rdi, [rsp + SUM_OFF]
    lea rsi, [rsp + GUESS_OFF]
    lea rdx, [rsp + QUOT_OFF]
    call bignum_add_bignum
    test eax, eax
    jnz .error_arithmetic

    ; next = sum >> 1, high-to-low with carry from the higher word.
    lea rdi, [rsp + NEXT_OFF]
    lea rsi, [rsp + SUM_OFF]
    mov ecx, 33
    rep movsq
    mov r8, [rsp + NEXT_OFF + LEN_OFFSET]
    xor r9d, r9d
    mov r10, r8
.shift_loop:
    test r10, r10
    jz .shift_done
    dec r10
    mov rax, [rsp + NEXT_OFF + r10*8]
    mov rdx, rax
    shr rax, 1
    shl r9, 63
    or rax, r9
    mov [rsp + NEXT_OFF + r10*8], rax
    and edx, 1
    mov r9, rdx
    jmp .shift_loop
.shift_done:
    mov r10, [rsp + NEXT_OFF + LEN_OFFSET]
.normalize_next:
    test r10, r10
    jz .compare_next
    cmp qword [rsp + NEXT_OFF + r10*8 - 8], 0
    jne .compare_next
    dec r10
    jmp .normalize_next

.compare_next:
    mov [rsp + NEXT_OFF + LEN_OFFSET], r10
    mov r8, [rsp + GUESS_OFF + LEN_OFFSET]
    cmp r10, r8
    ja .publish_guess
    jb .advance_guess
    mov r9, r10
.compare_equal:
    test r9, r9
    jz .publish_guess
    dec r9
    mov rax, [rsp + NEXT_OFF + r9*8]
    cmp rax, [rsp + GUESS_OFF + r9*8]
    ja .publish_guess
    jb .advance_guess
    jmp .compare_equal
.advance_guess:
    lea rdi, [rsp + GUESS_OFF]
    lea rsi, [rsp + NEXT_OFF]
    mov ecx, 33
    rep movsq
    jmp .newton_loop

.publish_guess:
    mov rdi, r12
    lea rsi, [rsp + GUESS_OFF]
    mov ecx, 33
    rep movsq
    xor eax, eax
    jmp .epilogue

.zero_result:
    mov rdi, r12
    xor eax, eax
    mov ecx, 33
    rep stosq
    xor eax, eax
    jmp .epilogue

.error_null:
    mov eax, ERR_NULL
    jmp .epilogue
.error_bad_length:
    mov eax, ERR_BAD_LENGTH
    jmp .epilogue
.error_arithmetic:
    mov eax, ERR_ARITHMETIC
.epilogue:
    add rsp, FRAME_BYTES
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
