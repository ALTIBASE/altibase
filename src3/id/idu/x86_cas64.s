;/
;/ Copyright 1999-2010, ALTIBASE Corporation or its subsidiaries.
;/ All rights reserved.
;/

;/
;/ $Id$
;/

	.file	"x86_cas64.s"

	.text
	.align  8
	.globl  iduCas32
	.type   iduCas32, @function
iduCas32:
    movl        %edx, %eax
	lock
	cmpxchgl    %esi, (%rdi)
    ret
	.size iduCas32, [.-iduCas32]
