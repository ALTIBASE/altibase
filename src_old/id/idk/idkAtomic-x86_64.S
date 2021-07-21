/* ****************************************************************** *
 * Copyright 1999-2006, ALTIBASE Corporation or its subsidiaries.
 * All rights reserved.
 * ****************************************************************** */

/* ****************************************************************** *
 * for use C++ preprocessing do: c++ -c -m64 x86_64.S -o atomic.o
 * It's call cpp | gas
 *
 * $Id: x86_64.S,v 1.3 2006/06/02 08:43:52 alex Exp $
 * ****************************************************************** */
	
	.file	"x86_64.S"


#define ENTER(x)    \
	.text; 			\
	.align  8; 		\
	.globl  x; 		\
	.type   x, @function;\
x:

#define LEAVE(a)    \
	ret;            \
	.size a, [.-a];

# ######### CPP -> GAS  code ######## #

ENTER(idkAtomicINC1)
	lock
	incb	(%rdi)
	movzbl  (%rdi), %eax 	# return walue
LEAVE(idkAtomicINC1)

ENTER(idkAtomicINC2)
	lock
	incw	(%rdi)
	movzwl  (%rdi), %eax 	# return walue
LEAVE(idkAtomicINC2)

ENTER(idkAtomicINC4)
	lock
	incl	(%rdi)
	movl    (%rdi), %eax  	# return new value
LEAVE(idkAtomicINC4)

ENTER(idkAtomicINC8)
	lock
	incq	(%rdi)
LEAVE(idkAtomicINC8)

ENTER(idkAtomicDEC1)
    lock
    decb    (%rdi)
	movzbl  (%rdi), %eax # return value
LEAVE(idkAtomicDEC1)

ENTER(idkAtomicDEC2)
    lock
    decw    (%rdi)
	movzwl  (%rdi), %eax # return walue
LEAVE(idkAtomicDEC2)

ENTER(idkAtomicDEC4)
    lock
    decl    (%rdi)
	movl    (%rdi), %eax  	# return new value
LEAVE(idkAtomicDEC4)

ENTER(idkAtomicDEC8)
    lock
    decq    (%rdi)
LEAVE(idkAtomicDEC8)

ENTER(idkAtomicADD1)
	lock
	addb	%sil, (%rdi)
LEAVE(idkAtomicADD1)

ENTER(idkAtomicADD2)
	lock
	addw	%si, (%rdi)
LEAVE(idkAtomicADD2)

ENTER(idkAtomicADD4)
	lock
	addl	%esi, (%rdi)
LEAVE(idkAtomicADD4)

ENTER(idkAtomicADD8)
	lock
	addq	%rsi, (%rdi)
LEAVE(idkAtomicADD8)

ENTER(idkAtomicCAS1)
	movzbl	%sil, %eax
	lock
	cmpxchgb %dl, (%rdi)
LEAVE(idkAtomicCAS1)

ENTER(idkAtomicCAS2)
	movzwl	%si, %eax
	lock
	cmpxchgw %dx, (%rdi)
LEAVE(idkAtomicCAS2)

ENTER(idkAtomicCAS4)
	movl	%esi, %eax
	lock
	cmpxchgl %edx, (%rdi)
LEAVE(idkAtomicCAS4)

ENTER(idkAtomicCAS8)
	movq	%rsi, %rax
	lock
	cmpxchgq %rdx, (%rdi)
LEAVE(idkAtomicCAS8)

#THE END
