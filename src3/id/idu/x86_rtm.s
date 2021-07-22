	.file "x86_rtm.s"

	.text
    .align    16
    .globl idurtmavailable
idurtmavailable:
    mov     $7, %eax
    cpuid
    mov     %ebx, %eax
    or      $0x800, %eax
    ret
    .type idurtmavailable,@function
    .size idurtmavailable,.-idurtmavailable

	.text
    .align    16
    .globl idurtmbegin
idurtmbegin:
    mov     $-1, %eax
    xbegin  idurtmret
idurtmret:
    ret
    .type idurtmbegin,@function
    .size idurtmbegin,.-idurtmbegin

	.text
    .align    16
    .globl idurtmend
idurtmend:
    xend
    ret
    .type idurtmend,@function
    .size idurtmend,.-idurtmend

	.text
    .align    16
    .globl idurtmabort
idurtmabort:
    xabort  $1
    ret
    .type idurtmabort,@function
    .size idurtmabort,.-idurtmabort

	.text
    .align    16
    .globl idurtmtest
idurtmtest:
    xtest
    jnz     idurtminprogress
    mov     $0, %rax
    jmp     idurtmtestreturn
idurtminprogress:
    mov     $1, %rax
idurtmtestreturn:
    ret
    .type idurtmtest,@function
    .size idurtmtest,.-idurtmtest

