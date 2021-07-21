;/
;/ Copyright 1999-2010, ALTIBASE Corporation or its subsidiaries.
;/ All rights reserved.
;/

;/
;/ $Id$
;/

	.file	"x86_cas32.s"

	.text
	.align  8
	.globl  iduCas32
	.type   iduCas32, @function
iduCas32:
	movl	4(%esp), %edx
	movl	12(%esp), %eax
	movl	8(%esp), %ecx
	lock
	cmpxchgl %ecx, (%edx)
    ret
	.size iduCas32, [.-iduCas32]
