/* ****************************************************************** *
 * Copyright 1999-2006, ALTIBASE Corporation or its subsidiaries.
 * All rights reserved.
 * ****************************************************************** */

/* ****************************************************************** * 
 * for use C++ preprocessing do: c++ -c -m32  i386.S -o atomic.o
 * It's call cpp | gas 
 * $Id: i386.S,v 1.2 2006/05/24 06:39:29 alex Exp $
 * ****************************************************************** */

#define ENTER(x)	\
.text; \
.align  8; \
.globl  x; \
.type   x, @function;\
x:

#define LEAVE(a)	\
	ret;			\
	.size a, [.-a];

/* *********** C++ to GAS compile  *********** */

.file	"i386.S"

ENTER(idkAtomicINC1)
	movl	4(%esp), %eax
	lock
	incb	(%eax)
LEAVE(idkAtomicINC1)

ENTER(idkAtomicINC2)	
	movl	4(%esp), %eax
	lock
	incw	(%eax)
LEAVE(idkAtomicINC2)

ENTER(idkAtomicINC4)
	movl	4(%esp), %eax
	lock
	incl	(%eax)
LEAVE(idkAtomicINC4)


ENTER(idkAtomicINC8)
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi	# %edi = target address
	movl  	(%edi), %eax
	movl	4(%edi), %edx	# %edx:%eax = old value
1:
	xorl	%ebx, %ebx
	xorl	%ecx, %ecx
	incl	%ebx			# %ecx:%ebx = 1
	addl	%eax, %ebx
	adcl	%edx, %ecx		# add in the carry from inc
	lock
	cmpxchg8b (%edi)		# try to stick it in
	jne	1b
	movl	%ebx, %eax
	movl	%ecx, %edx		# return new value
	popl	%ebx
	popl	%edi
LEAVE(idkAtomicINC8)


ENTER(idkAtomicDEC1)
	movl	4(%esp), %eax
	lock
	decb	(%eax)
LEAVE(idkAtomicDEC1)

ENTER(idkAtomicDEC2)
	movl	4(%esp), %eax
	lock
	decw	(%eax)
LEAVE(idkAtomicDEC2)

ENTER(idkAtomicDEC4)
	movl	4(%esp), %eax
	lock
	decl	(%eax)
LEAVE(idkAtomicDEC4)

ENTER(idkAtomicDEC8)
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi		# %edi = target address
	movl	(%edi), %eax
	movl	4(%edi), %edx		# %edx:%eax = old value
1:
	xorl	%ebx, %ebx
	xorl	%ecx, %ecx
	not	%ecx
	not	%ebx					# %ecx:%ebx = -1
	addl	%eax, %ebx
	adcl	%edx, %ecx			# add in the carry from inc
	lock
	cmpxchg8b (%edi)			# try to stick it in
	jne	1b
	movl	%ebx, %eax
	movl	%ecx, %edx			# return new value
	popl	%ebx
	popl	%edi
LEAVE(idkAtomicDEC8)

ENTER(idkAtomicADD1)
    movl    4(%esp), %eax
    movl    8(%esp), %ecx
    lock
    addb    %cl, (%eax)
LEAVE(idkAtomicADD1)

ENTER(idkAtomicADD2)
    movl    4(%esp), %eax
    movl    8(%esp), %ecx
    lock
    addw    %cx, (%eax)
LEAVE(idkAtomicADD2)

ENTER(idkAtomicADD4)
    movl    4(%esp), %eax
    movl    8(%esp), %ecx
    lock
    addl    %ecx, (%eax)
LEAVE(idkAtomicADD4)

ENTER(idkAtomicADD8)
    pushl   %edi
    pushl   %ebx
    movl    12(%esp), %edi  
    movl    (%edi), %eax
    movl    4(%edi), %edx
1:
    movl    16(%esp), %ebx
    movl    20(%esp), %ecx
    addl    %eax, %ebx
    adcl    %edx, %ecx
    lock
    cmpxchg8b (%edi)
    jne 1b
    movl    %ebx, %eax
    movl    %ecx, %edx
    popl    %ebx
    popl    %edi
LEAVE(idkAtomicADD8);

ENTER(idkAtomicCAS1)
	movl	4(%esp), %edx
	movzbl	8(%esp), %eax
	movb	12(%esp), %cl
	lock
	cmpxchgb %cl, (%edx)
LEAVE(idkAtomicCAS1)

ENTER(idkAtomicCAS2)
	movl	4(%esp), %edx
	movzwl	8(%esp), %eax
	movw	12(%esp), %cx
	lock
	cmpxchgw %cx, (%edx)
LEAVE(idkAtomicCAS2)

ENTER(idkAtomicCAS4)
	movl	4(%esp), %edx
	movl	8(%esp), %eax
	movl	12(%esp), %ecx
	lock
	cmpxchgl %ecx, (%edx)
LEAVE(idkAtomicCAS4)

ENTER(idkAtomicCAS8)
	pushl	%ebx
	pushl	%esi
	movl	12(%esp), %esi
	movl	16(%esp), %eax
	movl	20(%esp), %edx
	movl	24(%esp), %ebx
	movl	28(%esp), %ecx
	lock
	cmpxchg8b (%esi)
	popl	%esi
	popl	%ebx
LEAVE(idkAtomicCAS8)

#/* END */	
